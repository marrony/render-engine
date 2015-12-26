//
// Created by Marrony Neris on 12/22/15.
//

#include "Wavefront.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <map>

#define MAX_BUFFER 1024

static void mnInitWavefrontObject(WavefrontObject* object) {
    object->numberMaterials = 0;
    object->materials = nullptr;

    object->numberGroups = 0;
    object->groups = nullptr;

    object->numberVertices = 0;
    object->vertices = nullptr;
    object->normals = nullptr;
    object->tangent = nullptr;
    object->bitangent = nullptr;
    object->texture = nullptr;

    object->numberIndices = 0;
    object->indices = nullptr;
}

static WavefrontGroup* mnAddWavefrontGroup(HeapAllocator& allocator, WavefrontObject* object) {
    WavefrontGroup* groups = (WavefrontGroup*)allocator.reallocate(object->groups, sizeof(WavefrontGroup) * (object->numberGroups+1));

    if(groups != nullptr) {
        object->groups = groups;

        return &groups[object->numberGroups++];
    }

    return nullptr;
}

static WavefrontObject* mnAddWavefrontObject(HeapAllocator& allocator, Wavefront* wavefront) {
    WavefrontObject* objects = (WavefrontObject*)allocator.reallocate(wavefront->objects, sizeof(WavefrontObject) * (wavefront->numberObjects+1));

    if(objects != nullptr) {
        wavefront->objects = objects;

        mnInitWavefrontObject(&wavefront->objects[wavefront->numberObjects]);

        return &wavefront->objects[wavefront->numberObjects++];
    }

    return nullptr;
}

struct WavefrontVertexIndex {
    int vertex;
    int normal;
    int texture;

    bool operator<(const WavefrontVertexIndex& other) const {
        if(vertex != other.vertex)
            return vertex < other.vertex;

        if(normal != other.normal)
            return normal < other.normal;

        if(texture != other.texture)
            return texture < other.texture;

        return false;
    }
};

static int mnWavefrontVertexIndexFind(std::map<WavefrontVertexIndex, int>& faces, WavefrontVertexIndex& face) {
    std::map<WavefrontVertexIndex, int>::iterator it = faces.find(face);

    if(it != faces.end())
        return it->second;

    return -1;
}

struct WavefrontTemporary {
    int allocatedVertices;
    int allocatedIndices;
    int verticesParsed;
    int normalsParsed;
    int texturesParsed;
    int indexCount;
    int indexOffset;
    int vertexCount;

    bool forceNotIndexed;

    std::map<WavefrontVertexIndex, int> indicesHashed;

    WavefrontVertexIndex* allVertexIndices;
    Vector3* vertices;
    Vector3* normals;
    Vector2* textures;
    uint16_t* indices;
};

static void mnWavefrontTemporaryInit(HeapAllocator& allocator, WavefrontTemporary* temporary, bool forceNotIndexed) {
    temporary->allocatedVertices = 500*1024;
    temporary->allocatedIndices = 10*500*1024;
    temporary->verticesParsed = 0;
    temporary->normalsParsed = 0;
    temporary->texturesParsed = 0;
    temporary->indexCount = 0;
    temporary->indexOffset = 0;
    temporary->vertexCount = 0;
    temporary->forceNotIndexed = forceNotIndexed;

    temporary->allVertexIndices = (WavefrontVertexIndex*)allocator.allocate(sizeof(WavefrontVertexIndex) * temporary->allocatedIndices);
    temporary->vertices = (Vector3*)allocator.allocate(sizeof(Vector3) * temporary->allocatedVertices);
    temporary->normals = (Vector3*)allocator.allocate(sizeof(Vector3) * temporary->allocatedVertices);
    temporary->textures = (Vector2*)allocator.allocate(sizeof(Vector2) * temporary->allocatedVertices);
    temporary->indices = nullptr;

    if (!forceNotIndexed)
        temporary->indices = (uint16_t*)allocator.allocate(sizeof(uint16_t) * temporary->allocatedIndices);
}

static void mnWavefrontTemporaryDestroy(HeapAllocator& allocator, WavefrontTemporary* temporary) {
    allocator.deallocate(temporary->allVertexIndices);
    allocator.deallocate(temporary->vertices);
    allocator.deallocate(temporary->normals);
    allocator.deallocate(temporary->textures);

    if (temporary->indices != nullptr)
        allocator.deallocate(temporary->indices);
}

