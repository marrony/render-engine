//
// Created by Marrony Neris on 11/17/15.
//

#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "Shapes.h"

class ModelManager {
public:
    ModelManager(HeapAllocator& allocator, Device& device) : allocator(allocator), device(device) {
        modelCount = 0;
        modelAllocated = 16;
        models = (Resource*) allocator.allocate(modelAllocated * sizeof(Resource));
    }

    ~ModelManager() {
        for(uint32_t i = 0; i < modelCount; i++)
            destroy(&models[i]);

        allocator.deallocate(models);
    }

    Model* createSphere(float size, int numberSlices) {
        if(modelCount >= modelAllocated) {
            modelAllocated = modelAllocated * 3 / 2;
            models = (Resource*) allocator.reallocate(models, modelAllocated * sizeof(Resource));
        }

        uint32_t index = modelCount++;

        std::vector<Vector3> vertexData;
        std::vector<Vector3> normalData;
        std::vector<Vector2> textureData;
        std::vector<uint16_t> indexData;

        ::createSphere(size, numberSlices, vertexData, normalData, textureData, indexData);

        VertexBuffer vertexBuffer = device.createStaticVertexBuffer(vertexData.size()*sizeof(Vector3), vertexData.data());
        VertexBuffer normalBuffer = device.createStaticVertexBuffer(normalData.size()*sizeof(Vector3), normalData.data());
        VertexBuffer textureBuffer = device.createStaticVertexBuffer(textureData.size()*sizeof(Vector2), textureData.data());
        IndexBuffer indexBuffer = device.createIndexBuffer(indexData.size()*sizeof(uint16_t), indexData.data());

        VertexDeclaration vertexDeclaration[3];
        vertexDeclaration[0].buffer = vertexBuffer;
        vertexDeclaration[0].format = VertexFloat3;
        vertexDeclaration[0].offset = 0;
        vertexDeclaration[0].stride = 0;

        vertexDeclaration[1].buffer = normalBuffer;
        vertexDeclaration[1].format = VertexFloat3;
        vertexDeclaration[1].offset = 0;
        vertexDeclaration[1].stride = 0;

        vertexDeclaration[2].buffer = textureBuffer;
        vertexDeclaration[2].format = VertexFloat2;
        vertexDeclaration[2].offset = 0;
        vertexDeclaration[2].stride = 0;

        models[index].vertexBuffer[0] = vertexBuffer;
        models[index].vertexBuffer[1] = normalBuffer;
        models[index].vertexBuffer[2] = textureBuffer;
        models[index].indexBuffer = indexBuffer;
        models[index].vertexArray = device.createVertexArray(vertexDeclaration, 3, indexBuffer);
        models[index].model = Model::create(allocator, models[index].vertexArray, 1);

        Model::addMesh(allocator, models[index].model, 0, 0, indexData.size());

        return models[index].model;
    }
private:
    struct Resource {
        VertexBuffer vertexBuffer[3];
        IndexBuffer indexBuffer;
        VertexArray vertexArray;
        Model* model;
    };

    void destroy(Resource* resource) {
        device.destroyVertexArray(resource->vertexArray);
        device.destroyVertexBuffer(resource->vertexBuffer[0]);
        device.destroyVertexBuffer(resource->vertexBuffer[1]);
        device.destroyVertexBuffer(resource->vertexBuffer[2]);
        device.destroyIndexBuffer(resource->indexBuffer);
        Model::destroy(allocator, resource->model);
    }

    HeapAllocator& allocator;
    Device& device;

    Resource* models;
    uint32_t modelCount;
    uint32_t modelAllocated;
};

#endif //MODEL_MANAGER_H
