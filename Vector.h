//
// Created by Marrony Neris on 12/1/15.
//

#ifndef VECTOR_H
#define VECTOR_H

//////////////////////////////////////////////////////////////////////////////

void mnVector2Add(const float in0[2], const float in1[2], float out[2]) {
    out[0] = in0[0] + in1[0];
    out[1] = in0[1] + in1[1];
}

void mnVector3Add(const float in0[3], const float in1[3], float out[3]) {
    out[0] = in0[0] + in1[0];
    out[1] = in0[1] + in1[1];
    out[2] = in0[2] + in1[2];
}

void mnVector4Add(const float in0[4], const float in1[4], float out[4]) {
    out[0] = in0[0] + in1[0];
    out[1] = in0[1] + in1[1];
    out[2] = in0[2] + in1[2];
    out[3] = in0[3] + in1[3];
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2Sub(const float in0[2], const float in1[2], float out[2]) {
    out[0] = in0[0] - in1[0];
    out[1] = in0[1] - in1[1];
}

void mnVector3Sub(const float in0[3], const float in1[3], float out[3]) {
    out[0] = in0[0] - in1[0];
    out[1] = in0[1] - in1[1];
    out[2] = in0[2] - in1[2];
}

void mnVector4Sub(const float in0[4], const float in1[4], float out[4]) {
    out[0] = in0[0] - in1[0];
    out[1] = in0[1] - in1[1];
    out[2] = in0[2] - in1[2];
    out[3] = in0[3] - in1[3];
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2Mul(const float in0[2], const float in1[2], float out[2]) {
    out[0] = in0[0] * in1[0];
    out[1] = in0[1] * in1[1];
}

void mnVector3Mul(const float in0[3], const float in1[3], float out[3]) {
    out[0] = in0[0] * in1[0];
    out[1] = in0[1] * in1[1];
    out[2] = in0[2] * in1[2];
}

void mnVector4Mul(const float in0[4], const float in1[4], float out[4]) {
    out[0] = in0[0] * in1[0];
    out[1] = in0[1] * in1[1];
    out[2] = in0[2] * in1[2];
    out[3] = in0[3] * in1[3];
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2MulScalar(const float in0[2], const float in1, float out[2]) {
    out[0] = in0[0] * in1;
    out[1] = in0[1] * in1;
}

void mnVector3MulScalar(const float in0[3], const float in1, float out[3]) {
    out[0] = in0[0] * in1;
    out[1] = in0[1] * in1;
    out[2] = in0[2] * in1;
}

void mnVector4MulScalar(const float in0[4], const float in1, float out[4]) {
    out[0] = in0[0] * in1;
    out[1] = in0[1] * in1;
    out[2] = in0[2] * in1;
    out[3] = in0[3] * in1;
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2AddScalar(const float in0[2], const float in1, float out[2]) {
    out[0] = in0[0] + in1;
    out[1] = in0[1] + in1;
}

void mnVector3AddScalar(const float in0[3], const float in1, float out[3]) {
    out[0] = in0[0] + in1;
    out[1] = in0[1] + in1;
    out[2] = in0[2] + in1;
}

void mnVector4AddScalar(const float in0[4], const float in1, float out[4]) {
    out[0] = in0[0] + in1;
    out[1] = in0[1] + in1;
    out[2] = in0[2] + in1;
    out[3] = in0[3] + in1;
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2SubScalar(const float in0[2], const float in1, float out[2]) {
    out[0] = in0[0] - in1;
    out[1] = in0[1] - in1;
}

void mnVector3SubScalar(const float in0[3], const float in1, float out[3]) {
    out[0] = in0[0] - in1;
    out[1] = in0[1] - in1;
    out[2] = in0[2] - in1;
}

void mnVector4SubScalar(const float in0[4], const float in1, float out[4]) {
    out[0] = in0[0] - in1;
    out[1] = in0[1] - in1;
    out[2] = in0[2] - in1;
    out[3] = in0[3] - in1;
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2MulAddScalar(const float in0[2], const float in1, float out[2]) {
    out[0] += in0[0] * in1;
    out[1] += in0[1] * in1;
}

void mnVector3MulAddScalar(const float in0[3], const float in1, float out[3]) {
    out[0] += in0[0] * in1;
    out[1] += in0[1] * in1;
    out[2] += in0[2] * in1;
}

void mnVector4MulAddScalar(const float in0[4], const float in1, float out[4]) {
    out[0] += in0[0] * in1;
    out[1] += in0[1] * in1;
    out[2] += in0[2] * in1;
    out[3] += in0[3] * in1;
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2Div(const float in0[2], const float in1[2], float out[2]) {
    out[0] = in0[0] / in1[0];
    out[1] = in0[1] / in1[1];
}

void mnVector3Div(const float in0[3], const float in1[3], float out[3]) {
    out[0] = in0[0] / in1[0];
    out[1] = in0[1] / in1[1];
    out[2] = in0[2] / in1[2];
}

void mnVector4Div(const float in0[4], const float in1[4], float out[4]) {
    out[0] = in0[0] / in1[0];
    out[1] = in0[1] / in1[1];
    out[2] = in0[2] / in1[2];
    out[3] = in0[3] / in1[3];
}

//////////////////////////////////////////////////////////////////////////////

void mnVector3Cross(const float in0[3], const float in1[3], float out[3]) {
    float temp[3];

    temp[0] = in0[1]*in1[2] - in0[2]*in1[1];
    temp[1] = in0[2]*in1[0] - in0[0]*in1[2];
    temp[2] = in0[0]*in1[1] - in0[1]*in1[0];

    out[0] = temp[0];
    out[1] = temp[1];
    out[2] = temp[2];
}

//////////////////////////////////////////////////////////////////////////////

float mnVector2Dot(const float in0[2], const float in1[2]) {
    return in0[0]*in1[0] + in0[1]*in1[1];
}

float mnVector3Dot(const float in0[3], const float in1[3]) {
    return in0[0]*in1[0] + in0[1]*in1[1] + in0[2]*in1[2];
}

float mnVector4Dot(const float in0[4], const float in1[4]) {
    return in0[0]*in1[0] + in0[1]*in1[1] + in0[2]*in1[2] + in0[3]*in1[3];
}

//////////////////////////////////////////////////////////////////////////////

float mnVector2Length(const float in[2]) {
    return sqrtf(mnVector2Dot(in, in));
}

float mnVector3Length(const float in[3]) {
    return sqrtf(mnVector3Dot(in, in));
}

float mnVector4Length(const float in[4]) {
    return sqrtf(mnVector4Dot(in, in));
}

//////////////////////////////////////////////////////////////////////////////

void mnVector2Normalize(const float in[2], float out[2]) {
    const float length = mnVector2Length(in);
    const float invLength[2] = {1.0f / length, 1.0f / length};

    mnVector2Mul(in, invLength, out);
}

void mnVector3Normalize(const float in[3], float out[3]) {
    const float length = mnVector3Length(in);
    const float invLength[3] = {1.0f / length, 1.0f / length, 1.0f / length};

    mnVector3Mul(in, invLength, out);
}

void mnVector4Normalize(const float in[4], float out[4]) {
    const float length = mnVector4Length(in);
    const float invLength[4] = {1.0f / length, 1.0f / length, 1.0f / length, 1.0f / length};

    mnVector4Mul(in, invLength, out);
}

//////////////////////////////////////////////////////////////////////////////

union Vector2 {
    struct {
        float x, y;
    };
    float values[2];
};

union Vector3 {
    struct {
        float x, y, z;
    };
    float values[3];
};

union Vector4 {
    struct {
        float x, y, z, w;
    };
    float values[4];
};

#endif //VECTOR_H
