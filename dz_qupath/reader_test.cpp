#include "reader.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    using namespace dz_qupath;

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path_to_image_file>" << std::endl;
        return -1;
    }

    Reader reader(argv[1]);
    if (!reader.isValid())
    {
        std::cerr << "Failed to initialize reader" << std::endl;
        return -1;
    }

    reader.open();
    std::cout << "Image size: " << reader.getSizeX() << "x" << reader.getSizeY() << "x" << reader.getSizeZ() << "x"
              << reader.getSizeC() << "x" << reader.getSizeT() << std::endl;
    std::cout << "Image physical size: " << reader.getPhysSizeX() << "x" << reader.getPhysSizeY() << "x"
              << reader.getPhysSizeZ() << "x" << reader.getPhysSizeT() << std::endl;
    std::cout << "Image type: " << reader.pixelTypeStr(reader.getPixelType()) << std::endl;
    std::cout << "Image metadata: " << reader.getMetaXML() << std::endl;
    std::cout << "Image tile size: " << reader.getOptimalTileWidth() << "x" << reader.getOptimalTileHeight()
              << std::endl;
    std::cout << "Image level count: " << reader.getLevelCount() << std::endl;
    auto levelDimensions = reader.getLevelDimensions();
    for (int i = 0; i < levelDimensions.size(); ++i)
        std::cout << "Image level " << i << " size: " << levelDimensions[i].first << "x" << levelDimensions[i].second
                  << std::endl;
    auto downsamples = reader.getLevelDownsamples();
    for (int i = 0; i < downsamples.size(); ++i)
        std::cout << "Image level " << i << " downsample: " << downsamples[i] << std::endl;
    auto defaultThumbnail = reader.getDefaultThumbnail(0, 0);
    std::cout << "Default thumbnail size: " << defaultThumbnail.size() << std::endl;
    auto associatedImageNames = reader.getAssociatedImageNames();
    for (auto const& name : associatedImageNames)
    {
        std::cout << "Associated image: " << name << std::endl;
        auto image = reader.getAssociatedImage(name);
        std::cout << "Associated image size: " << image.size() << std::endl;
    }

    return 0;
}