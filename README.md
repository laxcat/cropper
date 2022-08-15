# Cropper

## Overview

Command line image cropping utility with pop-up window to allow user to visually set the crop.

Usage:

```
cropper <image> <size> <ratio (optional. default 1:1)> <suffix (optional. default _thumb)>
```

`<size>` can be any of the following. If both width and height are provided, ratio will be ignored.

- `100`
- `100x100`
- `w100`
- `h100`
- `w100h100`

Examples:

```
$ cropper input.png w200 16:9 _thumb
Will ouput image size: 200x113 (16:9)
Will use suffix: _thumb
Loaded: input.png (8000x6000)
Cropped: xy: (2223, 2001), wh: (3553, 1999)
Writing: input_thumb.png
```

```
$ cropper input.png 200 
Will ouput image size: 200x200 (1:1)
Will use suffix: _thumb
Loaded: input.png (8000x6000)
Cropped: xy: (2667, 1667), wh: (2667, 2667)
Writing: input_thumb.png
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
