//
// Created by Marrony Neris on 12/1/15.
//

#ifndef MATRIX_H
#define MATRIX_H

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Identity(float out[16]) {
    for(int i = 0; i < 16; i++)
        out[i] = 0;

    out[ 0] = 1;
    out[ 5] = 1;
    out[10] = 1;
    out[15] = 1;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Mul(const float in0[16], const float in1[16], float out[16]) {
    float temp[16];

    for(int i = 0; i < 16; i++)
        temp[i] = 0;

    mnVector4MulAddScalar(in0 +  0, in1[ 0], temp +  0);
    mnVector4MulAddScalar(in0 +  4, in1[ 1], temp +  0);
    mnVector4MulAddScalar(in0 +  8, in1[ 2], temp +  0);
    mnVector4MulAddScalar(in0 + 12, in1[ 3], temp +  0);

    mnVector4MulAddScalar(in0 +  0, in1[ 4], temp +  4);
    mnVector4MulAddScalar(in0 +  4, in1[ 5], temp +  4);
    mnVector4MulAddScalar(in0 +  8, in1[ 6], temp +  4);
    mnVector4MulAddScalar(in0 + 12, in1[ 7], temp +  4);

    mnVector4MulAddScalar(in0 +  0, in1[ 8], temp +  8);
    mnVector4MulAddScalar(in0 +  4, in1[ 9], temp +  8);
    mnVector4MulAddScalar(in0 +  8, in1[10], temp +  8);
    mnVector4MulAddScalar(in0 + 12, in1[11], temp +  8);

    mnVector4MulAddScalar(in0 +  0, in1[12], temp + 12);
    mnVector4MulAddScalar(in0 +  4, in1[13], temp + 12);
    mnVector4MulAddScalar(in0 +  8, in1[14], temp + 12);
    mnVector4MulAddScalar(in0 + 12, in1[15], temp + 12);

    for(int i = 0; i < 16; i++)
        out[i] = temp[i];
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Translate(const float v[3], float out[16]) {
    mnMatrix4Identity(out);

    out[12] = v[0];
    out[13] = v[1];
    out[14] = v[2];
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Scale(const float s[3], float out[16]) {
    mnMatrix4Identity(out);

    out[ 0] = s[0];
    out[ 5] = s[1];
    out[10] = s[2];
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4RotateX(float angle, float out[16]) {
    mnMatrix4Identity(out);

    float c = cosf(angle);
    float s = sinf(angle);

    out[ 5] = c;
    out[ 9] = -s;
    out[ 6] = s;
    out[10] = c;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4RotateY(float angle, float out[16]) {
    mnMatrix4Identity(out);

    float c = cosf(angle);
    float s = sinf(angle);

    out[ 0] = c;
    out[ 8] = s;
    out[ 2] = -s;
    out[10] = c;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4RotateZ(float angle, float out[16]) {
    mnMatrix4Identity(out);

    float c = cosf(angle);
    float s = sinf(angle);

    out[0] = c;
    out[4] = -s;
    out[1] = s;
    out[5] = c;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Transformation(const float axis[3], float angle, const float tx[3], const float sc[3], float out[16]) {
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1 - c;
    float x = axis[0];
    float y = axis[1];
    float z = axis[2];

    float xs = x*s;
    float ys = y*s;
    float zs = z*s;

    out[0] = t*x*x + c;    out[4] = t*x*y - zs;   out[ 8] = t*x*z + ys;   out[12] = tx[0];
    out[1] = t*x*y + zs;   out[5] = t*y*y + c;    out[ 9] = t*y*z - xs;   out[13] = tx[1];
    out[2] = t*x*z - ys;   out[6] = t*y*z + xs;   out[10] = t*z*z + c;    out[14] = tx[2];
    out[3] = 0;            out[7] = 0;            out[11] = 0;            out[15] = 1;

    mnVector4MulScalar(out + 0, sc[0], out + 0);
    mnVector4MulScalar(out + 4, sc[1], out + 4);
    mnVector4MulScalar(out + 8, sc[2], out + 8);
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4MulVector2(const float mat[16], const float v[2], float out[2]) {
    float temp[2] = {0, 0};

    mnVector2MulAddScalar(mat + 0, v[0], temp);
    mnVector2MulAddScalar(mat + 4, v[1], temp);

    out[0] = temp[0];
    out[1] = temp[1];
}

void mnMatrix4MulVector3(const float mat[16], const float v[3], float out[3]) {
    float temp[3] = {0, 0, 0};

    mnVector3MulAddScalar(mat + 0, v[0], temp);
    mnVector3MulAddScalar(mat + 4, v[1], temp);
    mnVector3MulAddScalar(mat + 8, v[2], temp);

    out[0] = temp[0];
    out[1] = temp[1];
    out[2] = temp[2];
}

void mnMatrix4MulVector4(const float mat[16], const float v[4], float out[4]) {
    float temp[4] = {0, 0, 0, 0};

    mnVector4MulAddScalar(mat +  0, v[0], temp);
    mnVector4MulAddScalar(mat +  4, v[1], temp);
    mnVector4MulAddScalar(mat +  8, v[2], temp);
    mnVector4MulAddScalar(mat + 12, v[3], temp);

    out[0] = temp[0];
    out[1] = temp[1];
    out[2] = temp[2];
    out[3] = temp[3];
}

void mnVector4MulMatrix4(const float v[4], const float mat[16], float out[4]) {
    float temp[4];

    temp[0] = mnVector4Dot(v, mat + 0);
    temp[1] = mnVector4Dot(v, mat + 4);
    temp[2] = mnVector4Dot(v, mat + 8);
    temp[3] = mnVector4Dot(v, mat + 12);

    out[0] = temp[0];
    out[1] = temp[1];
    out[2] = temp[2];
    out[3] = temp[3];
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4MulPoint2(const float mat[16], const float pt[2], float out[2]) {
    mnMatrix4MulVector2(mat, pt, out);
    mnVector2Add(mat + 12, out, out);

    if(out[1] != 0.0f && out[1] != 1.0f)
        mnVector2MulScalar(out, 1.0f / out[1], out);
}

void mnMatrix4MulPoint3(const float mat[16], const float pt[3], float out[3]) {
    mnMatrix4MulVector3(mat, pt, out);
    mnVector3Add(mat + 12, out, out);

    if(out[2] != 0.0f && out[2] != 1.0f)
        mnVector3MulScalar(out, 1.0f / out[2], out);
}

void mnMatrix4MulPoint4(const float mat[16], const float pt[4], float out[4]) {
    mnMatrix4MulVector4(mat, pt, out);

    if(out[3] != 0.0f && out[3] != 1.0f)
        mnVector4MulScalar(out, 1.0f / out[3], out);
}

void mnPoint4MulMatrix4(const float pt[4], const float mat[16], float out[4]) {
    mnVector4MulMatrix4(pt, mat, out);

    if(out[3] != 0.0f && out[3] != 1.0f)
        mnVector4MulScalar(out, 1.0f / out[3], out);
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Frustum(float left, float right, float bottom, float top, float znear, float zfar, float out[16]) {
    const float diffx = right - left;
    const float diffy = top - bottom;
    const float diffz = zfar - znear;
    const float znear2 = 2.0f * znear;

    const float a = (right + left) / diffx;
    const float b = (top + bottom) / diffy;
    const float c = -(zfar + znear) / diffz;
    const float d = -(2.0f * zfar * znear) / diffz;

    out[0] = znear2 / diffx;  out[4] = 0;               out[ 8] = a;   out[12] = 0;
    out[1] = 0;               out[5] = znear2 / diffy;  out[ 9] = b;   out[13] = 0;
    out[2] = 0;               out[6] = 0;               out[10] = c;   out[14] = d;
    out[3] = 0;               out[7] = 0;               out[11] = -1;  out[15] = 0;
}

//////////////////////////////////////////////////////////////////////////////

//default opengl RIGHT_HANDED = 0 and ZERO_TO_ONE = 0
//default dx RIGHT_HANDED = 1 and ZERO_TO_ONE = 1

#define RIGHT_HANDED 0
#define ZERO_TO_ONE 0

void mnMatrix4Ortho(float left, float right, float bottom, float top, float znear, float zfar, float out[16]) {
    float diffx = right - left;
    float diffy = top - bottom;
    float diffz = zfar - znear;

    float tx = -(right + left) / diffx;
    float ty = -(top + bottom) / diffy;

#if RIGHT_HANDED

#if ZERO_TO_ONE
    out[0] = 2.0f / diffx; out[4] = 0;            out[ 8] = 0;                        out[12] = 0;
    out[1] = 0;            out[5] = 2.0f / diffy; out[ 9] = 0;                        out[13] = 0;
    out[2] = 0;            out[6] = 0;            out[10] = -1.0f / diffz;            out[14] = 0;
    out[3] = tx;           out[7] = ty;           out[11] = -znear / diffz;           out[15] = 1;
#else
    out[0] = 2.0f / diffx; out[4] = 0;            out[ 8] = 0;                        out[12] = 0;
    out[1] = 0;            out[5] = 2.0f / diffy; out[ 9] = 0;                        out[13] = 0;
    out[2] = 0;            out[6] = 0;            out[10] = -2.0f / diffz;            out[14] = 0;
    out[3] = tx;           out[7] = ty;           out[11] = -(zfar + znear) / diffz;  out[15] = 1;
#endif

#else //RIGHT_HANDED

    #if ZERO_TO_ONE
    out[0] = 2.0f / diffx; out[4] = 0;            out[ 8] = 0;             out[12] = tx;
    out[1] = 0;            out[5] = 2.0f / diffy; out[ 9] = 0;             out[13] = ty;
    out[2] = 0;            out[6] = 0;            out[10] = 1.0f / diffz;  out[14] = -znear / diffz;
    out[3] = 0;            out[7] = 0;            out[11] = 0;             out[15] = 1;
#else
    out[0] = 2.0f / diffx; out[4] = 0;            out[ 8] = 0;             out[12] = tx;
    out[1] = 0;            out[5] = 2.0f / diffy; out[ 9] = 0;             out[13] = ty;
    out[2] = 0;            out[6] = 0;            out[10] = 2.0f / diffz;  out[14] = -(zfar + znear) / diffz;
    out[3] = 0;            out[7] = 0;            out[11] = 0;             out[15] = 1;
#endif

#endif
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Ortho(float left, float right, float bottom, float top, float out[16]) {
    return mnMatrix4Ortho(left, right, bottom, top, -1.0f, +1.0f, out);
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Perspective(float fovy, float aspect, float znear, float zfar, float out[16]) {
    float f = 1.0f / tanf(fovy / 2.0f);
    float diffz = zfar - znear;

#if RIGHT_HANDED

#if ZERO_TO_ONE
    out[0] = f / aspect;  out[4] = 0;  out[ 8] = 0;                            out[12] = 0;
    out[1] = 0;           out[5] = f;  out[ 9] = 0;                            out[13] = 0;
    out[2] = 0;           out[6] = 0;  out[10] = -zfar / diffz;                out[14] = -1;
    out[3] = 0;           out[7] = 0;  out[11] = -(zfar * znear) / diffz;      out[15] = 0;
#else
    out[0] = f / aspect;  out[4] = 0;  out[ 8] = 0;                            out[12] = 0;
    out[1] = 0;           out[5] = f;  out[ 9] = 0;                            out[13] = 0;
    out[2] = 0;           out[6] = 0;  out[10] = -(zfar+znear) / diffz;        out[14] = -1;
    out[3] = 0;           out[7] = 0;  out[11] = -(2 * zfar * znear) / diffz;  out[15] = 0;
#endif

#else //RIGHT_HANDED

#if ZERO_TO_ONE
    out[0] = f / aspect;  out[4] = 0;  out[ 8] = 0;                      out[12] = 0;
    out[1] = 0;           out[5] = f;  out[ 9] = 0;                      out[13] = 0;
    out[2] = 0;           out[6] = 0;  out[10] = zfar / diffz;           out[14] = -(zfar * znear) / diffz;
    out[3] = 0;           out[7] = 0;  out[11] = 1;                      out[15] = 0;
#else
    out[0] = f / aspect;  out[4] = 0;  out[ 8] = 0;                      out[12] = 0;
    out[1] = 0;           out[5] = f;  out[ 9] = 0;                      out[13] = 0;
    out[2] = 0;           out[6] = 0;  out[10] = (zfar+znear) / diffz;   out[14] = -(2 * zfar * znear) / diffz;
    out[3] = 0;           out[7] = 0;  out[11] = 1;                      out[15] = 0;
#endif

#endif //RIGHT_HANDED

}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4LookAt(const float eye[3], const float at[3], float out[16]) {
    float x[3], y[3], z[3];
    float up[3] = {0.0f, 1.0f, 0.0f};

#if RIGHT_HANDED
    mnVector3Sub(eye, at, z);
    mnVector3Normalize(z, z);

    if(1.0f - fabsf(z[1]) <= 0.005f) {
        z[0] = 0;
        z[1] = 0;
        z[2] = -1;
    } else {
        mnVector3Cross(up, z, x);
    }

    mnVector3Cross(z, x, y);

    float tx = mnVector3Dot(x, eye);
    float ty = mnVector3Dot(y, eye);
    float tz = mnVector3Dot(z, eye);

    out[0] = x[0];  out[4] = y[0];  out[ 8] = z[0];  out[12] = 0;
    out[1] = x[1];  out[5] = y[1];  out[ 9] = z[1];  out[13] = 0;
    out[2] = x[2];  out[6] = y[2];  out[10] = z[2];  out[14] = 0;
    out[3] = tx;    out[7] = ty;    out[11] = tz;    out[15] = 1;
#else
    mnVector3Sub(at, eye, z);
    mnVector3Normalize(z, z);

    if(1.0f - fabsf(z[1]) <= 0.005f) {
        z[0] = 0;
        z[1] = 0;
        z[2] = 1;
    } else {
        mnVector3Cross(up, z, x);
    }

    mnVector3Cross(z, x, y);

    float tx = -mnVector3Dot(x, eye);
    float ty = -mnVector3Dot(y, eye);
    float tz = -mnVector3Dot(z, eye);

    out[0] = x[0];  out[4] = x[1];  out[ 8] = x[2];  out[12] = tx;
    out[1] = y[0];  out[5] = y[1];  out[ 9] = y[2];  out[13] = ty;
    out[2] = z[0];  out[6] = z[1];  out[10] = z[2];  out[14] = tz;
    out[3] = 0;     out[7] = 0;     out[11] = 0;     out[15] = 1;
#endif
}

//////////////////////////////////////////////////////////////////////////////

union Matrix4 {
    float column[4][4];
    float values[16];
};

#endif //MATRIX_H
