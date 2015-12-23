//
// Created by Marrony Neris on 11/17/15.
//

#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "Shapes.h"
#include "Model.h"
#include "Material.h"
#include "ModelInstance.h"
#include "Wavefront.h"

const int MAX_MODEL_NAME = 8;

class ModelManager {
public:
    ModelManager(HeapAllocator& allocator, Device& device) : allocator(allocator), device(device) {
        modelCount = 0;
        modelAllocated = 16;
        models = (Resource*) allocator.allocate(modelAllocated * sizeof(Resource));
    }

    ~ModelManager() {
        assert(modelCount == 0);

        allocator.deallocate(models);
    }

    Model* findModel(const char* name) {
        for(uint32_t i = 0; i < modelCount; i++) {
            if(strncmp(models[i].name, name, MAX_MODEL_NAME) == 0) {
                models[i].refs++;
                return models[i].model;
            }
        }
        return nullptr;
    }

    Model* createSphere(const char* name, float size, int numberSlices) {
        assert(strlen(name) <= MAX_MODEL_NAME);

        uint32_t index = getSlot();

        Shape shape;

        mnCreateSphere(size, numberSlices, shape);

        VertexBuffer vertexBuffer = device.createStaticVertexBuffer(shape.numberVertices*sizeof(Vector3), shape.vertices);
        VertexBuffer normalBuffer = device.createStaticVertexBuffer(shape.numberVertices*sizeof(Vector3), shape.normals);
        VertexBuffer tangentBuffer = device.createStaticVertexBuffer(shape.numberVertices*sizeof(Vector3), shape.tangent);
        VertexBuffer textureBuffer = device.createStaticVertexBuffer(shape.numberVertices*sizeof(Vector2), shape.texture);
        IndexBuffer indexBuffer = device.createIndexBuffer(shape.numberIndices*sizeof(uint16_t), shape.indices);

        VertexDeclaration vertexDeclaration[4];
        vertexDeclaration[0].buffer = vertexBuffer;
        vertexDeclaration[0].format = VertexFloat3;
        vertexDeclaration[0].offset = 0;
        vertexDeclaration[0].stride = 0;

        vertexDeclaration[1].buffer = textureBuffer;
        vertexDeclaration[1].format = VertexFloat2;
        vertexDeclaration[1].offset = 0;
        vertexDeclaration[1].stride = 0;

        vertexDeclaration[2].buffer = normalBuffer;
        vertexDeclaration[2].format = VertexFloat3;
        vertexDeclaration[2].offset = 0;
        vertexDeclaration[2].stride = 0;

        vertexDeclaration[3].buffer = tangentBuffer;
        vertexDeclaration[3].format = VertexFloat3;
        vertexDeclaration[3].offset = 0;
        vertexDeclaration[3].stride = 0;

        models[index].vertexBuffer[0] = vertexBuffer;
        models[index].vertexBuffer[1] = textureBuffer;
        models[index].vertexBuffer[2] = normalBuffer;
        models[index].vertexBuffer[3] = tangentBuffer;
        models[index].indexBuffer = indexBuffer;
        models[index].vertexArray = device.createVertexArray(vertexDeclaration, 4, indexBuffer);
        models[index].model = Model::create(allocator, models[index].vertexArray, 1);
        strncpy(models[index].name, name, MAX_MODEL_NAME);
        models[index].refs = 1;

        Model::addMesh(allocator, models[index].model, 0, 0, shape.numberIndices);

        mnDestroyShape(shape);

        return models[index].model;
    }

