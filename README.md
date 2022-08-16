# Cropper

## Overview

Command line image cropping utility with pop-up window to allow user to visually set the crop.

Usage:

```
cropper <image> <output_size> <output_filename (optional)>
```

`<output_size>` is formated: 100x100 or 100. A single number is assumed to be square.
If `<output_filename>` is omitted: `_cropped` is appended to input image filename.

Examples:

```
$ cropper image.png 100x200 image_t.png
Will output image size: 100x200 (1:2)
Will output to: image_t.png
Loaded: image.png (8000x6000)
Cropped: xy: (2667, 333), wh: (2667, 5333)
Writing: image_t.png
```

```
$ cropper image.png 100
Will output image size: 100x100 (1:1)
Will output to: image_cropped.png
Loaded: image.png (8000x6000)
Cropped: xy: (2667, 1667), wh: (2667, 2667)
Writing: image_cropped.png
```

## Building

Only tested for macOS on arm. Built with desktop cross-platform libraries, so could also build for Linux and Windows, but this is untested.

```bash
cd build
cmake ..
make
```

## Third Party

|Library|Author|License Type|
|---|---|---|
|[CMake]|Kitware|3-Clause BSD|
|[BGFX]|Branimir Karadzic|BSD 2-Clause "Simplified" License|
|[GLFW]|Camilla Löwy|Zlib/libpng|
|[GLM]|g_truc|The Happy Bunny (Modified MIT)|
|[stb] image/resize/write|Sean Barrett|MIT|

[CMake]: <https://cmake.org/>
[GLFW]: <https://www.glfw.org/>
[stb]: <https://github.com/nothings/stb>
[GLM]: <https://github.com/g-truc/glm>
[BGFX]: <https://github.com/bkaradzic/bgfx>
