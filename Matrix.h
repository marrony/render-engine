//
// Created by Marrony Neris on 12/1/15.
//

#ifndef MATRIX_H
#define MATRIX_H

#define RIGHT_HANDED 0
#define ZERO_TO_ONE 0

#if RIGHT_HANDED
#define mnMatrix4Frustum mnMatrix4FrustumRH
#define mnMatrix4Ortho mnMatrix4OrthoRH
#define mnMatrix4Perspective mnMatrix4PerspectiveRH
#define mnMatrix4InfinitePerspective mnMatrix4InfinitePerspectiveRH
#define mnMatrix4LookAt mnMatrix4LookAtRH
#else
#define mnMatrix4Frustum mnMatrix4FrustumLH
#define mnMatrix4Ortho mnMatrix4OrthoLH
#define mnMatrix4Perspective mnMatrix4PerspectiveLH
#define mnMatrix4InfinitePerspective mnMatrix4InfinitePerspectiveLH
#define mnMatrix4LookAt mnMatrix4LookAtLH
#endif

/*
 * mnMatrix4Perspective is equivalent to
 *
 * fH = tan(fovy / 2) * znear;
 * fW = fH * aspect
 * mnMatrix4Frustum(-fW, +fW, -fH, +fH, znear, zfar, out);
 */

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
    out[ 6] = s;
    out[ 9] = -s;
    out[10] = c;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4RotateY(float angle, float out[16]) {
    mnMatrix4Identity(out);

    float c = cosf(angle);
    float s = sinf(angle);

    out[ 0] = c;
    out[ 2] = -s;
    out[ 8] = s;
    out[10] = c;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4RotateZ(float angle, float out[16]) {
    mnMatrix4Identity(out);

    float c = cosf(angle);
    float s = sinf(angle);

    out[0] = c;
    out[1] = s;
    out[4] = -s;
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

void mnVector2MulMatrix4(const float v[2], const float mat[16], float out[2]) {
    float temp[2];

    temp[0] = mnVector2Dot(v, mat + 0);
    temp[1] = mnVector2Dot(v, mat + 4);

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

void mnVector3MulMatrix4(const float v[3], const float mat[16], float out[3]) {
    float temp[3];

    temp[0] = mnVector3Dot(v, mat + 0);
    temp[1] = mnVector3Dot(v, mat + 4);
    temp[2] = mnVector3Dot(v, mat + 8);

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

void mnMatrix4FrustumRH(float left, float right, float bottom, float top, float znear, float zfar, float out[16]) {
    const float diffx = right - left;
    const float diffy = top - bottom;
    const float diffz = zfar - znear;
    const float znear2 = 2.0f * znear;

    const float a = (right + left) / diffx;
    const float b = (top + bottom) / diffy;
#if ZERO_TO_ONE
    const float c = -zfar / diffz;
    const float d = -(zfar * znear) / diffz;
#else
    const float c = -(zfar + znear) / diffz;
    const float d = -(2.0f * zfar * znear) / diffz;
#endif

    out[0] = znear2 / diffx;  out[4] = 0;               out[ 8] = a;   out[12] = 0;
    out[1] = 0;               out[5] = znear2 / diffy;  out[ 9] = b;   out[13] = 0;
    out[2] = 0;               out[6] = 0;               out[10] = c;   out[14] = d;
    out[3] = 0;               out[7] = 0;               out[11] = -1;  out[15] = 0;
}

void mnMatrix4FrustumLH(float left, float right, float bottom, float top, float znear, float zfar, float out[16]) {
    const float diffx = right - left;
    const float diffy = top - bottom;
    const float diffz = zfar - znear;
    const float znear2 = 2.0f * znear;

    const float a = (right + left) / diffx;
    const float b = (top + bottom) / diffy;
#if ZERO_TO_ONE
    const float c = zfar / diffz;
    const float d = -(zfar * znear) / diffz;
#else
    const float c = (zfar + znear) / diffz;
    const float d = -(2.0f * zfar * znear) / diffz;
#endif

    out[0] = znear2 / diffx;  out[4] = 0;               out[ 8] = a;   out[12] = 0;
    out[1] = 0;               out[5] = znear2 / diffy;  out[ 9] = b;   out[13] = 0;
    out[2] = 0;               out[6] = 0;               out[10] = c;   out[14] = d;
    out[3] = 0;               out[7] = 0;               out[11] = 1;   out[15] = 0;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4OrthoRH(float left, float right, float bottom, float top, float znear, float zfar, float out[16]) {
    float diffx = right - left;
    float diffy = top - bottom;
    float diffz = zfar - znear;

    float tx = -(right + left) / diffx;
    float ty = -(top + bottom) / diffy;

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
}

void mnMatrix4OrthoLH(float left, float right, float bottom, float top, float znear, float zfar, float out[16]) {
    float diffx = right - left;
    float diffy = top - bottom;
    float diffz = zfar - znear;

    float tx = -(right + left) / diffx;
    float ty = -(top + bottom) / diffy;

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
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4PerspectiveRH(float fovy, float aspect, float znear, float zfar, float out[16]) {
    float f = 1.0f / tanf(fovy / 2.0f);
    float diffz = znear - zfar;

#if ZERO_TO_ONE
    out[0] = f / aspect;  out[4] = 0;  out[ 8] = 0;                            out[12] = 0;
    out[1] = 0;           out[5] = f;  out[ 9] = 0;                            out[13] = 0;
    out[2] = 0;           out[6] = 0;  out[10] = zfar / diffz;                 out[14] = (zfar * znear) / diffz;
    out[3] = 0;           out[7] = 0;  out[11] = -1;                           out[15] = 0;
#else
    out[0] = f / aspect;  out[4] = 0;  out[ 8] = 0;                            out[12] = 0;
    out[1] = 0;           out[5] = f;  out[ 9] = 0;                            out[13] = 0;
    out[2] = 0;           out[6] = 0;  out[10] = (zfar+znear) / diffz;         out[14] = (2 * zfar * znear) / diffz;
    out[3] = 0;           out[7] = 0;  out[11] = -1;                           out[15] = 0;
#endif
}

void mnMatrix4PerspectiveLH(float fovy, float aspect, float znear, float zfar, float out[16]) {
    float f = 1.0f / tanf(fovy / 2.0f);
    float diffz = zfar - znear;

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
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4InfinitePerspectiveRH(float fovy, float aspect, float znear, float out[16]) {
    float range = tanf(fovy / 2.0f) * znear;
    float l = -range * aspect;
    float r = +range * aspect;
    float b = -range;
    float t = +range;
    float diffx = r - l;
    float diffy = t - b;

    out[0] = 2*znear/diffx;  out[4] = 0;              out[ 8] = 0;    out[12] = 0;
    out[1] = 0;              out[5] = 2*znear/diffy;  out[ 9] = 0;    out[13] = 0;
    out[2] = 0;              out[6] = 0;              out[10] = -1;   out[14] = -2*znear;
    out[3] = 0;              out[7] = 0;              out[11] = -1;   out[15] = 0;
}

void mnMatrix4InfinitePerspectiveLH(float fovy, float aspect, float znear, float out[16]) {
    float range = tanf(fovy / 2.0f) * znear;
    float l = -range * aspect;
    float r = +range * aspect;
    float b = -range;
    float t = +range;
    float diffx = r - l;
    float diffy = t - b;

    out[0] = 2*znear/diffx;  out[4] = 0;              out[ 8] = 0;    out[12] = 0;
    out[1] = 0;              out[5] = 2*znear/diffy;  out[ 9] = 0;    out[13] = 0;
    out[2] = 0;              out[6] = 0;              out[10] = 1;    out[14] = -2*znear;
    out[3] = 0;              out[7] = 0;              out[11] = 1;    out[15] = 0;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4LookAtRH(const float eye[3], const float at[3], float out[16]) {
    const float up[3] = {0, 1, 0};
    float s[3], u[3], f[3];

    mnVector3Sub(eye, at, f);
    mnVector3Normalize(f, f);

    mnVector3Cross(up, f, s);
    mnVector3Cross(f, s, u);

    float tx = -mnVector3Dot(s, eye);
    float ty = -mnVector3Dot(u, eye);
    float tz = -mnVector3Dot(f, eye);

    out[0] = s[0];  out[4] = s[1];  out[ 8] = s[2];  out[12] = tx;
    out[1] = u[0];  out[5] = u[1];  out[ 9] = u[2];  out[13] = ty;
    out[2] = f[0];  out[6] = f[1];  out[10] = f[2];  out[14] = tz;
    out[3] = 0;     out[7] = 0;     out[11] = 0;     out[15] = 1;
}

void mnMatrix4LookAtLH(const float eye[3], const float at[3], float out[16]) {
    const float up[3] = {0, 1, 0};
    float s[3], u[3], f[3];

    mnVector3Sub(at, eye, f);
    mnVector3Normalize(f, f);

    mnVector3Cross(up, f, s);
    mnVector3Cross(f, s, u);

    float tx = -mnVector3Dot(s, eye);
    float ty = -mnVector3Dot(u, eye);
    float tz = -mnVector3Dot(f, eye);

    out[0] = s[0];  out[4] = s[1];  out[ 8] = s[2];  out[12] = tx;
    out[1] = u[0];  out[5] = u[1];  out[ 9] = u[2];  out[13] = ty;
    out[2] = f[0];  out[6] = f[1];  out[10] = f[2];  out[14] = tz;
    out[3] = 0;     out[7] = 0;     out[11] = 0;     out[15] = 1;
}

//////////////////////////////////////////////////////////////////////////////

float determinant2x2(float a, float b, float c, float d) {
    return a*d - b*c;
}

float determinant3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
    float ca = +determinant2x2(e, f, h, i);
    float cb = -determinant2x2(d, f, g, i);
    float cc = +determinant2x2(d, e, g, h);

    return a*ca + b*cb + c*cc;
}

bool mnMatrix4Inverse(const float mat[16], float* detOut, float out[16]) {
    float a = +determinant3x3(mat[5], mat[9], mat[13], mat[6], mat[10], mat[14], mat[7], mat[11], mat[15]);
    float b = -determinant3x3(mat[1], mat[9], mat[13], mat[2], mat[10], mat[14], mat[3], mat[11], mat[15]);
    float c = +determinant3x3(mat[1], mat[5], mat[13], mat[2], mat[ 6], mat[14], mat[3], mat[ 7], mat[15]);
    float d = -determinant3x3(mat[1], mat[5], mat[ 9], mat[2], mat[ 6], mat[10], mat[3], mat[ 7], mat[11]);

    float det = a*mat[0] + b*mat[4] + c*mat[8] + d*mat[12];

    if(fabs(det) <= 0.001) {
        return false;
    }

    float e = -determinant3x3(mat[4], mat[8], mat[12], mat[6], mat[10], mat[14], mat[7], mat[11], mat[15]);
    float f = +determinant3x3(mat[0], mat[8], mat[12], mat[2], mat[10], mat[14], mat[3], mat[11], mat[15]);
    float g = -determinant3x3(mat[0], mat[4], mat[12], mat[2], mat[ 6], mat[14], mat[3], mat[ 7], mat[15]);
    float h = +determinant3x3(mat[0], mat[4], mat[ 8], mat[2], mat[ 6], mat[10], mat[3], mat[ 7], mat[11]);

    float i = +determinant3x3(mat[4], mat[8], mat[12], mat[5], mat[9], mat[13], mat[7], mat[11], mat[15]);
    float j = -determinant3x3(mat[0], mat[8], mat[12], mat[1], mat[9], mat[13], mat[3], mat[11], mat[15]);
    float k = +determinant3x3(mat[0], mat[4], mat[12], mat[1], mat[5], mat[13], mat[3], mat[ 7], mat[15]);
    float l = -determinant3x3(mat[0], mat[4], mat[ 8], mat[1], mat[5], mat[ 9], mat[3], mat[ 7], mat[11]);

    float m = -determinant3x3(mat[4], mat[8], mat[12], mat[5], mat[9], mat[13], mat[6], mat[10], mat[14]);
    float n = +determinant3x3(mat[0], mat[8], mat[12], mat[1], mat[9], mat[13], mat[2], mat[10], mat[14]);
    float o = -determinant3x3(mat[0], mat[4], mat[12], mat[1], mat[5], mat[13], mat[2], mat[ 6], mat[14]);
    float p = +determinant3x3(mat[0], mat[4], mat[ 8], mat[1], mat[5], mat[ 9], mat[2], mat[ 6], mat[10]);

    float invDet = 1.0f / det;

    out[0] = a*invDet;  out[4] = e*invDet;  out[ 8] = i*invDet;  out[12] = m*invDet;
    out[1] = b*invDet;  out[5] = f*invDet;  out[ 9] = j*invDet;  out[13] = n*invDet;
    out[2] = c*invDet;  out[6] = g*invDet;  out[10] = k*invDet;  out[14] = o*invDet;
    out[3] = d*invDet;  out[7] = h*invDet;  out[11] = l*invDet;  out[15] = p*invDet;

    if(detOut != nullptr)
        *detOut = det;

    return true;
}

//////////////////////////////////////////////////////////////////////////////

void mnMatrix4Transpose(const float mat[16], float out[16]) {
    out[0] = mat[0];
    out[5] = mat[5];
    out[10] = mat[10];
    out[15] = mat[15];

    {
        float a = mat[1];
        float b = mat[4];
        out[4] = a; out[1] = b;
    }

    {
        float a = mat[2];
        float b = mat[8];
        out[8] = a; out[2] = b;
    }

    {
        float a = mat[3];
        float b = mat[12];
        out[12] = a; out[3] = b;
    }

    {
        float a = mat[6];
        float b = mat[9];
        out[9] = a; out[6] = b;
    }

    {
        float a = mat[7];
        float b = mat[13];
        out[13] = a; out[7] = b;
    }

    {
        float a = mat[11];
        float b = mat[14];
        out[14] = a; out[11] = b;
    }
}

//////////////////////////////////////////////////////////////////////////////

union Matrix4 {
    float column[4][4];
    float values[16];
};

#endif //MATRIX_H
