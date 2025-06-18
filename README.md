#

## DeepZoomCpp

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

## Usage

I used it in my [`QtTilesViewer`](https://github.com/RoomOfAnalysis/QtTrials/tree/main/QtTilesViewer) demo project (`QtWebEngine` + `OpenSeaDragon`, communicated through `QWebChannel`), since i don't want to setup a server to serve the tiles.

If you want to support more formats, you may want to try my another [`DeepzoomGenerator`](https://github.com/RoomOfAnalysis/bioimread/blob/main/qpwrapper/deepzoom.hpp) which based on `bioformats`.
