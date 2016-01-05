//
// Created by Marrony Neris on 12/18/15.
//

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>

#include "Vector.h"
#include "Matrix.h"
#include "Allocator.h"
#include "Device.h"
#include "Commands.h"
#include "RenderQueue.h"
#include "Text.h"
#include "Material.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "Shaders.h"
#include "Wavefront.h"

Rect viewport = {};

float angle = 0;
bool autoAngle = true;

int baseColorIndex = 1;

Vector3 baseColors[] = {
        {0.560, 0.570, 0.580},
        {0.972, 0.960, 0.915},
        {0.913, 0.921, 0.925},
        {1.000, 0.766, 0.336},
        {0.955, 0.637, 0.538},
        {0.550, 0.556, 0.554},
        {0.660, 0.609, 0.526},
        {0.542, 0.497, 0.449},
        {0.662, 0.655, 0.634},
        {0.672, 0.637, 0.585},
};

const char* baseColorNames[] = {
        "Iron",
        "Silver",
        "Aluminum",
        "Gold",
        "Copper",
        "Chromium",
        "Nickel",
        "Titanium",
        "Cobalt",
        "Platinum",
};

struct In_MaterialData {
    Vector4 baseColor;
    float roughness;
    float metallic;
    float specularity;
    float ior;
    uint32_t useTBNMatrix;
    uint32_t approximationSpecular;
    uint32_t approximationDiffuse;
};

In_MaterialData materialData = {0};

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (!autoAngle && action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_LEFT:
            angle -= 0.01;
            break;
        case GLFW_KEY_RIGHT:
            angle += 0.01;
            break;
        }
    }

    if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_A:
            materialData.useTBNMatrix = !materialData.useTBNMatrix;
            break;
        case GLFW_KEY_S:
            materialData.approximationSpecular++;
            materialData.approximationSpecular %= 3;
            break;
        case GLFW_KEY_D:
            materialData.approximationDiffuse++;
            materialData.approximationDiffuse %= 2;
            break;
        case GLFW_KEY_SPACE:
            autoAngle = !autoAngle;
            break;
        case GLFW_KEY_0:
            baseColorIndex = 0;
            break;
        case GLFW_KEY_1:
            baseColorIndex = 1;
            break;
        case GLFW_KEY_2:
            baseColorIndex = 2;
            break;
        case GLFW_KEY_3:
            baseColorIndex = 3;
            break;
        case GLFW_KEY_4:
            baseColorIndex = 4;
            break;
        case GLFW_KEY_5:
            baseColorIndex = 5;
            break;
        case GLFW_KEY_6:
            baseColorIndex = 6;
            break;
        case GLFW_KEY_7:
            baseColorIndex = 7;
            break;
        case GLFW_KEY_8:
            baseColorIndex = 8;
            break;
        case GLFW_KEY_9:
            baseColorIndex = 9;
            break;

        case GLFW_KEY_UP:
            materialData.roughness += 0.05;
            break;
        case GLFW_KEY_DOWN:
            materialData.roughness -= 0.05;
            break;

        case GLFW_KEY_N:
            materialData.metallic -= 0.1;
            break;
        case GLFW_KEY_M:
            materialData.metallic += 0.1;
            break;

        case GLFW_KEY_K:
            materialData.specularity += 0.1;
            break;
        case GLFW_KEY_L:
            materialData.specularity -= 0.1;
            break;

        case GLFW_KEY_I:
            materialData.ior += 0.05;
            break;
        case GLFW_KEY_O:
            materialData.ior -= 0.05;
            break;
        }
    }

    materialData.roughness = std::max(0.0f, std::min(materialData.roughness, 1.0f));
    materialData.metallic = std::max(0.0f, std::min(materialData.metallic, 1.0f));
    materialData.specularity = std::max(0.0f, std::min(materialData.specularity, 1.0f));
    materialData.ior = std::max(0.0f, std::min(materialData.ior, 10.0f));
}

void framebuffer_callback(GLFWwindow* window, int width, int height) {
    viewport.width = width;
    viewport.height = height;
}

#define STR(x) #x

const char* draw_texture_vert = STR(
out vec2 vtx_Texture;

void main() {
    gl_Position = projection * view * instanceData[0].in_Rotation * vec4(in_Position, 1);
    vtx_Texture = in_Texture;
}
);

const char* draw_texture_frag = STR(
in vec2 vtx_Texture;

uniform sampler2D in_Texture;

layout(location = 0) out vec4 out_FragColor;

void main() {
    vec4 value = texture(in_Texture, vtx_Texture);

    out_FragColor = value;
}
);