    Model* loadWavefront(const char* filename) {
        uint32_t index = getSlot();

        Wavefront obj;

        mnLoadWavefront(allocator, filename, obj);

        WavefrontObject* currentObj = obj.objects;

        VertexBuffer vertexBuffer = device.createStaticVertexBuffer(currentObj->numberVertices*sizeof(Vector3), currentObj->vertices);
        VertexBuffer normalBuffer = device.createStaticVertexBuffer(currentObj->numberVertices*sizeof(Vector3), currentObj->normals);
        VertexBuffer tangentBuffer = device.createStaticVertexBuffer(currentObj->numberVertices*sizeof(Vector3), currentObj->tangent);
        VertexBuffer textureBuffer = device.createStaticVertexBuffer(currentObj->numberVertices*sizeof(Vector2), currentObj->texture);
        IndexBuffer indexBuffer = device.createIndexBuffer(currentObj->numberIndices*sizeof(uint16_t), currentObj->indices);

        VertexDeclaration vertexDeclaration[4];
        vertexDeclaration[0].buffer = vertexBuffer;
        vertexDeclaration[0].format = VertexFloat3;
        vertexDeclaration[0].offset = 0;
        vertexDeclaration[0].stride = 0;

        vertexDeclaration[1].buffer = textureBuffer;
        vertexDeclaration[1].format = VertexFloat2;
        vertexDeclaration[1].offset = 0;
        vertexDeclaration[1].stride = 0;

        vertexDeclaration[2].buffer = normalBuffer;
        vertexDeclaration[2].format = VertexFloat3;
        vertexDeclaration[2].offset = 0;
        vertexDeclaration[2].stride = 0;

        vertexDeclaration[3].buffer = tangentBuffer;
        vertexDeclaration[3].format = VertexFloat3;
        vertexDeclaration[3].offset = 0;
        vertexDeclaration[3].stride = 0;

        models[index].vertexBuffer[0] = vertexBuffer;
        models[index].vertexBuffer[1] = textureBuffer;
        models[index].vertexBuffer[2] = normalBuffer;
        models[index].vertexBuffer[3] = tangentBuffer;
        models[index].indexBuffer = indexBuffer;
        models[index].vertexArray = device.createVertexArray(vertexDeclaration, 4, indexBuffer);

        models[index].model = Model::create(allocator, models[index].vertexArray, currentObj->numberGroups);
        strncpy(models[index].name, "venus", MAX_MODEL_NAME);
        models[index].refs = 1;

        for(int i = 0; i < currentObj->numberGroups; i++) {
            WavefrontGroup* currentGroup = &currentObj->groups[i];

            Model::addMesh(allocator, models[index].model, i, currentGroup->startIndices, currentGroup->numberIndices);
        }

        mnDestroyWavefront(allocator, obj);

        return models[index].model;
    }

    Model* createQuad(const char* name) {
        assert(strlen(name) <= MAX_MODEL_NAME);

        uint32_t index = getSlot();

        Vector3 vertex[] = {
                -1.0, -1.0, 0.0,
                -1.0, +1.0, 0.0,
                +1.0, +1.0, 0.0,
                +1.0, -1.0, 0.0,
        };
        Vector2 texture[] = {
                0, 0,
                0, 1,
                1, 1,
                1, 0
        };
        Vector3 normal[] = {
                0.0, 0.0, 1.0,
                0.0, 0.0, 1.0,
                0.0, 0.0, 1.0,
                0.0, 0.0, 1.0,
        };
        Vector3 tangent[] = {
                1.0, 0.0, 0.0,
                1.0, 0.0, 0.0,
                1.0, 0.0, 0.0,
                1.0, 0.0, 0.0,
        };
        uint16_t indices[] = {0, 1, 3, 3, 1, 2};

        VertexBuffer vertexBuffer = device.createStaticVertexBuffer(sizeof(vertex), vertex);
        VertexBuffer textureBuffer = device.createStaticVertexBuffer(sizeof(texture), texture);
        VertexBuffer normalBuffer = device.createStaticVertexBuffer(sizeof(normal), normal);
        VertexBuffer tangentBuffer = device.createStaticVertexBuffer(sizeof(tangent), tangent);
        IndexBuffer indexBuffer = device.createIndexBuffer(sizeof(indices), indices);

        VertexDeclaration vertexDeclaration[4] = {};
        vertexDeclaration[0].buffer = vertexBuffer;
        vertexDeclaration[0].format = VertexFloat3;
        vertexDeclaration[0].offset = 0;
        vertexDeclaration[0].stride = 0;

        vertexDeclaration[1].buffer = textureBuffer;
        vertexDeclaration[1].format = VertexFloat2;
        vertexDeclaration[1].offset = 0;
        vertexDeclaration[1].stride = 0;

        vertexDeclaration[2].buffer = normalBuffer;
        vertexDeclaration[2].format = VertexFloat3;
        vertexDeclaration[2].offset = 0;
        vertexDeclaration[2].stride = 0;

        vertexDeclaration[3].buffer = tangentBuffer;
        vertexDeclaration[3].format = VertexFloat3;
        vertexDeclaration[3].offset = 0;
        vertexDeclaration[3].stride = 0;


        models[index].vertexBuffer[0] = vertexBuffer;
        models[index].vertexBuffer[1] = textureBuffer;
        models[index].vertexBuffer[2] = normalBuffer;
        models[index].vertexBuffer[3] = tangentBuffer;
        models[index].indexBuffer = indexBuffer;
        models[index].vertexArray = device.createVertexArray(vertexDeclaration, 4, indexBuffer);
        models[index].model = Model::create(allocator, models[index].vertexArray, 1);
        strncpy(models[index].name, name, MAX_MODEL_NAME);
        models[index].refs = 1;

        Model::addMesh(allocator, models[index].model, 0, 0, 6);

        return models[index].model;
    }

