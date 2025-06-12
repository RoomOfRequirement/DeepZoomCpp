#include "deepzoom.hpp"
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << ": <slide path>" << std::endl;
        return -1;
    }

    std::string url = argv[1];
    openslide_t* slide = openslide_open(url.c_str());
    if (!slide)
    {
        std::cerr << "Failed to open slide: " << openslide_get_error(slide) << std::endl;
        return -1;
    }
    else
    {
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_VENDOR); p)
            std::cout << "Slide Vendor: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_COMMENT); p)
            std::cout << "Slide Comment: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_BACKGROUND_COLOR); p)
            std::cout << "Background Color: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_MPP_X); p)
            std::cout << "MPP_X: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_MPP_Y); p)
            std::cout << "MPP_Y: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_WIDTH); p)
            std::cout << "Bounds Width: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_HEIGHT); p)
            std::cout << "Bounds Height: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_X); p)
            std::cout << "Bounds X: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_BOUNDS_Y); p)
            std::cout << "Bounds Y: " << p << std::endl;
        if (auto const* p = openslide_get_property_value(slide, OPENSLIDE_PROPERTY_NAME_OBJECTIVE_POWER); p)
            std::cout << "Objective Power: " << p << std::endl;
    }

    auto slide_handler = std::make_unique<DeepZoomGenerator>(slide, 254, 1);
    std::cout << slide_handler->get_dzi("jpeg") << std::endl;

    return 0;
}