const char* physically_based_shader_vert = STR(
out vec3 normalWS;
out vec3 normalCS;
out vec3 tangentWS;
out vec3 tangentCS;
out vec3 lightCS;
out vec3 lightWS;
out vec3 viewWS;
out vec3 viewCS;
out vec3 posWS;
out vec2 texCoord;

const vec3 u_lightPosition = vec3(8, 0, 0);

void main() {
    vec4 vertexWorld = instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1);
    vec4 normalWorld = instanceData[gl_InstanceID].in_Rotation * vec4(in_Normal, 0);
    vec4 tangentWorld = instanceData[gl_InstanceID].in_Rotation * vec4(in_Tangent, 0);

    posWS = normalize(in_Position);

    gl_Position = projection * view * vertexWorld;

    normalCS = normalize((view * normalWorld).xyz);
    normalWS = normalize(normalWorld.xyz);

    tangentCS = normalize((view * tangentWorld).xyz);
    tangentWS = normalize(tangentWorld.xyz);

    viewCS = normalize(-(view * vertexWorld).xyz);
    viewWS = normalize(cameraPosition.xyz - vertexWorld.xyz);

    lightCS = mat3(view) * u_lightPosition;
    lightWS = normalize(u_lightPosition - vertexWorld.xyz);

    texCoord = in_Texture;
}
);

