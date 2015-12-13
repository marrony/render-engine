//
// Created by Marrony Neris on 11/25/15.
//

#ifndef MODEL_INSTANCE_H
#define MODEL_INSTANCE_H

struct ModelInstance {
    struct PerMesh {
        CommandBuffer* draw;
        Material* material;
    };

    int instanceCount;
    CommandBuffer* state;
    Model* model;
    PerMesh perMesh[];

    static void draw(ModelInstance* modelInstance, uint64_t key, RenderQueue& renderQueue, CommandBuffer* globalState) {
        Model* model = modelInstance->model;

        for (int i = 0; i < model->meshCount; i++) {
            Material* material = modelInstance->perMesh[i].material;
            CommandBuffer* draw = modelInstance->perMesh[i].draw;

            CommandBuffer* commandBuffers[] = {
                    globalState,
                    modelInstance->state,
                    model->state,
                    material->state,
                    draw,
            };

            renderQueue.submit(key, commandBuffers, 5);
        }
    }

    static ModelInstance* create(HeapAllocator& allocator, Model* model, ConstantBuffer constantBuffer, int bindingPoint, const void* data, size_t size) {
        return createInstanced(allocator, model, 1, constantBuffer, bindingPoint, data, size);
    }

    static ModelInstance* createInstanced(HeapAllocator& allocator, Model* model, int instanceCount, ConstantBuffer constantBuffer, int bindingPoint, const void* data, size_t size) {
        size_t nbytes = sizeof(ModelInstance) + model->meshCount * sizeof(PerMesh);

        ModelInstance* modelInstance = (ModelInstance*) allocator.allocate(nbytes);

        modelInstance->state = CommandBuffer::create(allocator, 2);
        CopyConstantBuffer::create(modelInstance->state, constantBuffer, data, size);
        BindConstantBuffer::create(modelInstance->state, constantBuffer, bindingPoint);
        modelInstance->model = model;
        modelInstance->instanceCount = instanceCount;

        for (int i = 0; i < model->meshCount; i++) {
            modelInstance->perMesh[i].material = nullptr;

            Mesh* mesh = &model->meshes[i];

            if(instanceCount > 1) {
                modelInstance->perMesh[i].draw = CommandBuffer::create(allocator, 1);
                DrawTrianglesInstanced::create(modelInstance->perMesh[i].draw, mesh->offset, mesh->count, instanceCount);
            } else {
                modelInstance->perMesh[i].draw = mesh->draw;
            }
        }

        return modelInstance;
    }

    static void destroy(HeapAllocator& allocator, ModelInstance* modelInstance) {
        allocator.deallocate(modelInstance->state);
        if(modelInstance->instanceCount > 1) {
            for(int i = 0; i < modelInstance->model->meshCount; i++)
                CommandBuffer::destroy(allocator, modelInstance->perMesh[i].draw);
        }
        allocator.deallocate(modelInstance);
    }

    static void setMaterial(ModelInstance* modelInstance, int index, Material* material) {
        modelInstance->perMesh[index].material = material;
    }
};

#endif //MODEL_INSTANCE_H
