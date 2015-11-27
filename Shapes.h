//
// Created by Marrony Neris on 11/24/15.
//

#ifndef SHAPES_H
#define SHAPES_H

#include <vector>

void createSphere(float size, int numberSlices,
                  std::vector<Vector3>& vertex, std::vector<Vector3>& normal,
                  std::vector<Vector3>& tangent, std::vector<Vector2>& texture,
                  std::vector<uint16_t>& indices) {

    int numberParallels = numberSlices / 2;
    int numberVertices = (numberParallels + 1) * (numberSlices + 1);
    int numberIndices = numberParallels * numberSlices * 6;
    float angleStep = (2.0f * 3.1415926535897932384626433832795f) / (float) numberSlices;

    vertex.resize(numberVertices);
    normal.resize(numberVertices);
    tangent.resize(numberVertices);
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

            //TODO fix this
            Vector3 matrix[3] = {
                    1, 0, 0,
                    0, 1, 0,
                    0, 0, 1,
            };

            float angle = texture[index].x;
            float cos = cosf(angle);
            float sin = sinf(angle);

            matrix[0].x = cos;
            matrix[0].z = sin;
            matrix[2].x = -sin;
            matrix[2].z = cos;

            Vector3 v = {1, 0, 0};
            tangent[index].x = matrix[0].x*v.x + matrix[0].y*v.y + matrix[0].z*v.z;
            tangent[index].y = matrix[1].x*v.x + matrix[1].y*v.y + matrix[1].z*v.z;
            tangent[index].z = matrix[2].x*v.x + matrix[2].y*v.y + matrix[2].z*v.z;
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
