#include <benchmark/benchmark.h>

#include <vector>
#include <memory>
#include <random>
#include <iostream>

#include <QImage>
#include <QBuffer>

#include "../dz_openslide/deepzoom.hpp"
#include "../dz_qupath/deepzoom.hpp"

auto BM_dz_openslide_get_tile_jpg = [](benchmark::State& state, std::string const& file_path, int tile_size,
                                       int overlap, std::vector<std::tuple<int, int, int>> const& tiles) {
    auto slide = dz_openslide::DeepZoomGenerator(file_path, tile_size, overlap);
    size_t i = 0;
    for (auto _ : state)
    {
        auto [dz_level, col, row] = tiles[i++ % tiles.size()];
        auto const& [width, height, data] = slide.get_tile(dz_level, col, row);

        // encode to JPG
        auto img = QImage((const uchar*)data.data(), width, height, QImage::Format_ARGB32_Premultiplied);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "JPG", 90);

        benchmark::DoNotOptimize(byteArray);
    }
};

auto BM_dz_openslide_get_tile_png = [](benchmark::State& state, std::string const& file_path, int tile_size,
                                       int overlap, std::vector<std::tuple<int, int, int>> const& tiles) {
    auto slide = dz_openslide::DeepZoomGenerator(file_path, tile_size, overlap);
    size_t i = 0;
    for (auto _ : state)
    {
        auto [dz_level, col, row] = tiles[i++ % tiles.size()];
        auto const& [width, height, data] = slide.get_tile(dz_level, col, row);

        // encode to PNG
        auto img = QImage((const uchar*)data.data(), width, height, QImage::Format_ARGB32_Premultiplied);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");

        benchmark::DoNotOptimize(byteArray);
    }
};

auto BM_dz_qupath_get_tile = [](benchmark::State& state, std::string const& file_path, int tile_size, int overlap,
                                std::vector<std::tuple<int, int, int>> const& tiles) {
    auto slide = dz_qupath::DeepZoomGenerator(file_path, tile_size, overlap);
    size_t i = 0;
    for (auto _ : state)
    {
        auto [dz_level, col, row] = tiles[i++ % tiles.size()];
        auto img = slide.get_tile(dz_level, col, row);
        benchmark::DoNotOptimize(img);
    }
};

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <filepath> <tile_size> <overlap>" << std::endl;
        return 1;
    }

    std::string filepath(argv[1]);
    int tile_size = std::stoi(argv[2]);
    int overlap = std::stoi(argv[3]);

    std::cout << "filepath: " << filepath << " tile_size: " << tile_size << " overlap: " << overlap << std::endl;

    constexpr int n = 1 << 10;
    std::vector<std::tuple<int, int, int>> tiles(n);
    {
        auto slide = dz_openslide::DeepZoomGenerator(filepath, tile_size, overlap);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> ld(0, slide.level_count() - 1);
        for (size_t i = 0; i < tiles.size(); i++)
        {
            auto dz_level = ld(gen);
            auto [cols, rows] = slide.level_tiles()[dz_level];

            std::uniform_int_distribution<int> cold(0, cols - 1), rowd(0, rows - 1);
            auto col = cold(gen);
            auto row = rowd(gen);
            tiles[i] = std::make_tuple(dz_level, col, row);
        }
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RegisterBenchmark("BM_dz_openslide_get_tile_jpg", BM_dz_openslide_get_tile_jpg, filepath, tile_size,
                                 overlap, tiles)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RegisterBenchmark("BM_dz_openslide_get_tile_png", BM_dz_openslide_get_tile_png, filepath, tile_size,
                                 overlap, tiles)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RegisterBenchmark("BM_dz_qupath_get_tile", BM_dz_qupath_get_tile, filepath, tile_size, overlap, tiles)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
