#include "deepzoom.hpp"
#include <iostream>
#include <jpeglib.h>

std::string ARGB32_To_JPEG_Base64(std::vector<uint8_t> const& argb_bytes, int width, int height, int quality = 75);
std::string Base64_Encode(unsigned char const* src, size_t len);

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << ": <slide path>" << std::endl;
        return -1;
    }

    DeepZoomGenerator slide_handler(argv[1], 254, 1);
    if (!slide_handler.is_valid())
    {
        std::cerr << "Failed to open slide: " << argv[1] << std::endl;
        return -1;
    }

    std::cout << slide_handler.get_dzi("jpeg") << std::endl;

    auto const& [width, height, argb_bytes] = slide_handler.get_tile(slide_handler.level_count() / 2, 0, 0);
    std::cout << "Tile Size: " << width << "x" << height << std::endl;
    // you can check the base64 string in a browser by pasting it into the address bar
    std::cout << ARGB32_To_JPEG_Base64(argb_bytes, width, height) << std::endl;

    return 0;
}

std::string ARGB32_To_JPEG_Base64(std::vector<uint8_t> const& argb_bytes, int width, int height, int quality)
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

    std::vector<uint8_t> rgb(width * 3);
    while (cinfo.next_scanline < cinfo.image_height)
    {
        int j = cinfo.next_scanline;
        int n = width;
        uint8_t const* argb = argb_bytes.data() + j * width * 4 - 1;
        uint8_t* dest = rgb.data();
        while (--n >= 0)
        {
            // swap and convert ARGB to RGB
            dest[0] = argb[3];
            dest[1] = argb[2];
            dest[2] = argb[1];
            dest += 3;
            argb += 4;
        }

        JSAMPROW row_ptr = rgb.data();
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
