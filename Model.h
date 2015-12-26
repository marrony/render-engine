//
// Created by Marrony Neris on 11/10/15.
//

#ifndef MODEL_H
#define MODEL_H

struct Mesh {
    CommandBuffer* draw;
    int offset;
    int count;

    static void create(HeapAllocator& allocator, Mesh* mesh, int offset, int count, bool useIndex = true) {
        mesh->offset = offset;
        mesh->count = count;
        mesh->draw = CommandBuffer::create(allocator, 1);
        if (useIndex)
            DrawTriangles::create(mesh->draw, offset, count);
        else
            DrawArrays::create(mesh->draw, GL_TRIANGLES, offset, count);
    }

    static void destroy(HeapAllocator& allocator, Mesh* mesh) {
        CommandBuffer::destroy(allocator, mesh->draw);
    }
};

struct Model {
    CommandBuffer* state;
    bool hasIndices;
    int meshCount;
    Mesh meshes[];

    static Model* create(HeapAllocator& allocator, VertexArray vertexArray, int meshCount, bool hasIndices = true) {
        Model* model = (Model*) allocator.allocate(sizeof(Model) + meshCount * sizeof(Mesh));

        model->state = CommandBuffer::create(allocator, 1);
        BindVertexArray::create(model->state, vertexArray);
        model->hasIndices = hasIndices;
        model->meshCount = meshCount;

        return model;
    }

    static void addMesh(HeapAllocator& allocator, Model* model, int index, int offset, int count) {
        Mesh::create(allocator, &model->meshes[index], offset, count, model->hasIndices);
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

#endif //MODEL_H
