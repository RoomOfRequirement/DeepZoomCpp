#include "deepzoom.hpp"

#include <slideio/slideio/slideio.hpp>
#include <slideio/core/levelinfo.hpp>

#include <numeric>
#include <cmath>
#include <algorithm>
#include <iterator>
#include <bit>
#include <cstdint>

extern "C"
{
#define XMD_H
#include <jpeglib.h>
#ifdef const
#undef const
#endif
#include <png.h>
}

using namespace dz_slideio;

DeepZoomGenerator::DeepZoomGenerator(std::string filepath, int tile_size, int overlap, ImageFormat format,
                                     float quality)
    : m_tile_size(tile_size), m_overlap(overlap), m_format(format), m_quality(std ::clamp(quality, 0.f, 1.f))
{
    try
    {
        m_slide = slideio::openSlide(filepath);
    }
    catch (const std::exception& e)
    {
        printf("Error opening slide: %s\n", e.what());
        m_slide = nullptr;
        return;
    }
    if (!m_slide)
    {
        printf("Failed to open slide: %s\n", filepath.c_str());
        return;
    }

    auto scene_count = m_slide->getNumScenes();
    if (scene_count <= 0)
    {
        printf("No scenes found in slide: %s\n", filepath.c_str());
        m_slide = nullptr;
        return;
    }

    // iterate through scenes to find the largest one as main scene in case of first scene is label or macro
    int main_scene_index = -1;
    int64_t max_pixels = 0;
    for (auto i = 0; i < scene_count; i++)
    {
        auto scene = m_slide->getScene(i);
        if (scene && scene->getNumZoomLevels() > 0)
        {
            auto [sx, sy, sw, sh] = m_slide->getScene(i)->getRect();
            auto pixels = static_cast<int64_t>(sw) * sh * scene->getNumZSlices() * scene->getNumTFrames();
            if (pixels > max_pixels)
            {
                max_pixels = pixels;
                main_scene_index = i;
            }
        }
    }
    if (main_scene_index < 0)
    {
        printf("No valid scenes found in slide: %s\n", filepath.c_str());
        m_slide = nullptr;
        return;
    }

    auto scene = m_slide->getScene(main_scene_index);
    if (!scene)
    {
        printf("Failed to get scene from slide: %s\n", filepath.c_str());
        m_slide = nullptr;
        return;
    }

    auto [mpp_x, mpp_y] = scene->getResolution();
    m_mpp = (mpp_x + mpp_y) / 2. * 1e6; // convert to microns

    m_levels = scene->getNumZoomLevels();
    m_l_dimensions.reserve(m_levels);
    m_level_downsamples.reserve(m_levels);
    for (auto l = 0; l < m_levels; l++)
    {
        auto li = scene->getLevelInfo(l);
        m_l_dimensions.push_back({li->getSize().width, li->getSize().height});
        m_level_downsamples.push_back(1. / li->getScale());
    }

    m_dzl_dimensions.push_back(m_l_dimensions[0]);
    while (m_dzl_dimensions.back().first > 1 || m_dzl_dimensions.back().second > 1)
        m_dzl_dimensions.push_back({std::max(int64_t{1}, (m_dzl_dimensions.back().first + 1) / 2),
                                    std::max(int64_t{1}, (m_dzl_dimensions.back().second + 1) / 2)});
    std::reverse(m_dzl_dimensions.begin(), m_dzl_dimensions.end());
    m_dz_levels = m_dzl_dimensions.size();

    m_t_dimensions.reserve(m_dz_levels);
    for (const auto& d : m_dzl_dimensions)
        m_t_dimensions.push_back({static_cast<int64_t>(std::ceil(static_cast<double>(d.first) / m_tile_size)),
                                  static_cast<int64_t>(std::ceil(static_cast<double>(d.second) / m_tile_size))});

    std::vector<double> level_0_dz_downsamples;
    level_0_dz_downsamples.reserve(m_dz_levels);
    m_preferred_slide_levels.reserve(m_dz_levels);
    for (auto l = 0; l < m_dz_levels; l++)
    {
        auto d = std::pow(2, (m_dz_levels - l - 1));
        level_0_dz_downsamples.push_back(d);
        m_preferred_slide_levels.push_back(_get_best_level_for_downsample(d));
    }

    m_level_dz_downsamples.reserve(m_dz_levels);
    for (auto l = 0; l < m_dz_levels; l++)
        m_level_dz_downsamples.push_back(level_0_dz_downsamples[l] / m_level_downsamples[m_preferred_slide_levels[l]]);
}

