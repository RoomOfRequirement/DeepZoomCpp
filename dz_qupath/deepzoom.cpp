#include "deepzoom.hpp"
#include "reader.hpp"

#include <numeric>
#include <cmath>

using namespace dz_qupath;

DeepZoomGenerator::DeepZoomGenerator(std::string filepath, int tile_size, int overlap)
    : m_tile_size(tile_size), m_overlap(overlap)
{
    m_reader = std::make_unique<Reader>(filepath);
    if (!m_reader->isValid())
    {
        printf("Failed to open reader for: %s\n", filepath.c_str());
        m_reader = nullptr;
        return;
    }

    m_reader->open();

    m_mpp = (m_reader->getPhysSizeX() / m_reader->getSizeX() + m_reader->getPhysSizeY() / m_reader->getSizeY()) *
            1000. / 2.;

    m_levels = m_reader->getLevelCount();
    m_l_dimensions = m_reader->getLevelDimensions();

    m_dzl_dimensions.push_back(m_l_dimensions[0]);
    while (m_dzl_dimensions.back().first > 1 || m_dzl_dimensions.back().second > 1)
        m_dzl_dimensions.push_back({std::max(int{1}, (m_dzl_dimensions.back().first + 1) / 2),
                                    std::max(int{1}, (m_dzl_dimensions.back().second + 1) / 2)});
    std::reverse(m_dzl_dimensions.begin(), m_dzl_dimensions.end());
    m_dz_levels = m_dzl_dimensions.size();

    m_t_dimensions.reserve(m_dz_levels);
    for (const auto& d : m_dzl_dimensions)
        m_t_dimensions.push_back({static_cast<int>(std::ceil(static_cast<double>(d.first) / m_tile_size)),
                                  static_cast<int>(std::ceil(static_cast<double>(d.second) / m_tile_size))});

    m_level_downsamples = m_reader->getLevelDownsamples();

    m_level_0_dz_downsamples.reserve(m_dz_levels);
    m_preferred_slide_levels.reserve(m_dz_levels);
    for (auto l = 0; l < m_dz_levels; l++)
    {
        auto d = std::pow(2, (m_dz_levels - l - 1));
        m_level_0_dz_downsamples.push_back(d);
        m_preferred_slide_levels.push_back(_get_best_level_for_downsample(d));
    }

    m_level_dz_downsamples.reserve(m_dz_levels);
    for (auto l = 0; l < m_dz_levels; l++)
        m_level_dz_downsamples.push_back(m_level_0_dz_downsamples[l] /
                                         m_level_downsamples[m_preferred_slide_levels[l]]);
}

DeepZoomGenerator::~DeepZoomGenerator() = default;

bool DeepZoomGenerator::is_valid() const
{
    return m_reader != nullptr && m_reader->isValid();
}

int DeepZoomGenerator::level_count() const
{
    return m_dz_levels;
}

std::vector<std::pair<int, int>> DeepZoomGenerator::level_tiles() const
{
    return m_t_dimensions;
}

std::vector<std::pair<int, int>> DeepZoomGenerator::level_dimensions() const
{
    return m_dzl_dimensions;
}

int DeepZoomGenerator::tile_count() const
{
    return std::reduce(m_t_dimensions.cbegin(), m_t_dimensions.cend(), int{1},
                       [](auto s, auto const& d) { return s + d.first * d.second; });
}

std::vector<unsigned char> DeepZoomGenerator::get_tile(int dz_level, int col, int row) const
{
    auto [info, z_size] = _get_tile_info(dz_level, col, row);
    auto const& [l0_location, slide_level, l_size] = info;
    auto const& [width, height] = l_size;
    auto const& [xx, yy] = l0_location;
    auto level_downsample = m_level_downsamples[slide_level];

    return m_reader->readRegion(m_level_0_dz_downsamples[dz_level], xx, yy,
                                static_cast<int>(std::ceil(width * level_downsample)),
                                static_cast<int>(std::ceil(height * level_downsample)), 0, 0);
}

std::tuple<std::pair<int, int>, int, std::pair<int, int>> DeepZoomGenerator::get_tile_coordinates(int dz_level, int col,
                                                                                                  int row) const
{
    return std::get<0>(_get_tile_info(dz_level, col, row));
}

std::pair<int, int> DeepZoomGenerator::get_tile_dimensions(int dz_level, int col, int row) const
{
    return std::get<1>(_get_tile_info(dz_level, col, row));
}

std::string DeepZoomGenerator::get_dzi() const
{
    auto const& [width, height] = m_l_dimensions[0];
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n \
<Image xmlns = \"http://schemas.microsoft.com/deepzoom/2008\"\n \
  Format=\"png\"\n \
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

std::pair<std::tuple<std::pair<int, int>, // l0_location
                     int,                 // slide_level
                     std::pair<int, int>  // l_size
                     >,
          std::pair<int, int> // z_size
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
    auto l0_location = std::make_pair(static_cast<int>(l_downsample * l_location.first),
                                      static_cast<int>(l_downsample * l_location.second));
    auto l_size =
        std::make_pair(std::min(static_cast<int>(std::ceil(l_dz_downsample * z_size.first)),
                                m_l_dimensions[slide_level].first - static_cast<int>(std::ceil(l_location.first))),
                       std::min(static_cast<int>(std::ceil(l_dz_downsample * z_size.second)),
                                m_l_dimensions[slide_level].second - static_cast<int>(std::ceil(l_location.second))));
    return std::make_pair(std::make_tuple(l0_location, slide_level, l_size), z_size);
}

// https://github.com/openslide/openslide/blob/main/src/openslide.c#L419
int DeepZoomGenerator::_get_best_level_for_downsample(double downsample) const
{
    if (downsample < m_level_downsamples[0]) return 0;
    for (int i = 1; i < m_levels; i++)
        if (downsample < m_level_downsamples[i]) return i - 1;
    return m_levels - 1;

    // return m_reader->getPreferredResolutionLevel(downsample);
}
