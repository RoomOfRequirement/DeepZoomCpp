#pragma once

#include <string>
#include <array>
#include <memory>
#include <vector>
#include <optional>

namespace dz_qupath
{
    class Reader
    {
    public:
        enum class PixelType : int
        {
            UINT8 = 0,
            INT8,
            UINT16,
            INT16,
            UINT32,
            INT32,
            FLOAT,
            DOUBLE,
        };

        static std::string pixelTypeStr(PixelType pixelType);
        static int getBytesPerPixel(PixelType pixelType);

    public:
        Reader(std::string filePath);
        ~Reader();

        bool isValid() const;

        void open();
        void close();

        std::string getMetaXML() const;

        int getSizeX() const;
        int getSizeY() const;
        int getSizeZ() const;
        int getSizeC() const;
        int getSizeT() const;
        // Note: physical size can be NAN
        // mm
        double getPhysSizeX() const;
        // mm
        double getPhysSizeY() const;
        // mm
        double getPhysSizeZ() const;
        // s
        double getPhysSizeT() const;
        PixelType getPixelType() const;
        int getBitsPerPixel() const;
        int getBytesPerPixel() const;
        // RGBA
        std::optional<std::array<int, 4>> getChannelColor(int channel) const;
        std::string getChannelName(int channel) const;

        int getOptimalTileWidth() const;
        int getOptimalTileHeight() const;

        int getLevelCount() const;
        std::vector<std::pair<int, int>> getLevelDimensions() const;
        std::vector<double> getLevelDownsamples() const;
        int getPreferredResolutionLevel(double downsample) const;
        double getPreferredDownsampleFactor(double downsample) const;
        // PNG bytes
        std::vector<unsigned char> readRegion(double downsample, int x, int y, int w, int h, int z, int t) const;
        std::vector<unsigned char> readRegion(int level, int x, int y, int w, int h, int z, int t) const;
        std::vector<unsigned char> readTile(int level, int x, int y, int w, int h, int z, int t) const;
        std::vector<unsigned char> getDefaultThumbnail(int z, int t) const;
        std::vector<std::string> getAssociatedImageNames() const;
        std::vector<unsigned char> getAssociatedImage(std::string const& name) const;

    private:
        struct impl;
        std::unique_ptr<impl> pimpl;
    };
} // namespace dz_qupath