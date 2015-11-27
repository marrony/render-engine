//
// Created by Marrony Neris on 11/25/15.
//

#ifndef MATERIAL_H
#define MATERIAL_H

struct MaterialBumpedDiffuse {
    Program program;
    int mainUnit;
    Texture2D mainTex;
    Sampler mainSampler;
    int bumpUnit;
    Texture2D bumpMap;
    Sampler bumpSampler;
};

struct Material {
    CommandBuffer* state;

    static Material* create(HeapAllocator& allocator, MaterialBumpedDiffuse* diffuse) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = CommandBuffer::create(allocator, 5);
        BindProgram::create(material->state, diffuse->program);
        BindTexture::create(material->state, diffuse->program, diffuse->mainTex, diffuse->mainUnit);
        BindSampler::create(material->state, diffuse->program, diffuse->mainSampler, diffuse->mainUnit);
        BindTexture::create(material->state, diffuse->program, diffuse->bumpMap, diffuse->bumpUnit);
        BindSampler::create(material->state, diffuse->program, diffuse->bumpSampler, diffuse->bumpUnit);
        return material;
    }

    static void destroy(HeapAllocator& allocator, Material* material) {
        allocator.deallocate(material->state);
        allocator.deallocate(material);
    }
};

#endif //MATERIAL_H