const char* physically_based_shader_frag = STR(
layout(std140) uniform in_MaterialData {
    vec4 baseColor;
    float roughness;
    float metallic;
    float specularity;
    float ior;
    bool useTBNMatrix;
    int approximationSpecular;
    int approximationDiffuse;
};

uniform samplerCube skyboxIrradiance;
uniform samplerCube skyboxCube;
uniform samplerCube prefilterEnv;
uniform sampler2D integrateBRDFTex;
uniform sampler2D normalTexture;

in vec3 normalWS;
in vec3 normalCS;
in vec3 tangentWS;
in vec3 tangentCS;
in vec3 lightCS;
in vec3 lightWS;
in vec3 viewWS;
in vec3 viewCS;
in vec3 posWS;
in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

const float M_E        = 2.71828182845904523536028747135266250 ;  /* e              */
const float M_LOG2E    = 1.44269504088896340735992468100189214 ;  /* log2(e)        */
const float M_LOG10E   = 0.434294481903251827651128918916605082;  /* log10(e)       */
const float M_LN2      = 0.693147180559945309417232121458176568;  /* loge(2)        */
const float M_LN10     = 2.30258509299404568401799145468436421 ;  /* loge(10)       */
const float M_PI       = 3.14159265358979323846264338327950288 ;  /* pi             */
const float M_PI_2     = 1.57079632679489661923132169163975144 ;  /* pi/2           */
const float M_PI_4     = 0.785398163397448309615660845819875721;  /* pi/4           */
const float M_1_PI     = 0.318309886183790671537767526745028724;  /* 1/pi           */
const float M_2_PI     = 0.636619772367581343075535053490057448;  /* 2/pi           */
const float M_2_SQRTPI = 1.12837916709551257389615890312154517 ;  /* 2/sqrt(pi)     */
const float M_SQRT2    = 1.41421356237309504880168872420969808 ;  /* sqrt(2)        */
const float M_SQRT1_2  = 0.707106781186547524400844362104849039;  /* 1/sqrt(2)      */

vec4 textureCube(samplerCube cube, vec3 V) {
    return texture(cube, V*vec3(1, -1, 1));
}

vec4 textureCubeLod(samplerCube cube, vec3 V, float lod) {
    return textureLod(cube, V*vec3(1, -1, 1), lod);
}

vec3 lerp(vec3 x, vec3 y, float a) {
    return x + (y - x)*a;
}

/*
 * f(l, v) = F(l, h) * G(l, v, h) * D(h)
 *           ---------------------------
 *            4 * dot(n, l) * dot(n, v)
 *
 * G_Vis(l, v, h) =         G(l, v, h)
 *                  -------------------------
 *                  4 * dot(n, l) * dot(n, v)
 *
 * f(l, v) = F(l, h) * D(h) * G_Vis(l, v, h)
 *
 * NdotL * G_Vis * (4 * VdotH / NdotH) = G * VdotH / (NdotH * NdotV)
 */

/*
 * Function F
 */
vec3 F_schlick(vec3 F0, float VdotH) {
    float Fc = pow(1 - VdotH, 5);
    return F0 + (1 - F0) * Fc;
}

vec3 F_conductor(float VdotH, float n, float k) {
    float n1 = n - 1;
    float n2 = n + 1;
    float k2 = k * k;

    return vec3((n1*n1 + 4*n*pow(1 - VdotH, 5) + k2) / (n2*n2 + k2));
}

/*
 * Function D
 */
float D_blinn(float roughness, float NdotH) {
    float alpha = roughness*roughness;
    return ((alpha + 2) / 2*M_PI) * pow(NdotH, alpha);
}

float D_ggx(float roughness, float NdotH) {
    float alpha = roughness*roughness;
    float NdotH2 = NdotH * NdotH;
    float alpha2 = alpha * alpha;
    float d = NdotH2 * (alpha2 - 1) + 1;

    return alpha2 / (M_PI * d * d);
}

float D_beckmann(float roughness, float NdotH) {
    float alpha = roughness*roughness;
    float NdotH2 = NdotH * NdotH;
    float alpha2 = alpha * alpha;

    float e = exp((NdotH2 - 1) / (alpha2 * NdotH2));
    return e / (M_PI * alpha2 * NdotH2 * NdotH2);
}

/*
 *          G(l, v, h)
 * GVis = -----------------
 *        4 * NdotL * NdotV
 */

/*
 *   NdotL * NdotV     1
 * ----------------- = -
 * 4 * NdotL * NdotV   4
 */
float GVis_implicit() {
    return 0.25;
}

float G1_smithGGX(float alpha, float NdotX) {
    float NdotX2 = NdotX * NdotX;
    float alpha2 = alpha * alpha;

    return NdotX + sqrt(NdotX2 * (1 - alpha2) + alpha2);
}

/*
 * 2 * NdotV   2 * NdotL           1           4 * NdotV * NdotL                            1
 * --------- * --------- * ----------------- = ----------------------------------- = ---------------
 *   GGX(v)      GGX(l)    4 * NdotL * NdotV   4 * NdotL * NdotV * GGX(v) * GGX(l)   GGX(v) * GGX(l)
 */
float GVis_smithGGX(float roughness, float NdotV, float NdotL) {
    float alpha = roughness*roughness;

    return 1 / (G1_smithGGX(alpha, NdotL) * G1_smithGGX(alpha, NdotV));
}

float GVis_smithJoint(float roughness, float NdotV, float NdotL) {
    float alpha = roughness*roughness;

    float v = NdotL * (NdotV * (1 - alpha) + alpha);
    float l = NdotV * (NdotL * (1 - alpha) + alpha);

    return 0.5 * (1 / (l + v));
}

/*
 * NdotL * NdotV            1                   1
 * -------------  * ----------------- = -----------------
 * VdotH * VdotH    4 * NdotL * NdotV   4 * VdotH * VdotH
 */
float GVis_kelemen(float VdotH) {
    return 1.0 / (4.0 * VdotH * VdotH);
}

float _G_schlick(float NdotX, float k) {
    return NdotX * (1 - k) + k;
}

/*
 *   NdotV         NdotL             1                     1
 * ---------- * ---------- * ----------------- = ---------------------------
 * schlick(v)   schlick(l)   4 * NdotL * NdotV   4 * schlick(v) * schlick(l)
 */
float GVis_schlickGGX(float roughness, float NdotV, float NdotL) {
    float k = roughness*roughness*0.5;
    float _v = _G_schlick(NdotV, k);
    float _l = _G_schlick(NdotL, k);
    return 1.0 / (4.0 * _v * _l);
}

/*
 *   NdotV * NdotL             1                      1
 * ----------------- * ----------------- = ---------------------
 * max(NdotV, NdotL)   4 * NdotL * NdotV   4 * max(NdotV, NdotL)
 */
float GVis_neumann(float NdotV, float NdotL) {
    return 1 / (4 * max(NdotL, NdotV));
}

/*
 * Function G
 */
float G_implicit(float NdotV, float NdotL) {
    return NdotV * NdotL;
}

float G_neumann(float NdotV, float NdotL) {
    return (NdotV * NdotL) / max(NdotV, NdotL);
}

float G_kelemen(float NdotV, float NdotL, float VdotH) {
    return (NdotV * NdotL) / (VdotH * VdotH);
}

/*
 * 2 * NdotV   2 * NdotL
 * --------- * ---------
 *   GGX(v)      GGX(l)
 */
float G_smithGGX(float roughness, float NdotV, float NdotL) {
    float alpha = roughness*roughness;
    float v = 2*(NdotV) / G1_smithGGX(alpha, NdotV);
    float l = 2*(NdotL) / G1_smithGGX(alpha, NdotL);

    return v*l;
}

/*
 *    NdotV        NdotL
 * ---------- * ----------
 * schlick(v)   schlick(l)
 */
float G_schlick_beckmann(float roughness, float NdotV, float NdotL) {
//    float alpha = roughness*roughness;
//    float k = alpha * sqrt(2 / M_PI);
    float alpha = roughness*roughness;
    float k = alpha * 0.5;

    float v = NdotV / _G_schlick(NdotV, k);
    float l = NdotL / _G_schlick(NdotL, k);

    return v * l;
}

float G_cooktorrance(float NdotH, float NdotL, float NdotV, float VdotH) {
    float NdotL_clamped = max(NdotL, 0.0);
    float NdotV_clamped = max(NdotV, 0.0);

    return min( min( 2.0 * NdotH * NdotV_clamped / VdotH, 2.0 * NdotH * NdotL_clamped / VdotH), 1.0);
}

/*
 *
 */
vec3 Diffuse_lambert(vec3 diffuseColor) {
    return diffuseColor / M_PI;
}

vec3 Diffuse_burley(vec3 diffuseColor, float roughness, float VdotH, float NdotV, float NdotL) {
    float FD90 = 0.5 + 2 * VdotH * VdotH * roughness;
    float _v = 1 + (FD90 - 1) * pow(1 - NdotV, 5);
    float _l = 1 + (FD90 - 1) * pow(1 - NdotL, 5);
//    return diffuseColor * (_v * _l * M_1_PI);
    return diffuseColor * (_v * _l * (1 - M_1_PI * roughness));
}

vec3 TangentToWorld( vec3 Vec, vec3 TangentZ ) {
    vec3 UpVector = abs(TangentZ.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 TangentX = normalize( cross( UpVector, TangentZ ) );
    vec3 TangentY = cross( TangentZ, TangentX );
    return TangentX * Vec.x + TangentY * Vec.y + TangentZ * Vec.z;
}

/*
 * http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
 *
 * Hn = { xi = (i/N, phi(i)) for i = 0, ..., N-1 }
 *
 *          a0 + a1 + ... + ar
 * phi(i) = --   --         --
 *          2    2^2        2^r+1
 */
vec2 hammersley2D(uint i, uint N) {
    float E1 = fract( float(i) / float(N));
    float E2 = float( bitfieldReverse(i)) * 2.3283064365386963e-10; // 0x100000000
    return vec2( E1, E2 );
}

vec2 randomizeLinear(uint i, uint N) {
    float s = sqrt(float(N));

    uint x = i / uint(s);
    uint y = i % uint(s);

    return vec2(x/s, y/x);
}

vec3 ImportanceSampleGGX(vec2 Xi, float roughness) {
    float a = roughness * roughness;

    float phi = 2 * M_PI * Xi.x;
    float cosTheta = sqrt( (1 - Xi.y) / (1 + (a*a - 1) * Xi.y) );
    float sinTheta = sqrt( 1 - cosTheta * cosTheta );

    vec3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    return H;
}

vec4 CosineSampleHemisphere( vec2 E ) {
    float Phi = 2 * M_PI * E.x;
    float CosTheta = sqrt( E.y );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );

    vec3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;

    float PDF = CosTheta / M_PI;

    return vec4( H, PDF );
}

const uint NumSamples = 128;

//input world space
vec3 diffuseIBL(vec3 diffuseColor, float roughness, vec3 N, vec3 V) {
    vec3 diffuseLighting = vec3(3);

    float NdotV = max(0, dot(N, V));

    for(uint i = 0; i < NumSamples; i++) {
        vec2 Xi = hammersley2D(i, NumSamples);
        vec3 L = TangentToWorld(CosineSampleHemisphere(Xi).rgb, N);
        vec3 H = normalize(V + L);

        float NdotL = max(0, dot(N, L));
        float NdotH = max(0, dot(N, H));
        float VdotH = max(0, dot(V, H));

        if(NdotL > 0) {
            vec3 Li = textureCube(skyboxCube, L).rgb;

            diffuseLighting += Li * Diffuse_burley(diffuseColor, roughness, VdotH, NdotV, NdotL);
//            diffuseLighting += Li * Diffuse_lambert(diffuseColor);
//            diffuseLighting += Li * diffuseColor;
        }
    }

    return diffuseLighting / NumSamples;
}

//input world space
vec3 specularIBL(vec3 F0, float roughness, vec3 N, vec3 V) {
    vec3 specularLighting = vec3(0);

    for(uint i = 0; i < NumSamples; i++) {
        vec2 Xi = hammersley2D(i, NumSamples);
        vec3 H = TangentToWorld(ImportanceSampleGGX(Xi, roughness), N);
        vec3 L = reflect(-V, H);

        float NdotV = max(0, dot(N, V));
        float NdotL = max(0, dot(N, L));
        float NdotH = max(0, dot(N, H));
        float VdotH = max(0, dot(V, H));

        if(NdotL > 0) {
            vec3 Li = textureCube(skyboxCube, L).rgb;

            vec3 F = F_schlick(F0, VdotH);

//            float D = D_blinn(roughness, NdotH);
//            float D = D_ggx(roughness, NdotH);
//            float D = D_beckmann(roughness, NdotH);

//            float G_Vis = GVis_smithGGX(roughness, NdotV, NdotL);
            float G_Vis = GVis_smithJoint(roughness, NdotV, NdotL);
            specularLighting += Li * F * (NdotL * G_Vis * (4 * VdotH / NdotH));

//            float G = G_smithGGX(roughness, NdotV, NdotL);
//            float G = G_schlick_beckmann(roughness, NdotV, NdotL);

//            float D = D_blinn(roughness, NdotH);
//            float D = D_ggx(roughness, NdotH);
//            float D = D_beckmann(roughness, NdotH);
//            specularLighting += Li * F * D * VdotH / (NdotH * NdotV);
        }
    }

    return specularLighting / NumSamples;
}

////////////////////////////////////////////////////

//input world space
vec3 filterEnvMap(float roughness, vec3 N, vec3 V) {
    vec3 filterColor = vec3(0);
    float totalWeight = 0;

    for(uint i = 0; i < NumSamples; i++) {
        vec2 Xi = hammersley2D(i, NumSamples);
        vec3 H = TangentToWorld(ImportanceSampleGGX(Xi, roughness), N);
        vec3 L = reflect(-V, H);

        float NdotL = max(0, dot(N, L));
        if (NdotL > 0) {
            filterColor += textureCube(skyboxCube, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    return filterColor / max(totalWeight, 0.001);
}

//input world space [N = V = R]
vec3 prefilterEnvMap(float roughness, vec3 R) {
    vec3 prefilterColor = vec3(0);
    float totalWeight = 0;

    for(uint i = 0; i < NumSamples; i++) {
        vec2 Xi = hammersley2D(i, NumSamples);
        vec3 H = TangentToWorld(ImportanceSampleGGX(Xi, roughness), R);
        vec3 L = reflect(-R, H);

        float NdotL = max(0, dot(R, L));
        if (NdotL > 0) {
            prefilterColor += textureCube(skyboxCube, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    return prefilterColor / max(totalWeight, 0.001);
}

//input view space?
vec2 integrateBRDF(float roughness, float NdotV) {
    vec3 V;
    V.x = sqrt(1 - NdotV * NdotV);
    V.y = 0;
    V.z = NdotV;

    float A = 0;
    float B = 0;
    float C = 0;

    for(uint i = 0; i < NumSamples; i++) {
        vec2 Xi = hammersley2D(i, NumSamples);
        vec3 H = ImportanceSampleGGX(Xi, roughness);
        vec3 L = reflect(-V, H);

        float NdotL = max(0, L.z);
        float NdotH = max(0, H.z);
        float VdotH = max(0, dot(V, H));

        if(NdotL > 0) {
            //f = DFG / (4*NdotV*NdotL)

            float G_Vis = GVis_smithGGX(roughness, NdotV, NdotL);
            float NdotL_G_Vis_Pdf = NdotL * G_Vis * (4 * VdotH / NdotH);

            float Fc = pow(1 - VdotH, 5);
            A += (1 - Fc) * NdotL_G_Vis_Pdf;
            B += Fc * NdotL_G_Vis_Pdf;
        }
    }

    return vec2(A, B) / NumSamples;
}

////input world space
vec3 approximateSpecularIBL(vec3 specularColor, float roughness, vec3 N, vec3 V) {
    float NdotV = max(0, dot(N, V));
    vec3 R = reflect(-V, N);

    vec2 size = textureSize(prefilterEnv, 0);
    float maxLod = log2(max(size.x, size.y));

    vec3 prefilteredColor;
    vec2 envBRDF;

    if(approximationSpecular == 1) {
        prefilteredColor = prefilterEnvMap(roughness, R);
        envBRDF = integrateBRDF(roughness, NdotV);
    } else {
        prefilteredColor = textureCubeLod(prefilterEnv, R, roughness * maxLod).rgb;
        envBRDF = texture(integrateBRDFTex, vec2(roughness, NdotV)).rg;
    }

    return prefilteredColor * ( specularColor*envBRDF.x + envBRDF.y );
}

void main() {
    vec3 N = normalize(normalWS);
    vec3 L = normalize(lightWS);
    vec3 V = normalize(viewWS);
    vec3 R = reflect(-V, N);

    if(useTBNMatrix) {
        vec3 T = normalize(tangentWS);
        vec3 B = normalize(cross(T, N));

        mat3 TBN = mat3(T, B, N);

        vec3 normal = texture(normalTexture, texCoord).xyz * 2 - 1;
        N = normalize(TBN * normal);
    }

//    vec3 _F0 = vec3((1 - ior) / (1 + ior));
//    vec3 F0 = mix(_F0*_F0, baseColor.rgb, 1-metallic);

    float dieletric = 0.08 * specularity;
    vec3 diffuseColor = baseColor.rgb - baseColor.rgb*metallic;
    vec3 specularColor = (dieletric - dieletric*metallic) + baseColor.rgb*metallic;
    vec3 F0 = specularColor;

    vec3 specular = vec3(0);
    if(approximationSpecular == 0)
        specular = specularIBL(F0, roughness, N, V);
    else
        specular = approximateSpecularIBL(F0, roughness, N, V);

    vec3 diffuse = vec3(0);
    if(approximationDiffuse == 0)
        diffuse = diffuseIBL(diffuseColor, roughness, N, V);
    else
        diffuse = diffuseColor * textureCube(skyboxIrradiance, N).rgb;

    fragColor.rgb = diffuse + specular;
}
);

int main() {
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Physically Based Rendering", NULL, NULL);

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_callback);

    glfwMakeContextCurrent(window);

    glfwSwapInterval(0);

    if (gl3wInit()) {
        return -1;
    }

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = w;
    viewport.height = h;

    HeapAllocator heapAllocator;

    Device device;

    ModelManager modelManager(heapAllocator, device);
    TextureManager textureManager(heapAllocator, device);
    TextManager textManager(heapAllocator, device);

    Font fontBig = textManager.loadFont("./fonts/OpenSans-Bold.ttf", 96);
    Font fontSmall = textManager.loadFont("./fonts/OpenSans-Bold.ttf", 48);

    const int BINDING_POINT_INSTANCE_DATA = 0;
    const int BINDING_POINT_FRAME_DATA = 1;
    const int BINDING_POINT_MATERIAL_DATA = 2;

    const int NUMBER_QUADS = 6;
    const int NUMBER_SPHERES = 4;
    const int WIDTH = 1024;
    const int HEIGHT = 1024;

    ImageCube irradianceImg;
    ImageCube prefilterEnvImg[8];
    Image integrateBRDFImg;
    ImageCube skyboxImg;

#define IMG_PATH "images/LancellottiChapel"

    loadCube(heapAllocator, IMG_PATH"/diffuse_irradiance.irr", irradianceImg);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_0.irr", prefilterEnvImg[0]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_1.irr", prefilterEnvImg[1]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_2.irr", prefilterEnvImg[2]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_3.irr", prefilterEnvImg[3]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_4.irr", prefilterEnvImg[4]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_5.irr", prefilterEnvImg[5]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_6.irr", prefilterEnvImg[6]);
    loadCube(heapAllocator, IMG_PATH"/prefilter_env_map_7.irr", prefilterEnvImg[7]);

    loadImage(heapAllocator, IMG_PATH"/integrate_brdf.irr", integrateBRDFImg);

    readJpeg(heapAllocator, IMG_PATH"/posx.jpg", skyboxImg.faces[POSITIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/negx.jpg", skyboxImg.faces[NEGATIVE_X]);
    readJpeg(heapAllocator, IMG_PATH"/posy.jpg", skyboxImg.faces[POSITIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/negy.jpg", skyboxImg.faces[NEGATIVE_Y]);
    readJpeg(heapAllocator, IMG_PATH"/posz.jpg", skyboxImg.faces[POSITIVE_Z]);
    readJpeg(heapAllocator, IMG_PATH"/negz.jpg", skyboxImg.faces[NEGATIVE_Z]);

    TextureCube skyboxIrradiance = createTextureCube(device, &irradianceImg, 1);
    TextureCube prefilterEnv = createTextureCube(device, prefilterEnvImg, 8);
    TextureCube skyboxCube = createTextureCube(device, &skyboxImg, 1);
    Texture2D integrateBRDF = createTexture2D(device, integrateBRDFImg);

    Texture2D skybox[6];

    for(int i = 0; i < 6; i++) {
        skybox[i] = device.createRGBTexture(skyboxImg.faces[i].width, skyboxImg.faces[i].height, skyboxImg.faces[i].pixels);

        heapAllocator.deallocate(skyboxImg.faces[i].pixels);
        heapAllocator.deallocate(irradianceImg.faces[i].pixels);
    }

    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 6; j++)
            heapAllocator.deallocate(prefilterEnvImg[i].faces[j].pixels);
    }

    heapAllocator.deallocate(integrateBRDFImg.pixels);

    Texture2D normalTexture = textureManager.loadTexture("images/lion_ddn.tga");

    Model* sphereModel = modelManager.createSphere("sphere01", 1.0, 20);
    Model* quadModel = modelManager.createQuad("quad");
    Model* wavefront = modelManager.loadWavefront("models/venus.obj");

    struct In_InstanceData {
        Matrix4 in_Rotation;
        Vector4 in_Color;
    };

    struct In_FrameData {
        Matrix4 projection;
        Matrix4 view;
        Vector4 cameraPosition;
    };

    ConstantBuffer sphereConstantBuffer = device.createConstantBuffer(NUMBER_SPHERES * sizeof(In_InstanceData));
    ConstantBuffer frameConstantBuffer = device.createConstantBuffer(sizeof(In_FrameData));
    ConstantBuffer materialConstantBuffer = device.createConstantBuffer(sizeof(In_MaterialData));

    ConstantBuffer skyboxConstantBuffer[6];
    for(int i = 0; i < 6; i++)
        skyboxConstantBuffer[i] = device.createConstantBuffer(sizeof(In_InstanceData));

    In_InstanceData sphereData[NUMBER_SPHERES];
    In_InstanceData quadData[NUMBER_QUADS];
    In_FrameData frameData;

    ModelInstance* sphereInstances = modelManager.createModelInstance(sphereModel, NUMBER_SPHERES, sphereConstantBuffer, BINDING_POINT_INSTANCE_DATA);

    Program physicallyBasedShader = device.createProgram(commonSource, physically_based_shader_vert, physically_based_shader_frag, nullptr);
    Program drawTexture = device.createProgram(commonSource, draw_texture_vert, draw_texture_frag, nullptr);

    device.setTextureBindingPoint(physicallyBasedShader, "skyboxIrradiance", 0);
    device.setTextureBindingPoint(physicallyBasedShader, "skyboxCube", 1);
    device.setTextureBindingPoint(physicallyBasedShader, "normalTexture", 2);
    device.setTextureBindingPoint(physicallyBasedShader, "prefilterEnv", 3);
    device.setTextureBindingPoint(physicallyBasedShader, "integrateBRDFTex", 4);

    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);
    device.setConstantBufferBindingPoint(physicallyBasedShader, "in_MaterialData", BINDING_POINT_MATERIAL_DATA);

    device.setTextureBindingPoint(drawTexture, "in_Texture", 0);

    device.setConstantBufferBindingPoint(drawTexture, "in_FrameData", BINDING_POINT_FRAME_DATA);
    device.setConstantBufferBindingPoint(drawTexture, "in_InstanceData", BINDING_POINT_INSTANCE_DATA);

    Texture2D depthTexture = device.createRG32FTexture(WIDTH, HEIGHT, nullptr);
    Texture2D debugTexture = device.createRGBATexture(WIDTH, HEIGHT, nullptr);
    Framebuffer depthFramebuffer = device.createFramebuffer();

    device.bindTextureToFramebuffer(depthFramebuffer, depthTexture, 0);
    device.bindTextureToFramebuffer(depthFramebuffer, debugTexture, 1);
    assert(device.isFramebufferComplete(depthFramebuffer));

    float sc[3] = {1, 1, 1};
    Matrix4 scale;
    mnMatrix4Scale(sc, scale.values);

    float tx0[3] = {0, 0, -3};
    float tx1[3] = {0, 0, -1};
    float tx2[3] = {0, 0, +1};
    float tx3[3] = {0, 0, +3};
    mnMatrix4Translate(tx0, sphereData[0].in_Rotation.values);
    mnMatrix4Mul(sphereData[0].in_Rotation.values, scale.values, sphereData[0].in_Rotation.values);
    mnMatrix4Translate(tx1, sphereData[1].in_Rotation.values);
    mnMatrix4Mul(sphereData[1].in_Rotation.values, scale.values, sphereData[1].in_Rotation.values);
    mnMatrix4Translate(tx2, sphereData[2].in_Rotation.values);
    mnMatrix4Mul(sphereData[2].in_Rotation.values, scale.values, sphereData[2].in_Rotation.values);
    mnMatrix4Translate(tx3, sphereData[3].in_Rotation.values);
    mnMatrix4Mul(sphereData[3].in_Rotation.values, scale.values, sphereData[3].in_Rotation.values);

    CommandBuffer* scenePassCommon = nullptr;
    CommandBuffer* scenePass = nullptr;

    CommandBuffer* skyboxPass[6] = {nullptr};

    {
        float X_AXIS[3] = {1, 0, 0};
        float Y_AXIS[3] = {0, 1, 0};
        float Z_AXIS[3] = {0, 0, 1};
        float sc[3] = {10, 10, 10};

        {
            float tx[3] = {10, 0, 0};
            mnMatrix4Transformation(Y_AXIS, M_PI_2, tx, sc, quadData[POSITIVE_X].in_Rotation.values);
        }

        {
            float tx[3] = {-10, 0, 0};
            mnMatrix4Transformation(Y_AXIS, -M_PI_2, tx, sc, quadData[NEGATIVE_X].in_Rotation.values);
        }

        {
            float tx[3] = {0, 10, 0};
            mnMatrix4Transformation(X_AXIS, -M_PI_2, tx, sc, quadData[POSITIVE_Y].in_Rotation.values);
        }

        {
            float tx[3] = {0, -10, 0};
            mnMatrix4Transformation(X_AXIS, M_PI_2, tx, sc, quadData[NEGATIVE_Y].in_Rotation.values);
        }

        {
            float tx[3] = {0, 0, 10};
            mnMatrix4Transformation(Y_AXIS, 0, tx, sc, quadData[POSITIVE_Z].in_Rotation.values);
        }

        {
            float tx[3] = {0, 0, -10};
            mnMatrix4Transformation(Y_AXIS, M_PI, tx, sc, quadData[NEGATIVE_Z].in_Rotation.values);
        }
    }

    RenderQueue renderQueue(device, heapAllocator);

    double current = glfwGetTime();
    double inc = 0;
    int fps = 0;
    int fps2 = 0;

    float angleLight = 0;

    materialData.roughness = 0.5;
    materialData.metallic = 1.0;
    materialData.specularity = 0.5;
    materialData.ior = 0.0;
    materialData.approximationSpecular = 0;
    materialData.approximationDiffuse = 0;
    baseColorIndex = 0;

    while (!glfwWindowShouldClose(window)) {
        double c = glfwGetTime();
        double d = c - current;
        inc += d;
        current = c;
        fps++;

        if (inc > 1) {
            fps2 = fps;
            fps = 0;
            inc = 0;
        }

        angleLight += 0.3 * d;
        if (autoAngle) {
            angle += 0.3 * d;
        }

        float aspect = viewport.width / viewport.height;
        float at[3] = {0, 0, 0};
        float eye[3] = {cosf(angle)*8, cosf(angle)*5, sinf(angle)*8};
        float upCam[3] = {0, 1, 0};
        mnMatrix4Perspective(55 * M_PI / 180.0, aspect, 0.01, 30, frameData.projection.values);
        mnMatrix4LookAt(eye, at, upCam, frameData.view.values);

        frameData.cameraPosition.x = eye[0];
        frameData.cameraPosition.y = eye[1];
        frameData.cameraPosition.z = eye[2];

        materialData.baseColor.x = baseColors[baseColorIndex].x;
        materialData.baseColor.y = baseColors[baseColorIndex].y;
        materialData.baseColor.z = baseColors[baseColorIndex].z;

        device.copyConstantBuffer(sphereConstantBuffer, sphereData, sizeof(sphereData));
        device.copyConstantBuffer(frameConstantBuffer, &frameData, sizeof(In_FrameData));
        device.copyConstantBuffer(materialConstantBuffer, &materialData, sizeof(In_MaterialData));

        for(int i = 0; i < 6; i++)
            device.copyConstantBuffer(skyboxConstantBuffer[i], &quadData[i], sizeof(In_InstanceData));

        if (scenePassCommon == nullptr) {
            scenePassCommon = CommandBuffer::create(heapAllocator, 100);

            BindConstantBuffer::create(scenePassCommon, frameConstantBuffer, BINDING_POINT_FRAME_DATA);
            BindConstantBuffer::create(scenePassCommon, materialConstantBuffer, BINDING_POINT_MATERIAL_DATA);

            BindFramebuffer::create(scenePassCommon, {0});
            SetDrawBuffers::create(scenePassCommon, 0xffffffff);
            SetViewport::create(scenePassCommon, 0, &viewport);
            ClearColor::create(scenePassCommon, 0, 0.0, 0.0, 0.0, 1);
            SetBlend::disable(scenePassCommon, 0);

#if RIGHT_HANDED
            ClearDepthStencil::create(scenePassCommon, 1.0, 0x00);
            SetDepthTest::create(scenePassCommon, true, GL_LEQUAL);
            SetCullFace::create(scenePassCommon, true, GL_BACK, GL_CCW);
#else
            ClearDepthStencil::create(scenePassCommon, 0.0, 0x00);
            SetDepthTest::create(scenePassCommon, true, GL_GEQUAL);
            SetCullFace::create(scenePassCommon, true, GL_BACK, GL_CW);
            glDepthRange(1, 0);
#endif
        }

        if (scenePass == nullptr) {
            scenePass = CommandBuffer::create(heapAllocator, 10);

            BindProgram::create(scenePass, physicallyBasedShader);
            BindTexture::create(scenePass, skyboxIrradiance, {0}, 0);
            BindTexture::create(scenePass, skyboxCube, {0}, 1);
            BindTexture::create(scenePass, normalTexture, {0}, 2);
            BindTexture::create(scenePass, prefilterEnv, {0}, 3);
            BindTexture::create(scenePass, integrateBRDF, {0}, 4);
        }

        renderQueue.submit(0, &scenePassCommon, 1);
        ModelInstance::drawNoMaterial(sphereInstances, 0, renderQueue, scenePass);

        for(int i = 0; i < 6; i++) {
            if (skyboxPass[i] == nullptr) {
                skyboxPass[i] = CommandBuffer::create(heapAllocator, 100);

                BindConstantBuffer::create(skyboxPass[i], frameConstantBuffer, BINDING_POINT_FRAME_DATA);
                BindConstantBuffer::create(skyboxPass[i], skyboxConstantBuffer[i], BINDING_POINT_INSTANCE_DATA);

                BindProgram::create(skyboxPass[i], drawTexture);
                BindTexture::create(skyboxPass[i], skybox[i], {0}, 0);
            }

            Model::draw(quadModel, 0, renderQueue, skyboxPass[i]);
        }

        renderQueue.sendToDevice();

#if 0
        glBindFramebuffer(GL_READ_FRAMEBUFFER, depthFramebuffer.id); CHECK_ERROR;
        glReadBuffer(GL_COLOR_ATTACHMENT1); CHECK_ERROR;
//        glReadBuffer(GL_DEPTH_ATTACHMENT); CHECK_ERROR;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); CHECK_ERROR;

        glBlitFramebuffer(
                0, 0, WIDTH, HEIGHT,
                0, 0, viewport.width, viewport.height,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
        ); CHECK_ERROR;
#endif

        const float color[3] = {1, 1, 1};
        textManager.printText(fontSmall, {0}, color, 10, 540, "Bump: %s", materialData.useTBNMatrix ? "Yes" : "No");
        textManager.printText(fontSmall, {0}, color, 10, 480, "Specular: %s",
                (materialData.approximationSpecular == 0) ? "Real Time IBL" : ((materialData.approximationSpecular == 1) ? "Real Time Approx. IBL" : "Approx. IBL Tex"));
        textManager.printText(fontSmall, {0}, color, 10, 420, "Diffuse: %s",
                (materialData.approximationDiffuse == 0) ? "Real Time IBL" : "Approx. IBL Tex");
        textManager.printText(fontSmall, {0}, color, 10, 360, "Base color: %s", baseColorNames[baseColorIndex]);
        textManager.printText(fontSmall, {0}, color, 10, 300, "IOR: %.2f", materialData.ior);
        textManager.printText(fontSmall, {0}, color, 10, 240, "Specularity: %.2f", materialData.specularity);
        textManager.printText(fontSmall, {0}, color, 10, 180, "Metallic: %.2f", materialData.metallic);
        textManager.printText(fontSmall, {0}, color, 10, 120, "Roughness: %.2f", materialData.roughness);
        textManager.printText(fontBig, {0}, color, 10, 30, "Physically Based Rendering");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    textureManager.unloadTexture(normalTexture);

    modelManager.destroyModel(sphereModel);
    modelManager.destroyModel(quadModel);
    modelManager.destroyModel(wavefront);

    modelManager.destroyModelInstance(sphereInstances);

    device.destroyConstantBuffer(sphereConstantBuffer);
    device.destroyConstantBuffer(frameConstantBuffer);
    device.destroyConstantBuffer(materialConstantBuffer);
    for(int i = 0; i < 6; i++)
        device.destroyConstantBuffer(skyboxConstantBuffer[i]);

    device.destroyTexture(skyboxIrradiance);
    device.destroyTexture(prefilterEnv);
    device.destroyTexture(skyboxCube);
    device.destroyTexture(depthTexture);
    device.destroyTexture(debugTexture);
    device.destroyTexture(integrateBRDF);
    for(int i = 0; i < 6; i++)
        device.destroyTexture(skybox[i]);

    device.destroyFramebuffer(depthFramebuffer);

    device.destroyProgram(physicallyBasedShader);
    device.destroyProgram(drawTexture);

    heapAllocator.deallocate(scenePassCommon);
    heapAllocator.deallocate(scenePass);
    for(int i = 0; i < 6; i++)
        heapAllocator.deallocate(skyboxPass[i]);
}
