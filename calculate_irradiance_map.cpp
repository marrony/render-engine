//
// Created by Marrony Neris on 12/18/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include <chrono>
#include <thread>
#include <algorithm>

#include "Vector.h"
#include "Matrix.h"
#include "Device.h"
#include "Allocator.h"
#include "TextureManager.h"

#define IMG_PATH "images/LancellottiChapel"
#define NUM_SAMPLES 128
#define OUTPUT_SIZE 128

void imageGet3(const Image& image, int x, int y, float pixels[3]) {
    int offsety = y*image.format*image.width;
    int offsetx = x*image.format;

    pixels[0] = image.pixels[offsety + offsetx + 0] / 255.0f;
    pixels[1] = image.pixels[offsety + offsetx + 1] / 255.0f;
    pixels[2] = image.pixels[offsety + offsetx + 2] / 255.0f;
}

void imageSet3(Image& image, int x, int y, float pixels[3]) {
    int offsety = y*image.format*image.width;
    int offsetx = x*image.format;

    image.pixels[offsety + offsetx + 0] = pixels[0] * 255.0f;
    image.pixels[offsety + offsetx + 1] = pixels[1] * 255.0f;
    image.pixels[offsety + offsetx + 2] = pixels[2] * 255.0f;
}

void imageSet3(Image& image, int x, int y, float r, float g, float b) {
    float pixels[3] = {r, g, b};
    imageSet3(image, x, y, pixels);
}

void sampleFace(const Image& input, float sc, float tc, float ma, float color[3]) {
    ma = fabs(ma);

    float s = (sc/ma + 1) * 0.5;
    float t = (tc/ma + 1) * 0.5;

    int x = s * (input.width - 1);
    int y = t * (input.height - 1);

    assert(x < input.width);
    assert(y < input.height);

    imageGet3(input, x, y, color);
}

void sampleCube(const ImageCube& input, float vector[3], float color[3]) {
    float x = fabs(vector[0]);
    float y = fabs(vector[1]);
    float z = fabs(vector[2]);

    float temp[3];
    float count = 0;

    mnVector3MulScalar(color, 0, color);

    if(x >= y && x >= z) {
        if(vector[0] >= 0)
            sampleFace(input.faces[POSITIVE_X], -vector[2], +vector[1], vector[0], temp);
        else
            sampleFace(input.faces[NEGATIVE_X], +vector[2], +vector[1], vector[0], temp);

        mnVector3Add(color, temp, color);
        count += 1;
    }

    if(y >= x && y >= z) {
        if(vector[1] >= 0)
            sampleFace(input.faces[POSITIVE_Y], +vector[0], -vector[2], vector[1], temp);
        else
            sampleFace(input.faces[NEGATIVE_Y], +vector[0], +vector[2], vector[1], temp);

        mnVector3Add(color, temp, color);
        count += 1;
    }

    if(z >= x && z >= y) {
        if(vector[2] >= 0)
            sampleFace(input.faces[POSITIVE_Z], +vector[0], +vector[1], vector[2], temp);
        else
            sampleFace(input.faces[NEGATIVE_Z], -vector[0], +vector[1], vector[2], temp);

        mnVector3Add(color, temp, color);
        count += 1;
    }

    mnVector3MulScalar(color, 1.0 / count, color);
}

float* tangentToWorld( const float v[3], const float tangentZ[3], float out[3] ) {
    float tangentX[3];
    float tangentY[3];

    if(fabs(tangentZ[2]) < 0.999) {
        float up[3] = {0,0,1};
        mnVector3Cross(up, tangentZ, tangentX);
    } else {
        float up[3] = {1,0,0};
        mnVector3Cross(up, tangentZ, tangentX);
    }

    mnVector3Normalize(tangentX, tangentX);
    mnVector3Cross(tangentZ, tangentX, tangentY);

    float temp[3];
    mnVector3MulScalar(tangentX, v[0], temp);
    mnVector3MulAddScalar(tangentY, v[1], temp);
    mnVector3MulAddScalar(tangentZ, v[2], temp);

    out[0] = temp[0];
    out[1] = temp[1];
    out[2] = temp[2];
    return out;
}

uint32_t bitfieldReverse(uint32_t bits) {
    bits = ( bits << 16) | ( bits >> 16);
    bits = ( (bits & 0x00ff00ff) << 8 ) | ( (bits & 0xff00ff00) >> 8 );
    bits = ( (bits & 0x0f0f0f0f) << 4 ) | ( (bits & 0xf0f0f0f0) >> 4 );
    bits = ( (bits & 0x33333333) << 2 ) | ( (bits & 0xcccccccc) >> 2 );
    bits = ( (bits & 0x55555555) << 1 ) | ( (bits & 0xaaaaaaaa) >> 1 );
    return bits;
}