DeepZoomGenerator::~DeepZoomGenerator() = default;

bool DeepZoomGenerator::is_valid() const
{
    return m_slide != nullptr;
}

int DeepZoomGenerator::level_count() const
{
    return m_dz_levels;
}

std::vector<std::pair<int64_t, int64_t>> DeepZoomGenerator::level_tiles() const
{
    return m_t_dimensions;
}

std::vector<std::pair<int64_t, int64_t>> DeepZoomGenerator::level_dimensions() const
{
    return m_dzl_dimensions;
}

int64_t DeepZoomGenerator::tile_count() const
{
    return std::reduce(m_t_dimensions.cbegin(), m_t_dimensions.cend(), int64_t{1},
                       [](auto s, auto const& d) { return s + d.first * d.second; });
}

std::tuple<int64_t, int64_t, std::vector<uint8_t>> DeepZoomGenerator::get_tile_bytes(int dz_level, int col,
                                                                                     int row) const
{
    auto const& [l0_location, slide_level, l_size] = get_tile_coordinates(dz_level, col, row);
    int l_width = static_cast<int>(l_size.first);
    int l_height = static_cast<int>(l_size.second);
    auto xx = static_cast<int>(l0_location.first);
    auto yy = static_cast<int>(l0_location.second);

    auto l_downsample = m_level_downsamples[slide_level];
    auto ww = static_cast<int>(std::ceil(l_width * l_downsample));  // l0 width
    auto hh = static_cast<int>(std::ceil(l_height * l_downsample)); // l0 height

    auto scene = m_slide->getScene(0);

    // TODO: enumerate channel data types and channel numbers
    //auto channel_count = scene->getNumChannels();
    //auto channel_data_type = scene->getChannelDataType(0);

    auto block_size = std::make_tuple(l_width, l_height);
    auto buffer_size = scene->getBlockSize(block_size, 0, 3, 1, 1);

    std::vector<uint8_t> block_buffer(buffer_size);
    scene->readResampledBlock(std::make_tuple(xx, yy, ww, hh), block_size, block_buffer.data(), buffer_size);

    return std::make_tuple(static_cast<int64_t>(l_width), static_cast<int64_t>(l_height), std::move(block_buffer));
}

std::vector<uint8_t> DeepZoomGenerator::get_tile(int dz_level, int col, int row) const
{
    auto const& [width, height, bytes] = get_tile_bytes(dz_level, col, row);
    auto const quality = static_cast<int>(m_quality * 100);
    if (m_format == ImageFormat::JPG)
        return encode_bytes_to_jpeg(bytes, static_cast<int>(width), static_cast<int>(height), quality);
    else if (m_format == ImageFormat::PNG)
        return encode_bytes_to_png(bytes, static_cast<int>(width), static_cast<int>(height),
                                   std::clamp((100 - quality) / 10, 0, 9));
    return {};
}

std::tuple<std::pair<int64_t, int64_t>, int, std::pair<int64_t, int64_t>> DeepZoomGenerator::get_tile_coordinates(
    int dz_level, int col, int row) const
{
    return std::get<0>(_get_tile_info(dz_level, col, row));
}

std::pair<int64_t, int64_t> DeepZoomGenerator::get_tile_dimensions(int dz_level, int col, int row) const
{
    return std::get<1>(_get_tile_info(dz_level, col, row));
}

std::string DeepZoomGenerator::get_dzi() const
{
    auto const& [width, height] = m_l_dimensions[0];
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n \
<Image xmlns = \"http://schemas.microsoft.com/deepzoom/2008\"\n \
  Format=\"" +
           std::string((m_format == ImageFormat::PNG) ? "png" : "jpg") + "\"\n \
  Overlap=\"" +
           std::to_string(m_overlap) + "\"\n \
  TileSize=\"" +
           std::to_string(m_tile_size) + "\"\n \
  >\n \
  <Size\n \
    Height=\"" +
           std::to_string(height) + "\"\n \
    Width=\"" +
           std::to_string(width) + "\"\n \
  />\n \
</Image>";
}

double DeepZoomGenerator::get_mpp() const
{
    return m_mpp;
}

std::pair<std::tuple<std::pair<int64_t, int64_t>, // l0_location
                     int,                         // slide_level
                     std::pair<int64_t, int64_t>  // l_size
                     >,
          std::pair<int64_t, int64_t> // z_size
          >
