//
// Created by Marrony Neris on 11/10/15.
//

#ifndef MODEL_H
#define MODEL_H

struct Mesh {
    State* state;
    DrawCommand* draw;

    static Mesh* create(LinearAllocator& allocator, int offset, int count) {
        Mesh* mesh = (Mesh*) allocator.allocate(sizeof(Mesh));

        mesh->state = State::create(allocator, 0);
        mesh->draw = DrawTriangles::create(allocator, offset, count);

        return mesh;
    }
};

struct Model {
    State* state;
    int meshCount;
    Mesh* meshes[];

    static Model* create(LinearAllocator& allocator, VertexArray vertexArray, int meshCount) {
        Model* model = (Model*) allocator.allocate(sizeof(Model) + meshCount * sizeof(Mesh*));

        model->state = State::create(allocator, 1);
        model->state->commands[0] = BindVertexArray::create(allocator, vertexArray);
        model->meshCount = meshCount;

        return model;
    }
};

struct Material {
    State* state;

    static Material* create(LinearAllocator& allocator, Program program, Texture2D texture, Sampler sampler, int index) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = State::create(allocator, 3);
        material->state->commands[0] = BindProgram::create(allocator, program);
        material->state->commands[1] = BindTexture::create(allocator, program, texture, index);
        material->state->commands[2] = BindSampler::create(allocator, program, sampler, index);

        return material;
    }
};

struct ModelInstance {
    State* state;
    Model* model;
    Material* materials[];

    void draw(uint64_t key, RenderQueue& renderQueue, State* globalState) {
        for (int i = 0; i < model->meshCount; i++) {
            Mesh* mesh = model->meshes[i];
            Material* material = materials[i];

            State* states[] = {
                    globalState,
                    state,
                    model->state,
                    material->state,
                    mesh->state,
            };

            renderQueue.submit(key, mesh->draw, states, 5);
        }
    }

    static ModelInstance* create(LinearAllocator& allocator, Model* model) {
        size_t nbytes = sizeof(ModelInstance) + model->meshCount * sizeof(Material*);
        ModelInstance* modelInstance = (ModelInstance*) allocator.allocate(nbytes);

        modelInstance->state = State::create(allocator, 0);
        modelInstance->model = model;

        return modelInstance;
    }
};

#endif //MODEL_H