void hammersley2D( uint32_t i, uint32_t N, float out[2] ) {
    float E1 = float(i) / float(N);
    float E2 = float(bitfieldReverse(i)) * 2.3283064365386963e-10;

    out[0] = E1;
    out[1] = E2;
}

float* uniformSampleHemisphere( float E[2], float Out[4] ) {
    float Phi = 2 * M_PI * E[0];
    float CosTheta = E[1];
    float SinTheta = sqrtf( 1 - CosTheta * CosTheta );

    Out[0] = SinTheta * cosf( Phi );
    Out[1] = SinTheta * sinf( Phi );
    Out[2] = CosTheta;
    Out[3] = CosTheta / M_PI;

    return Out;
}

float* cosineSampleHemisphere( float E[2], float Out[4] ) {
    float Phi = 2 * M_PI * E[0];
    float CosTheta = sqrtf( E[1] );
    float SinTheta = sqrtf( 1 - CosTheta * CosTheta );

    Out[0] = SinTheta * cosf( Phi );
    Out[1] = SinTheta * sinf( Phi );
    Out[2] = CosTheta;
    Out[3] = CosTheta / M_PI;

    return Out;
}

void normalForFace(int face, int x, int y, int width, int height, float N[3]) {
    float s = width == 1 ? 0.5 : x / (float)(width - 1);
    float t = height == 1 ? 0.5 : y / (float)(height - 1);

    float xx = s * 2 - 1;
    float yy = t * 2 - 1;

    if(face == POSITIVE_Y) {
        float temp[3] = {xx,  1, -yy};
        mnVector3Normalize(temp, N);
    } else if(face == NEGATIVE_Y) {
        float temp[3] = {xx, -1,  yy};
        mnVector3Normalize(temp, N);
    } else if(face == POSITIVE_X) {
        float temp[3] = {1, yy, -xx};
        mnVector3Normalize(temp, N);
    } else if(face == NEGATIVE_X) {
        float temp[3] = {-1, yy, xx};
        mnVector3Normalize(temp, N);
    } else if(face == POSITIVE_Z) {
        float temp[3] = {xx, yy, 1};
        mnVector3Normalize(temp, N);
    } else if(face == NEGATIVE_Z) {
        float temp[3] = {-xx, yy, -1};
        mnVector3Normalize(temp, N);
    }
}

void computeIrradiance(int face, const ImageCube& input, const float N[3], float irradiance[3]) {
    float sampledColour[3] = {0, 0, 0};

    for(int i = 0; i < NUM_SAMPLES; i++) {
        float Xi[2];
        float L[4];
        float H[3];

        hammersley2D(i, NUM_SAMPLES, Xi);
        cosineSampleHemisphere(Xi, L);
        tangentToWorld(L, N, L);

        float NdotL = mnVector3Dot(N, L);
        if(NdotL > 0) {
            float Li[3];

            sampleCube(input, L, Li);

            mnVector3Add(sampledColour, Li, sampledColour);
        }
    }

    mnVector3MulScalar(sampledColour, 1 / (float)NUM_SAMPLES, irradiance);
}

void computeFaceIrradiance(int face, const ImageCube& input, Image& output) {
    for(int y = 0; y < output.height; y++) {
        for(int x = 0; x < output.width; x++) {
            float N[3];
            float outColor[3];

            normalForFace(face, x, y, output.width, output.height, N);
            computeIrradiance(face, input, N, outColor);

            imageSet3(output, x, y, outColor);
        }
    }
}

void computeDiffuseIrradiance(HeapAllocator& allocator, const ImageCube& input, int width, int height, ImageCube& output) {
    for(int face = 0; face < 6; face++) {
        output.faces[face].width = width;
        output.faces[face].height = height;
        output.faces[face].format = 3;
        output.faces[face].pixels = (uint8_t*)allocator.allocate(3 * width * height);
        memset(output.faces[face].pixels, 0, 3 * width * height);
    }

    auto start = std::chrono::high_resolution_clock::now();

#if 1
    for(int face = 0; face < 6; face++) {
        printf("Calculating irradiance for face %d\n", face);

        auto start = std::chrono::high_resolution_clock::now();

        computeFaceIrradiance(face, input, output.faces[face]);

        auto end = std::chrono::high_resolution_clock::now();

        long long milli = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        printf("Took %lld millis\n", milli);
    }
#else
    std::thread tr[6];

    for(int face = 0; face < 6; face++) {
        tr[face] = std::thread([=]() {
            computeFaceIrradiance(face, input, output[face]);
        });
    }

    for(int face = 0; face < 6; face++)
        tr[face].join();
#endif

    auto end = std::chrono::high_resolution_clock::now();
    long long milli = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf("Total time: %lld millis\n", milli);
}

