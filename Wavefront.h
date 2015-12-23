//
// Created by Marrony Neris on 12/22/15.
//

#ifndef WAVEFRONT_H
#define WAVEFRONT_H

#include "Vector.h"
#include "Allocator.h"

#include <stdint.h>

struct WavefrontMaterial {
    char name[32];
};

struct WavefrontGroup {
    char name[32];
    char materialName[32];
    int startIndices;
    int numberIndices;
};

struct WavefrontObject {
    int numberMaterials;
    WavefrontMaterial* materials;

    int numberGroups;
    WavefrontGroup* groups;

    int numberVertices;
    Vector3* vertices;
    Vector3* normals;
    Vector3* tangent;
    Vector3* bitangent;
    Vector2* texture;

    int numberIndices;
    uint16_t* indices;
};

struct Wavefront {
    int numberObjects;
    WavefrontObject* objects;
};

bool mnLoadWavefront(HeapAllocator& allocator, const char* filename, Wavefront& wavefront);
void mnDestroyWavefront(HeapAllocator& allocator, Wavefront& wavefront);

#endif //WAVEFRONT_H
