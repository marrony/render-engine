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

//////////////////////////////////////////////////////////////////////////////

union Matrix4 {
    float column[4][4];
    float values[16];
};

#endif //MATRIX_H