void ImportanceSampleGGX(const float Xi[2], float roughness, float H[3]) {
    float a = roughness * roughness;

    float phi = 2 * M_PI * Xi[0];
    float cosTheta = sqrtf( (1 - Xi[1]) / (1 + (a*a - 1) * Xi[1]) );
    float sinTheta = sqrtf( 1 - cosTheta * cosTheta );

    H[0] = sinTheta * cosf(phi);
    H[1] = sinTheta * sinf(phi);
    H[2] = cosTheta;
}

//I - 2.0 * dot ( N , I ) * N
void reflect(float k, const float I[3], const float N[3], float out[3]) {
    float Ii[3];

    mnVector3MulScalar(I, k, Ii);

    float dot = 2.0f * mnVector3Dot(N, Ii);

    mnVector3MulScalar(N, dot, out);
    mnVector3Sub(Ii, out, out);
}

////input world space [N = V = R]
void prefilterEnvMap(const ImageCube& input, float roughness, const float R[3], float out[3]) {
    float prefilterColor[3] = {0, 0, 0};
    float totalWeight = 0;

    for(uint32_t i = 0; i < NUM_SAMPLES; i++) {
        float Xi[2];
        float H[3];
        float L[3];

        hammersley2D(i, NUM_SAMPLES, Xi);
        ImportanceSampleGGX(Xi, roughness, H);
        tangentToWorld(H, R, H);
        reflect(-1, R, H, L);

        float NdotL = std::max(0.0f, mnVector3Dot(R, L));
        if (NdotL > 0) {
            float Li[3];

            sampleCube(input, L, Li);

            mnVector3MulAddScalar(Li, NdotL, prefilterColor);
            totalWeight += NdotL;
        }
    }

    mnVector3MulScalar(prefilterColor, 1.0f / std::max(totalWeight, 0.001f), out);
}

void computePrefilterEnvMapForFace(int face, const ImageCube& input, float roughness, Image& output) {
    for(int y = 0; y < output.height; y++) {
        for(int x = 0; x < output.width; x++) {
            float N[3];
            float outColor[3];

            normalForFace(face, x, y, output.width, output.height, N);
            prefilterEnvMap(input, roughness, N, outColor);

            imageSet3(output, x, y, outColor);
        }
    }
}

