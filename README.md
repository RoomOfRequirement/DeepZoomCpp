#

## DeepZoomCpp

It consists of two `DeepZoomGenerator` implementations:
  - `dz_openslide/DeepZoomGenerator` based on `openslide`
  - `dz_qupath/DeepZoomGenerator` based on `QuPath/Bioformats`

### `dz_openslide/DeepZoomGenerator`

Port of [`openslide-python`](https://github.com/openslide/openslide-python)'s [`DeepZoomGenerator`](https://github.com/openslide/openslide-python/blob/main/openslide/deepzoom.py) for C++.

> OpenSlide can read virtual slides in several formats:
> - Aperio (.svs, .tif)
> - DICOM (.dcm)
> - Hamamatsu (.ndpi, .vms, .vmu)
> - Leica (.scn)
> - MIRAX (.mrxs)
> - Philips (.tiff)
> - Sakura (.svslide)
> - Trestle (.tif)
> - Ventana (.bif, .tif)
> - Zeiss (.czi)
> - Generic tiled TIFF (.tif)

The `DeepZoomGenerator` only depends on `openslide`, you need to compile the `openslide` library first or download the pre-compiled [binaries](https://openslide.org/download/).
The demo `main.cpp` has additional dependency - `libjpeg-turbo` to perform JPEG encoding. To test the demo, you can download some slides from `openslide`'s [test data](https://openslide.cs.cmu.edu/download/openslide-testdata/) or from `openslide`'s [online demo](https://openslide.org/demo/).

Please notice the `openslide`'s license is LGPL-2.1.

Current `openslide` version: 4.0.0.8.

You can also use this [`vendors` branch](https://github.com/Harold2017/openslide_more_vendors/tree/vendors), which supports more image formats and keeps tracking the main branch of official `openslide`. To build the `vendors` branch, you may try this [`openslide-bin`](https://github.com/Harold2017/openslide-bin) fork and follow the `SDPC.md` to easily build the `openslide` library (with python wheels) within a docker container.

### `dz_qupath/DeepZoomGenerator`

Implement similar functionality as `openslide`'s `DeepZoomGenerator` with [`QuPath`](https://github.com/qupath/qupath) and [`Bioformats`](https://github.com/ome/bioformats) to support more image formats.

> Bio-Formats supports [more than a hundred file formats](https://bio-formats.readthedocs.io/en/stable/supported-formats.html).

The necessary jars are included in the `dz_qupath/qupath_jars`.

Please notice the `QuPath`'s license is GPLv3.

Current `QuPath` version is `0.6.0-rc5`, `Bioformats` version is `8.1.1`.

## Usage

I used `openslide/DeepZoomGenerator` in my [`QtTilesViewer`](https://github.com/RoomOfAnalysis/QtTrials/tree/main/QtTilesViewer) demo project (`QtWebEngine` + `OpenSeaDragon`, communicated through `QWebChannel`), since i don't want to setup a server to serve the tiles.

And used `qupath/DeepZoomGenerator` in my [`bioimread/tilesviewer`](https://github.com/RoomOfAnalysis/bioimread/blob/main/qpwrapper/tilesviewer.cpp) to support more formats.

Please notice the difference of `getTile` between `dz_openslide/DeepZoomGenerator` and `qupath/DeepZoomGenerator`:
- the former one supports ICC profile and NOT do resizing for the tiles, which means the width and height of the returned tile is not always equal to those specified in `get_tile_dimensions`
- the PNG compression level does NOT work for the latter one due to the limitation of `javax.imageio.ImageIO`
- details can be found in the code base

## Benchmarks

Please see [here](dz_bench/bench.md).