static void mnWavefrontCopy(HeapAllocator& allocator, WavefrontObject* currentObject, WavefrontTemporary* temporary) {
    currentObject->numberVertices = temporary->vertexCount;
    currentObject->vertices = (Vector3*)allocator.allocate(sizeof(Vector3) * temporary->vertexCount);
    currentObject->normals = (Vector3*)allocator.allocate(sizeof(Vector3) * temporary->vertexCount);
    currentObject->tangent = (Vector3*)allocator.allocate(sizeof(Vector3) * temporary->vertexCount);
    currentObject->texture = (Vector2*)allocator.allocate(sizeof(Vector2) * temporary->vertexCount);

    if(!temporary->forceNotIndexed) {
        currentObject->numberIndices = temporary->indexCount;
        currentObject->indices = (uint16_t*)allocator.allocate(sizeof(uint16_t) * temporary->indexCount);
        memcpy(currentObject->indices, temporary->indices, sizeof(uint16_t) * temporary->indexCount);
    }

    for(int i = 0; i < temporary->vertexCount; i++) {
        WavefrontVertexIndex vertexIndex = temporary->allVertexIndices[i];
        int index = temporary->forceNotIndexed ? i : temporary->indicesHashed[vertexIndex];

        assert(index < temporary->indexCount);

        if(currentObject->vertices) {
            assert(vertexIndex.vertex <= temporary->verticesParsed);
            currentObject->vertices[index] = temporary->vertices[vertexIndex.vertex - 1];
        }

        if(temporary->normalsParsed > 0 && currentObject->normals) {
            assert(vertexIndex.normal <= temporary->normalsParsed);
            currentObject->normals[index] = temporary->normals[vertexIndex.normal - 1];
            currentObject->tangent[index] = {0, 0, 0};
        } else {
            currentObject->normals[index] = {0, 0, 0};
            currentObject->tangent[index] = {0, 0, 0};
        }

        if(temporary->texturesParsed > 0 && currentObject->texture) {
            assert(vertexIndex.texture <= temporary->texturesParsed);
            currentObject->texture[index] = temporary->textures[vertexIndex.texture - 1];
        } else {
            currentObject->texture[index] = {0, 0};
        }
    }

    int indexCount = temporary->indexCount - temporary->indexOffset;

    if(indexCount > 0) {
        WavefrontGroup* currentGroup = mnAddWavefrontGroup(allocator, currentObject);
        currentGroup->startIndices = temporary->indexOffset;
        currentGroup->numberIndices = indexCount;
    }

    temporary->vertexCount = 0;
    temporary->indexCount = 0;
    temporary->indexOffset = 0;
}

