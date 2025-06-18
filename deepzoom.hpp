#pragma once

#include <openslide.h>

#include <cstdint>
#include <vector>
#include <string>
#include <utility>

class DeepZoomGenerator
{
public:
    DeepZoomGenerator(openslide_t* slide, int tile_size = 254, int overlap = 1, bool limit_bounds = false);
    ~DeepZoomGenerator() = default;

    DeepZoomGenerator(DeepZoomGenerator const&) = delete;
    DeepZoomGenerator& operator=(DeepZoomGenerator const&) = delete;

    DeepZoomGenerator(DeepZoomGenerator&&) = default;
    DeepZoomGenerator& operator=(DeepZoomGenerator&&) = default;

    // deepzoom levels
    int level_count() const;
    // tile dimensions <col, row>
    std::vector<std::pair<int64_t, int64_t>> level_tiles() const;
    // deepzoom level dimensions <col, row>
    std::vector<std::pair<int64_t, int64_t>> level_dimensions() const;
    int64_t tile_count() const;
    // <width, height, ARGB_Premultiplied_bytes>
    std::tuple<int64_t, int64_t, std::vector<uint8_t>> get_tile(int dz_level, int col, int row) const;
    // <<x, y>, slide_level, <width, height>>
    std::tuple<std::pair<int64_t, int64_t>, int, std::pair<int64_t, int64_t>> get_tile_coordinates(int dz_level,
                                                                                                   int col,
                                                                                                   int row) const;
    // <width, height>
    std::pair<int64_t, int64_t> get_tile_dimensions(int dz_level, int col, int row) const;
    // XML
    std::string get_dzi(std::string const& format) const;

    double get_mpp() const;

private:
    auto _get_tile_info(int dz_level, int col, int row) const
        -> std::pair<std::tuple<std::pair<int64_t, int64_t>, // l0_location
                                int,                         // slide_level
                                std::pair<int64_t, int64_t>  // l_size
                                >,
                     std::pair<int64_t, int64_t> // z_size
                     >;

private:
    openslide_t* m_slide = nullptr;
    int64_t m_tile_size =
        512; // the width and height of a single tile, for best viewer performance, tile_size + 2 * overlap should be a power of two
    int m_overlap = 1;                             // the number of extra pixels to add to each interior edge of a tile
    bool m_limit_bounds = false;                   // true to render only the non-empty slide region
    std::pair<int64_t, int64_t> m_l0_offset{0, 0}; // level 0 coordinate offset
    double m_mpp = 1e-6;
    int m_levels = 0;                                          // slide levels
    int m_dz_levels = 0;                                       // deepzoom levels
    std::vector<std::pair<int64_t, int64_t>> m_l_dimensions;   // slide level dimensions
    std::vector<std::pair<int64_t, int64_t>> m_dzl_dimensions; // deepzoom level dimensions
    std::vector<std::pair<int64_t, int64_t>> m_t_dimensions;   // tile dimensions
    std::vector<int> m_preferred_slide_levels;                 // preferred slide levels for each deepzoom level
    std::vector<double> m_level_downsamples;                   // slide level downsample factors
    std::vector<double> m_level_dz_downsamples;                // deepzoom level downsample factors
    std::string m_background_color = "#ffffff";
};
