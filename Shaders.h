//
// Created by Marrony Neris on 11/26/15.
//

#ifndef SHADERS_H
#define SHADERS_H

#define STR(x) #x

const char* commonSource = STR(
struct InstanceData {
    mat4 in_Rotation;
    vec4 in_Color;
};

struct VertexData {
    vec4 position;
    mat3 tbn;
    vec2 texture;
    vec4 color;
};

\n#ifdef SEPARATE_VERTEX_SHADER\n
out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};
\n#endif\n

struct LightData {
    vec3 position;
    vec3 color;
};

vec3 calculateLight(vec3 position, vec3 normal, vec4 albedo, LightData light) {
    vec3 lightVec = light.position - position;
    vec3 lightDir = normalize(lightVec);
    return max(dot(normal, lightDir), 0) * albedo.rgb * light.color;
}

struct GBuffer {
    vec3 position;
    vec3 normal;
    vec4 color;
};

void writeGBuffer(GBuffer gBuffer, out vec4 buffer0, out vec4 buffer1, out vec4 buffer2) {
    buffer0 = vec4(gBuffer.position, 1);
    buffer1 = vec4(gBuffer.normal, 1);
    buffer2 = gBuffer.color;
}

GBuffer readGBuffer(vec4 buffer0, vec4 buffer1, vec4 buffer2) {
    GBuffer gBuffer;
    gBuffer.position = buffer0.xyz;
    gBuffer.normal = buffer1.xyz;
    gBuffer.color = buffer2;
    return gBuffer;
}

);

const char* vertexSource = STR(
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_Texture;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec3 in_Tangent;

layout(std140) uniform in_FrameData {
    mat4 projection;
    mat4 view;
};

layout(std140) uniform in_InstanceData {
    InstanceData instanceData[4];
};

out VertexData vtx;

void main() {
    vtx.position = instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1);
    gl_Position = projection * view * vtx.position;
    vtx.texture = in_Texture;

    vec3 normal = normalize(mat3(instanceData[gl_InstanceID].in_Rotation) * in_Normal);
    vec3 tangent = normalize(mat3(instanceData[gl_InstanceID].in_Rotation) * in_Tangent);
    vec3 bitangent = cross(tangent, normal);

    vtx.tbn = mat3(tangent, bitangent, normal);
    vtx.color = instanceData[gl_InstanceID].in_Color;
}
);

const char* geometrySource = STR(
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in VertexData vtx[];

out VertexData geo;

void main() {
    gl_Position = gl_in[0].gl_Position;
    geo.position = vtx[0].position;
    geo.tbn = vtx[0].tbn;
    geo.texture = vtx[0].texture;
    geo.color = vtx[0].color;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    geo.position = vtx[1].position;
    geo.tbn = vtx[1].tbn;
    geo.texture = vtx[1].texture;
    geo.color = vtx[1].color;
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    geo.position = vtx[2].position;
    geo.tbn = vtx[2].tbn;
    geo.texture = vtx[2].texture;
    geo.color = vtx[2].color;
    EmitVertex();

    EndPrimitive();
}
);

const char* fragmentSource = STR(
in VertexData vtx;

layout(std140) uniform in_InstanceData {
    InstanceData instanceData[4];
};

uniform sampler2D in_MainTex;
uniform sampler2D in_BumpMap;

layout(location = 0) out vec4 out_Position;
layout(location = 1) out vec4 out_Normal;
layout(location = 2) out vec4 out_Albedo;

vec3 getNormal() {
    return texture(in_BumpMap, vtx.texture).xyz * 2.0 - 1.0;
}

vec4 getBaseColor() {
    return vec4(texture(in_MainTex, vtx.texture).rgb * vtx.color.rgb, 1);
}

vec4 getPosition() {
    return vtx.position;
}

void main() {
    GBuffer gBuffer;
    gBuffer.position = getPosition().xyz;
    gBuffer.normal = normalize(vtx.tbn * getNormal());
    gBuffer.color = getBaseColor();

    writeGBuffer(gBuffer, out_Position, out_Normal, out_Albedo);
}
);

