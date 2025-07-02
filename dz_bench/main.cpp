#include <benchmark/benchmark.h>

#include <vector>
#include <memory>
#include <random>
#include <iostream>

#ifdef QT_GUI_LIB
#include <QImage>
#include <QBuffer>
#endif

#include "../dz_openslide/deepzoom.hpp"
#include "../dz_qupath/deepzoom.hpp"

auto BM_dz_openslide_get_tile = [](benchmark::State& state, std::string const& file_path, int tile_size, int overlap,
                                   std::vector<std::tuple<int, int, int>> const& tiles,
                                   std::string const& format = "jpg", float quality = 0.75f) {
    auto slide = dz_openslide::DeepZoomGenerator(file_path, tile_size, overlap, false,
                                                 (format == "jpg" ? dz_openslide::DeepZoomGenerator::ImageFormat::JPG :
                                                                    dz_openslide::DeepZoomGenerator::ImageFormat::PNG),
                                                 quality);
    size_t i = 0;
    for (auto _ : state)
    {
        auto [dz_level, col, row] = tiles[i++ % tiles.size()];
        auto img = slide.get_tile(dz_level, col, row);
        benchmark::DoNotOptimize(img);
    }
};

#ifdef QT_GUI_LIB
auto BM_dz_openslide_get_tile_qimg = [](benchmark::State& state, std::string const& file_path, int tile_size,
                                        int overlap, std::vector<std::tuple<int, int, int>> const& tiles,
                                        std::string const& format = "jpg", float quality = 0.75f) {
    auto slide = dz_openslide::DeepZoomGenerator(file_path, tile_size, overlap);
    size_t i = 0;
    for (auto _ : state)
    {
        auto [dz_level, col, row] = tiles[i++ % tiles.size()];
        auto const& [width, height, data] = slide.get_tile_pixels(dz_level, col, row);

        auto img = QImage((const uchar*)data.data(), width, height, QImage::Format_ARGB32_Premultiplied);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, format.c_str(), static_cast<int>(quality * 100));

        benchmark::DoNotOptimize(byteArray);
    }
};
#endif

auto BM_dz_qupath_get_tile = [](benchmark::State& state, std::string const& file_path, int tile_size, int overlap,
                                std::vector<std::tuple<int, int, int>> const& tiles, std::string const& format = "jpg",
                                float quality = 0.75f) {
    auto slide = dz_qupath::DeepZoomGenerator(file_path, tile_size, overlap,
                                              (format == "jpg" ? dz_qupath::DeepZoomGenerator::ImageFormat::JPG :
                                                                 dz_qupath::DeepZoomGenerator::ImageFormat::PNG),
                                              quality);
    size_t i = 0;
    for (auto _ : state)
    {
        auto [dz_level, col, row] = tiles[i++ % tiles.size()];
        auto img = slide.get_tile(dz_level, col, row);
        benchmark::DoNotOptimize(img);
    }
};

// ./dz_bench.exe 'xxx.tiff' 254 1 --benchmark_out="res_int_256.json" --benchmark_out_format=json
int main(int argc, char* argv[])
{
    benchmark::Initialize(&argc, argv);

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

    // for parsing results: template(<>) + argument(/)
    std::string name_surfix = "<int>/" + std::to_string(tile_size + overlap * 2);

    benchmark::RegisterBenchmark("openslide_jpg" + name_surfix, BM_dz_openslide_get_tile, filepath, tile_size, overlap,
                                 tiles, "jpg", 0.9f)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RegisterBenchmark("openslide_png" + name_surfix, BM_dz_openslide_get_tile, filepath, tile_size, overlap,
                                 tiles, "png", 1.f)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
#ifdef QT_GUI_LIB
    benchmark::RegisterBenchmark("openslide_jpg_q" + name_surfix, BM_dz_openslide_get_tile_qimg, filepath, tile_size,
                                 overlap, tiles, "jpg", 0.9f)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RegisterBenchmark("openslide_png_q" + name_surfix, BM_dz_openslide_get_tile_qimg, filepath, tile_size,
                                 overlap, tiles, "png", 1.f)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
#endif
    benchmark::RegisterBenchmark("qupath_jpg" + name_surfix, BM_dz_qupath_get_tile, filepath, tile_size, overlap, tiles,
                                 "jpg", 0.9f)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RegisterBenchmark("qupath_png" + name_surfix, BM_dz_qupath_get_tile, filepath, tile_size, overlap, tiles,
                                 "png", 1.f)
        ->Unit(benchmark::kMillisecond)
        ->Arg(n)
        ->MeasureProcessCPUTime()
        ->UseRealTime()
        ->Iterations(200)
        ->Repetitions(5);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
}
