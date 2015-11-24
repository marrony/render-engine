//
// Created by Marrony Neris on 11/24/15.
//

#ifndef SHAPES_H
#define SHAPES_H

#include <vector>

struct Vector2 {
    float x, y;
};

struct Vector3 {
    float x, y, z;
};

void createSphere(float size, int numberSlices,
                  std::vector<Vector3>& vertex, std::vector<Vector3>& normal,
                  std::vector<Vector2>& texture, std::vector<uint16_t>& indices) {

    int numberParallels = numberSlices / 2;
    int numberVertices = (numberParallels + 1) * (numberSlices + 1);
    int numberIndices = numberParallels * numberSlices * 6;
    float angleStep = (2.0f * 3.1415926535897932384626433832795f) / (float) numberSlices;

    vertex.resize(numberVertices);
    normal.resize(numberVertices);
    texture.resize(numberVertices);
    indices.resize(numberIndices);

    for (int i = 0; i < numberParallels + 1; i++) {
        for (int j = 0; j < numberSlices + 1; j++) {
            int index = i * (numberSlices + 1) + j;

            vertex[index].x = size * sinf(angleStep * i) * sinf(angleStep * j);
            vertex[index].y = size * cosf(angleStep * i);
            vertex[index].z = size * sinf(angleStep * i) * cosf(angleStep * j);

            normal[index].x = vertex[index].x / size;
            normal[index].y = vertex[index].y / size;
            normal[index].z = vertex[index].z / size;

            texture[index].x = (float) j / (float) numberSlices;
            texture[index].y = 1.0f - (float) i / (float) numberParallels;
        }
    }

    int index = 0;
    for (int i = 0; i < numberParallels; i++) {
        for (int j = 0; j < numberSlices; j++) {
            int i0 = i;
            int i1 = i + 1;
            int j0 = j;
            int j1 = j + 1;

            indices[index++] = i0 * (numberSlices + 1) + j0;
            indices[index++] = i1 * (numberSlices + 1) + j0;
            indices[index++] = i1 * (numberSlices + 1) + j1;

            indices[index++] = i0 * (numberSlices + 1) + j0;
            indices[index++] = i1 * (numberSlices + 1) + j1;
            indices[index++] = i0 * (numberSlices + 1) + j1;
        }
    }
}

#endif //SHAPES_H
