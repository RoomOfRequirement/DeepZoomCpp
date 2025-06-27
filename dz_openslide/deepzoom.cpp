#include "deepzoom.hpp"

extern "C"
{
#define XMD_H
#include <jpeglib.h>
#ifdef const
#undef const
#endif
#include <png.h>
#include <openslide.h>
}

#include <memory>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <iterator>

using namespace dz_openslide;

DeepZoomGenerator::DeepZoomGenerator(std::string filepath, int tile_size, int overlap, bool limit_bounds,
                                     ImageFormat format, float quality)
    : m_tile_size(tile_size), m_overlap(overlap), m_limit_bounds(limit_bounds), m_format(format),
      m_quality(std ::clamp(quality, 0.f, 1.f))
{
    m_slide = openslide_open(filepath.c_str());
    if (!m_slide)
    {
        printf("Failed to open slide: %s\n", openslide_get_error(m_slide));
        return;
    }

    if (auto mpp_x = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_MPP_X); mpp_x)
        if (auto mpp_y = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_MPP_Y); mpp_y)
            m_mpp = (std::strtod(mpp_x, nullptr) + std::strtod(mpp_y, nullptr)) / 2.;

    m_levels = openslide_get_level_count(m_slide);
    m_l_dimensions.reserve(m_levels);
    int64_t w = -1, h = -1;
    for (auto l = 0; l < m_levels; l++)
    {
        openslide_get_level_dimensions(m_slide, l, &w, &h);
        m_l_dimensions.push_back({w, h});
    }

    if (m_limit_bounds)
    {
        if (auto const* p = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_X); p)
            m_l0_offset.first = std::strtol(p, nullptr, 10);
        if (auto const* p = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_Y); p)
            m_l0_offset.second = std::strtol(p, nullptr, 10);

        auto l0_lim = m_l_dimensions[0];
        std::pair<double, double> size_scale{1., 1.};
        if (auto const* p = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_WIDTH); p)
            size_scale.first = std::strtol(p, nullptr, 10) / static_cast<double>(l0_lim.first);
        if (auto const* p = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_HEIGHT); p)
            size_scale.second = std::strtol(p, nullptr, 10) / static_cast<double>(l0_lim.second);

        for (auto& d : m_l_dimensions)
        {
            d.first = static_cast<int64_t>(std::ceil(d.first * size_scale.first));
            d.second = static_cast<int64_t>(std::ceil(d.second * size_scale.second));
        }
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
        m_preferred_slide_levels.push_back(openslide_get_best_level_for_downsample(m_slide, d));
    }

    m_level_downsamples.reserve(m_levels);
    for (auto l = 0; l < m_levels; l++)
        m_level_downsamples.push_back(openslide_get_level_downsample(m_slide, l));

    m_level_dz_downsamples.reserve(m_dz_levels);
    for (auto l = 0; l < m_dz_levels; l++)
        m_level_dz_downsamples.push_back(level_0_dz_downsamples[l] / m_level_downsamples[m_preferred_slide_levels[l]]);

    if (auto bg_color = openslide_get_property_value(m_slide, OPENSLIDE_PROPERTY_NAME_BACKGROUND_COLOR); bg_color)
        m_background_color = std::string("#") + bg_color;
    m_icc_profile = _get_icc_profile();
}

DeepZoomGenerator::~DeepZoomGenerator()
{
    if (m_slide)
    {
        openslide_close(m_slide);
        m_slide = nullptr;
    }
}

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

std::tuple<int64_t, int64_t, std::vector<uint32_t>> dz_openslide::DeepZoomGenerator::get_tile_pixels(int dz_level,
                                                                                                     int col,
                                                                                                     int row) const
{
    auto const& [l0_location, slide_level, l_size] = get_tile_coordinates(dz_level, col, row);
    auto const& [width, height] = l_size;
    auto const& [xx, yy] = l0_location;

    std::vector<uint32_t> buf(width * height);
    openslide_read_region(m_slide, buf.data(), xx, yy, slide_level, width, height);
    return std::make_tuple(width, height, std::move(buf));
}

std::tuple<int64_t, int64_t, std::vector<uint8_t>> DeepZoomGenerator::get_tile_bytes(int dz_level, int col,
                                                                                     int row) const
{
    auto const& [width, height, buf] = get_tile_pixels(dz_level, col, row);

    // https://openslide.org/docs/premultiplied-argb/
    std::vector<uint8_t> data;
    data.reserve(width * height * 4);
    uint32_t p = 0;
    for (int64_t i = 0; i < width * height; i++)
    {
        p = buf[i];
        // according to the docs: OpenSlide emits samples as uint32_t, so on little-endian systems the output will need to be byte-swapped relative to the input.
        // i have to reverse the order to obtain the correct color
        data.push_back(p);       // b
        data.push_back(p >> 8);  // g
        data.push_back(p >> 16); // r
        data.push_back(p >> 24); // a
    }
    return std::make_tuple(width, height, std::move(data));
}

