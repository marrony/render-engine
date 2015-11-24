//
// Created by Marrony Neris on 11/10/15.
//

#ifndef MODEL_H
#define MODEL_H

struct Material {
    CommandBuffer* state;

    static Material* create(HeapAllocator& allocator, Program program, Texture2D texture, Sampler sampler, int index) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = CommandBuffer::create(allocator, 3);
        BindProgram::create(material->state, program);
        BindTexture::create(material->state, program, texture, index);
        BindSampler::create(material->state, program, sampler, index);

        return material;
    }

    static void destroy(HeapAllocator& allocator, Material* material) {
        allocator.deallocate(material->state);
        allocator.deallocate(material);
    }
};

struct Mesh {
    CommandBuffer* draw;
    int offset;
    int count;

    static void create(HeapAllocator& allocator, Mesh* mesh, int offset, int count) {
        mesh->offset = offset;
        mesh->count = count;
        mesh->draw = CommandBuffer::create(allocator, 1);
        DrawTriangles::create(mesh->draw, offset, count);
    }

    static void destroy(HeapAllocator& allocator, Mesh* mesh) {
        CommandBuffer::destroy(allocator, mesh->draw);
    }
};

struct Model {
    CommandBuffer* state;
    int meshCount;
    Mesh meshes[];

    static Model* create(HeapAllocator& allocator, VertexArray vertexArray, int meshCount) {
        Model* model = (Model*) allocator.allocate(sizeof(Model) + meshCount * sizeof(Mesh));

        model->state = CommandBuffer::create(allocator, 1);
        BindVertexArray::create(model->state, vertexArray);
        model->meshCount = meshCount;

        return model;
    }

    static void addMesh(HeapAllocator& allocator, Model* model, int index, int offset, int count) {
        Mesh::create(allocator, &model->meshes[index], offset, count);
    }

    static void destroy(HeapAllocator& allocator, Model* model) {
        allocator.deallocate(model->state);
        for(int i = 0; i < model->meshCount; i++)
            Mesh::destroy(allocator, &model->meshes[i]);
        allocator.deallocate(model);
    }

    static void draw(Model* model, uint64_t key, RenderQueue& renderQueue, CommandBuffer* globalState) {
        for (int i = 0; i < model->meshCount; i++) {
            CommandBuffer* commandBuffers[] = {
                    globalState,
                    model->state,
                    model->meshes[i].draw
            };

            renderQueue.submit(key, commandBuffers, 3);
        }
    }
};

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

    static ModelInstance* create(HeapAllocator& allocator, Model* model, ConstantBuffer constantBuffer, const void* data, size_t size) {
        return createInstanced(allocator, model, 1, constantBuffer, data, size);
    }

    static ModelInstance* createInstanced(HeapAllocator& allocator, Model* model, int instanceCount, ConstantBuffer constantBuffer, const void* data, size_t size) {
        size_t nbytes = sizeof(ModelInstance) + model->meshCount * sizeof(PerMesh);

        ModelInstance* modelInstance = (ModelInstance*) allocator.allocate(nbytes);

        modelInstance->state = CommandBuffer::create(allocator, 2);
        CopyConstantBuffer::create(modelInstance->state, constantBuffer, data, size);
        BindConstantBuffer::create(modelInstance->state, constantBuffer, 0);
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

#endif //MODEL_H
