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

struct MaterialTransparency {
    Program program;
    int mainUnit;
    Texture2D mainTex;
    Sampler mainSampler;
    int bumpUnit;
    Texture2D bumpMap;
    Sampler bumpSampler;
    float alpha;
};

struct Material {
    CommandBuffer* state;

    static Material* create(HeapAllocator& allocator, MaterialBumpedDiffuse* diffuse) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = CommandBuffer::create(allocator, 10);
        BindProgram::create(material->state, diffuse->program);
        SetDepthTest::create(material->state, true, GL_LEQUAL);
        SetBlend::create(material->state, false, 0, GL_NONE, GL_NONE, GL_NONE);
        SetBlend::create(material->state, false, 1, GL_NONE, GL_NONE, GL_NONE);
        SetBlend::create(material->state, false, 2, GL_NONE, GL_NONE, GL_NONE);

        if(diffuse->mainUnit != -1) {
            BindTexture::create(material->state, diffuse->program, diffuse->mainTex, diffuse->mainUnit);
            BindSampler::create(material->state, diffuse->program, diffuse->mainSampler, diffuse->mainUnit);
        }

        if(diffuse->bumpUnit != -1) {
            BindTexture::create(material->state, diffuse->program, diffuse->bumpMap, diffuse->bumpUnit);
            BindSampler::create(material->state, diffuse->program, diffuse->bumpSampler, diffuse->bumpUnit);
        }

        return material;
    }

    static Material* create(HeapAllocator& allocator, MaterialTransparency* transparency) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->state = CommandBuffer::create(allocator, 10);
        BindProgram::create(material->state, transparency->program);
        SetDepthTest::create(material->state, true, GL_LEQUAL);
        SetBlend::create(material->state, true, 0, GL_FUNC_ADD, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        if(transparency->mainUnit != -1) {
            BindTexture::create(material->state, transparency->program, transparency->mainTex, transparency->mainUnit);
            BindSampler::create(material->state, transparency->program, transparency->mainSampler, transparency->mainUnit);
        }

        if(transparency->bumpUnit != -1) {
            BindTexture::create(material->state, transparency->program, transparency->bumpMap, transparency->bumpUnit);
            BindSampler::create(material->state, transparency->program, transparency->bumpSampler, transparency->bumpUnit);
        }

        return material;
    }

    static void destroy(HeapAllocator& allocator, Material* material) {
        allocator.deallocate(material->state);
        allocator.deallocate(material);
    }
};

#endif //MATERIAL_H
