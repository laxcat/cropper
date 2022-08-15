#pragma once
#include <stdio.h>

struct VertCropper {
    float x;
    float y;
    float z;

    float nx;
    float ny;
    float nz;

    float u;
    float v;
};

inline int toString(VertCropper const * v, char * out, int maxToWrite) {
    int triedToWrite = snprintf(out, maxToWrite,
        "xyz (%0.2f, %0.2f, %0.2f), norm (%0.2f, %0.2f, %0.2f), uv (%0.2f, %0.2f)",
        v->x, v->y, v->z,
        v->nx, v->ny, v->nz,
        v->u, v->v
    );
    return (triedToWrite < maxToWrite) ? triedToWrite : maxToWrite;
}