    void destroyModel(Model* model) {
        uint32_t index;

        if (findModel(model, index))
            destroy(index);
    }

    ModelInstance* createModelInstance(Model* model, int instanceCount, ConstantBuffer constantBuffer, int bindingPoint) {
        uint32_t index;

        if (findModel(model, index)) {
            models[index].refs++;

            if (instanceCount > 1)
                return ModelInstance::createInstanced(allocator, model, instanceCount, constantBuffer, bindingPoint);

            return ModelInstance::create(allocator, model, constantBuffer, bindingPoint);
        }

        return nullptr;
    }

    void destroyModelInstance(ModelInstance* modelInstance) {
        uint32_t index;

        if (findModel(modelInstance->model, index)) {
            destroy(index);

            ModelInstance::destroy(allocator, modelInstance);
        }
    }
private:
    struct Resource {
        char name[MAX_MODEL_NAME+1];
        VertexBuffer vertexBuffer[4];
        IndexBuffer indexBuffer;
        VertexArray vertexArray;
        Model* model;
        uint32_t refs;
    };

    void destroy(uint32_t index) {
        models[index].refs--;

        if (models[index].refs == 0) {
            destroy(&models[index]);

            modelCount--;
            std::swap(models[index], models[modelCount]);
        }
    }

    void destroy(Resource* resource) {
        device.destroyVertexArray(resource->vertexArray);
        device.destroyVertexBuffer(resource->vertexBuffer[0]);
        device.destroyVertexBuffer(resource->vertexBuffer[1]);
        device.destroyVertexBuffer(resource->vertexBuffer[2]);
        device.destroyVertexBuffer(resource->vertexBuffer[3]);
        device.destroyIndexBuffer(resource->indexBuffer);
        Model::destroy(allocator, resource->model);
    }

    uint32_t getSlot() {
        if(modelCount >= modelAllocated) {
            modelAllocated = modelAllocated * 3 / 2;
            models = (Resource*) allocator.reallocate(models, modelAllocated * sizeof(Resource));
        }
        return modelCount++;
    }

    bool findModel(Model* model, uint32_t& index) {
        for (uint32_t i = 0; i < modelCount; i++) {
            if (models[i].model == model) {
                index = i;
                return true;
            }
        }
        return false;
    }

    HeapAllocator& allocator;
    Device& device;

    Resource* models;
    uint32_t modelCount;
    uint32_t modelAllocated;
};

#endif //MODEL_MANAGER_H
