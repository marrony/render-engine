//
// Created by Marrony Neris on 11/24/15.
//

#ifndef SHAPES_H
#define SHAPES_H

#include <vector>

struct Shape {
    int32_t numberVertices;
    int32_t numberIndices;
    Vector3* vertices;
    Vector3* normals;
    Vector3* tangent;
    Vector3* bitangent;
    Vector2* texture;
    uint16_t* indices;
};

void mnCreateSphere(float size, int numberSlices, Shape& shape) {

    const float angleStep = (2.0f * 3.1415926535897932384626433832795f) / (float) numberSlices;
    const int numberParallels = numberSlices / 2;

    int numberVertices = (numberParallels + 1) * (numberSlices + 1);
    int numberIndices = numberParallels * numberSlices * 6;

    shape.numberVertices = numberVertices;
    shape.numberIndices = numberIndices;
    shape.vertices = (Vector3*)malloc(sizeof(Vector3) * numberVertices);
    shape.normals = (Vector3*)malloc(sizeof(Vector3) * numberVertices);
    shape.tangent = (Vector3*)malloc(sizeof(Vector3) * numberVertices);
    shape.bitangent = (Vector3*)malloc(sizeof(Vector3) * numberVertices);
    shape.texture = (Vector2*)malloc(sizeof(Vector2) * numberVertices);
    shape.indices = (uint16_t*)malloc(sizeof(uint16_t) * numberIndices);

    for (int i = 0; i < numberParallels + 1; i++) {
        for (int j = 0; j < numberSlices + 1; j++) {
            int index = i * (numberSlices + 1) + j;

            shape.vertices[index].x = size * sinf(angleStep * i) * sinf(angleStep * j);
            shape.vertices[index].y = size * cosf(angleStep * i);
            shape.vertices[index].z = size * sinf(angleStep * i) * cosf(angleStep * j);

            shape.normals[index].x = shape.vertices[index].x / size;
            shape.normals[index].y = shape.vertices[index].y / size;
            shape.normals[index].z = shape.vertices[index].z / size;

            shape.texture[index].x = (float) j / (float) numberSlices;
            shape.texture[index].y = 1.0f - (float) i / (float) numberParallels;

            //TODO fix this
            Vector3 matrix[3] = {
                    1, 0, 0,
                    0, 1, 0,
                    0, 0, 1,
            };

            float angle = shape.texture[index].x;
            float cos = cosf(angle);
            float sin = sinf(angle);

            matrix[0].x = cos;
            matrix[0].z = sin;
            matrix[2].x = -sin;
            matrix[2].z = cos;

            Vector3 v = {1, 0, 0};
            shape.tangent[index].x = matrix[0].x*v.x + matrix[0].y*v.y + matrix[0].z*v.z;
            shape.tangent[index].y = matrix[1].x*v.x + matrix[1].y*v.y + matrix[1].z*v.z;
            shape.tangent[index].z = matrix[2].x*v.x + matrix[2].y*v.y + matrix[2].z*v.z;
        }
    }

    int index = 0;
    for (int i = 0; i < numberParallels; i++) {
        for (int j = 0; j < numberSlices; j++) {
            int i0 = i;
            int i1 = i + 1;
            int j0 = j;
            int j1 = j + 1;

            shape.indices[index++] = i0 * (numberSlices + 1) + j0;
            shape.indices[index++] = i1 * (numberSlices + 1) + j0;
            shape.indices[index++] = i1 * (numberSlices + 1) + j1;

            shape.indices[index++] = i0 * (numberSlices + 1) + j0;
            shape.indices[index++] = i1 * (numberSlices + 1) + j1;
            shape.indices[index++] = i0 * (numberSlices + 1) + j1;
        }
    }
}

void mnDestroyShape(Shape& shape) {
    free(shape.vertices); shape.vertices = nullptr;
    free(shape.normals); shape.normals = nullptr;
    free(shape.tangent); shape.tangent = nullptr;
    free(shape.bitangent); shape.bitangent = nullptr;
    free(shape.texture); shape.texture = nullptr;
    free(shape.indices); shape.indices = nullptr;
    shape.numberVertices = 0;
    shape.numberIndices = 0;
}

#endif //SHAPES_H