DeepZoomGenerator::_get_tile_info(int dz_level, int col, int row) const
{
    // assert((dz_level >= 0 && dz_level < m_dz_levels), "invalid dz level");
    // assert((col >= 0 && col < m_t_dimensions[dz_level].first), "invalid dz col");
    // assert((row >= 0 && row < m_t_dimensions[dz_level].second), "invalid dz row");

    auto slide_level = m_preferred_slide_levels[dz_level];
    auto z_overlap_tl = std::make_pair(m_overlap * int(col != 0), m_overlap * int(row != 0));
    auto z_overlap_br = std::make_pair(m_overlap * int(col != m_t_dimensions[dz_level].first - 1),
                                       m_overlap * int(row != m_t_dimensions[dz_level].second - 1));
    auto z_location = std::make_pair(m_tile_size * col, m_tile_size * row);
    auto z_size = std::make_pair(std::min(m_tile_size, m_dzl_dimensions[dz_level].first - z_location.first) +
                                     z_overlap_tl.first + z_overlap_br.first,
                                 std::min(m_tile_size, m_dzl_dimensions[dz_level].second - z_location.second) +
                                     z_overlap_tl.second + z_overlap_br.second);
    auto l_dz_downsample = m_level_dz_downsamples[dz_level];
    auto l_location = std::make_pair(l_dz_downsample * (z_location.first - z_overlap_tl.first),
                                     l_dz_downsample * (z_location.second - z_overlap_tl.second));
    auto l_downsample = m_level_downsamples[slide_level];
    auto l0_location = std::make_pair(static_cast<int64_t>(l_downsample * l_location.first) + m_l0_offset.first,
                                      static_cast<int64_t>(l_downsample * l_location.second) + m_l0_offset.second);
    auto l_size = std::make_pair(
        std::min(static_cast<int64_t>(std::ceil(l_dz_downsample * z_size.first)),
                 m_l_dimensions[slide_level].first - static_cast<int64_t>(std::ceil(l_location.first))),
        std::min(static_cast<int64_t>(std::ceil(l_dz_downsample * z_size.second)),
                 m_l_dimensions[slide_level].second - static_cast<int64_t>(std::ceil(l_location.second))));
    return std::make_pair(std::make_tuple(l0_location, slide_level, l_size), z_size);
}

int DeepZoomGenerator::_get_best_level_for_downsample(double downsample) const
{
    if (m_levels <= 0) return 0;
    // find the best level for the given downsample
    auto it = std::lower_bound(m_level_downsamples.cbegin(), m_level_downsamples.cend(), downsample);
    if (it == m_level_downsamples.cend())
        return m_levels - 1; // return the last level if downsample is larger than the largest level downsample
    return static_cast<int>(std::distance(m_level_downsamples.cbegin(), it));
}

std::vector<uint8_t> DeepZoomGenerator::encode_bytes_to_jpeg(std::vector<uint8_t> const& bytes, int width, int height,
                                                             int quality)
{
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char* mem_buffer = nullptr;
    unsigned long encoded_size;
    jpeg_mem_dest(&cinfo, &mem_buffer, &encoded_size);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    // may consumes more time and memory but with better quality and smaller size
    cinfo.optimize_coding = TRUE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    // disable chroma subsampling for very high quality
    if (quality > 90)
    {
        cinfo.comp_info[0].v_samp_factor = 1;
        cinfo.comp_info[0].h_samp_factor = 1;
    }

    jpeg_start_compress(&cinfo, TRUE);

    for (int j = 0; j < height; j++)
    {
        JSAMPROW row_ptr = const_cast<JSAMPROW>(bytes.data()) + j * width * 3;
        jpeg_write_scanlines(&cinfo, &row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    std::vector<uint8_t> res(mem_buffer, mem_buffer + encoded_size);

    free(mem_buffer);

    return res;
}

std::vector<uint8_t> DeepZoomGenerator::encode_bytes_to_png(std::vector<uint8_t> const& bytes, int width, int height,
                                                            int compression_level)
{
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return {};
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr, NULL);
        return {};
    }
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return {};
    }

    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png_ptr, compression_level);

    std::vector<uint8_t> buffer;
    buffer.reserve(static_cast<size_t>(width) * height * 4);
    auto write_callback = [](png_structp png_ptr, png_bytep data, png_size_t length) {
        auto* p = static_cast<std::vector<uint8_t>*>(png_get_io_ptr(png_ptr));
        std::copy(data, data + length, std::back_inserter(*p));
    };
    png_set_write_fn(png_ptr, &buffer, write_callback, nullptr);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    for (int j = 0; j < height; j++)
        png_write_row(png_ptr, bytes.data() + j * width * 3);

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    buffer.shrink_to_fit();

    return buffer;
}
