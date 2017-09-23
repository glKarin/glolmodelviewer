#ifndef _G_IMATH_EXT_H
#define _G_IMATH_EXT_H

#include <OpenEXR/ImathMatrix.h>
#include <OpenEXR/ImathQuat.h>

using namespace Imath;

template <class T>
Matrix44<T> g_transpose_row_matrix(const Matrix44<T> &mat)
{
	Matrix44<T> m;
	m.x[0][0] = mat.x[0][0];
	m.x[0][1] = mat.x[1][0];
	m.x[0][2] = mat.x[2][0];
	m.x[0][3] = mat.x[3][0];

	m.x[1][0] = mat.x[0][1];
	m.x[1][1] = mat.x[1][1];
	m.x[1][2] = mat.x[2][1];
	m.x[1][3] = mat.x[3][1];

	m.x[2][0] = mat.x[0][2];
	m.x[2][1] = mat.x[1][2];
	m.x[2][2] = mat.x[2][2];
	m.x[2][3] = mat.x[3][2];

	m.x[3][0] = mat.x[0][3];
	m.x[3][1] = mat.x[1][3];
	m.x[3][2] = mat.x[2][3];
	m.x[3][3] = mat.x[3][3];

	return m;
}

template <class T>
Matrix44<T> g_quat_to_matrix(const T r[], const T p[])
{
	Quat<T> q;
	q.v.x = r[0];
	q.v.y = r[1];
	q.v.z = r[2];
	q.r = r[3];
	Matrix44<float> m;
	m = q.toMatrix44();
	//m.setAxisAngle(q.axis(), q.angle());
	/*
	m[0][3] = p[0];
	m[1][3] = p[1];
	m[2][3] = p[2];
	*/
	m[3][0] = p[0];
	m[3][1] = p[1];
	m[3][2] = p[2];
	m[3][3] = (T)1;
	return g_transpose_row_matrix(m);
}

template <class T>
Matrix44<T> g_transpose_matrix(const Matrix44<T> &mat)
{
	Matrix44<T> m;
	m.x[0][0] = mat.x[0][0];
	m.x[1][0] = mat.x[0][1];
	m.x[2][0] = mat.x[0][2];
	m.x[3][0] = mat.x[0][3];

	m.x[0][1] = mat.x[1][0];
	m.x[1][1] = mat.x[1][1];
	m.x[2][1] = mat.x[1][2];
	m.x[3][1] = mat.x[1][3];

	m.x[0][2] = mat.x[2][0];
	m.x[1][2] = mat.x[2][1];
	m.x[2][2] = mat.x[2][2];
	m.x[3][2] = mat.x[2][3];

	m.x[0][3] = mat.x[3][0];
	m.x[1][3] = mat.x[3][1];
	m.x[2][3] = mat.x[3][2];
	m.x[3][3] = mat.x[3][3];

	return m;
}

template <class T>
Vec3<T> g_vector3_mult_matrix44 (const Vec3<T> &v, const Matrix44<T> &m)
{
    T x = T(v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + m[0][3]);
    T y = T(v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + m[1][3]);
    T z = T(v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + m[2][3]);
    T w = T(v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2] + m[3][3]);

    return Vec3<T> (x / w, y / w, z / w);
}

#endif