bool mnLoadWavefront(HeapAllocator& allocator, const char* filename, Wavefront& wavefront, bool forceNotIndexed) {
    FILE* file = fopen(filename, "r");
    assert(file != NULL);

    WavefrontTemporary temporary;
    mnWavefrontTemporaryInit(allocator, &temporary, forceNotIndexed);

    wavefront.numberObjects = 0;
    wavefront.objects = nullptr;

    char id[32];
    char buffer[MAX_BUFFER];

    WavefrontObject* currentObject = nullptr;

    while(!feof(file)) {
        if(!fgets(buffer, MAX_BUFFER, file))
            break;

        if(strncmp(buffer, "vn", 2) == 0) {
            Vector3 normal;

            sscanf(buffer, "%s %f %f %f", id, &normal.x, &normal.y, &normal.z);

            assert(temporary.normalsParsed < temporary.allocatedVertices);
            temporary.normals[temporary.normalsParsed++] = normal;
        } else if(strncmp(buffer, "vt", 2) == 0) {
            Vector2 texture;

            sscanf(buffer, "%s %f %f", id, &texture.x, &texture.y);

            assert(temporary.texturesParsed < temporary.allocatedVertices);
            temporary.textures[temporary.texturesParsed++] = texture;
        } else if(strncmp(buffer, "v", 1) == 0) {
            Vector3 vertex;

            sscanf(buffer, "%s %f %f %f", id, &vertex.x, &vertex.y, &vertex.z);

            assert(temporary.verticesParsed < temporary.allocatedVertices);
            temporary.vertices[temporary.verticesParsed++] = vertex;
        } else if(strncmp(buffer, "f", 1) == 0) {
            const char* delimiters = " \r\n";

            char* token = strtok(buffer, delimiters);
            assert(token[0] == 'f');

            int indexParsed = 0;
            WavefrontVertexIndex vertexIndex[3];

            while((token = strtok(nullptr, delimiters)) != nullptr) {
                vertexIndex[indexParsed].vertex = 0;
                vertexIndex[indexParsed].normal = 0;
                vertexIndex[indexParsed].texture = 0;

                bool parsed =
                    sscanf(token, "%d/%d/%d", &vertexIndex[indexParsed].vertex, &vertexIndex[indexParsed].texture, &vertexIndex[indexParsed].normal) == 3 ||
                    sscanf(token, "%d//%d", &vertexIndex[indexParsed].vertex, &vertexIndex[indexParsed].normal) == 2 ||
                    sscanf(token, "%d/%d", &vertexIndex[indexParsed].vertex, &vertexIndex[indexParsed].texture) == 2 ||
                    sscanf(token, "%d", &vertexIndex[indexParsed].vertex) == 1;

                if(!parsed)
                    break;

                indexParsed++;

                if(indexParsed >= 3) {
                    for(int i = 0; i < 3; i++) {
                        temporary.allVertexIndices[temporary.vertexCount] = vertexIndex[i];

                        int index = mnWavefrontVertexIndexFind(temporary.indicesHashed, vertexIndex[i]);

                        if (forceNotIndexed || index == -1) {
                            temporary.indicesHashed[vertexIndex[i]] = temporary.vertexCount;
                            index = temporary.vertexCount;
                            temporary.vertexCount++;
                        }

                        if (!forceNotIndexed) {
                            assert(temporary.indexCount < temporary.allocatedIndices);
                            temporary.indices[temporary.indexCount] = index;
                        }

                        temporary.indexCount++;
                    }

                    vertexIndex[1] = vertexIndex[2];
                    indexParsed = 2;
                }
            }
        } else if(strncmp(buffer, "g", 1) == 0) {
            char name[32];

            sscanf(buffer, "%s %s", id, name);

            int indexCount = temporary.indexCount - temporary.indexOffset;

            if(indexCount > 0) {
                if(currentObject == nullptr)
                    currentObject = mnAddWavefrontObject(allocator, &wavefront);

                WavefrontGroup* currentGroup = mnAddWavefrontGroup(allocator, currentObject);
                currentGroup->startIndices = temporary.indexOffset;
                currentGroup->numberIndices = indexCount;

                temporary.indexOffset = temporary.indexCount;
            }
        } else if(strncmp(buffer, "s", 1) == 0) {
            char name[32];

            sscanf(buffer, "%s %s", id, name);

            int indexCount = temporary.indexCount - temporary.indexOffset;

            if(indexCount > 0) {
                if(currentObject == nullptr)
                    currentObject = mnAddWavefrontObject(allocator, &wavefront);

                WavefrontGroup* currentGroup = mnAddWavefrontGroup(allocator, currentObject);
                currentGroup->startIndices = temporary.indexOffset;
                currentGroup->numberIndices = indexCount;

                temporary.indexOffset = temporary.indexCount;
            }
        } else if(strncmp(buffer, "o", 1) == 0) {
            char name[32];

            sscanf(buffer, "%s %s", id, name);

            if(currentObject == nullptr)
                currentObject = mnAddWavefrontObject(allocator, &wavefront);

            mnWavefrontCopy(allocator, currentObject, &temporary);

            currentObject = nullptr;
        }
    }

    if(currentObject == nullptr)
        currentObject = mnAddWavefrontObject(allocator, &wavefront);

    mnWavefrontCopy(allocator, currentObject, &temporary);

    mnWavefrontTemporaryDestroy(allocator, &temporary);

    return false;
}

void mnDestroyWavefront(HeapAllocator& allocator, Wavefront& wavefront) {
    for(int i = 0; i < wavefront.numberObjects; i++) {
        allocator.deallocate(wavefront.objects[i].groups);
        allocator.deallocate(wavefront.objects[i].vertices);
        allocator.deallocate(wavefront.objects[i].normals);
        allocator.deallocate(wavefront.objects[i].tangent);
        allocator.deallocate(wavefront.objects[i].texture);

        if (wavefront.objects[i].indices != nullptr)
            allocator.deallocate(wavefront.objects[i].indices);
    }

    allocator.deallocate(wavefront.objects);
}
