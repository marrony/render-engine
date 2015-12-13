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
    int passCount;
    CommandBuffer* state[4];

    static Material* create(HeapAllocator& allocator, MaterialBumpedDiffuse* diffuse) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->passCount = 1;
        material->state[0] = CommandBuffer::create(allocator, 10);
        BindProgram::create(material->state[0], diffuse->program);
#if RIGHT_HANDED
        SetDepthTest::create(material->state[0], true, GL_LEQUAL);
#else
        SetDepthTest::create(material->state[0], true, GL_GEQUAL);
#endif
        SetBlend::disable(material->state[0], 0);
        SetBlend::disable(material->state[0], 1);
        SetBlend::disable(material->state[0], 2);

        if(diffuse->mainUnit != -1) {
            BindTexture::create(material->state[0], diffuse->program, diffuse->mainTex, diffuse->mainSampler, diffuse->mainUnit);
        }

        if(diffuse->bumpUnit != -1) {
            BindTexture::create(material->state[0], diffuse->program, diffuse->bumpMap, diffuse->bumpSampler, diffuse->bumpUnit);
        }

        return material;
    }

    static Material* create(HeapAllocator& allocator, MaterialTransparency* transparency) {
        Material* material = (Material*) allocator.allocate(sizeof(Material));

        material->passCount = 2;

        material->state[0] = CommandBuffer::create(allocator, 10);
        BindProgram::create(material->state[0], transparency->program);
#if RIGHT_HANDED
        SetDepthTest::create(material->state[0], true, GL_LEQUAL);
        SetCullFace::create(material->state[0], true, GL_FRONT, GL_CCW);
#else
        SetDepthTest::create(material->state[0], true, GL_GEQUAL);
        SetCullFace::create(material->state[0], true, GL_FRONT, GL_CW);
#endif
        SetBlend::create(material->state[0], true, 0, GL_FUNC_ADD, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        if(transparency->mainUnit != -1) {
            BindTexture::create(material->state[0], transparency->program, transparency->mainTex, transparency->mainSampler, transparency->mainUnit);
        }

        if(transparency->bumpUnit != -1) {
            BindTexture::create(material->state[0], transparency->program, transparency->bumpMap, transparency->bumpSampler, transparency->bumpUnit);
        }

        material->state[1] = CommandBuffer::create(allocator, 10);
        BindProgram::create(material->state[1], transparency->program);
#if RIGHT_HANDED
        SetDepthTest::create(material->state[1], true, GL_LEQUAL);
        SetCullFace::create(material->state[1], true, GL_BACK, GL_CCW);
#else
        SetDepthTest::create(material->state[1], true, GL_GEQUAL);
        SetCullFace::create(material->state[1], true, GL_BACK, GL_CW);
#endif
        SetBlend::create(material->state[1], true, 0, GL_FUNC_ADD, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        if(transparency->mainUnit != -1) {
            BindTexture::create(material->state[1], transparency->program, transparency->mainTex, transparency->mainSampler, transparency->mainUnit);
        }

        if(transparency->bumpUnit != -1) {
            BindTexture::create(material->state[1], transparency->program, transparency->bumpMap, transparency->bumpSampler, transparency->bumpUnit);
        }

        return material;
    }

    static void destroy(HeapAllocator& allocator, Material* material) {
        for(int i = 0; i < material->passCount; i++)
            allocator.deallocate(material->state[i]);
        allocator.deallocate(material);
    }
};

#endif //MATERIAL_H
