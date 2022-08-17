#include <time.h>
#include <stb/stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>
#include <stb/stb_image_write.h>
#include "../engine/engine.h"
#include "../engine/MrManager.h"
#include "../engine/dev/print.h"
#include "../engine/common/bgfx_extra.h"
#include "../engine/common/Rect.h"
#include "CropperQuad.h"

// embedded shader
#include "shader/cropper/vs_cropper.metal.bin.geninc"
#include "shader/cropper/fs_cropper.metal.bin.geninc"

// populated by args
char const * filename = nullptr;
char outputFilename[256] = {0};
size2 outputSize = {0, 0};
int ratioNumer = 0;
int ratioDenom = 0;
int screenTexDim = 0;
glm::vec2 cropHalfSize = {0.f, 0.f};

// loaded
byte_t * imageData = nullptr;
byte_t * screenData = nullptr;

// created
Renderable * imagePlane;
Renderable * screenPlane;
bgfx::ProgramHandle program;
Texture imageTexture;
Texture screenTexture;

// state
float panScale = 1.0f;
bool zoomPosDirty = true;
float zoomLinear = 0.0f;
glm::vec2 pos = {0.0f, 0.0f};
Rect cameraRect;
Rect imageRect;
Rect cropRect;

int parseArgs(Args const & args) {
    char const * usageMsg =
        "Usage:\n"
        "cropper <image> <output_size> <output_filename (optional)>\n"
        "\n"
        "<output_size> is formated: 100x100 or 100. A single number is assumed to be square.\n"
        "If <output_filename> is omitted: _cropped is appended to input image filename.\n"
        "\n"
        "Examples:\n"
        "cropper image.png 100x200 image_t.png\n"
        "cropper image.png 100\n";

    if (args.c < 3) {
        printf("Missing parameters.\n\n%s", usageMsg);
        return 1;
    }

    filename = args.v[1];

    // parse output size
    char const * sizeStr = args.v[2];
    int sizeStrLen = (int)strlen(sizeStr);
    // copy into temp string for traversal
    char * tempSizeStr = (char * )malloc(sizeStrLen + 1);
    strcpy(tempSizeStr, sizeStr);
    // looking for width and height strings
    // wh can take the following formats
    // 100
    // 100x100
    char * wStr = tempSizeStr; // defaults to start of string
    char * hStr = tempSizeStr;
    for (int i = 0; i < sizeStrLen; ++i) {
        // x found. width string behind, height string ahead.
        if (tempSizeStr[i] == 'x') {
            tempSizeStr[i] = '\0';
            wStr = tempSizeStr;
            hStr = &tempSizeStr[i+1];
            break;
        }
        // unexpected character
        if (tempSizeStr[i] < '0' || tempSizeStr[i] > '9') {
            wStr = nullptr;
            hStr = nullptr;
            break;
        }
    }
    outputSize.w =
        (wStr) ? atoi(wStr) :
        0;
    outputSize.h =
        (hStr == wStr) ? outputSize.w :
        (hStr) ? atoi(hStr) :
        0;
    // check to make sure we both dimensions
    if (!outputSize.w || !outputSize.h) {
        printf("Failed. Could not parse size \"%s\"\n", sizeStr);
        return 1;
    }
    int div = gcd(outputSize.w, outputSize.h);
    ratioNumer = outputSize.w / div;
    ratioDenom = outputSize.h / div;

    // parse output filename
    // if not provided, append suffix to input image
    if (args.c < 4) {
        // determine output filename
        outputFilename[0] = '\0';
        strncat(outputFilename, filename, 255);
        // remove extension from input filename
        int sz = (int)strlen(outputFilename);
        for (int i = sz - 1; sz >= 0; --i) {
            if (outputFilename[i] == '.') {
                outputFilename[i] = '\0';
                break;
            }
        }
        strncat(outputFilename, "_cropped.png", 255-strlen(outputFilename));
    }
    // output was provided
    else {
        snprintf(outputFilename, sizeof(outputFilename), "%s", args.v[3]);
    }

    return 0;
}

void resetCamera(Camera & c) {
    c.target      = {0.0f, 0.0f, 0.0f};
    // c.target      = {QuadSize.x/2.f, QuadSize.y/2.f, 0.f};
    c.distance    = (float)mm.windowSize.h;
}

void updateCameraRect() {
    float winw = (float)mm.windowSize.w;
    float winh = (float)mm.windowSize.h;
    cameraRect = {-winw/2.f, -winh/2.f, winw, winh};
}