std::vector<uint8_t> DeepZoomGenerator::get_tile(int dz_level, int col, int row, bool with_icc_profile) const
{
    auto const& [width, height, pixels] = get_tile_pixels(dz_level, col, row);
    auto const quality = static_cast<int>(m_quality * 100);
    if (m_format == ImageFormat::JPG)
        return encode_pixels_to_jpeg(pixels, static_cast<int>(width), static_cast<int>(height), quality,
                                     with_icc_profile ? m_icc_profile : std::vector<uint8_t>{});
    else if (m_format == ImageFormat::PNG)
        return encode_pixels_to_png(pixels, static_cast<int>(width), static_cast<int>(height),
                                    std::clamp((100 - quality) / 10, 0, 9),
                                    with_icc_profile ? m_icc_profile : std::vector<uint8_t>{});
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

std::vector<uint8_t> dz_openslide::DeepZoomGenerator::get_icc_profile() const
{
    return m_icc_profile;
}

std::vector<uint8_t> dz_openslide::DeepZoomGenerator::encode_pixels_to_jpeg(std::vector<uint32_t> const& pixels,
                                                                            int width, int height, int quality,
                                                                            std::vector<uint8_t> const& icc_profile)
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

    if (!icc_profile.empty())
    {
        constexpr char const* icc_profile_name = "ICC Profile"; // 12 bytes
        // Wiki: Exif metadata are restricted in size to 64 kB in JPEG images because according to the specification this information must be contained within a single JPEG APP1 segment.
        constexpr int max_marker_size = 65533;
        constexpr int max_icc_marker_size = max_marker_size - (12 + 2);
        int profile_size = static_cast<int>(icc_profile.size());
        const int markers = (profile_size + (max_icc_marker_size - 1)) / max_icc_marker_size;
        if (markers < 256)
            for (int i = 1, index = 0; i <= markers; i++)
            {
                auto const marker_size = std::min(max_icc_marker_size, profile_size - index);
                auto const block = icc_profile_name + char(i) + char(markers) +
                                   std::string(icc_profile.begin() + index, icc_profile.begin() + index + marker_size);
                jpeg_write_marker(&cinfo, JPEG_APP0 + 2, reinterpret_cast<const JOCTET*>(block.data()), marker_size);
                index += marker_size;
            }
    }

    std::vector<uint8_t> rgb(width * 3);
    for (int j = 0; j < height; j++)
    {
        uint32_t const* argb = pixels.data() + j * width;
        uint8_t* dest = rgb.data();
        for (int i = 0; i < width; i++)
        {
            uint32_t p = argb[i];
            dest[0] = p >> 16;
            dest[1] = p >> 8;
            dest[2] = p;
            dest += 3;
        }

        JSAMPROW row_ptr = rgb.data();
        jpeg_write_scanlines(&cinfo, &row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    std::vector<uint8_t> res(mem_buffer, mem_buffer + encoded_size);

    free(mem_buffer);

    return res;
}

std::vector<uint8_t> dz_openslide::DeepZoomGenerator::encode_pixels_to_png(std::vector<uint32_t> const& pixels,
                                                                           int width, int height, int compression_level,
                                                                           std::vector<uint8_t> const& icc_profile)
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

    // since the argb_bytes is premultiplied ARGB, we can just discard the alpha and convert it to RGB
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png_ptr, compression_level);

#ifdef PNG_iCCP_SUPPORTED
    if (!icc_profile.empty())
    {
        png_set_iCCP(png_ptr, info_ptr, "ICC Profile", PNG_COMPRESSION_TYPE_DEFAULT, icc_profile.data(),
                     icc_profile.size());
    }
#endif

    std::vector<uint8_t> buffer;
    buffer.reserve(static_cast<size_t>(width) * height * 4);
    auto write_callback = [](png_structp png_ptr, png_bytep data, png_size_t length) {
        auto* p = static_cast<std::vector<uint8_t>*>(png_get_io_ptr(png_ptr));
        //p->insert(p->end(), data, data + length);
        std::copy(data, data + length, std::back_inserter(*p));
    };
    png_set_write_fn(png_ptr, &buffer, write_callback, nullptr);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    std::vector<uint8_t> rgb(width * 3);
    for (int j = 0; j < height; j++)
    {
        uint32_t const* argb = pixels.data() + j * width;
        uint8_t* dest = rgb.data();
        for (int i = 0; i < width; i++)
        {
            uint32_t p = argb[i];
            dest[0] = p >> 16;
            dest[1] = p >> 8;
            dest[2] = p;
            dest += 3;
        }
        png_write_row(png_ptr, rgb.data());
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    buffer.shrink_to_fit();

    return buffer;
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

std::vector<uint8_t> dz_openslide::DeepZoomGenerator::_get_icc_profile() const
{
    auto icc_profile_size = openslide_get_icc_profile_size(m_slide);
    std::vector<uint8_t> icc_profile(icc_profile_size);
    openslide_read_icc_profile(m_slide, icc_profile.data());
    return icc_profile;
}