//
// Created by Marrony Neris on 11/10/15.
//

#ifndef MODEL_H
#define MODEL_H

struct Mesh {
    CommandBuffer* draw;

    static Mesh* create(LinearAllocator& allocator, int offset, int count) {
        Mesh* mesh = (Mesh*) allocator.allocate(sizeof(Mesh));

        mesh->draw = CommandBuffer::create(allocator, 1);
        DrawTriangles::create(mesh->draw, offset, count);

        return mesh;
    }
};

struct Model {
    CommandBuffer* state;
    int meshCount;
    Mesh* meshes[];

    static Model* create(LinearAllocator& allocator, VertexArray vertexArray, int meshCount) {
        Model* model = (Model*) allocator.allocate(sizeof(Model) + meshCount * sizeof(Mesh*));

        model->state = CommandBuffer::create(allocator, 1);
        BindVertexArray::create(model->state, vertexArray);
        model->meshCount = meshCount;

        return model;
    }
};

struct Material {
    CommandBuffer* state;

    static Material* create(LinearAllocator& allocator, Program program, Texture2D texture, Sampler sampler, int index) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = CommandBuffer::create(allocator, 3);
        BindProgram::create(material->state, program);
        BindTexture::create(material->state, program, texture, index);
        BindSampler::create(material->state, program, sampler, index);

        return material;
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

    static ModelInstance* create(LinearAllocator& allocator, Model* model) {
        size_t nbytes = sizeof(ModelInstance) + model->meshCount * sizeof(Material*);
        ModelInstance* modelInstance = (ModelInstance*) allocator.allocate(nbytes);

        modelInstance->state = CommandBuffer::create(allocator, 0);
        modelInstance->model = model;

        return modelInstance;
    }
};

#endif //MODEL_H