void updatePositions() {
    if (!zoomPosDirty) return;
    // print("mm window ratio %f\n", mm.camera.windowRatio);

    // main image calc position
    imageRect.move(pos);
    float zoom = powf(10, zoomLinear/100.f);
    float mpx = mm.mousePos.x - (float)mm.windowSize.w/2.f;
    float mpy = (float)mm.windowSize.h/2.f - mm.mousePos.y;
    glm::vec2 zoomCenter = {
        (mpx - imageRect.x) / imageRect.w,
        (mpy - imageRect.y) / imageRect.h,
    };
    imageRect.zoom(zoom, zoomCenter);
    pos = {0.f, 0.f};
    zoomLinear = 0.f;

    // set main image pos/scale
    glm::mat4 imageModel{1.f};
    imageModel = glm::translate(imageModel, {imageRect.pos(), 0.f});
    imageModel = glm::scale(imageModel, {imageRect.size(), 1.f});
    imagePlane->meshes[0].model = imageModel;

    // screen calc
    float w = (float)mm.windowSize.w;
    float h = (float)mm.windowSize.h;
    float s;
    glm::vec2 sp;
    if (mm.camera.windowRatio > 1.0f) {
        s = w;
        sp = {-w/2.f, -h/2.f - (w - h)/2.f};
    }
    else {
        s = h;
        sp = {-w/2.f - (h - w)/2.f, -h/2.f};
    }

    // set screen pos/scale
    glm::mat4 screenModel{1.f};
    screenModel = glm::translate(screenModel, {sp, 1.f});
    screenModel = glm::scale(screenModel, {s, s, 1.f});
    screenPlane->meshes[0].model = screenModel;

    // calc image crop rect
    float imgSclInv = (float)imageTexture.img.width / imageRect.size().x;
    cropRect.x = roundf((-(float)imageRect.x - cropHalfSize.x * s) * imgSclInv);
    cropRect.y = roundf((-(float)imageRect.y - cropHalfSize.y * s) * imgSclInv);
    cropRect.w = roundf(cropHalfSize.x * s * 2.f * imgSclInv);
    cropRect.h = roundf(cropHalfSize.y * s * 2.f * imgSclInv);
    // char buf[100];
    // print("cropRect: %s\n", cropRect.toString(buf, 100));

    zoomPosDirty = false;
}

void crop() {
    // crop at full size (just image data copy)
    byte_t * croppedData = (byte_t *)malloc(cropRect.w * cropRect.h * 4);
    for (int y = 0; y < cropRect.h; ++y) {
        for (int x = 0; x < cropRect.w; ++x) {
            int dstI = x + y * cropRect.w;
            int srcI = (cropRect.x + x) + (cropRect.y + y) * imageTexture.img.width;
            ((uint32_t *)croppedData)[dstI] = ((uint32_t *)imageData)[srcI];
        }
    }

    // crop
    byte_t * newImg = (byte_t *)malloc(outputSize.w * outputSize.h * 4);
    stbir_resize_uint8(
        croppedData,    cropRect.w,    cropRect.h,    0,
        newImg,         outputSize.w,  outputSize.h,  0, 4
    );

    print("Cropped: xy: (%d, %d), wh: (%d, %d)\n",
        (int)cropRect.x, (int)cropRect.y, (int)cropRect.w, (int)cropRect.h);

    // write file (always png for now)
    stbi_flip_vertically_on_write(1);
    stbi_write_png(outputFilename, outputSize.w, outputSize.h, 4, newImg, 0);
    //stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

    print("Writing: %s\n", outputFilename);

    glfwSetWindowShouldClose(mm.window, 1);
}