const char* vertexTransparencySource = STR(
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_Texture;
layout(location = 2) in vec3 in_Normal;
layout(location = 3) in vec3 in_Tangent;

layout(std140) uniform in_FrameData {
    mat4 projection;
    mat4 view;
};

layout(std140) uniform in_InstanceData {
    InstanceData instanceData[4];
};

out VertexData vtx;

void main() {
    vtx.position = instanceData[gl_InstanceID].in_Rotation * vec4(in_Position, 1);
    gl_Position = projection * view * vtx.position;
    vtx.texture = in_Texture;

    vec3 normal = normalize(mat3(instanceData[gl_InstanceID].in_Rotation) * in_Normal);
    vec3 tangent = normalize(mat3(instanceData[gl_InstanceID].in_Rotation) * in_Tangent);
    vec3 bitangent = cross(tangent, normal);

    vtx.tbn = mat3(tangent, bitangent, normal);
    vtx.color = instanceData[gl_InstanceID].in_Color;
}
);

const char* fragmentTransparencySource = STR(
in VertexData vtx;

layout(std140) uniform in_InstanceData {
    InstanceData instanceData[4];
};

layout(std140) uniform in_LightData {
    LightData lightData[3];
};

uniform sampler2D in_MainTex;
uniform sampler2D in_BumpMap;

layout(location = 0) out vec4 out_Color;

vec4 getBaseColor() {
    return texture(in_MainTex, vtx.texture);
}

vec3 getNormal() {
    return texture(in_BumpMap, vtx.texture).xyz * 2.0 - 1.0;
}

vec4 getPosition() {
    return vtx.position;
}

void main() {
    vec3 normal = getNormal();
    vec4 albedo = getBaseColor();
    vec3 position = getPosition().xyz;

    normal = normalize(vtx.tbn * normal);

    float alpha = 0.5;

    out_Color.rgb = alpha * albedo.rgb * 0.2;
    out_Color.rgb += alpha * calculateLight(position, normal, albedo, lightData[0]);
    out_Color.rgb += alpha * calculateLight(position, normal, albedo, lightData[1]);
    out_Color.rgb += alpha * calculateLight(position, normal, albedo, lightData[2]);
    out_Color.a = alpha;
}
);

////////////////////////////////////////////////////////////////////////////////////
const char* quadVertexSource = STR(
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec2 in_Texture;

out vec2 vtx_Texture;

void main() {
    gl_Position = vec4(in_Position, 1);
    vtx_Texture = in_Texture;
}
);

const char* quadGeometrySource = STR(
layout (triangles) in;
layout (triangle_strip, max_vertices=4) out;

in vec2 vtx_Texture[];

out vec2 geo_Texture;

void main() {
    gl_Position = gl_in[0].gl_Position;
    geo_Texture = vtx_Texture[0];
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    geo_Texture = vtx_Texture[1];
    EmitVertex();

    gl_Position = (gl_in[0].gl_Position + gl_in[2].gl_Position) * 0.5;
    geo_Texture = (vtx_Texture[0] + vtx_Texture[2]) * 0.5;
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    geo_Texture = vtx_Texture[2];
    EmitVertex();

    EndPrimitive();
}
);

const char* quadFragmentSource = STR(
in vec2 geo_Texture;

layout(std140) uniform in_LightData {
    LightData lightData[3];
};

uniform sampler2D in_Position;
uniform sampler2D in_Normal;
uniform sampler2D in_Albedo;

layout(location = 0) out vec4 out_FragColor;

void main() {
    vec4 buffer0 = texture(in_Position, geo_Texture);
    vec4 buffer1 = texture(in_Normal, geo_Texture);
    vec4 buffer2 = texture(in_Albedo, geo_Texture);
    GBuffer gBuffer = readGBuffer(buffer0, buffer1, buffer2);

    vec3 position = gBuffer.position;
    vec3 normal = gBuffer.normal;
    vec4 albedo = gBuffer.color;

    vec3 lighting = albedo.rgb * 0.2;
    lighting += calculateLight(position, normal, albedo, lightData[0]);
    lighting += calculateLight(position, normal, albedo, lightData[1]);
    lighting += calculateLight(position, normal, albedo, lightData[2]);

    out_FragColor.rgb = lighting;
    out_FragColor.a = 1;
}
);

const char* copyFragmentSource = STR(
in vec2 geo_Texture;

uniform sampler2D in_Texture;

layout(location = 0) out vec4 out_FragColor;

void main() {
    out_FragColor = texture(in_Texture, geo_Texture);
}
);

#undef STR

#endif //SHADERS_H