void computePrefilterEnvMap(HeapAllocator& allocator, const ImageCube& input, float roughness, int width, int height, ImageCube& output) {
    for(int face = 0; face < 6; face++) {
        output.faces[face].width = width;
        output.faces[face].height = height;
        output.faces[face].format = 3;
        output.faces[face].pixels = (uint8_t*)allocator.allocate(3 * width * height);
        memset(output.faces[face].pixels, 0, 3 * width * height);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for(int face = 0; face < 6; face++)
        computePrefilterEnvMapForFace(face, input, roughness, output.faces[face]);

    if(width == 1 && height == 1) {
        float color[3] = {0, 0, 0};

        for(int i = 0; i < 6; i++) {
            float temp[3];

            imageGet3(output.faces[i], 0, 0, temp);
            mnVector3Add(color, temp, color);
        }

        mnVector3MulScalar(color, 1.0f / 6.0f, color);

        for(int i = 0; i < 6; i++)
            imageSet3(output.faces[i], 0, 0, color);
    }

    auto end = std::chrono::high_resolution_clock::now();
    long long milli = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    printf("Total time: %lld millis\n", milli);
}

float G1_smithGGX(float alpha, float NdotX) {
    float NdotX2 = NdotX * NdotX;
    float alpha2 = alpha * alpha;

    return NdotX + sqrtf(NdotX2 * (1 - alpha2) + alpha2);
}

float GVis_smithGGX(float roughness, float NdotV, float NdotL) {
    float alpha = roughness*roughness;

    return 1 / (G1_smithGGX(alpha, NdotL) * G1_smithGGX(alpha, NdotV));
}

float G_smithGGX(float roughness, float NdotV, float NdotL) {
    float alpha = roughness*roughness;
    float v = 2*(NdotV) / G1_smithGGX(alpha, NdotV);
    float l = 2*(NdotL) / G1_smithGGX(alpha, NdotL);

    return v*l;
}

void integrateBRDF(float roughness, float NdotV, float out[2]) {
    float V[3];
    V[0] = sqrtf(1 - NdotV * NdotV);
    V[1] = 0;
    V[2] = NdotV;

    float A = 0;
    float B = 0;

    for(int i = 0; i < NUM_SAMPLES; i++) {
        float Xi[2];
        float H[3];
        float L[3];

        hammersley2D(i, NUM_SAMPLES, Xi);
        ImportanceSampleGGX(Xi, roughness, H);
        reflect(-1, V, H, L);

        float NdotL = std::max(0.0f, L[2]);
        float NdotH = std::max(0.0f, H[2]);
        float VdotH = std::max(0.0f, mnVector3Dot(V, H));

        if(NdotL > 0) {
#if 1
            float G_Vis = GVis_smithGGX(roughness, NdotV, NdotL);
            float NdotL_G_Vis_Pdf = NdotL * G_Vis * (4 * VdotH / NdotH);
#else
            float G = G_smithGGX(roughness, NdotV, NdotL);
            float NdotL_G_Vis_Pdf = G * VdotH / (NdotH * NdotV);
#endif

            float Fc = pow(1 - VdotH, 5);
            A += (1 - Fc) * NdotL_G_Vis_Pdf;
            B += Fc * NdotL_G_Vis_Pdf;
        }
    }

    out[0] = A / (float)NUM_SAMPLES;
    out[1] = B / (float)NUM_SAMPLES;
}

void calculateIntegrateBRDF(HeapAllocator& allocator, int width, int height, Image& output) {
    output.width = width;
    output.height = height;
    output.format = 3;
    output.pixels = (uint8_t*)allocator.allocate(3 * width * height);
    memset(output.pixels, 0, 3 * width * height);

    for(int x = 0; x < width; x++) {
        float roughness = x / float(width - 1);

        for(int y = 0; y < height; y++) {
            float outColor[3] = {0, 0, 0};

            float NdotV = y / float(height - 1);

            integrateBRDF(roughness, NdotV, outColor);

            imageSet3(output, x, y, outColor);
        }
    }
}

int main() {
    srand(time(nullptr));

    HeapAllocator heapAllocator;

    ImageCube input;
    readJpeg(heapAllocator, IMG_PATH"/posx.jpg", input.faces[POSITIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/negx.jpg", input.faces[NEGATIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/posy.jpg", input.faces[POSITIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/negy.jpg", input.faces[NEGATIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/posz.jpg", input.faces[POSITIVE_Z]);
    readJpeg(heapAllocator, IMG_PATH"/negz.jpg", input.faces[NEGATIVE_Z]);

#if 1
    {
        ImageCube output;
        computeDiffuseIrradiance(heapAllocator, input, OUTPUT_SIZE, OUTPUT_SIZE, output);
        saveCube(IMG_PATH"/diffuse_irradiance.irr", output);

        for(int i = 0; i < 6; i++)
            heapAllocator.deallocate(output.faces[i].pixels);
    }
#endif

#if 1
    int size = OUTPUT_SIZE;
    float maxMips = log2f((float)size);

    for(float mip = 0; mip <= maxMips; mip += 1) {
        char filename[1024];
        snprintf(filename, 1024, "%s/prefilter_env_map_%d.irr", IMG_PATH, (int)mip);

        ImageCube output;

        float roughness = mip / maxMips;

        printf("Calculating prefilterEnvMap(roughness=%.2f, width=%d, height=%d)\n", roughness, size, size);

        computePrefilterEnvMap(heapAllocator, input, roughness, size, size, output);

        saveCube(filename, output);

        for(int i = 0; i < 6; i++)
            heapAllocator.deallocate(output.faces[i].pixels);

        size >>= 1;
    }
#endif

#if 1
    {
        Image output;
        calculateIntegrateBRDF(heapAllocator, OUTPUT_SIZE, OUTPUT_SIZE, output);
        saveImage(IMG_PATH"/integrate_brdf.irr", output);
        heapAllocator.deallocate(output.pixels);
    }
#endif

    for(int i = 0; i < 6; i++)
        heapAllocator.deallocate(input.faces[i].pixels);

    return 0;
}
