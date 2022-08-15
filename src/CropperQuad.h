#pragma once
#include "../engine/shader/shaders/cropper/VertCropper.h"


namespace CropperQuad {

    inline Mesh & create(
        Renderable * r,
        VertCropper * vbufdata,
        uint16_t * ibufdata,
        glm::vec2 size,
        int materialId = -1,
        bool dynamic = false
    ) {
        float w = size.x;
        float h = size.y;

        // y-up
        VertCropper vdata[] = {
            // pos       // norm            // tex
            {0, 0, 0,    0.f, 0.f, 1.f,     0.f, 0.f},
            {w, 0, 0,    0.f, 0.f, 1.f,     1.f, 0.f},
            {w, h, 0,    0.f, 0.f, 1.f,     1.f, 1.f},
            {0, h, 0,    0.f, 0.f, 1.f,     0.f, 1.f},
        };

        // cw winding
        uint16_t idata[] = {
            0, 2, 1, 
            0, 3, 2,
        };

        Mesh m;
        m.renderableKey = r->key;
        m.materialId = materialId;

        memcpy(ibufdata, idata, sizeof(idata));
        auto iref = bgfx::makeRef(ibufdata, sizeof(idata));
        m.ibuf = bgfx::createIndexBuffer(iref);

        // TODO: move layout to more central location
        bgfx::VertexLayout layout;
        layout.begin();
        layout.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float);
        layout.add(bgfx::Attrib::Normal,    3, bgfx::AttribType::Float);
        layout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
        layout.end();

        memcpy(vbufdata, vdata, sizeof(vdata));
        auto vref = bgfx::makeRef(vbufdata, sizeof(vdata));
        if (dynamic) {
            m.dynvbuf = bgfx::createDynamicVertexBuffer(vref, layout);
        }
        else {
            auto vbuf = bgfx::createVertexBuffer(vref, layout);
            m.vbufs.push_back(vbuf);
        }

        r->meshes.push_back(m);

        return r->meshes.back();
    }

    inline size_t vbufferSize() {
        return sizeof(VertCropper) * 4;
    }

    inline size_t ibufferSize() {
        return sizeof(uint16_t) * 6;
    }

    inline size_t buffersSize(size_t count = 1) {
        return (vbufferSize() + ibufferSize()) * count;
    }

    inline void allocateBufferWithCount(Renderable * r, size_t count = 1) {
        r->bufferSize = buffersSize(count);
        r->buffer = (byte_t *)malloc(r->bufferSize);
    }

    // !!!
    // buffer layout: 
    // quad 0 vbuf, quad 0 ibuf, quad 1 vbuf, quad 1 ibuf, etc
    inline VertCropper * vbufferForIndex(Renderable * r, size_t quadIndex) {
        return (VertCropper *)(r->buffer + buffersSize(quadIndex));
    }
    inline uint16_t * ibufferForIndex(Renderable * r, size_t quadIndex) {
        return (uint16_t *)(r->buffer + buffersSize(quadIndex) + vbufferSize());
    }

    inline void updateVBuffer(Renderable * r, size_t meshIndex) {
        bgfx::update(
            r->meshes[meshIndex].dynvbuf,
            0,
            bgfx::makeRef(vbufferForIndex(r, 0), vbufferSize())
        );
    }

    inline Mesh & create(
            Renderable * r, 
            size_t index,
            glm::vec2 size,
            int materialId = -1,
            bool dynamic = false
        ) {
        return create(r, vbufferForIndex(r, index), ibufferForIndex(r, index), size, materialId, dynamic);
    }
}
