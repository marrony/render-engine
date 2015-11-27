//
// Created by Marrony Neris on 11/26/15.
//

#ifndef SHADERS_H
#define SHADERS_H

#define STR(x) #x

const char* vertexSource = STR(
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec3 in_Tangent;
layout(location = 3) in vec2 in_Texture;

struct InstanceData {
    mat3 in_Rotation;
    vec4 in_Color;
    vec4 in_Offset_Scale;
};

layout(std140) uniform in_InstanceData {
    InstanceData instanceData[4];
};

out VertexData {
    vec3 position;
    mat3 tbn;
    vec2 texture;
    vec4 color;
} vtx;

void main() {
    vec3 offset = instanceData[gl_InstanceID].in_Offset_Scale.xyz;
    float scale = instanceData[gl_InstanceID].in_Offset_Scale.w;
    vtx.position = (instanceData[gl_InstanceID].in_Rotation * in_Position * scale) + offset;
    gl_Position = vec4(vtx.position, 1);
    vtx.texture = in_Texture;

    vec3 normal = normalize(instanceData[gl_InstanceID].in_Rotation * in_Normal);
    vec3 tangent = normalize(instanceData[gl_InstanceID].in_Rotation * in_Tangent);
    vec3 bitangent = cross(tangent, normal);

    vtx.tbn = mat3(tangent, bitangent, normal);
    vtx.color = instanceData[gl_InstanceID].in_Color;
}
);

const char* geometrySource = STR(
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in VertexData {
    vec3 position;
    mat3 tbn;
    vec2 texture;
    vec4 color;
} vtx[];


out GeometryData {
    vec3 position;
    mat3 tbn;
    vec2 texture;
    vec4 color;
} geo;

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
in VertexData {
    vec3 position;
    mat3 tbn;
    vec2 texture;
    vec4 color;
} vtx;

in GeometryData {
    vec3 position;
    mat3 tbn;
    vec2 texture;
    vec4 color;
} geo;

struct InstanceData {
    mat3 in_Rotation;
    vec4 in_Color;
    vec4 in_Offset_Scale;
};

layout(std140) uniform in_InstanceData {
    InstanceData instanceData[4];
};

uniform sampler2D in_MainTex;
uniform sampler2D in_BumpMap;

layout(location = 0) out vec3 out_Position;
layout(location = 1) out vec3 out_Normal;
layout(location = 2) out vec4 out_Albedo;

void main() {
    vec3 normal = texture(in_BumpMap, geo.texture).xyz * 2.0 - 1.0;

    out_Position = geo.position;
    out_Normal = normalize(geo.tbn * normal);
    out_Albedo.rgb = texture(in_MainTex, geo.texture).rgb * geo.color.rgb;
    out_Albedo.w = 1;
}
);

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
    vec3 in_LightPos[3];
};

uniform sampler2D in_Position;
uniform sampler2D in_Normal;
uniform sampler2D in_Albedo;

layout(location = 0) out vec4 out_FragColor;

vec3 calculateLight(vec3 position, vec3 normal, vec4 albedo, vec3 lightPos, vec3 lightColor) {
    vec3 lightVec = lightPos - position;
    vec3 lightDir = normalize(lightVec);
    return max(dot(normal, lightDir), 0) * albedo.rgb * lightColor;
}

void main() {
    vec3 position = texture(in_Position, geo_Texture).rgb;
    vec3 normal = texture(in_Normal, geo_Texture).rgb;
    vec4 albedo = texture(in_Albedo, geo_Texture);

    vec3 lighting = albedo.rgb * 0.2;
    lighting += calculateLight(position, normal, albedo, in_LightPos[0], vec3(1.0, 0.5, 0.5));
    lighting += calculateLight(position, normal, albedo, in_LightPos[1], vec3(0.5, 1.0, 0.5));
    lighting += calculateLight(position, normal, albedo, in_LightPos[2], vec3(0.5, 0.5, 1.0));

    out_FragColor.rgb = lighting;
    out_FragColor.a = 1;
}
);

#undef STR

#endif //SHADERS_H
