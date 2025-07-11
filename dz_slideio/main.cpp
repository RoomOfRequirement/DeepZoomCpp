#include "deepzoom.hpp"

#include <iostream>
#include <algorithm>
#include <memory>
#include <jpeglib.h>

// #include <slideio/slideio/slideio.hpp>
// #include <slideio/core/levelinfo.hpp>

std::string RGB32_To_JPEG_Base64(std::vector<uint8_t> const& rgb_bytes, int width, int height, int quality = 75);
std::string Base64_Encode(unsigned char const* src, size_t len);

template <typename T, typename Alloc, template <typename, typename> class C>
std::ostream& operator<<(std::ostream& os, C<T, Alloc> const& seq)
{
    os << "[";
    size_t i = 0;
    for (auto it = seq.cbegin(); it != seq.cend(); it++)
    {
        if (i++ > 0) os << ", ";
        os << "\"" << *it << "\"";
    }
    os << "]";
    return os;
}

int main(int argc, char* argv[])
{
    using namespace dz_slideio;

    if (argc < 2)
    {
        std::cerr
            << "Usage: " << argv[0]
            << ": <slide path> <format(jpg/png, default=jpg)> <quality(0-100, default=75)> <tile_size(default=254)> <overlap(default=1)> <dz_level(default=0)> <dz_col(default=0)> <dz_row(default=0)>"
            << std::endl;
        return -1;
    }

    std::string format = "jpg";
    int quality = 75;
    int tile_size = 254;
    int overlap = 1;
    int dz_level = 0;
    int dz_col = 0;
    int dz_row = 0;
    if (argc > 2)
    {
        if (std::string(argv[2]) == "png") format = "png";
        if (argc > 3) quality = std::stoi(argv[3]);
        if (argc > 4) tile_size = std::stoi(argv[4]);
        if (argc > 5) overlap = std::stoi(argv[5]);
        if (argc > 6) dz_level = std::stoi(argv[6]);
        if (argc > 7) dz_col = std::stoi(argv[7]);
        if (argc > 8) dz_row = std::stoi(argv[8]);
    }

    // FIXME: slideio cannot handle pyramidal tiff slides, its number of zoom levels is always 0
    DeepZoomGenerator slide_handler(argv[1], tile_size, overlap,
                                    format == "png" ? DeepZoomGenerator::ImageFormat::PNG :
                                                      DeepZoomGenerator::ImageFormat::JPG,
                                    format == "jpg" ? std::clamp(quality / 100.f, 0.f, 1.f) : 0.75f);
    if (!slide_handler.is_valid())
    {
        std::cerr << "Failed to open slide: " << argv[1] << std::endl;
        return -1;
    }

    std::cout << slide_handler.get_dzi() << std::endl;
    std::cout << "dz_levels: " << slide_handler.level_count() << std::endl;
    for (auto i = 0; i < slide_handler.level_count(); i++)
    {
        auto [w, h] = slide_handler.level_tiles()[i];
        std::cout << "Level " << i << ": (Rows: " << w << ", Cols: " << h << ")" << std::endl;
    }

    auto const& tile = slide_handler.get_tile(dz_level, dz_col, dz_row);
    std::cout << "data:image/" + format + ";base64," + Base64_Encode(tile.data(), tile.size()) << std::endl;

    // std::string filepath(argv[1]);
    // std::cout << "Opening slide: " << filepath << std::endl;

    // auto slide_handler = slideio::openSlide(filepath);
    // if (!slide_handler)
    // {
    //    std::cerr << "Failed to open slide: " << argv[1] << std::endl;
    //    return -1;
    // }

    // // https://github.com/Booritas/slideio/blob/master/src/slideio/slideio/slide.hpp
    // auto scene_count = slide_handler->getNumScenes();
    // std::cout << "Number of scenes: " << scene_count << std::endl;
    // auto metadata = slide_handler->getRawMetadata();
    // std::cout << "RawMetadata: " << metadata << std::endl;
    // auto auxImageNames = slide_handler->getAuxImageNames();
    // std::cout << "Auxiliary images: " << auxImageNames << std::endl;

    // // https://github.com/Booritas/slideio/blob/master/src/slideio/slideio/scene.hpp
    // auto scene = slide_handler->getScene(0);
    // std::cout << "Scene: " << scene->getName() << std::endl;
    // auto scene_metadata = scene->getRawMetadata();
    // std::cout << "Scene RawMetadata: " << scene_metadata << std::endl;
    // auto [x, y, w, h] = scene->getRect();
    // std::cout << "Scene Rect: (" << x << ", " << y << ", " << w << ", " << h << ")" << std::endl;
    // auto [mpp_x, mpp_y] = scene->getResolution();
    // mpp_x *= 1e6; // convert to microns
    // mpp_y *= 1e6; // convert to microns
    // std::cout << "Scene Resolution: (" << mpp_x << ", " << mpp_y << ")" << std::endl;
    // auto channel_count = scene->getNumChannels();
    // std::cout << "Scene Channel Count: " << channel_count << std::endl;
    // auto zoom_levels = scene->getNumZoomLevels();
    // std::cout << "Scene Zoom Levels: " << zoom_levels << std::endl;
    // for (auto i = 0; i < zoom_levels; ++i)
    // {
    //    auto li = scene->getLevelInfo(i);
    //    std::cout << "Zoom Level " << i << ": (" << *li << ")" << std::endl;
    // }
    // auto block_size = scene->getBlockSize(std::make_tuple(512, 512), 0, 3, 1, 1);
    // std::cout << "Scene Block Size: (" << block_size << ")" << std::endl;
    // std::vector<uint8_t> block_buffer(block_size);
    // scene->readBlock(std::make_tuple(w / 2 - 256, h / 2 - 256, 512, 512), block_buffer.data(), block_size);
    // std::cout << RGB32_To_JPEG_Base64(block_buffer, 512, 512, 75) << std::endl;

    return 0;
}

std::string RGB32_To_JPEG_Base64(std::vector<uint8_t> const& rgb_bytes, int width, int height, int quality)
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

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
        JSAMPROW row_ptr = const_cast<uint8_t*>(rgb_bytes.data()) + cinfo.next_scanline * width * 3;
        jpeg_write_scanlines(&cinfo, &row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    auto str = "data:image/jpg;base64," + Base64_Encode(mem_buffer, encoded_size);

    free(mem_buffer);

    return str;
}

/*
* Base64 encoding/decoding (RFC1341)
* Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
*/
std::string Base64_Encode(unsigned char const* src, size_t len)
{
    static const unsigned char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    unsigned char *out, *pos;
    unsigned char const *end, *in;

    size_t olen;

    olen = 4 * ((len + 2) / 3);

    if (olen < len) return std::string();

    std::string outStr;
    outStr.resize(olen);
    out = (unsigned char*)&outStr[0];

    end = src + len;
    in = src;
    pos = out;
    while (end - in >= 3)
    {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
    }

    if (end - in)
    {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1)
        {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
    }

    return outStr;
}
