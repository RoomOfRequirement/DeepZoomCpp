#include "deepzoom.hpp"
#include <iostream>
#include <algorithm>
#include <memory>
#include <jpeglib.h>
#include <png.h>

#ifdef QT_GUI_LIB
#include <QImage>
#include <QBuffer>
#endif

std::string ARGB32_To_JPEG_Base64(std::vector<uint8_t> const& argb_bytes, int width, int height, int quality = 75);
std::string ARGB32_To_PNG_Base64(std::vector<uint8_t> const& argb_bytes, int width, int height,
                                 int compression_level = 3 /*0-9*/);
std::string Base64_Encode(unsigned char const* src, size_t len);

int main(int argc, char* argv[])
{
    using namespace dz_openslide;

    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0]
                  << ": <slide path> <format(jpg/png, default=jpg)> <quality(0-100, default=75)>" << std::endl;
        return -1;
    }

    std::string format = "jpg";
    int quality = 75;
    if (argc > 2)
    {
        if (std::string(argv[2]) == "png") format = "png";
        if (argc > 3) quality = std::stoi(argv[3]);
    }

    DeepZoomGenerator slide_handler(argv[1], 254, 1, false,
                                    format == "png" ? DeepZoomGenerator::ImageFormat::PNG :
                                                      DeepZoomGenerator::ImageFormat::JPG,
                                    format == "jpg" ? std::clamp(quality / 100.f, 0.f, 1.f) : 0.75f);
    if (!slide_handler.is_valid())
    {
        std::cerr << "Failed to open slide: " << argv[1] << std::endl;
        return -1;
    }

    std::cout << slide_handler.get_dzi() << std::endl;
    std::cout << "icc_profile size: " << slide_handler.get_icc_profile().size() << std::endl;

    auto const& tile = slide_handler.get_tile(slide_handler.level_count() / 2, 0, 0, true);
    std::cout << "data:image/" + format + ";base64," + Base64_Encode(tile.data(), tile.size()) << std::endl;

    // // output without icc
    // auto const& [width, height, argb_bytes] = slide_handler.get_tile_bytes(slide_handler.level_count() / 2, 0, 0);
    // std::cout << ARGB32_To_JPEG_Base64(argb_bytes, width, height, 75) << std::endl;
    // std::cout << ARGB32_To_PNG_Base64(argb_bytes, width, height) << std::endl;

#ifdef QT_GUI_LIB
    auto const& [w, h, pixels] = slide_handler.get_tile_pixels(slide_handler.level_count() / 2, 0, 0);
    auto img = QImage((const uchar*)pixels.data(), w, h, QImage::Format_ARGB32_Premultiplied);
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, format.c_str(), quality);
    std::cout << "data:image/" + format + ";base64," << byteArray.toBase64().toStdString() << std::endl;
#endif

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
        uint8_t const* argb = argb_bytes.data() + j * width * 4;
        uint8_t* dest = rgb.data();
        while (--n >= 0)
        {
            // convert ARGB to RGB
            dest[0] = argb[1];
            dest[1] = argb[2];
            dest[2] = argb[3];
            dest += 3;
            argb += 4;
        }

        JSAMPROW row_ptr = rgb.data();
        jpeg_write_scanlines(&cinfo, &row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    // std::cout << "ARGB32_To_JPEG_Base64 - Encoded size: " << encoded_size << std::endl;

    auto str = "data:image/jpg;base64," + Base64_Encode(mem_buffer, encoded_size);

    free(mem_buffer);

    return str;
}

std::string ARGB32_To_PNG_Base64(std::vector<uint8_t> const& argb_bytes, int width, int height, int compression_level)
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

    std::vector<uint8_t> buffer;
    buffer.reserve(width * height * 4);
    auto write_callback = [](png_structp png_ptr, png_bytep data, png_size_t length) {
        auto* p = static_cast<std::vector<uint8_t>*>(png_get_io_ptr(png_ptr));
        p->insert(p->end(), data, data + length);
    };
    png_set_write_fn(png_ptr, &buffer, write_callback, nullptr);

    png_write_info(png_ptr, info_ptr);

    std::vector<uint8_t> rgba(width * 3);
    for (int i = 0; i < height; i++)
    {
        int n = width;
        uint8_t const* argb = argb_bytes.data() + i * width * 4;
        uint8_t* dest = rgba.data();
        while (--n >= 0)
        {
            // convert ARGB to RGB
            dest[0] = argb[1];
            dest[1] = argb[2];
            dest[2] = argb[3];
            dest += 3;
            argb += 4;
        }
        png_write_row(png_ptr, rgba.data());
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    // std::cout << "ARGB32_To_PNG_Base64 - Encoded size: " << buffer.size() << std::endl;

    return "data:image/png;base64," + Base64_Encode((unsigned char const*)buffer.data(), buffer.size());
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