int preWindow(EngineSetup & setup) {
    int err = 0;
    err = parseArgs(setup.args);
    if (err) return err;
    printf("Will output image size: %dx%d (%d:%d)\n", outputSize.w, outputSize.h, ratioNumer, ratioDenom);
    printf("Will output to: %s\n", outputFilename);

    int x, y, w, h;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &x, &y, &w, &h);
    Rect r{(float)x, (float)y, (float)w, (float)h};
    Rect d = r.aspectFit((float)ratioNumer / (float)ratioDenom);
    d.zoom(0.7f);
    // char buf[100];
    // printf("%s\n", d.toString(buf, 100));

    setup.requestWindowPosition = {(int)roundf(d.x), (int)roundf(d.y), (int)roundf(d.w), (int)roundf(d.h)};

    // Screen texture is set up like this
    // +--------+--------+--------+
    // |        |        |        |
    // |        |        |        |
    // |        |        |        |
    // +--------+--------+--------+
    // |        |        |        |
    // |        |   X    |        |
    // |        |        |        |
    // +--------+--------+--------+
    // |        |        |        |
    // |        |        |        |
    // |        |        |        |
    // +--------+--------+--------+
    // Most of the scren will be filled in opaque, with one "cut out" hole with
    // alpha 0 set. We need this hole of ratio ratioNumer/ratioDenom to aspect
    // fit inside center square X. In order to determine the pixel size of the
    // screen, we need to look for the smallest whole integers we can do this
    // with.
    //
    // Let's zoom in on X:
    // +---------------------------+
    // |                           |
    // |                           |
    // |+-------------------------+|
    // ||            .            ||
    // ||            .            ||
    // ||            .            ||
    // ||            .            ||
    // ||            .            ||
    // |+-------------------------+|
    // |                           |
    // |                           |
    // +---------------------------+
    // If we want to place a hole with a 2:1 in the center square, we need at
    // least a 4x4 square to do it on even pixels. So our ratio gets multiplied
    // to 4:2, and placed at (0,1) in a 4x4 field. This 4x4 field is then the
    // size the other 8 squares, which determines the size of the whole screen
    // texture (which is always square).
    //
    // In this example the texture would be 12x12, with a total byte size of
    // 576. We are assuming non POT texture sizes are fine.

    int num = ratioNumer;
    int den = ratioDenom;
    int diff;
    while ((diff = abs(num - den)) % 2) {
        num *= 2;
        den *= 2;
    }
    // bc is the biggest component
    // cx/cy are xy coords relative to center screen square
    int bc, cx, cy;
    // screen hole is wide
    if (num > den) {
        bc = num;
        cx = 0;
        cy = diff / 2;
    }
    // screen whole is tall (or square)
    else {
        bc = den;
        cx = diff / 2;
        cy = 0;
    }
    screenTexDim = bc * 3;
    int pxSize = screenTexDim * screenTexDim; // squared, 9 squares in screen
    int byteSize = pxSize * 4;
    screenData = (byte_t *)malloc(byteSize);
    // fill all in opaque
    // loop on px, not bytes, then fill with 4-byte value
    for (int i = 0; i < pxSize; ++i) {
        ((uint32_t *)screenData)[i] = 0xff000000; // abgr, because little endian
        // printf("filling index %d as opaque\n", i);
    }
    // fill our hole with alpha
    // upper x/y
    int ux = bc + cx + num;
    int uy = bc + cy + den;
    // loop on px, not bytes, then fill with 4-byte value
    for (int y = bc + cy; y < uy; ++y) {
        for (int x = bc + cx; x < ux; ++x) {
            ((uint32_t *)screenData)[x+y*screenTexDim] = 0x00000000; // abgr
            // printf("filling %3d,%3d as transparent\n", x, y);
        }
    }
    // printf("num %d, den %d\n", num, den);
    // printf("screenTexDim %d, pxSize %d, byteSize %d\n", screenTexDim, pxSize, byteSize);

    cropHalfSize.x = (float)num / (float)screenTexDim / 2.f;
    cropHalfSize.y = (float)den / (float)screenTexDim / 2.f;

    // print("crop half size %f %f\n", cropHalfSize.x, cropHalfSize.y);

    return 0;
}

