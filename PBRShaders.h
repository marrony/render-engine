#ifndef PBRSHADERS_H
#define PBRSHADERS_H

#define STR(x) #x

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
 *
 *   G * VdotH
 * ------------- = NdotL * G_Vis * (4 * VdotH / NdotH)
 * NdotH * NdotV
 *
 *   G * VdotH     G_Vis
 * ------------- * -----
 * NdotH * NdotV   G_Vis
 *
 *   G * VdotH             4 * NdotL * NdotV
 * ------------- * G_Vis * -----------------
 * NdotH * NdotV                  G
 *
 * 4 * VdotH * NdotL
 * ----------------- * G_Vis
       NdotH
 *
 *
 * Probability Distribution Function
 *
 *       D * NdotH
 * pdf = ---------
 *       4 * VdotH
 *
 *                 D * F * G                      1           D * F * G                1
 * INTEGRAL { ----------------- * NdotL * dw } ~~ - SUM { ----------------- * NdotL * --- }
 *            4 * NdotL * NdotV                   N       4 * NdotL * NdotV           pdf
 *
 *  1            D * F * G              4 * VdotH
 *  - SUM { ----------------- * NdotL * --------- }
 *  N       4 * NdotL * NdotV           D * NdotH
 *
 *  1         F * G * VdotH
 *  - SUM { -----------------}
 *  N         NdotH * NdotV
 */

/*
 * Function F
 */
vec3 F_schlick(vec3 F0, float VdotH) {
    float Fc = pow(1 - VdotH, 5);
    return F0*(1 - Fc) + Fc;
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
    float alpha2 = alpha*alpha;
    float n = 2 / alpha2 - 2;

    return ((n + 2) / 2*M_PI) * pow(NdotH, n);
}

float D_ggx(float roughness, float NdotH) {
    float alpha = roughness*roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;
    float d = NdotH2 * (alpha2 - 1) + 1;

    return alpha2 / (M_PI * d * d);
}

float D_beckmann(float roughness, float NdotH) {
    float alpha = roughness*roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;

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

/*
 * min(1, (2*NdotH*NdotV) / VdotH, (2*NdotH*NdotL) / VdotH)
 */
float G_cooktorrance(float NdotH, float NdotL, float NdotV, float VdotH) {
    float v = (2.0 * NdotH * NdotV) / VdotH;
    float l = (2.0 * NdotH * NdotL) / VdotH;

    return min(min(v, l) , 1.0);
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

//            float G_Vis = GVis_smithJoint(roughness, NdotV, NdotL);
            float G_Vis = GVis_smithGGX(roughness, NdotV, NdotL);
            specularLighting += Li * F * ((4 * VdotH * NdotL) / NdotH) * G_Vis;

//            float G = G_smithGGX(roughness, NdotV, NdotL);
//            float G = G_schlick_beckmann(roughness, NdotV, NdotL);
//            specularLighting += Li * F * G * VdotH / (NdotH * NdotV);
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

    return vec2(A, B) / NumSamples;
}

////input world space
vec3 approximateSpecularIBL(vec3 F0, float roughness, vec3 N, vec3 V) {
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

    //envBRDF.x += (1 - Fc)
    //envBRDF.y += Fc
    //prefilteredColor * (F0*(1 - Fc) + Fc)

    return prefilteredColor * ( F0*envBRDF.x + envBRDF.y );
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

#undef STR

#endif //PBRSHADERS_H
