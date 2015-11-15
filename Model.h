//
// Created by Marrony Neris on 11/10/15.
//

#ifndef MODEL_H
#define MODEL_H

struct Mesh {
    CommandBuffer* draw;

    static Mesh* create(HeapAllocator& allocator, int offset, int count) {
        Mesh* mesh = (Mesh*) allocator.allocate(sizeof(Mesh));

        mesh->draw = CommandBuffer::create(allocator, 1);
        DrawTriangles::create(mesh->draw, offset, count);

        return mesh;
    }

    static void destroy(HeapAllocator& allocator, Mesh* mesh) {
        allocator.deallocate(mesh->draw);
        allocator.deallocate(mesh);
    }
};

struct Model {
    CommandBuffer* state;
    int meshCount;
    Mesh* meshes[];

    static Model* create(HeapAllocator& allocator, VertexArray vertexArray, int meshCount) {
        Model* model = (Model*) allocator.allocate(sizeof(Model) + meshCount * sizeof(Mesh*));

        model->state = CommandBuffer::create(allocator, 1);
        BindVertexArray::create(model->state, vertexArray);
        model->meshCount = meshCount;

        return model;
    }

    static void destroy(HeapAllocator& allocator, Model* model) {
        allocator.deallocate(model->state);
        for(int i = 0; i < model->meshCount; i++)
            Mesh::destroy(allocator, model->meshes[i]);
        allocator.deallocate(model);
    }
};

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

struct ModelInstance {
    CommandBuffer* state;
    Model* model;
    Material* materials[];

    void draw(uint64_t key, RenderQueue& renderQueue, CommandBuffer* globalState) {
        for (int i = 0; i < model->meshCount; i++) {
            Mesh* mesh = model->meshes[i];
            Material* material = materials[i];

            CommandBuffer* commandBuffers[] = {
                    globalState,
                    state,
                    model->state,
                    material->state,
                    mesh->draw,
            };

            renderQueue.submit(key, commandBuffers, 5);
        }
    }

    static ModelInstance* create(HeapAllocator& allocator, Model* model) {
        size_t nbytes = sizeof(ModelInstance) + model->meshCount * sizeof(Material*);
        ModelInstance* modelInstance = (ModelInstance*) allocator.allocate(nbytes);

        modelInstance->state = CommandBuffer::create(allocator, 0);
        modelInstance->model = model;

        return modelInstance;
    }

    static void destroy(HeapAllocator& allocator, ModelInstance* modelInstance) {
        allocator.deallocate(modelInstance->state);
        allocator.deallocate(modelInstance);
    }
};

#endif //MODEL_H