int postInit(Args const & args) {
    // program = mm.memSys.loadProgram(mm.assetsPath, "vs_cropper", "fs_cropper");
    program = createBGFXProgram(vs_cropper_bin, vs_cropper_bin_len, fs_cropper_bin, fs_cropper_bin_len);

    auto caps = bgfx::getCaps();
    int maxts = caps->limits.maxTextureSize;
    // print("TEX SIZE %d\n", maxts);

    int imgw, imgh, imgc;
    stbi_set_flip_vertically_on_load(1);
    imageData = stbi_load(filename, &imgw, &imgh, &imgc, 0);

    if (!imageData) {
        print("Failed. Could not load %s\n", filename);
        return 1;
    }

    updateCameraRect();
    imageRect = cameraRect.aspectFit((float)imgw, (float)imgh);

    if (imgw > maxts || imgh > maxts) {
        print("Failed. %s (%dx%d) too big for max texture size: %d\n", filename, imgw, imgh, maxts);
        return 1;
    }

    print("Loaded: %s (%dx%d)\n", filename, imgw, imgh);
    uint64_t texFlags = BGFX_SAMPLER_MAG_POINT|BGFX_SAMPLER_U_MIRROR|BGFX_SAMPLER_V_MIRROR;

    // setup image material/texture
    Material imageMat;
    setName(imageMat.name, "image_plane_mat");
    imageMat.baseColor = {1.f, 1.f, 1.f, 1.f};
    imageMat.roughness() = 1.f;
    imageMat.metallic() = 0.f;
    imageMat.specular() = 0.f;
    auto imageTex = imageTexture.createImmutable(imgw, imgh, 4, imageData, texFlags);

    // setup imagePlane
    imagePlane = mm.rendSys.create(program, "image_plane");
    CropperQuad::allocateBufferWithCount(imagePlane, 1);
    CropperQuad::create(imagePlane, 0, {1.f, 1.f}, 0, true);
    imagePlane->meshes[0].images.color = 0; // index of texture in imagePlane->textures
    imagePlane->textures.push_back(imageTex);
    imagePlane->materials.push_back(imageMat);

    // setup screen material/texture
    Material screenMat;
    setName(screenMat.name, "screen_plane_mat");
    screenMat.baseColor = {1.f, 1.f, 1.f, 0.5f};
    screenMat.roughness() = 1.f;
    screenMat.metallic() = 0.f;
    screenMat.specular() = 0.f;
    auto screenTex = screenTexture.createImmutable(screenTexDim, screenTexDim, 4, screenData, texFlags);

    // setup imagePlane
    screenPlane = mm.rendSys.create(program, "screen_plane");
    CropperQuad::allocateBufferWithCount(screenPlane, 1);
    CropperQuad::create(screenPlane, 0, {1.0f, 1.0f}, 0, true);
    screenPlane->meshes[0].images.color = 0; // index of texture in screenPlane->textures
    screenPlane->textures.push_back(screenTex);
    screenPlane->materials.push_back(screenMat);

    mm.rendSys.lights.dirDataDirAsEuler[0].x = -M_PI_2;
    mm.rendSys.lights.dirStrengthAmbientAt (0) = 1.f;
    mm.rendSys.lights.dirStrengthDiffuseAt (0) = 0.f;
    mm.rendSys.lights.dirStrengthSpecularAt(0) = 0.f;
    mm.rendSys.lights.dirStrengthOverallAt (0) = 1.f;

    mm.camera.projType = Camera::ProjType::Ortho;
    mm.camera.orthoResetFn = resetCamera;

    mm.camera.reset();
    updatePositions();

    return 0;
}

void preDraw() {
    updatePositions();
}

void postResize() {
    zoomPosDirty = true;
    updateCameraRect();
    updatePositions();
    mm.camera.reset();
    mm.updateTime(glfwGetTime());
    mm.draw();
}

void preShutdown() {
    // imageTexture.destroy();
    screenTexture.destroy();
    bgfx::destroy(program);
    if (imageData) stbi_image_free(imageData);
    if (screenData) free(screenData);
}

void keyEvent(Event const & e) {
    static bool leftShiftDown = false;
    static bool rightShiftDown = false;
    if (e.key == GLFW_KEY_LEFT_SHIFT)  leftShiftDown = (bool)e.action;
    if (e.key == GLFW_KEY_RIGHT_SHIFT) rightShiftDown = (bool)e.action;
    panScale = (leftShiftDown || rightShiftDown) ? 0.3f : 1.0f;

    if (e.action == GLFW_PRESS) {
        // set alpha of screen
        int key;
        if ((key = glfwNumberKey(e.key)) != -1) {
            screenPlane->materials[0].baseColor.a = (key == 0) ? 1.f : (float)key * .1f;
        }

        // crop (c, space, or enter)
        if (e.key == GLFW_KEY_C || e.key == GLFW_KEY_SPACE || e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER) {
            crop();
        }
    }
}

void mousePosEvent(Event const & e) {
    if (mm.mouseIsDown) {
        float winDistRatio = mm.camera.distance / (float)mm.windowSize.h;
        pos.x += (e.x - mm.mousePrevPos.x) * winDistRatio * panScale;
        pos.y -= (e.y - mm.mousePrevPos.y) * winDistRatio * panScale;
        zoomPosDirty = true;
    }
}

void scrollEvent(Event const & e) {
    zoomLinear += e.y * panScale;
    zoomPosDirty = true;
}

int main(int argc, char ** argv) {
    return main_desktop({
        .args = {argc, argv},
        .preWindow = preWindow,
        .postInit = postInit,
        .preDraw = preDraw,
        .postResize = postResize,
        .preShutdown = preShutdown,
        .keyEvent = keyEvent,
        .mousePosEvent = mousePosEvent,
        .scrollEvent = scrollEvent,
        .windowTitle = "Cropper",
        .cameraControl = false,
        .limits = {.minw=200, .minh=200},
    });
}
