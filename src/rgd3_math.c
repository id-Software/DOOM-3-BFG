/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_math.c - see rgd3_math.h. Algorithms follow
   neo/idlib/math/Math.h/.cpp (16-bit approximations, FloatToBits),
   Vector.cpp, Matrix.cpp, Quat.cpp, Angles.cpp, Rotation.cpp, Plane.cpp,
   Pluecker.cpp, Simd_Generic.cpp (MinMax and the joint loops),
   neo/idlib/bv/Bounds.cpp, Sphere.cpp, Box.cpp,
   neo/idlib/geometry/JointTransform.h/.cpp and DrawVert.h/.cpp.
   Where the original mixes float and double (Matrix.cpp inverses, Vector.cpp
   atan2/acos, Angles.cpp normalize, Plane/Sphere intersections) the same
   precision is used here. */

/* ===================================================================== */
/* scalar                                                                 */
/* ===================================================================== */

/* range reduction shared by Sin16/Cos16/SinCos16: a in [-PI/2, PI/2],
   *d = sign for the cosine */
static float rgd3_math_reduce_pi(float a, float* d)
{
	if ((a < 0.0f) || (a >= RGD3_MATH_TWO_PI))
		a -= floorf(a * RGD3_MATH_ONEOVER_TWOPI) * RGD3_MATH_TWO_PI;
	if (a < RGD3_MATH_PI)
	{
		if (a > RGD3_MATH_HALF_PI)
		{
			a = RGD3_MATH_PI - a;
			*d = -1.0f;
		}
		else
		{
			*d = 1.0f;
		}
	}
	else
	{
		if (a > RGD3_MATH_PI + RGD3_MATH_HALF_PI)
		{
			a = a - RGD3_MATH_TWO_PI;
			*d = 1.0f;
		}
		else
		{
			a = RGD3_MATH_PI - a;
			*d = -1.0f;
		}
	}
	return a;
}

static float rgd3_math_sin_poly(float a, float s)
{
	return a * (((((-2.39e-08f * s + 2.7526e-06f) * s - 1.98409e-04f) * s + 8.3333315e-03f) * s - 1.666666664e-01f) * s +
				1.0f);
}

static float rgd3_math_cos_poly(float s)
{
	return ((((-2.605e-07f * s + 2.47609e-05f) * s - 1.3888397e-03f) * s + 4.16666418e-02f) * s - 4.999999963e-01f) * s +
		   1.0f;
}

float rgd3_math_sin16(float a)
{
	float d;
	a = rgd3_math_reduce_pi(a, &d);
	return rgd3_math_sin_poly(a, a * a);
}

float rgd3_math_cos16(float a)
{
	float d;
	a = rgd3_math_reduce_pi(a, &d);
	return d * rgd3_math_cos_poly(a * a);
}

void rgd3_math_sincos16(float a, float* s, float* c)
{
	float d, t;
	a = rgd3_math_reduce_pi(a, &d);
	t = a * a;
	*s = rgd3_math_sin_poly(a, t);
	*c = d * rgd3_math_cos_poly(t);
}

float rgd3_math_tan16(float a)
{
	float s;
	int reciprocal;
	if ((a < 0.0f) || (a >= RGD3_MATH_PI))
		a -= floorf(a * RGD3_MATH_ONEOVER_PI) * RGD3_MATH_PI;
	if (a < RGD3_MATH_HALF_PI)
	{
		if (a > RGD3_MATH_ONEFOURTH_PI)
		{
			a = RGD3_MATH_HALF_PI - a;
			reciprocal = 1;
		}
		else
		{
			reciprocal = 0;
		}
	}
	else
	{
		if (a > RGD3_MATH_HALF_PI + RGD3_MATH_ONEFOURTH_PI)
		{
			a = a - RGD3_MATH_PI;
			reciprocal = 0;
		}
		else
		{
			a = RGD3_MATH_HALF_PI - a;
			reciprocal = 1;
		}
	}
	s = a * a;
	s = a * ((((((9.5168091e-03f * s + 2.900525e-03f) * s + 2.45650893e-02f) * s + 5.33740603e-02f) * s +
			   1.333923995e-01f) *
				  s +
			  3.333314036e-01f) *
				 s +
			 1.0f);
	if (reciprocal)
		return 1.0f / s;
	return s;
}

static float rgd3_math_asin_poly(float a)
{
	return (((-0.0187293f * a + 0.0742610f) * a - 0.2121144f) * a + 1.5707288f) * rgd3_math_sqrt(1.0f - a);
}

float rgd3_math_asin16(float a)
{
	if (a < 0.0f)
	{
		if (a <= -1.0f)
			return -RGD3_MATH_HALF_PI;
		a = fabsf(a);
		return rgd3_math_asin_poly(a) - RGD3_MATH_HALF_PI;
	}
	if (a >= 1.0f)
		return RGD3_MATH_HALF_PI;
	return RGD3_MATH_HALF_PI - rgd3_math_asin_poly(a);
}

float rgd3_math_acos16(float a)
{
	if (a < 0.0f)
	{
		if (a <= -1.0f)
			return RGD3_MATH_PI;
		a = fabsf(a);
		return RGD3_MATH_PI - rgd3_math_asin_poly(a);
	}
	if (a >= 1.0f)
		return 0.0f;
	return rgd3_math_asin_poly(a);
}

static float rgd3_math_atan_poly(float a, float s)
{
	return (((((((((0.0028662257f * s - 0.0161657367f) * s + 0.0429096138f) * s - 0.0752896400f) * s + 0.1065626393f) *
					s -
				0.1420889944f) *
				   s +
			   0.1999355085f) *
				  s -
			  0.3333314528f) *
			 s) +
			1.0f) *
		   a;
}

float rgd3_math_atan16(float a)
{
	float s;
	if (fabsf(a) > 1.0f)
	{
		a = 1.0f / a;
		s = a * a;
		s = -rgd3_math_atan_poly(a, s);
		if (a < 0.0f)
			return s - RGD3_MATH_HALF_PI;
		return s + RGD3_MATH_HALF_PI;
	}
	s = a * a;
	return rgd3_math_atan_poly(a, s);
}

float rgd3_math_atan2_16(float y, float x)
{
	float a, s;
	if (fabsf(y) > fabsf(x))
	{
		a = x / y;
		s = a * a;
		s = -rgd3_math_atan_poly(a, s);
		if (a < 0.0f)
			return s - RGD3_MATH_HALF_PI;
		return s + RGD3_MATH_HALF_PI;
	}
	a = y / x;
	s = a * a;
	return rgd3_math_atan_poly(a, s);
}

float rgd3_math_exp16(float f)
{
	float x = f * 1.44269504088896340f; /* 1 / log( 2 ) */
	u32 bits = rgd3_math_float_bits(x);
	int s = (bits & 0x80000000u) ? -1 : 0;
	int e = (int)((bits >> 23) & 0xff) - 127;
	int m = (int)(bits & 0x007fffffu) | (1 << 23);
	int i, p;
	float y, x2, q;
	/* integer part, x86 shift-count masking as the original relies on */
	if (e < 0)
		p = 0;
	else
		p = m >> ((23 - e) & 31);
	i = p ^ s;
	y = rgd3_math_bits_float(((u32)i + 127u) << 23);
	x -= (float)i;
	if (x >= 0.5f)
	{
		x -= 0.5f;
		y *= 1.4142135623730950488f; /* sqrt( 2 ) */
	}
	x2 = x * x;
	{
		float pp = x * (7.2152891511493f + x2 * 0.0576900723731f);
		q = 20.8189237930062f + x2;
		x = y * (q + pp) / (q - pp);
	}
	return x;
}

float rgd3_math_log16(float f)
{
	u32 i = rgd3_math_float_bits(f);
	int exponent = (int)((i >> 23) & 0xff) - 127;
	float y, y2;
	i -= (u32)(exponent + 1) << 23; /* value in [.5, 1> */
	y = rgd3_math_bits_float(i);
	y *= 1.4142135623730950488f; /* sqrt( 2 ) */
	y = (y - 1.0f) / (y + 1.0f);
	y2 = y * y;
	y = y * (2.000000000046727f +
			 y2 * (0.666666635059382f + y2 * (0.4000059794795f + y2 * (0.28525381498f + y2 * 0.2376245609f))));
	y += 0.693147180559945f * ((float)exponent + 0.5f);
	return y;
}

float rgd3_math_pow16(float x, float y)
{
	return rgd3_math_exp16(y * rgd3_math_log16(x));
}

int rgd3_math_float_to_bits(float f, int exponentBits, int mantissaBits)
{
	int sign, exponent, mantissa, maxBits, minBits;
	u32 i, value;
	float max, min;

	maxBits = (((1 << (exponentBits - 1)) - 1) << mantissaBits) | ((1 << mantissaBits) - 1);
	minBits = (((1 << exponentBits) - 2) << mantissaBits) | 1;
	max = rgd3_math_bits_to_float(maxBits, exponentBits, mantissaBits);
	min = rgd3_math_bits_to_float(minBits, exponentBits, mantissaBits);
	if (f >= 0.0f)
	{
		if (f >= max)
			return maxBits;
		else if (f <= min)
			return minBits;
	}
	else
	{
		if (f <= -max)
			return (maxBits | (1 << (exponentBits + mantissaBits)));
		else if (f >= -min)
			return (minBits | (1 << (exponentBits + mantissaBits)));
	}
	exponentBits--;
	i = rgd3_math_float_bits(f);
	sign = (int)((i >> 31) & 1);
	exponent = (int)((i >> 23) & 0xff) - 127;
	mantissa = (int)(i & 0x007fffffu);
	value = (u32)sign << (1 + exponentBits + mantissaBits);
	value |= ((((u32)exponent >> 31) << exponentBits) | ((u32)(exponent < 0 ? -exponent : exponent) &
														  ((1u << exponentBits) - 1)))
			 << mantissaBits;
	value |= (u32)mantissa >> (23 - mantissaBits);
	return (int)value;
}

float rgd3_math_bits_to_float(int i, int exponentBits, int mantissaBits)
{
	static const int exponentSign[2] = {1, -1};
	int sign, exponent, mantissa;
	u32 value;

	exponentBits--;
	sign = i >> (1 + exponentBits + mantissaBits);
	exponent = ((i >> mantissaBits) & ((1 << exponentBits) - 1)) * exponentSign[(i >> (exponentBits + mantissaBits)) & 1];
	mantissa = (i & ((1 << mantissaBits) - 1)) << (23 - mantissaBits);
	value = ((u32)sign << 31) | ((u32)(exponent + 127) << 23) | (u32)mantissa;
	return rgd3_math_bits_float(value);
}

/* ===================================================================== */
/* Vec3                                                                   */
/* ===================================================================== */

float rgd3_vec3_to_yaw(const Rgd3Vec3* v)
{
	float yaw;
	if ((v->y == 0.0f) && (v->x == 0.0f))
		return 0.0f;
	yaw = (float)(atan2((double)v->y, (double)v->x) * (double)RGD3_MATH_M_RAD2DEG);
	if (yaw < 0.0f)
		yaw += 360.0f;
	return yaw;
}

float rgd3_vec3_to_pitch(const Rgd3Vec3* v)
{
	float forward, pitch;
	if ((v->x == 0.0f) && (v->y == 0.0f))
		return (v->z > 0.0f) ? 90.0f : 270.0f;
	forward = rgd3_math_sqrt(v->x * v->x + v->y * v->y);
	pitch = (float)(atan2((double)v->z, (double)forward) * (double)RGD3_MATH_M_RAD2DEG);
	if (pitch < 0.0f)
		pitch += 360.0f;
	return pitch;
}

Rgd3Angles rgd3_vec3_to_angles(const Rgd3Vec3* v)
{
	float forward, yaw, pitch;
	if ((v->x == 0.0f) && (v->y == 0.0f))
	{
		yaw = 0.0f;
		pitch = (v->z > 0.0f) ? 90.0f : 270.0f;
	}
	else
	{
		yaw = (float)(atan2((double)v->y, (double)v->x) * (double)RGD3_MATH_M_RAD2DEG);
		if (yaw < 0.0f)
			yaw += 360.0f;
		forward = rgd3_math_sqrt(v->x * v->x + v->y * v->y);
		pitch = (float)(atan2((double)v->z, (double)forward) * (double)RGD3_MATH_M_RAD2DEG);
		if (pitch < 0.0f)
			pitch += 360.0f;
	}
	return rgd3_angles(-pitch, yaw, 0.0f);
}

void rgd3_vec3_to_mat3(const Rgd3Vec3* v, Rgd3Mat3* out)
{
	Rgd3Vec3 fwd = *v;
	Rgd3Vec3 left;
	float d = fwd.x * fwd.x + fwd.y * fwd.y;
	if (!d)
	{
		left = rgd3_vec3(1.0f, 0.0f, 0.0f);
	}
	else
	{
		d = rgd3_math_inv_sqrt(d);
		left = rgd3_vec3(-fwd.y * d, fwd.x * d, 0.0f);
	}
	out->mat[0] = fwd;
	out->mat[1] = left;
	out->mat[2] = rgd3_vec3_cross(fwd, left);
}

int rgd3_vec3_project_along_plane(Rgd3Vec3* v, const Rgd3Vec3* normal, float epsilon, float overBounce)
{
	Rgd3Vec3 cross = rgd3_vec3_cross(rgd3_vec3_cross(*v, *normal), *v);
	float len;
	rgd3_vec3_normalize(&cross);
	len = rgd3_vec3_dot(*normal, cross);
	if (fabsf(len) < epsilon)
		return 0;
	cross = rgd3_vec3_scale(cross, overBounce * rgd3_vec3_dot(*normal, *v) / len);
	*v = rgd3_vec3_sub(*v, cross);
	return 1;
}

void rgd3_vec3_project_self_onto_sphere(Rgd3Vec3* v, float radius)
{
	float rsqr = radius * radius;
	float len = rgd3_vec3_length(*v);
	if (len < rsqr * 0.5f)
		v->z = (float)sqrt((double)(rsqr - len));
	else
		v->z = (float)((double)rsqr / (2.0f * sqrt((double)len)));
}

Rgd3Vec3 rgd3_vec3_slerp(const Rgd3Vec3* v1, const Rgd3Vec3* v2, float t)
{
	float omega, cosom, sinom, scale0, scale1;
	if (t <= 0.0f)
		return *v1;
	if (t >= 1.0f)
		return *v2;
	cosom = rgd3_vec3_dot(*v1, *v2);
	if ((1.0f - cosom) > 1e-6)
	{
		omega = (float)acos((double)cosom);
		sinom = (float)sin((double)omega);
		scale0 = (float)(sin((double)((1.0f - t) * omega)) / sinom);
		scale1 = (float)(sin((double)(t * omega)) / sinom);
	}
	else
	{
		scale0 = 1.0f - t;
		scale1 = t;
	}
	return rgd3_vec3_add(rgd3_vec3_scale(*v1, scale0), rgd3_vec3_scale(*v2, scale1));
}

/* ===================================================================== */
/* Mat3                                                                   */
/* ===================================================================== */

int rgd3_mat3_is_symmetric(const Rgd3Mat3* m, float epsilon)
{
	if (fabsf(m->mat[0].y - m->mat[1].x) > epsilon)
		return 0;
	if (fabsf(m->mat[0].z - m->mat[2].x) > epsilon)
		return 0;
	if (fabsf(m->mat[1].z - m->mat[2].y) > epsilon)
		return 0;
	return 1;
}

int rgd3_mat3_is_diagonal(const Rgd3Mat3* m, float epsilon)
{
	if (fabsf(m->mat[0].y) > epsilon || fabsf(m->mat[0].z) > epsilon || fabsf(m->mat[1].x) > epsilon ||
		fabsf(m->mat[1].z) > epsilon || fabsf(m->mat[2].x) > epsilon || fabsf(m->mat[2].y) > epsilon)
		return 0;
	return 1;
}

int rgd3_mat3_inverse_self(Rgd3Mat3* m)
{
	float a[3][3], inv[3][3];
	double det, invDet;
	int i, j;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			a[i][j] = m->mat[i].v[j];
	}
	inv[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];
	inv[1][0] = a[1][2] * a[2][0] - a[1][0] * a[2][2];
	inv[2][0] = a[1][0] * a[2][1] - a[1][1] * a[2][0];
	det = a[0][0] * inv[0][0] + a[0][1] * inv[1][0] + a[0][2] * inv[2][0];
	if (fabsf((float)det) < RGD3_MATRIX_INVERSE_EPSILON)
		return 0;
	invDet = 1.0f / det;
	inv[0][1] = a[0][2] * a[2][1] - a[0][1] * a[2][2];
	inv[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];
	inv[1][1] = a[0][0] * a[2][2] - a[0][2] * a[2][0];
	inv[1][2] = a[0][2] * a[1][0] - a[0][0] * a[1][2];
	inv[2][1] = a[0][1] * a[2][0] - a[0][0] * a[2][1];
	inv[2][2] = a[0][0] * a[1][1] - a[0][1] * a[1][0];
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			m->mat[i].v[j] = (float)(inv[i][j] * invDet);
	}
	return 1;
}

/* BFG's InverseFastSelf takes the same cofactor path (#if 1) */
int rgd3_mat3_inverse_fast_self(Rgd3Mat3* m)
{
	return rgd3_mat3_inverse_self(m);
}

int rgd3_mat3_inverse(const Rgd3Mat3* m, Rgd3Mat3* out)
{
	*out = *m;
	return rgd3_mat3_inverse_self(out);
}

int rgd3_mat3_inverse_fast(const Rgd3Mat3* m, Rgd3Mat3* out)
{
	*out = *m;
	return rgd3_mat3_inverse_fast_self(out);
}

void rgd3_mat3_ortho_normalize(const Rgd3Mat3* m, Rgd3Mat3* out)
{
	Rgd3Mat3 ortho = *m;
	rgd3_vec3_normalize(&ortho.mat[0]);
	ortho.mat[2] = rgd3_vec3_cross(m->mat[0], m->mat[1]);
	rgd3_vec3_normalize(&ortho.mat[2]);
	ortho.mat[1] = rgd3_vec3_cross(m->mat[2], m->mat[0]);
	rgd3_vec3_normalize(&ortho.mat[1]);
	*out = ortho;
}

void rgd3_mat3_ortho_normalize_self(Rgd3Mat3* m)
{
	rgd3_vec3_normalize(&m->mat[0]);
	m->mat[2] = rgd3_vec3_cross(m->mat[0], m->mat[1]);
	rgd3_vec3_normalize(&m->mat[2]);
	m->mat[1] = rgd3_vec3_cross(m->mat[2], m->mat[0]);
	rgd3_vec3_normalize(&m->mat[1]);
}

Rgd3Angles rgd3_mat3_to_angles(const Rgd3Mat3* m)
{
	Rgd3Angles angles;
	float s = rgd3_math_sqrt(m->mat[0].x * m->mat[0].x + m->mat[0].y * m->mat[0].y);
	if (s > RGD3_MATH_FLT_EPSILON)
	{
		angles.pitch = RGD3_RAD2DEG(-atan2f(m->mat[0].z, s));
		angles.yaw = RGD3_RAD2DEG(atan2f(m->mat[0].y, m->mat[0].x));
		angles.roll = RGD3_RAD2DEG(atan2f(m->mat[1].z, m->mat[2].z));
	}
	else
	{
		angles.pitch = m->mat[0].z < 0.0f ? 90.0f : -90.0f;
		angles.yaw = RGD3_RAD2DEG(-atan2f(m->mat[1].x, m->mat[1].y));
		angles.roll = 0.0f;
	}
	return angles;
}

Rgd3Quat rgd3_mat3_to_quat(const Rgd3Mat3* m)
{
	static const int next[3] = {1, 2, 0};
	Rgd3Quat q;
	float trace, s, t;
	int i, j, k;

	trace = m->mat[0].x + m->mat[1].y + m->mat[2].z;
	if (trace > 0.0f)
	{
		t = trace + 1.0f;
		s = rgd3_math_inv_sqrt(t) * 0.5f;
		q.v[3] = s * t;
		q.v[0] = (m->mat[2].y - m->mat[1].z) * s;
		q.v[1] = (m->mat[0].z - m->mat[2].x) * s;
		q.v[2] = (m->mat[1].x - m->mat[0].y) * s;
	}
	else
	{
		i = 0;
		if (m->mat[1].y > m->mat[0].x)
			i = 1;
		if (m->mat[2].z > m->mat[i].v[i])
			i = 2;
		j = next[i];
		k = next[j];
		t = (m->mat[i].v[i] - (m->mat[j].v[j] + m->mat[k].v[k])) + 1.0f;
		s = rgd3_math_inv_sqrt(t) * 0.5f;
		q.v[i] = s * t;
		q.v[3] = (m->mat[k].v[j] - m->mat[j].v[k]) * s;
		q.v[j] = (m->mat[j].v[i] + m->mat[i].v[j]) * s;
		q.v[k] = (m->mat[k].v[i] + m->mat[i].v[k]) * s;
	}
	return q;
}

Rgd3CQuat rgd3_mat3_to_cquat(const Rgd3Mat3* m)
{
	return rgd3_quat_to_cquat(rgd3_mat3_to_quat(m));
}

void rgd3_mat3_to_rotation(const Rgd3Mat3* m, Rgd3Rotation* out)
{
	static const int next[3] = {1, 2, 0};
	Rgd3Rotation r;
	float trace, s, t, lengthSqr;
	int i, j, k;

	trace = m->mat[0].x + m->mat[1].y + m->mat[2].z;
	if (trace > 0.0f)
	{
		t = trace + 1.0f;
		s = rgd3_math_inv_sqrt(t) * 0.5f;
		r.angle = s * t;
		r.vec.x = (m->mat[2].y - m->mat[1].z) * s;
		r.vec.y = (m->mat[0].z - m->mat[2].x) * s;
		r.vec.z = (m->mat[1].x - m->mat[0].y) * s;
	}
	else
	{
		i = 0;
		if (m->mat[1].y > m->mat[0].x)
			i = 1;
		if (m->mat[2].z > m->mat[i].v[i])
			i = 2;
		j = next[i];
		k = next[j];
		t = (m->mat[i].v[i] - (m->mat[j].v[j] + m->mat[k].v[k])) + 1.0f;
		s = rgd3_math_inv_sqrt(t) * 0.5f;
		r.vec.v[i] = s * t;
		r.angle = (m->mat[k].v[j] - m->mat[j].v[k]) * s;
		r.vec.v[j] = (m->mat[j].v[i] + m->mat[i].v[j]) * s;
		r.vec.v[k] = (m->mat[k].v[i] + m->mat[i].v[k]) * s;
	}
	r.angle = rgd3_math_acos(r.angle);
	lengthSqr = rgd3_vec3_length_sqr(r.vec);
	if ((fabsf(r.angle) < 1e-10f) || (lengthSqr < 1e-10f))
	{
		r.vec = rgd3_vec3(0.0f, 0.0f, 1.0f);
		r.angle = 0.0f;
	}
	else
	{
		r.vec = rgd3_vec3_scale(r.vec, rgd3_math_inv_sqrt(lengthSqr));
		r.angle *= 2.0f * RGD3_MATH_M_RAD2DEG;
	}
	r.origin = rgd3_vec3_zero();
	r.axis = *m;
	r.axisValid = 1;
	*out = r;
}

void rgd3_mat3_to_mat4(const Rgd3Mat3* m, Rgd3Mat4* out)
{
	Rgd3Mat4 r;
	r.mat[0] = rgd3_vec4(m->mat[0].x, m->mat[1].x, m->mat[2].x, 0.0f);
	r.mat[1] = rgd3_vec4(m->mat[0].y, m->mat[1].y, m->mat[2].y, 0.0f);
	r.mat[2] = rgd3_vec4(m->mat[0].z, m->mat[1].z, m->mat[2].z, 0.0f);
	r.mat[3] = rgd3_vec4(0.0f, 0.0f, 0.0f, 1.0f);
	*out = r;
}

Rgd3Vec3 rgd3_mat3_to_angular_velocity(const Rgd3Mat3* m)
{
	Rgd3Rotation r;
	rgd3_mat3_to_rotation(m, &r);
	return rgd3_vec3_scale(r.vec, RGD3_DEG2RAD(r.angle));
}

/* ===================================================================== */
/* Mat4                                                                   */
/* ===================================================================== */

#define RGD3_M4(r, c) (m->mat[r].v[c])

float rgd3_mat4_determinant(const Rgd3Mat4* m)
{
	float det2_01_01 = RGD3_M4(0, 0) * RGD3_M4(1, 1) - RGD3_M4(0, 1) * RGD3_M4(1, 0);
	float det2_01_02 = RGD3_M4(0, 0) * RGD3_M4(1, 2) - RGD3_M4(0, 2) * RGD3_M4(1, 0);
	float det2_01_03 = RGD3_M4(0, 0) * RGD3_M4(1, 3) - RGD3_M4(0, 3) * RGD3_M4(1, 0);
	float det2_01_12 = RGD3_M4(0, 1) * RGD3_M4(1, 2) - RGD3_M4(0, 2) * RGD3_M4(1, 1);
	float det2_01_13 = RGD3_M4(0, 1) * RGD3_M4(1, 3) - RGD3_M4(0, 3) * RGD3_M4(1, 1);
	float det2_01_23 = RGD3_M4(0, 2) * RGD3_M4(1, 3) - RGD3_M4(0, 3) * RGD3_M4(1, 2);
	float det3_201_012 = RGD3_M4(2, 0) * det2_01_12 - RGD3_M4(2, 1) * det2_01_02 + RGD3_M4(2, 2) * det2_01_01;
	float det3_201_013 = RGD3_M4(2, 0) * det2_01_13 - RGD3_M4(2, 1) * det2_01_03 + RGD3_M4(2, 3) * det2_01_01;
	float det3_201_023 = RGD3_M4(2, 0) * det2_01_23 - RGD3_M4(2, 2) * det2_01_03 + RGD3_M4(2, 3) * det2_01_02;
	float det3_201_123 = RGD3_M4(2, 1) * det2_01_23 - RGD3_M4(2, 2) * det2_01_13 + RGD3_M4(2, 3) * det2_01_12;
	return (-det3_201_123 * RGD3_M4(3, 0) + det3_201_023 * RGD3_M4(3, 1) - det3_201_013 * RGD3_M4(3, 2) +
			det3_201_012 * RGD3_M4(3, 3));
}

int rgd3_mat4_inverse_self(Rgd3Mat4* m)
{
	double det, invDet;
	float det2_01_01 = RGD3_M4(0, 0) * RGD3_M4(1, 1) - RGD3_M4(0, 1) * RGD3_M4(1, 0);
	float det2_01_02 = RGD3_M4(0, 0) * RGD3_M4(1, 2) - RGD3_M4(0, 2) * RGD3_M4(1, 0);
	float det2_01_03 = RGD3_M4(0, 0) * RGD3_M4(1, 3) - RGD3_M4(0, 3) * RGD3_M4(1, 0);
	float det2_01_12 = RGD3_M4(0, 1) * RGD3_M4(1, 2) - RGD3_M4(0, 2) * RGD3_M4(1, 1);
	float det2_01_13 = RGD3_M4(0, 1) * RGD3_M4(1, 3) - RGD3_M4(0, 3) * RGD3_M4(1, 1);
	float det2_01_23 = RGD3_M4(0, 2) * RGD3_M4(1, 3) - RGD3_M4(0, 3) * RGD3_M4(1, 2);
	float det3_201_012 = RGD3_M4(2, 0) * det2_01_12 - RGD3_M4(2, 1) * det2_01_02 + RGD3_M4(2, 2) * det2_01_01;
	float det3_201_013 = RGD3_M4(2, 0) * det2_01_13 - RGD3_M4(2, 1) * det2_01_03 + RGD3_M4(2, 3) * det2_01_01;
	float det3_201_023 = RGD3_M4(2, 0) * det2_01_23 - RGD3_M4(2, 2) * det2_01_03 + RGD3_M4(2, 3) * det2_01_02;
	float det3_201_123 = RGD3_M4(2, 1) * det2_01_23 - RGD3_M4(2, 2) * det2_01_13 + RGD3_M4(2, 3) * det2_01_12;
	float det2_03_01, det2_03_02, det2_03_03, det2_03_12, det2_03_13, det2_03_23;
	float det2_13_01, det2_13_02, det2_13_03, det2_13_12, det2_13_13, det2_13_23;
	float det3_203_012, det3_203_013, det3_203_023, det3_203_123;
	float det3_213_012, det3_213_013, det3_213_023, det3_213_123;
	float det3_301_012, det3_301_013, det3_301_023, det3_301_123;

	det = (-det3_201_123 * RGD3_M4(3, 0) + det3_201_023 * RGD3_M4(3, 1) - det3_201_013 * RGD3_M4(3, 2) +
		   det3_201_012 * RGD3_M4(3, 3));
	if (fabsf((float)det) < RGD3_MATRIX_INVERSE_EPSILON)
		return 0;
	invDet = 1.0f / det;

	det2_03_01 = RGD3_M4(0, 0) * RGD3_M4(3, 1) - RGD3_M4(0, 1) * RGD3_M4(3, 0);
	det2_03_02 = RGD3_M4(0, 0) * RGD3_M4(3, 2) - RGD3_M4(0, 2) * RGD3_M4(3, 0);
	det2_03_03 = RGD3_M4(0, 0) * RGD3_M4(3, 3) - RGD3_M4(0, 3) * RGD3_M4(3, 0);
	det2_03_12 = RGD3_M4(0, 1) * RGD3_M4(3, 2) - RGD3_M4(0, 2) * RGD3_M4(3, 1);
	det2_03_13 = RGD3_M4(0, 1) * RGD3_M4(3, 3) - RGD3_M4(0, 3) * RGD3_M4(3, 1);
	det2_03_23 = RGD3_M4(0, 2) * RGD3_M4(3, 3) - RGD3_M4(0, 3) * RGD3_M4(3, 2);
	det2_13_01 = RGD3_M4(1, 0) * RGD3_M4(3, 1) - RGD3_M4(1, 1) * RGD3_M4(3, 0);
	det2_13_02 = RGD3_M4(1, 0) * RGD3_M4(3, 2) - RGD3_M4(1, 2) * RGD3_M4(3, 0);
	det2_13_03 = RGD3_M4(1, 0) * RGD3_M4(3, 3) - RGD3_M4(1, 3) * RGD3_M4(3, 0);
	det2_13_12 = RGD3_M4(1, 1) * RGD3_M4(3, 2) - RGD3_M4(1, 2) * RGD3_M4(3, 1);
	det2_13_13 = RGD3_M4(1, 1) * RGD3_M4(3, 3) - RGD3_M4(1, 3) * RGD3_M4(3, 1);
	det2_13_23 = RGD3_M4(1, 2) * RGD3_M4(3, 3) - RGD3_M4(1, 3) * RGD3_M4(3, 2);

	det3_203_012 = RGD3_M4(2, 0) * det2_03_12 - RGD3_M4(2, 1) * det2_03_02 + RGD3_M4(2, 2) * det2_03_01;
	det3_203_013 = RGD3_M4(2, 0) * det2_03_13 - RGD3_M4(2, 1) * det2_03_03 + RGD3_M4(2, 3) * det2_03_01;
	det3_203_023 = RGD3_M4(2, 0) * det2_03_23 - RGD3_M4(2, 2) * det2_03_03 + RGD3_M4(2, 3) * det2_03_02;
	det3_203_123 = RGD3_M4(2, 1) * det2_03_23 - RGD3_M4(2, 2) * det2_03_13 + RGD3_M4(2, 3) * det2_03_12;

	det3_213_012 = RGD3_M4(2, 0) * det2_13_12 - RGD3_M4(2, 1) * det2_13_02 + RGD3_M4(2, 2) * det2_13_01;
	det3_213_013 = RGD3_M4(2, 0) * det2_13_13 - RGD3_M4(2, 1) * det2_13_03 + RGD3_M4(2, 3) * det2_13_01;
	det3_213_023 = RGD3_M4(2, 0) * det2_13_23 - RGD3_M4(2, 2) * det2_13_03 + RGD3_M4(2, 3) * det2_13_02;
	det3_213_123 = RGD3_M4(2, 1) * det2_13_23 - RGD3_M4(2, 2) * det2_13_13 + RGD3_M4(2, 3) * det2_13_12;

	det3_301_012 = RGD3_M4(3, 0) * det2_01_12 - RGD3_M4(3, 1) * det2_01_02 + RGD3_M4(3, 2) * det2_01_01;
	det3_301_013 = RGD3_M4(3, 0) * det2_01_13 - RGD3_M4(3, 1) * det2_01_03 + RGD3_M4(3, 3) * det2_01_01;
	det3_301_023 = RGD3_M4(3, 0) * det2_01_23 - RGD3_M4(3, 2) * det2_01_03 + RGD3_M4(3, 3) * det2_01_02;
	det3_301_123 = RGD3_M4(3, 1) * det2_01_23 - RGD3_M4(3, 2) * det2_01_13 + RGD3_M4(3, 3) * det2_01_12;

	RGD3_M4(0, 0) = (float)(-det3_213_123 * invDet);
	RGD3_M4(1, 0) = (float)(+det3_213_023 * invDet);
	RGD3_M4(2, 0) = (float)(-det3_213_013 * invDet);
	RGD3_M4(3, 0) = (float)(+det3_213_012 * invDet);

	RGD3_M4(0, 1) = (float)(+det3_203_123 * invDet);
	RGD3_M4(1, 1) = (float)(-det3_203_023 * invDet);
	RGD3_M4(2, 1) = (float)(+det3_203_013 * invDet);
	RGD3_M4(3, 1) = (float)(-det3_203_012 * invDet);

	RGD3_M4(0, 2) = (float)(+det3_301_123 * invDet);
	RGD3_M4(1, 2) = (float)(-det3_301_023 * invDet);
	RGD3_M4(2, 2) = (float)(+det3_301_013 * invDet);
	RGD3_M4(3, 2) = (float)(-det3_301_012 * invDet);

	RGD3_M4(0, 3) = (float)(-det3_201_123 * invDet);
	RGD3_M4(1, 3) = (float)(+det3_201_023 * invDet);
	RGD3_M4(2, 3) = (float)(-det3_201_013 * invDet);
	RGD3_M4(3, 3) = (float)(+det3_201_012 * invDet);
	return 1;
}

#undef RGD3_M4

int rgd3_mat4_inverse_fast_self(Rgd3Mat4* m4)
{
	float r0[2][2], r1[2][2], r2[2][2], r3[2][2];
	float a, det, invDet;
	float m[16];
	int i;

	for (i = 0; i < 4; i++)
	{
		m[i * 4 + 0] = m4->mat[i].x;
		m[i * 4 + 1] = m4->mat[i].y;
		m[i * 4 + 2] = m4->mat[i].z;
		m[i * 4 + 3] = m4->mat[i].w;
	}

	/* r0 = m0.Inverse() */
	det = m[0 * 4 + 0] * m[1 * 4 + 1] - m[0 * 4 + 1] * m[1 * 4 + 0];
	if (fabsf(det) < RGD3_MATRIX_INVERSE_EPSILON)
		return 0;
	invDet = 1.0f / det;
	r0[0][0] = m[1 * 4 + 1] * invDet;
	r0[0][1] = -m[0 * 4 + 1] * invDet;
	r0[1][0] = -m[1 * 4 + 0] * invDet;
	r0[1][1] = m[0 * 4 + 0] * invDet;

	/* r1 = r0 * m1 */
	r1[0][0] = r0[0][0] * m[0 * 4 + 2] + r0[0][1] * m[1 * 4 + 2];
	r1[0][1] = r0[0][0] * m[0 * 4 + 3] + r0[0][1] * m[1 * 4 + 3];
	r1[1][0] = r0[1][0] * m[0 * 4 + 2] + r0[1][1] * m[1 * 4 + 2];
	r1[1][1] = r0[1][0] * m[0 * 4 + 3] + r0[1][1] * m[1 * 4 + 3];

	/* r2 = m2 * r1 */
	r2[0][0] = m[2 * 4 + 0] * r1[0][0] + m[2 * 4 + 1] * r1[1][0];
	r2[0][1] = m[2 * 4 + 0] * r1[0][1] + m[2 * 4 + 1] * r1[1][1];
	r2[1][0] = m[3 * 4 + 0] * r1[0][0] + m[3 * 4 + 1] * r1[1][0];
	r2[1][1] = m[3 * 4 + 0] * r1[0][1] + m[3 * 4 + 1] * r1[1][1];

	/* r3 = r2 - m3 */
	r3[0][0] = r2[0][0] - m[2 * 4 + 2];
	r3[0][1] = r2[0][1] - m[2 * 4 + 3];
	r3[1][0] = r2[1][0] - m[3 * 4 + 2];
	r3[1][1] = r2[1][1] - m[3 * 4 + 3];

	/* r3.InverseSelf() */
	det = r3[0][0] * r3[1][1] - r3[0][1] * r3[1][0];
	if (fabsf(det) < RGD3_MATRIX_INVERSE_EPSILON)
		return 0;
	invDet = 1.0f / det;
	a = r3[0][0];
	r3[0][0] = r3[1][1] * invDet;
	r3[0][1] = -r3[0][1] * invDet;
	r3[1][0] = -r3[1][0] * invDet;
	r3[1][1] = a * invDet;

	/* r2 = m2 * r0 */
	r2[0][0] = m[2 * 4 + 0] * r0[0][0] + m[2 * 4 + 1] * r0[1][0];
	r2[0][1] = m[2 * 4 + 0] * r0[0][1] + m[2 * 4 + 1] * r0[1][1];
	r2[1][0] = m[3 * 4 + 0] * r0[0][0] + m[3 * 4 + 1] * r0[1][0];
	r2[1][1] = m[3 * 4 + 0] * r0[0][1] + m[3 * 4 + 1] * r0[1][1];

	/* m2 = r3 * r2 */
	m[2 * 4 + 0] = r3[0][0] * r2[0][0] + r3[0][1] * r2[1][0];
	m[2 * 4 + 1] = r3[0][0] * r2[0][1] + r3[0][1] * r2[1][1];
	m[3 * 4 + 0] = r3[1][0] * r2[0][0] + r3[1][1] * r2[1][0];
	m[3 * 4 + 1] = r3[1][0] * r2[0][1] + r3[1][1] * r2[1][1];

	/* m0 = r0 - r1 * m2 */
	m[0 * 4 + 0] = r0[0][0] - r1[0][0] * m[2 * 4 + 0] - r1[0][1] * m[3 * 4 + 0];
	m[0 * 4 + 1] = r0[0][1] - r1[0][0] * m[2 * 4 + 1] - r1[0][1] * m[3 * 4 + 1];
	m[1 * 4 + 0] = r0[1][0] - r1[1][0] * m[2 * 4 + 0] - r1[1][1] * m[3 * 4 + 0];
	m[1 * 4 + 1] = r0[1][1] - r1[1][0] * m[2 * 4 + 1] - r1[1][1] * m[3 * 4 + 1];

	/* m1 = r1 * r3 */
	m[0 * 4 + 2] = r1[0][0] * r3[0][0] + r1[0][1] * r3[1][0];
	m[0 * 4 + 3] = r1[0][0] * r3[0][1] + r1[0][1] * r3[1][1];
	m[1 * 4 + 2] = r1[1][0] * r3[0][0] + r1[1][1] * r3[1][0];
	m[1 * 4 + 3] = r1[1][0] * r3[0][1] + r1[1][1] * r3[1][1];

	/* m3 = -r3 */
	m[2 * 4 + 2] = -r3[0][0];
	m[2 * 4 + 3] = -r3[0][1];
	m[3 * 4 + 2] = -r3[1][0];
	m[3 * 4 + 3] = -r3[1][1];

	for (i = 0; i < 4; i++)
		m4->mat[i] = rgd3_vec4(m[i * 4 + 0], m[i * 4 + 1], m[i * 4 + 2], m[i * 4 + 3]);
	return 1;
}

int rgd3_mat4_inverse(const Rgd3Mat4* m, Rgd3Mat4* out)
{
	*out = *m;
	return rgd3_mat4_inverse_self(out);
}

int rgd3_mat4_inverse_fast(const Rgd3Mat4* m, Rgd3Mat4* out)
{
	*out = *m;
	return rgd3_mat4_inverse_fast_self(out);
}

/* ===================================================================== */
/* Quat / CQuat                                                           */
/* ===================================================================== */

void rgd3_quat_to_mat3(const Rgd3Quat* q, Rgd3Mat3* out)
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q->x + q->x;
	y2 = q->y + q->y;
	z2 = q->z + q->z;

	xx = q->x * x2;
	xy = q->x * y2;
	xz = q->x * z2;

	yy = q->y * y2;
	yz = q->y * z2;
	zz = q->z * z2;

	wx = q->w * x2;
	wy = q->w * y2;
	wz = q->w * z2;

	out->mat[0].x = 1.0f - (yy + zz);
	out->mat[0].y = xy - wz;
	out->mat[0].z = xz + wy;

	out->mat[1].x = xy + wz;
	out->mat[1].y = 1.0f - (xx + zz);
	out->mat[1].z = yz - wx;

	out->mat[2].x = xz - wy;
	out->mat[2].y = yz + wx;
	out->mat[2].z = 1.0f - (xx + yy);
}

void rgd3_quat_to_mat4(const Rgd3Quat* q, Rgd3Mat4* out)
{
	Rgd3Mat3 m;
	rgd3_quat_to_mat3(q, &m);
	rgd3_mat3_to_mat4(&m, out);
}

Rgd3Angles rgd3_quat_to_angles(const Rgd3Quat* q)
{
	Rgd3Mat3 m;
	rgd3_quat_to_mat3(q, &m);
	return rgd3_mat3_to_angles(&m);
}

/* shared by quat/angles ToRotation: vec = xyz, angle from w */
static void rgd3_rotation_from_quat_parts(Rgd3Vec3 vec, float w, Rgd3Rotation* out)
{
	float angle = rgd3_math_acos(w);
	if (angle == 0.0f)
	{
		vec = rgd3_vec3(0.0f, 0.0f, 1.0f);
	}
	else
	{
		rgd3_vec3_normalize(&vec);
		rgd3_vec3_fix_degenerate_normal(&vec);
		angle *= 2.0f * RGD3_MATH_M_RAD2DEG;
	}
	rgd3_rotation_set(out, rgd3_vec3_zero(), vec, angle);
}

void rgd3_quat_to_rotation(const Rgd3Quat* q, Rgd3Rotation* out)
{
	rgd3_rotation_from_quat_parts(rgd3_vec3(q->x, q->y, q->z), q->w, out);
}

Rgd3Vec3 rgd3_quat_to_angular_velocity(const Rgd3Quat* q)
{
	Rgd3Vec3 vec = rgd3_vec3(q->x, q->y, q->z);
	rgd3_vec3_normalize(&vec);
	return rgd3_vec3_scale(vec, rgd3_math_acos(q->w));
}

Rgd3Quat rgd3_quat_slerp(const Rgd3Quat* from, const Rgd3Quat* to, float t)
{
	Rgd3Quat temp;
	float omega, cosom, sinom, scale0, scale1;

	if (t <= 0.0f)
		return *from;
	if (t >= 1.0f)
		return *to;
	if (rgd3_quat_compare(*from, *to))
		return *to;

	cosom = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;
	if (cosom < 0.0f)
	{
		temp = rgd3_quat_neg(*to);
		cosom = -cosom;
	}
	else
	{
		temp = *to;
	}

	if ((1.0f - cosom) > 1e-6f)
	{
		scale0 = 1.0f - cosom * cosom;
		sinom = rgd3_math_inv_sqrt(scale0);
		omega = rgd3_math_atan2_16(scale0 * sinom, cosom);
		scale0 = rgd3_math_sin16((1.0f - t) * omega) * sinom;
		scale1 = rgd3_math_sin16(t * omega) * sinom;
	}
	else
	{
		scale0 = 1.0f - t;
		scale1 = t;
	}
	return rgd3_quat_add(rgd3_quat_scale(*from, scale0), rgd3_quat_scale(temp, scale1));
}

Rgd3Quat rgd3_quat_lerp(const Rgd3Quat* from, const Rgd3Quat* to, float t)
{
	Rgd3Quat r;
	float cosom, scale0, scale1, s;

	if (t <= 0.0f)
		return *from;
	if (t >= 1.0f)
		return *to;
	if (rgd3_quat_compare(*from, *to))
		return *to;

	cosom = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;
	scale0 = 1.0f - t;
	scale1 = (cosom >= 0.0f) ? t : -t;

	r.x = scale0 * from->x + scale1 * to->x;
	r.y = scale0 * from->y + scale1 * to->y;
	r.z = scale0 * from->z + scale1 * to->z;
	r.w = scale0 * from->w + scale1 * to->w;

	s = rgd3_math_inv_sqrt(r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w);
	r.x *= s;
	r.y *= s;
	r.z *= s;
	r.w *= s;
	return r;
}

void rgd3_cquat_to_mat3(const Rgd3CQuat* q, Rgd3Mat3* out)
{
	Rgd3Quat full = rgd3_cquat_to_quat(*q);
	rgd3_quat_to_mat3(&full, out);
}

Rgd3Angles rgd3_cquat_to_angles(const Rgd3CQuat* q)
{
	Rgd3Quat full = rgd3_cquat_to_quat(*q);
	return rgd3_quat_to_angles(&full);
}

void rgd3_cquat_to_rotation(const Rgd3CQuat* q, Rgd3Rotation* out)
{
	Rgd3Quat full = rgd3_cquat_to_quat(*q);
	rgd3_quat_to_rotation(&full, out);
}

/* ===================================================================== */
/* Angles                                                                 */
/* ===================================================================== */

void rgd3_angles_normalize360(Rgd3Angles* a)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		float v = a->v[i];
		if ((v >= 360.0f) || (v < 0.0f))
		{
			v = (float)((double)v - floor((double)(v / 360.0f)) * 360.0);
			if (v >= 360.0f)
				v -= 360.0f;
			if (v < 0.0f)
				v += 360.0f;
			a->v[i] = v;
		}
	}
}

void rgd3_angles_normalize180(Rgd3Angles* a)
{
	rgd3_angles_normalize360(a);
	if (a->pitch > 180.0f)
		a->pitch -= 360.0f;
	if (a->yaw > 180.0f)
		a->yaw -= 360.0f;
	if (a->roll > 180.0f)
		a->roll -= 360.0f;
}

void rgd3_angles_to_vectors(const Rgd3Angles* a, Rgd3Vec3* forward, Rgd3Vec3* right, Rgd3Vec3* up)
{
	float sr, sp, sy, cr, cp, cy;

	rgd3_math_sincos(RGD3_DEG2RAD(a->yaw), &sy, &cy);
	rgd3_math_sincos(RGD3_DEG2RAD(a->pitch), &sp, &cp);
	rgd3_math_sincos(RGD3_DEG2RAD(a->roll), &sr, &cr);

	if (forward)
		*forward = rgd3_vec3(cp * cy, cp * sy, -sp);
	if (right)
		*right = rgd3_vec3(-sr * sp * cy + cr * sy, -sr * sp * sy + -cr * cy, -sr * cp);
	if (up)
		*up = rgd3_vec3(cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp);
}

Rgd3Vec3 rgd3_angles_to_forward(const Rgd3Angles* a)
{
	float sp, sy, cp, cy;
	rgd3_math_sincos(RGD3_DEG2RAD(a->yaw), &sy, &cy);
	rgd3_math_sincos(RGD3_DEG2RAD(a->pitch), &sp, &cp);
	return rgd3_vec3(cp * cy, cp * sy, -sp);
}

Rgd3Quat rgd3_angles_to_quat(const Rgd3Angles* a)
{
	float sx, cx, sy, cy, sz, cz;
	float sxcy, cxcy, sxsy, cxsy;

	rgd3_math_sincos(RGD3_DEG2RAD(a->yaw) * 0.5f, &sz, &cz);
	rgd3_math_sincos(RGD3_DEG2RAD(a->pitch) * 0.5f, &sy, &cy);
	rgd3_math_sincos(RGD3_DEG2RAD(a->roll) * 0.5f, &sx, &cx);

	sxcy = sx * cy;
	cxcy = cx * cy;
	sxsy = sx * sy;
	cxsy = cx * sy;

	return rgd3_quat(cxsy * sz - sxcy * cz, -cxsy * cz - sxcy * sz, sxsy * cz - cxcy * sz, cxcy * cz + sxsy * sz);
}

void rgd3_angles_to_rotation(const Rgd3Angles* a, Rgd3Rotation* out)
{
	Rgd3Quat q;
	if (a->pitch == 0.0f)
	{
		if (a->yaw == 0.0f)
		{
			rgd3_rotation_set(out, rgd3_vec3_zero(), rgd3_vec3(-1.0f, 0.0f, 0.0f), a->roll);
			return;
		}
		if (a->roll == 0.0f)
		{
			rgd3_rotation_set(out, rgd3_vec3_zero(), rgd3_vec3(0.0f, 0.0f, -1.0f), a->yaw);
			return;
		}
	}
	else if (a->yaw == 0.0f && a->roll == 0.0f)
	{
		rgd3_rotation_set(out, rgd3_vec3_zero(), rgd3_vec3(0.0f, -1.0f, 0.0f), a->pitch);
		return;
	}
	q = rgd3_angles_to_quat(a);
	rgd3_rotation_from_quat_parts(rgd3_vec3(q.x, q.y, q.z), q.w, out);
}

void rgd3_angles_to_mat3(const Rgd3Angles* a, Rgd3Mat3* out)
{
	float sr, sp, sy, cr, cp, cy;

	rgd3_math_sincos(RGD3_DEG2RAD(a->yaw), &sy, &cy);
	rgd3_math_sincos(RGD3_DEG2RAD(a->pitch), &sp, &cp);
	rgd3_math_sincos(RGD3_DEG2RAD(a->roll), &sr, &cr);

	out->mat[0] = rgd3_vec3(cp * cy, cp * sy, -sp);
	out->mat[1] = rgd3_vec3(sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp);
	out->mat[2] = rgd3_vec3(cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp);
}

void rgd3_angles_to_mat4(const Rgd3Angles* a, Rgd3Mat4* out)
{
	Rgd3Mat3 m;
	rgd3_angles_to_mat3(a, &m);
	rgd3_mat3_to_mat4(&m, out);
}

Rgd3Vec3 rgd3_angles_to_angular_velocity(const Rgd3Angles* a)
{
	Rgd3Rotation r;
	rgd3_angles_to_rotation(a, &r);
	return rgd3_vec3_scale(r.vec, RGD3_DEG2RAD(r.angle));
}

/* ===================================================================== */
/* Rotation                                                               */
/* ===================================================================== */

static void rgd3_rotation_compute_mat3(const Rgd3Rotation* r, Rgd3Mat3* axis)
{
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
	float a, c, s, x, y, z;

	a = r->angle * (RGD3_MATH_M_DEG2RAD * 0.5f);
	rgd3_math_sincos(a, &s, &c);

	x = r->vec.x * s;
	y = r->vec.y * s;
	z = r->vec.z * s;

	x2 = x + x;
	y2 = y + y;
	z2 = z + z;

	xx = x * x2;
	xy = x * y2;
	xz = x * z2;

	yy = y * y2;
	yz = y * z2;
	zz = z * z2;

	wx = c * x2;
	wy = c * y2;
	wz = c * z2;

	axis->mat[0].x = 1.0f - (yy + zz);
	axis->mat[0].y = xy - wz;
	axis->mat[0].z = xz + wy;

	axis->mat[1].x = xy + wz;
	axis->mat[1].y = 1.0f - (xx + zz);
	axis->mat[1].z = yz - wx;

	axis->mat[2].x = xz - wy;
	axis->mat[2].y = yz + wx;
	axis->mat[2].z = 1.0f - (xx + yy);
}

const Rgd3Mat3* rgd3_rotation_to_mat3(Rgd3Rotation* r)
{
	if (!r->axisValid)
	{
		rgd3_rotation_compute_mat3(r, &r->axis);
		r->axisValid = 1;
	}
	return &r->axis;
}

void rgd3_rotation_get_mat3(const Rgd3Rotation* r, Rgd3Mat3* out)
{
	if (r->axisValid)
		*out = r->axis;
	else
		rgd3_rotation_compute_mat3(r, out);
}

void rgd3_rotation_to_mat4(const Rgd3Rotation* r, Rgd3Mat4* out)
{
	Rgd3Mat3 m;
	rgd3_rotation_get_mat3(r, &m);
	rgd3_mat3_to_mat4(&m, out);
}

Rgd3Quat rgd3_rotation_to_quat(const Rgd3Rotation* r)
{
	float a, s, c;
	a = r->angle * (RGD3_MATH_M_DEG2RAD * 0.5f);
	rgd3_math_sincos(a, &s, &c);
	return rgd3_quat(r->vec.x * s, r->vec.y * s, r->vec.z * s, c);
}

Rgd3Angles rgd3_rotation_to_angles(const Rgd3Rotation* r)
{
	Rgd3Mat3 m;
	rgd3_rotation_get_mat3(r, &m);
	return rgd3_mat3_to_angles(&m);
}

Rgd3Vec3 rgd3_rotation_to_angular_velocity(const Rgd3Rotation* r)
{
	return rgd3_vec3_scale(r->vec, RGD3_DEG2RAD(r->angle));
}

Rgd3Vec3 rgd3_rotation_rotate_point(const Rgd3Rotation* r, const Rgd3Vec3* p)
{
	Rgd3Mat3 m;
	rgd3_rotation_get_mat3(r, &m);
	return rgd3_vec3_add(rgd3_mat3_mul_vec(&m, rgd3_vec3_sub(*p, r->origin)), r->origin);
}

void rgd3_rotation_normalize180(Rgd3Rotation* r)
{
	r->angle = (float)((double)r->angle - floor((double)(r->angle / 360.0f)) * 360.0);
	if (r->angle > 180.0f)
		r->angle -= 360.0f;
	else if (r->angle < -180.0f)
		r->angle += 360.0f;
}

void rgd3_rotation_normalize360(Rgd3Rotation* r)
{
	r->angle = (float)((double)r->angle - floor((double)(r->angle / 360.0f)) * 360.0);
	if (r->angle > 360.0f)
		r->angle -= 360.0f;
	else if (r->angle < 0.0f)
		r->angle += 360.0f;
}

/* ===================================================================== */
/* Plane                                                                  */
/* ===================================================================== */

int rgd3_plane_type(const Rgd3Plane* p)
{
	if (p->a == 0.0f)
	{
		if (p->b == 0.0f)
			return p->c > 0.0f ? RGD3_PLANETYPE_Z : RGD3_PLANETYPE_NEGZ;
		else if (p->c == 0.0f)
			return p->b > 0.0f ? RGD3_PLANETYPE_Y : RGD3_PLANETYPE_NEGY;
		return RGD3_PLANETYPE_ZEROX;
	}
	else if (p->b == 0.0f)
	{
		if (p->c == 0.0f)
			return p->a > 0.0f ? RGD3_PLANETYPE_X : RGD3_PLANETYPE_NEGX;
		return RGD3_PLANETYPE_ZEROY;
	}
	else if (p->c == 0.0f)
	{
		return RGD3_PLANETYPE_ZEROZ;
	}
	return RGD3_PLANETYPE_NONAXIAL;
}

int rgd3_plane_plane_intersection(const Rgd3Plane* p, const Rgd3Plane* other, Rgd3Vec3* start, Rgd3Vec3* dir)
{
	double n00, n01, n11, det, invDet, f0, f1;
	Rgd3Vec3 n0 = rgd3_plane_normal(*p);
	Rgd3Vec3 n1 = rgd3_plane_normal(*other);

	n00 = rgd3_vec3_length_sqr(n0);
	n01 = rgd3_vec3_dot(n0, n1);
	n11 = rgd3_vec3_length_sqr(n1);
	det = n00 * n11 - n01 * n01;
	if (fabsf((float)det) < 1e-6f)
		return 0;
	invDet = 1.0f / det;
	f0 = (n01 * other->d - n11 * p->d) * invDet;
	f1 = (n01 * p->d - n00 * other->d) * invDet;
	*dir = rgd3_vec3_cross(n0, n1);
	*start = rgd3_vec3_add(rgd3_vec3_scale(n0, (float)f0), rgd3_vec3_scale(n1, (float)f1));
	return 1;
}

int rgd3_plane_height_fit(Rgd3Plane* p, const Rgd3Vec3* points, int numPoints)
{
	float sumXX = 0.0f, sumXY = 0.0f, sumXZ = 0.0f;
	float sumYY = 0.0f, sumYZ = 0.0f;
	Rgd3Vec3 sum, average, dir;
	Rgd3Mat2 m;
	int i;

	if (numPoints == 1)
	{
		*p = rgd3_plane(0.0f, 0.0f, 1.0f, -points[0].z);
		return 1;
	}
	if (numPoints == 2)
	{
		dir = rgd3_vec3_sub(points[1], points[0]);
		rgd3_plane_set_normal(p, rgd3_vec3_cross(rgd3_vec3_cross(dir, rgd3_vec3(0, 0, 1)), dir));
		rgd3_plane_normalize(p, 1);
		p->d = -rgd3_vec3_dot(rgd3_plane_normal(*p), points[0]);
		return 1;
	}

	sum = rgd3_vec3_zero();
	for (i = 0; i < numPoints; i++)
		sum = rgd3_vec3_add(sum, points[i]);
	average = rgd3_vec3_div(sum, (float)numPoints);

	for (i = 0; i < numPoints; i++)
	{
		dir = rgd3_vec3_sub(points[i], average);
		sumXX += dir.x * dir.x;
		sumXY += dir.x * dir.y;
		sumXZ += dir.x * dir.z;
		sumYY += dir.y * dir.y;
		sumYZ += dir.y * dir.z;
	}

	rgd3_mat2_set(&m, sumXX, sumXY, sumXY, sumYY);
	if (!rgd3_mat2_inverse_self(&m))
		return 0;

	p->a = -sumXZ * m.mat[0].x - sumYZ * m.mat[0].y;
	p->b = -sumXZ * m.mat[1].x - sumYZ * m.mat[1].y;
	p->c = 1.0f;
	rgd3_plane_normalize(p, 1);
	p->d = -(p->a * average.x + p->b * average.y + p->c * average.z);
	return 1;
}

/* ===================================================================== */
/* Pluecker                                                               */
/* ===================================================================== */

int rgd3_pluecker_from_planes(Rgd3Pluecker* pl, const Rgd3Plane* p1, const Rgd3Plane* p2)
{
	pl->p[0] = -(p1->v[2] * -p2->v[3] - p2->v[2] * -p1->v[3]);
	pl->p[1] = -(p2->v[1] * -p1->v[3] - p1->v[1] * -p2->v[3]);
	pl->p[2] = p1->v[1] * p2->v[2] - p2->v[1] * p1->v[2];
	pl->p[3] = -(p1->v[0] * -p2->v[3] - p2->v[0] * -p1->v[3]);
	pl->p[4] = p1->v[0] * p2->v[1] - p2->v[0] * p1->v[1];
	pl->p[5] = p1->v[0] * p2->v[2] - p2->v[0] * p1->v[2];
	return (pl->p[2] != 0.0f || pl->p[5] != 0.0f || pl->p[4] != 0.0f);
}

int rgd3_pluecker_to_line(const Rgd3Pluecker* pl, Rgd3Vec3* start, Rgd3Vec3* end)
{
	Rgd3Vec3 dir1 = rgd3_vec3(pl->p[3], -pl->p[1], pl->p[0]);
	Rgd3Vec3 dir2 = rgd3_vec3(-pl->p[2], pl->p[5], -pl->p[4]);
	float d = rgd3_vec3_dot(dir2, dir2);
	if (d == 0.0f)
		return 0;
	*start = rgd3_vec3_scale(rgd3_vec3_cross(dir2, dir1), (1.0f / d));
	*end = rgd3_vec3_add(*start, dir2);
	return 1;
}

int rgd3_pluecker_to_ray(const Rgd3Pluecker* pl, Rgd3Vec3* start, Rgd3Vec3* dir)
{
	Rgd3Vec3 dir1 = rgd3_vec3(pl->p[3], -pl->p[1], pl->p[0]);
	float d;
	*dir = rgd3_vec3(-pl->p[2], pl->p[5], -pl->p[4]);
	d = rgd3_vec3_dot(*dir, *dir);
	if (d == 0.0f)
		return 0;
	*start = rgd3_vec3_scale(rgd3_vec3_cross(*dir, dir1), (1.0f / d));
	return 1;
}

float rgd3_pluecker_distance3d_sqr(const Rgd3Pluecker* a, const Rgd3Pluecker* b)
{
	const float* p = a->p;
	const float* q = b->p;
	float d, s;
	Rgd3Vec3 dir;

	dir.x = -q[5] * p[4] - q[4] * -p[5];
	dir.y = q[4] * p[2] - q[2] * p[4];
	dir.z = q[2] * -p[5] - -q[5] * p[2];
	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
		return -1.0f; /* FIXME in idlib: parallel lines */
	d = q[4] * (p[2] * dir.y - -p[5] * dir.x) + q[5] * (p[2] * dir.z - p[4] * dir.x) +
		q[2] * (-p[5] * dir.z - p[4] * dir.y);
	s = rgd3_pluecker_permuted_inner_product(a, b) / d;
	return rgd3_vec3_dot(dir, dir) * (s * s);
}

/* ===================================================================== */
/* Bounds                                                                 */
/* ===================================================================== */

float rgd3_bounds_radius(const Rgd3Bounds* b)
{
	float total = 0.0f;
	int i;
	for (i = 0; i < 3; i++)
	{
		float b0 = fabsf(b->b[0].v[i]);
		float b1 = fabsf(b->b[1].v[i]);
		if (b0 > b1)
			total += b0 * b0;
		else
			total += b1 * b1;
	}
	return rgd3_math_sqrt(total);
}

float rgd3_bounds_radius_from(const Rgd3Bounds* b, const Rgd3Vec3* center)
{
	float total = 0.0f;
	int i;
	for (i = 0; i < 3; i++)
	{
		float b0 = fabsf(center->v[i] - b->b[0].v[i]);
		float b1 = fabsf(b->b[1].v[i] - center->v[i]);
		if (b0 > b1)
			total += b0 * b0;
		else
			total += b1 * b1;
	}
	return rgd3_math_sqrt(total);
}

/* center distance d1 and the projected half extent d2 */
static void rgd3_bounds_plane_extent(const Rgd3Bounds* b, const Rgd3Plane* plane, float* d1, float* d2)
{
	Rgd3Vec3 center = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	*d1 = rgd3_plane_distance(*plane, center);
	*d2 = fabsf((b->b[1].x - center.x) * plane->a) + fabsf((b->b[1].y - center.y) * plane->b) +
		  fabsf((b->b[1].z - center.z) * plane->c);
}

float rgd3_bounds_plane_distance(const Rgd3Bounds* b, const Rgd3Plane* plane)
{
	float d1, d2;
	rgd3_bounds_plane_extent(b, plane, &d1, &d2);
	if (d1 - d2 > 0.0f)
		return d1 - d2;
	if (d1 + d2 < 0.0f)
		return d1 + d2;
	return 0.0f;
}

int rgd3_bounds_plane_side(const Rgd3Bounds* b, const Rgd3Plane* plane, float epsilon)
{
	float d1, d2;
	rgd3_bounds_plane_extent(b, plane, &d1, &d2);
	if (d1 - d2 > epsilon)
		return RGD3_PLANESIDE_FRONT;
	if (d1 + d2 < -epsilon)
		return RGD3_PLANESIDE_BACK;
	return RGD3_PLANESIDE_CROSS;
}

int rgd3_bounds_line_intersection(const Rgd3Bounds* b, const Rgd3Vec3* start, const Rgd3Vec3* end)
{
	const Rgd3Vec3 center = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	const Rgd3Vec3 extents = rgd3_vec3_sub(b->b[1], center);
	const Rgd3Vec3 lineDir = rgd3_vec3_scale(rgd3_vec3_sub(*end, *start), 0.5f);
	const Rgd3Vec3 lineCenter = rgd3_vec3_add(*start, lineDir);
	const Rgd3Vec3 dir = rgd3_vec3_sub(lineCenter, center);
	Rgd3Vec3 cross;
	float ld0, ld1, ld2;

	ld0 = fabsf(lineDir.x);
	if (fabsf(dir.x) > extents.x + ld0)
		return 0;
	ld1 = fabsf(lineDir.y);
	if (fabsf(dir.y) > extents.y + ld1)
		return 0;
	ld2 = fabsf(lineDir.z);
	if (fabsf(dir.z) > extents.z + ld2)
		return 0;

	cross = rgd3_vec3_cross(lineDir, dir);
	if (fabsf(cross.x) > extents.y * ld2 + extents.z * ld1)
		return 0;
	if (fabsf(cross.y) > extents.x * ld2 + extents.z * ld0)
		return 0;
	if (fabsf(cross.z) > extents.x * ld1 + extents.y * ld0)
		return 0;
	return 1;
}

int rgd3_bounds_ray_intersection(const Rgd3Bounds* b, const Rgd3Vec3* start, const Rgd3Vec3* dir, float* scale)
{
	int i, ax0, ax1, ax2, side, inside;
	float f, sc = 0.0f;
	Rgd3Vec3 hit;

	ax0 = -1;
	inside = 0;
	for (i = 0; i < 3; i++)
	{
		if (start->v[i] < b->b[0].v[i])
			side = 0;
		else if (start->v[i] > b->b[1].v[i])
			side = 1;
		else
		{
			inside++;
			continue;
		}
		if (dir->v[i] == 0.0f)
			continue;
		f = (start->v[i] - b->b[side].v[i]);
		if (ax0 < 0 || fabsf(f) > fabsf(sc * dir->v[i]))
		{
			sc = -(f / dir->v[i]);
			ax0 = i;
		}
	}

	if (ax0 < 0)
	{
		*scale = 0.0f;
		/* true if the start point is inside the bounds */
		return (inside == 3);
	}

	*scale = sc;
	ax1 = (ax0 + 1) % 3;
	ax2 = (ax0 + 2) % 3;
	hit.v[ax1] = start->v[ax1] + sc * dir->v[ax1];
	hit.v[ax2] = start->v[ax2] + sc * dir->v[ax2];

	return (hit.v[ax1] >= b->b[0].v[ax1] && hit.v[ax1] <= b->b[1].v[ax1] && hit.v[ax2] >= b->b[0].v[ax2] &&
			hit.v[ax2] <= b->b[1].v[ax2]);
}

void rgd3_bounds_from_transformed(Rgd3Bounds* b, const Rgd3Bounds* bounds, const Rgd3Vec3* origin,
								  const Rgd3Mat3* axis)
{
	Rgd3Vec3 center, extents, rotatedExtents;
	int i;

	center = rgd3_vec3_scale(rgd3_vec3_add(bounds->b[0], bounds->b[1]), 0.5f);
	extents = rgd3_vec3_sub(bounds->b[1], center);

	for (i = 0; i < 3; i++)
	{
		rotatedExtents.v[i] = fabsf(extents.x * axis->mat[0].v[i]) + fabsf(extents.y * axis->mat[1].v[i]) +
							  fabsf(extents.z * axis->mat[2].v[i]);
	}

	center = rgd3_vec3_add(*origin, rgd3_mat3_mul_vec(axis, center));
	b->b[0] = rgd3_vec3_sub(center, rotatedExtents);
	b->b[1] = rgd3_vec3_add(center, rotatedExtents);
}

void rgd3_bounds_rotate(const Rgd3Bounds* b, const Rgd3Mat3* rotation, Rgd3Bounds* out)
{
	Rgd3Vec3 zero = rgd3_vec3_zero();
	rgd3_bounds_from_transformed(out, b, &zero, rotation);
}

/* idSIMD MinMax: starts at +-INFINITY (1e30) */
void rgd3_bounds_from_points(Rgd3Bounds* b, const Rgd3Vec3* points, int numPoints)
{
	int i;
	rgd3_bounds_clear(b);
	for (i = 0; i < numPoints; i++)
	{
		const Rgd3Vec3* v = &points[i];
		if (v->x < b->b[0].x)
			b->b[0].x = v->x;
		if (v->x > b->b[1].x)
			b->b[1].x = v->x;
		if (v->y < b->b[0].y)
			b->b[0].y = v->y;
		if (v->y > b->b[1].y)
			b->b[1].y = v->y;
		if (v->z < b->b[0].z)
			b->b[0].z = v->z;
		if (v->z > b->b[1].z)
			b->b[1].z = v->z;
	}
}

void rgd3_bounds_from_point_translation(Rgd3Bounds* b, const Rgd3Vec3* point, const Rgd3Vec3* translation)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		if (translation->v[i] < 0.0f)
		{
			b->b[0].v[i] = point->v[i] + translation->v[i];
			b->b[1].v[i] = point->v[i];
		}
		else
		{
			b->b[0].v[i] = point->v[i];
			b->b[1].v[i] = point->v[i] + translation->v[i];
		}
	}
}

void rgd3_bounds_from_bounds_translation(Rgd3Bounds* b, const Rgd3Bounds* bounds, const Rgd3Vec3* origin,
										 const Rgd3Mat3* axis, const Rgd3Vec3* translation)
{
	int i;
	if (rgd3_mat3_is_rotated(axis))
	{
		rgd3_bounds_from_transformed(b, bounds, origin, axis);
	}
	else
	{
		b->b[0] = rgd3_vec3_add(bounds->b[0], *origin);
		b->b[1] = rgd3_vec3_add(bounds->b[1], *origin);
	}
	for (i = 0; i < 3; i++)
	{
		if (translation->v[i] < 0.0f)
			b->b[0].v[i] += translation->v[i];
		else
			b->b[1].v[i] += translation->v[i];
	}
}

/* BoundsForPointRotation: only for rotations < 180 degrees */
static void rgd3_bounds_for_point_rotation(const Rgd3Vec3* start, const Rgd3Rotation* rotation, Rgd3Bounds* bounds)
{
	Rgd3Vec3 v1, v2, origin, axis, end;
	float radiusSqr;
	int i;

	end = rgd3_rotation_rotate_point(rotation, start);
	axis = rotation->vec;
	origin = rgd3_vec3_add(rotation->origin,
						   rgd3_vec3_scale(axis, rgd3_vec3_dot(axis, rgd3_vec3_sub(*start, rotation->origin))));
	radiusSqr = rgd3_vec3_length_sqr(rgd3_vec3_sub(*start, origin));
	v1 = rgd3_vec3_cross(rgd3_vec3_sub(*start, origin), axis);
	v2 = rgd3_vec3_cross(rgd3_vec3_sub(end, origin), axis);

	for (i = 0; i < 3; i++)
	{
		float s = start->v[i];
		float e = end.v[i];
		/* the derivative changes sign along this axis during the rotation */
		if ((v1.v[i] > 0.0f && v2.v[i] < 0.0f) || (v1.v[i] < 0.0f && v2.v[i] > 0.0f))
		{
			if ((0.5f * (s + e) - origin.v[i]) > 0.0f)
			{
				bounds->b[0].v[i] = (s < e) ? s : e;
				bounds->b[1].v[i] = origin.v[i] + rgd3_math_sqrt(radiusSqr * (1.0f - axis.v[i] * axis.v[i]));
			}
			else
			{
				bounds->b[0].v[i] = origin.v[i] - rgd3_math_sqrt(radiusSqr * (1.0f - axis.v[i] * axis.v[i]));
				bounds->b[1].v[i] = (s > e) ? s : e;
			}
		}
		else if (s > e)
		{
			bounds->b[0].v[i] = e;
			bounds->b[1].v[i] = s;
		}
		else
		{
			bounds->b[0].v[i] = s;
			bounds->b[1].v[i] = e;
		}
	}
}

void rgd3_bounds_from_point_rotation(Rgd3Bounds* b, const Rgd3Vec3* point, const Rgd3Rotation* rotation)
{
	if (fabsf(rotation->angle) < 180.0f)
	{
		rgd3_bounds_for_point_rotation(point, rotation, b);
	}
	else
	{
		float radius = rgd3_vec3_length(rgd3_vec3_sub(*point, rotation->origin));
		/* FIXME in idlib: these bounds are usually way larger */
		b->b[0] = rgd3_vec3_splat(-radius);
		b->b[1] = rgd3_vec3_splat(radius);
	}
}

void rgd3_bounds_from_bounds_rotation(Rgd3Bounds* b, const Rgd3Bounds* bounds, const Rgd3Vec3* origin,
									  const Rgd3Mat3* axis, const Rgd3Rotation* rotation)
{
	Rgd3Bounds src = *bounds;
	Rgd3Bounds rBounds;
	Rgd3Vec3 point;
	int i;

	if (fabsf(rotation->angle) < 180.0f)
	{
		point = rgd3_vec3_add(rgd3_mat3_mul_vec(axis, src.b[0]), *origin);
		rgd3_bounds_for_point_rotation(&point, rotation, b);
		for (i = 1; i < 8; i++)
		{
			point.x = src.b[(i ^ (i >> 1)) & 1].x;
			point.y = src.b[(i >> 1) & 1].y;
			point.z = src.b[(i >> 2) & 1].z;
			point = rgd3_vec3_add(rgd3_mat3_mul_vec(axis, point), *origin);
			rgd3_bounds_for_point_rotation(&point, rotation, &rBounds);
			rgd3_bounds_add_bounds(b, &rBounds);
		}
	}
	else
	{
		float radius;
		/* idlib uses ( maxs - mins ) / 2 here, not the center */
		point = rgd3_vec3_scale(rgd3_vec3_sub(src.b[1], src.b[0]), 0.5f);
		radius = rgd3_vec3_length(rgd3_vec3_sub(src.b[1], point)) +
				 rgd3_vec3_length(rgd3_vec3_sub(point, rotation->origin));
		b->b[0] = rgd3_vec3_splat(-radius);
		b->b[1] = rgd3_vec3_splat(radius);
	}
}

void rgd3_bounds_to_points(const Rgd3Bounds* b, Rgd3Vec3 points[8])
{
	int i;
	for (i = 0; i < 8; i++)
	{
		points[i].x = b->b[(i ^ (i >> 1)) & 1].x;
		points[i].y = b->b[(i >> 1) & 1].y;
		points[i].z = b->b[(i >> 2) & 1].z;
	}
}

/* ===================================================================== */
/* Sphere                                                                 */
/* ===================================================================== */

float rgd3_sphere_plane_distance(const Rgd3Sphere* s, const Rgd3Plane* plane)
{
	float d = rgd3_plane_distance(*plane, s->origin);
	if (d > s->radius)
		return d - s->radius;
	if (d < -s->radius)
		return d + s->radius;
	return 0.0f;
}

int rgd3_sphere_plane_side(const Rgd3Sphere* s, const Rgd3Plane* plane, float epsilon)
{
	float d = rgd3_plane_distance(*plane, s->origin);
	if (d > s->radius + epsilon)
		return RGD3_PLANESIDE_FRONT;
	if (d < -s->radius - epsilon)
		return RGD3_PLANESIDE_BACK;
	return RGD3_PLANESIDE_CROSS;
}

int rgd3_sphere_line_intersection(const Rgd3Sphere* s, const Rgd3Vec3* start, const Rgd3Vec3* end)
{
	Rgd3Vec3 r, st, e;
	float a;

	st = rgd3_vec3_sub(*start, s->origin);
	e = rgd3_vec3_sub(*end, s->origin);
	r = rgd3_vec3_sub(e, st);
	a = rgd3_vec3_dot(rgd3_vec3_neg(st), r);
	if (a <= 0)
		return (rgd3_vec3_dot(st, st) < s->radius * s->radius);
	else if (a >= rgd3_vec3_dot(r, r))
		return (rgd3_vec3_dot(e, e) < s->radius * s->radius);
	r = rgd3_vec3_add(st, rgd3_vec3_scale(r, (a / rgd3_vec3_dot(r, r))));
	return (rgd3_vec3_dot(r, r) < s->radius * s->radius);
}

int rgd3_sphere_ray_intersection(const Rgd3Sphere* s, const Rgd3Vec3* start, const Rgd3Vec3* dir, float* scale1,
								 float* scale2)
{
	double a, b, c, d, sqrtd;
	Rgd3Vec3 p;

	p = rgd3_vec3_sub(*start, s->origin);
	a = rgd3_vec3_dot(*dir, *dir);
	b = rgd3_vec3_dot(*dir, p);
	c = rgd3_vec3_dot(p, p) - s->radius * s->radius;
	d = b * b - c * a;
	if (d < 0.0f)
		return 0;
	sqrtd = rgd3_math_sqrt((float)d);
	a = 1.0f / a;
	*scale1 = (float)((-b + sqrtd) * a);
	*scale2 = (float)((-b - sqrtd) * a);
	return 1;
}

void rgd3_sphere_from_points(Rgd3Sphere* s, const Rgd3Vec3* points, int numPoints)
{
	Rgd3Bounds mm;
	float radiusSqr = 0.0f, dist;
	int i;

	rgd3_bounds_from_points(&mm, points, numPoints);
	s->origin = rgd3_vec3_scale(rgd3_vec3_add(mm.b[0], mm.b[1]), 0.5f);
	for (i = 0; i < numPoints; i++)
	{
		dist = rgd3_vec3_length_sqr(rgd3_vec3_sub(points[i], s->origin));
		if (dist > radiusSqr)
			radiusSqr = dist;
	}
	s->radius = rgd3_math_sqrt(radiusSqr);
}

void rgd3_sphere_from_sphere_translation(Rgd3Sphere* s, const Rgd3Sphere* sphere, const Rgd3Vec3* start,
										 const Rgd3Vec3* translation)
{
	Rgd3Sphere src = *sphere;
	s->origin = rgd3_vec3_add(rgd3_vec3_add(*start, src.origin), rgd3_vec3_scale(*translation, 0.5f));
	s->radius = rgd3_math_sqrt(0.5f * rgd3_vec3_length_sqr(*translation)) + src.radius;
}

void rgd3_sphere_from_point_rotation(Rgd3Sphere* s, const Rgd3Vec3* point, const Rgd3Rotation* rotation)
{
	Rgd3Vec3 end = rgd3_rotation_rotate_point(rotation, point);
	s->origin = rgd3_vec3_scale(rgd3_vec3_add(*point, end), 0.5f);
	s->radius = rgd3_math_sqrt(0.5f * rgd3_vec3_length_sqr(rgd3_vec3_sub(end, *point)));
}

void rgd3_sphere_from_sphere_rotation(Rgd3Sphere* s, const Rgd3Sphere* sphere, const Rgd3Vec3* start,
									  const Rgd3Rotation* rotation)
{
	Rgd3Sphere src = *sphere;
	Rgd3Vec3 end = rgd3_rotation_rotate_point(rotation, &src.origin);
	s->origin = rgd3_vec3_add(*start, rgd3_vec3_scale(rgd3_vec3_add(src.origin, end), 0.5f));
	s->radius = rgd3_math_sqrt(0.5f * rgd3_vec3_length_sqr(rgd3_vec3_sub(end, src.origin))) + src.radius;
}

/* ===================================================================== */
/* Box                                                                    */
/* ===================================================================== */

static void rgd3_box_plane_extent(const Rgd3Box* box, const Rgd3Plane* plane, float* d1, float* d2)
{
	*d1 = rgd3_plane_distance(*plane, box->center);
	*d2 = fabsf(box->extents.x * plane->a) + fabsf(box->extents.y * plane->b) + fabsf(box->extents.z * plane->c);
}

float rgd3_box_plane_distance(const Rgd3Box* box, const Rgd3Plane* plane)
{
	float d1, d2;
	rgd3_box_plane_extent(box, plane, &d1, &d2);
	if (d1 - d2 > 0.0f)
		return d1 - d2;
	if (d1 + d2 < 0.0f)
		return d1 + d2;
	return 0.0f;
}

int rgd3_box_plane_side(const Rgd3Box* box, const Rgd3Plane* plane, float epsilon)
{
	float d1, d2;
	rgd3_box_plane_extent(box, plane, &d1, &d2);
	if (d1 - d2 > epsilon)
		return RGD3_PLANESIDE_FRONT;
	if (d1 + d2 < -epsilon)
		return RGD3_PLANESIDE_BACK;
	return RGD3_PLANESIDE_CROSS;
}

int rgd3_box_intersects_box(const Rgd3Box* box, const Rgd3Box* a)
{
	const Rgd3Vec3* ax = box->axis.mat;
	const Rgd3Vec3* bx = a->axis.mat;
	const float* e = box->extents.v;
	const float* ae = a->extents.v;
	Rgd3Vec3 dir;	   /* vector between centers */
	float c[3][3];	   /* c = axis.Transpose() * a.axis */
	float ac[3][3];	   /* absolute values of c */
	float axisdir[3];  /* axis[i] * dir */
	float d, e0, e1;   /* distance between centers and projected extents */
	int i, j;

	dir = rgd3_vec3_sub(a->center, box->center);

	/* axes A0..A2 */
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			c[i][j] = rgd3_vec3_dot(ax[i], bx[j]);
			ac[i][j] = fabsf(c[i][j]);
		}
		axisdir[i] = rgd3_vec3_dot(ax[i], dir);
		d = fabsf(axisdir[i]);
		e0 = e[i];
		e1 = ae[0] * ac[i][0] + ae[1] * ac[i][1] + ae[2] * ac[i][2];
		if (d > e0 + e1)
			return 0;
	}

	/* axes B0..B2 */
	for (j = 0; j < 3; j++)
	{
		d = fabsf(rgd3_vec3_dot(bx[j], dir));
		e0 = e[0] * ac[0][j] + e[1] * ac[1][j] + e[2] * ac[2][j];
		e1 = ae[j];
		if (d > e0 + e1)
			return 0;
	}

	/* A0 x B0..B2 */
	d = fabsf(axisdir[2] * c[1][0] - axisdir[1] * c[2][0]);
	e0 = e[1] * ac[2][0] + e[2] * ac[1][0];
	e1 = ae[1] * ac[0][2] + ae[2] * ac[0][1];
	if (d > e0 + e1)
		return 0;
	d = fabsf(axisdir[2] * c[1][1] - axisdir[1] * c[2][1]);
	e0 = e[1] * ac[2][1] + e[2] * ac[1][1];
	e1 = ae[0] * ac[0][2] + ae[2] * ac[0][0];
	if (d > e0 + e1)
		return 0;
	d = fabsf(axisdir[2] * c[1][2] - axisdir[1] * c[2][2]);
	e0 = e[1] * ac[2][2] + e[2] * ac[1][2];
	e1 = ae[0] * ac[0][1] + ae[1] * ac[0][0];
	if (d > e0 + e1)
		return 0;

	/* A1 x B0..B2 */
	d = fabsf(axisdir[0] * c[2][0] - axisdir[2] * c[0][0]);
	e0 = e[0] * ac[2][0] + e[2] * ac[0][0];
	e1 = ae[1] * ac[1][2] + ae[2] * ac[1][1];
	if (d > e0 + e1)
		return 0;
	d = fabsf(axisdir[0] * c[2][1] - axisdir[2] * c[0][1]);
	e0 = e[0] * ac[2][1] + e[2] * ac[0][1];
	e1 = ae[0] * ac[1][2] + ae[2] * ac[1][0];
	if (d > e0 + e1)
		return 0;
	d = fabsf(axisdir[0] * c[2][2] - axisdir[2] * c[0][2]);
	e0 = e[0] * ac[2][2] + e[2] * ac[0][2];
	e1 = ae[0] * ac[1][1] + ae[1] * ac[1][0];
	if (d > e0 + e1)
		return 0;

	/* A2 x B0..B2 */
	d = fabsf(axisdir[1] * c[0][0] - axisdir[0] * c[1][0]);
	e0 = e[0] * ac[1][0] + e[1] * ac[0][0];
	e1 = ae[1] * ac[2][2] + ae[2] * ac[2][1];
	if (d > e0 + e1)
		return 0;
	d = fabsf(axisdir[1] * c[0][1] - axisdir[0] * c[1][1]);
	e0 = e[0] * ac[1][1] + e[1] * ac[0][1];
	e1 = ae[0] * ac[2][2] + ae[2] * ac[2][0];
	if (d > e0 + e1)
		return 0;
	d = fabsf(axisdir[1] * c[0][2] - axisdir[0] * c[1][2]);
	e0 = e[0] * ac[1][2] + e[1] * ac[0][2];
	e1 = ae[0] * ac[2][1] + ae[1] * ac[2][0];
	if (d > e0 + e1)
		return 0;
	return 1;
}

int rgd3_box_line_intersection(const Rgd3Box* box, const Rgd3Vec3* start, const Rgd3Vec3* end)
{
	const Rgd3Vec3* ax = box->axis.mat;
	float ld[3];
	Rgd3Vec3 lineDir = rgd3_vec3_scale(rgd3_vec3_sub(*end, *start), 0.5f);
	Rgd3Vec3 lineCenter = rgd3_vec3_add(*start, lineDir);
	Rgd3Vec3 dir = rgd3_vec3_sub(lineCenter, box->center);
	Rgd3Vec3 cross;

	ld[0] = fabsf(rgd3_vec3_dot(lineDir, ax[0]));
	if (fabsf(rgd3_vec3_dot(dir, ax[0])) > box->extents.x + ld[0])
		return 0;
	ld[1] = fabsf(rgd3_vec3_dot(lineDir, ax[1]));
	if (fabsf(rgd3_vec3_dot(dir, ax[1])) > box->extents.y + ld[1])
		return 0;
	ld[2] = fabsf(rgd3_vec3_dot(lineDir, ax[2]));
	if (fabsf(rgd3_vec3_dot(dir, ax[2])) > box->extents.z + ld[2])
		return 0;

	cross = rgd3_vec3_cross(lineDir, dir);
	if (fabsf(rgd3_vec3_dot(cross, ax[0])) > box->extents.y * ld[2] + box->extents.z * ld[1])
		return 0;
	if (fabsf(rgd3_vec3_dot(cross, ax[1])) > box->extents.x * ld[2] + box->extents.z * ld[0])
		return 0;
	if (fabsf(rgd3_vec3_dot(cross, ax[2])) > box->extents.x * ld[1] + box->extents.y * ld[0])
		return 0;
	return 1;
}

static int rgd3_box_plane_clip(float denom, float numer, float* scale0, float* scale1)
{
	if (denom > 0.0f)
	{
		if (numer > denom * *scale1)
			return 0;
		if (numer > denom * *scale0)
			*scale0 = numer / denom;
		return 1;
	}
	else if (denom < 0.0f)
	{
		if (numer > denom * *scale0)
			return 0;
		if (numer > denom * *scale1)
			*scale1 = numer / denom;
		return 1;
	}
	return (numer <= 0.0f);
}

int rgd3_box_ray_intersection(const Rgd3Box* box, const Rgd3Vec3* start, const Rgd3Vec3* dir, float* scale1,
							  float* scale2)
{
	Rgd3Vec3 localStart, localDir;
	const float* e = box->extents.v;

	/* ( v ) * axis.Transpose() */
	localStart = rgd3_mat3_transpose_mul_vec(&box->axis, rgd3_vec3_sub(*start, box->center));
	localDir = rgd3_mat3_transpose_mul_vec(&box->axis, *dir);

	*scale1 = -RGD3_MATH_INFINITY;
	*scale2 = RGD3_MATH_INFINITY;
	return rgd3_box_plane_clip(localDir.x, -localStart.x - e[0], scale1, scale2) &&
		   rgd3_box_plane_clip(-localDir.x, localStart.x - e[0], scale1, scale2) &&
		   rgd3_box_plane_clip(localDir.y, -localStart.y - e[1], scale1, scale2) &&
		   rgd3_box_plane_clip(-localDir.y, localStart.y - e[1], scale1, scale2) &&
		   rgd3_box_plane_clip(localDir.z, -localStart.z - e[2], scale1, scale2) &&
		   rgd3_box_plane_clip(-localDir.z, localStart.z - e[2], scale1, scale2);
}

void rgd3_box_to_points(const Rgd3Box* box, Rgd3Vec3 points[8])
{
	Rgd3Vec3 ax[3], temp[4];

	ax[0] = rgd3_vec3_scale(box->axis.mat[0], box->extents.x);
	ax[1] = rgd3_vec3_scale(box->axis.mat[1], box->extents.y);
	ax[2] = rgd3_vec3_scale(box->axis.mat[2], box->extents.z);
	temp[0] = rgd3_vec3_sub(box->center, ax[0]);
	temp[1] = rgd3_vec3_add(box->center, ax[0]);
	temp[2] = rgd3_vec3_sub(ax[1], ax[2]);
	temp[3] = rgd3_vec3_add(ax[1], ax[2]);
	points[0] = rgd3_vec3_sub(temp[0], temp[3]);
	points[1] = rgd3_vec3_sub(temp[1], temp[3]);
	points[2] = rgd3_vec3_add(temp[1], temp[2]);
	points[3] = rgd3_vec3_add(temp[0], temp[2]);
	points[4] = rgd3_vec3_sub(temp[0], temp[2]);
	points[5] = rgd3_vec3_sub(temp[1], temp[2]);
	points[6] = rgd3_vec3_add(temp[1], temp[3]);
	points[7] = rgd3_vec3_add(temp[0], temp[3]);
}

/* ===================================================================== */
/* joints                                                                 */
/* ===================================================================== */

void rgd3_jointmat_mul_self(Rgd3JointMat* j, const Rgd3JointMat* a)
{
	float* m = j->mat;
	const float* am = a->mat;
	float tmp[3];
	int c;

	/* columns 0..3 of the 3x4 are rotated by a's 3x3 */
	for (c = 0; c < 4; c++)
	{
		tmp[0] = m[0 * 4 + c] * am[0 * 4 + 0] + m[1 * 4 + c] * am[0 * 4 + 1] + m[2 * 4 + c] * am[0 * 4 + 2];
		tmp[1] = m[0 * 4 + c] * am[1 * 4 + 0] + m[1 * 4 + c] * am[1 * 4 + 1] + m[2 * 4 + c] * am[1 * 4 + 2];
		tmp[2] = m[0 * 4 + c] * am[2 * 4 + 0] + m[1 * 4 + c] * am[2 * 4 + 1] + m[2 * 4 + c] * am[2 * 4 + 2];
		m[0 * 4 + c] = tmp[0];
		m[1 * 4 + c] = tmp[1];
		m[2 * 4 + c] = tmp[2];
	}
	m[0 * 4 + 3] += am[0 * 4 + 3];
	m[1 * 4 + 3] += am[1 * 4 + 3];
	m[2 * 4 + 3] += am[2 * 4 + 3];
}

void rgd3_jointmat_untransform_self(Rgd3JointMat* j, const Rgd3JointMat* a)
{
	float* m = j->mat;
	const float* am = a->mat;
	float tmp[3];
	int c;

	m[0 * 4 + 3] -= am[0 * 4 + 3];
	m[1 * 4 + 3] -= am[1 * 4 + 3];
	m[2 * 4 + 3] -= am[2 * 4 + 3];

	for (c = 0; c < 4; c++)
	{
		tmp[0] = m[0 * 4 + c] * am[0 * 4 + 0] + m[1 * 4 + c] * am[1 * 4 + 0] + m[2 * 4 + c] * am[2 * 4 + 0];
		tmp[1] = m[0 * 4 + c] * am[0 * 4 + 1] + m[1 * 4 + c] * am[1 * 4 + 1] + m[2 * 4 + c] * am[2 * 4 + 1];
		tmp[2] = m[0 * 4 + c] * am[0 * 4 + 2] + m[1 * 4 + c] * am[1 * 4 + 2] + m[2 * 4 + c] * am[2 * 4 + 2];
		m[0 * 4 + c] = tmp[0];
		m[1 * 4 + c] = tmp[1];
		m[2 * 4 + c] = tmp[2];
	}
}

void rgd3_jointmat_invert(Rgd3JointMat* j)
{
	float* m = j->mat;
	float tmp[3];

	/* negate inverse rotated translation part */
	tmp[0] = m[0 * 4 + 0] * m[0 * 4 + 3] + m[1 * 4 + 0] * m[1 * 4 + 3] + m[2 * 4 + 0] * m[2 * 4 + 3];
	tmp[1] = m[0 * 4 + 1] * m[0 * 4 + 3] + m[1 * 4 + 1] * m[1 * 4 + 3] + m[2 * 4 + 1] * m[2 * 4 + 3];
	tmp[2] = m[0 * 4 + 2] * m[0 * 4 + 3] + m[1 * 4 + 2] * m[1 * 4 + 3] + m[2 * 4 + 2] * m[2 * 4 + 3];
	m[0 * 4 + 3] = -tmp[0];
	m[1 * 4 + 3] = -tmp[1];
	m[2 * 4 + 3] = -tmp[2];

	/* transpose rotation part */
	tmp[0] = m[0 * 4 + 1];
	m[0 * 4 + 1] = m[1 * 4 + 0];
	m[1 * 4 + 0] = tmp[0];
	tmp[1] = m[0 * 4 + 2];
	m[0 * 4 + 2] = m[2 * 4 + 0];
	m[2 * 4 + 0] = tmp[1];
	tmp[2] = m[1 * 4 + 2];
	m[1 * 4 + 2] = m[2 * 4 + 1];
	m[2 * 4 + 1] = tmp[2];
}

void rgd3_jointmat_multiply(const Rgd3JointMat* m1, const Rgd3JointMat* m2, Rgd3JointMat* out)
{
	const float* a = m1->mat;
	const float* b = m2->mat;
	float* r = out->mat;
	int row;

	for (row = 0; row < 3; row++)
	{
		r[row * 4 + 0] = a[row * 4 + 0] * b[0 * 4 + 0] + a[row * 4 + 1] * b[1 * 4 + 0] + a[row * 4 + 2] * b[2 * 4 + 0];
		r[row * 4 + 1] = a[row * 4 + 0] * b[0 * 4 + 1] + a[row * 4 + 1] * b[1 * 4 + 1] + a[row * 4 + 2] * b[2 * 4 + 1];
		r[row * 4 + 2] = a[row * 4 + 0] * b[0 * 4 + 2] + a[row * 4 + 1] * b[1 * 4 + 2] + a[row * 4 + 2] * b[2 * 4 + 2];
		r[row * 4 + 3] = a[row * 4 + 0] * b[0 * 4 + 3] + a[row * 4 + 1] * b[1 * 4 + 3] + a[row * 4 + 2] * b[2 * 4 + 3] +
						 a[row * 4 + 3];
	}
}

void rgd3_jointmat_inverse_multiply(const Rgd3JointMat* m1, const Rgd3JointMat* m2, Rgd3JointMat* out)
{
	const float* a = m1->mat;
	const float* b = m2->mat;
	float* r = out->mat;
	float dst[3];
	int row;

	for (row = 0; row < 3; row++)
	{
		r[row * 4 + 0] = a[row * 4 + 0] * b[0 * 4 + 0] + a[row * 4 + 1] * b[0 * 4 + 1] + a[row * 4 + 2] * b[0 * 4 + 2];
		r[row * 4 + 1] = a[row * 4 + 0] * b[1 * 4 + 0] + a[row * 4 + 1] * b[1 * 4 + 1] + a[row * 4 + 2] * b[1 * 4 + 2];
		r[row * 4 + 2] = a[row * 4 + 0] * b[2 * 4 + 0] + a[row * 4 + 1] * b[2 * 4 + 1] + a[row * 4 + 2] * b[2 * 4 + 2];
	}

	dst[0] = -(b[0 * 4 + 0] * b[0 * 4 + 3] + b[1 * 4 + 0] * b[1 * 4 + 3] + b[2 * 4 + 0] * b[2 * 4 + 3]);
	dst[1] = -(b[0 * 4 + 1] * b[0 * 4 + 3] + b[1 * 4 + 1] * b[1 * 4 + 3] + b[2 * 4 + 1] * b[2 * 4 + 3]);
	dst[2] = -(b[0 * 4 + 2] * b[0 * 4 + 3] + b[1 * 4 + 2] * b[1 * 4 + 3] + b[2 * 4 + 2] * b[2 * 4 + 3]);

	for (row = 0; row < 3; row++)
	{
		r[row * 4 + 3] =
			a[row * 4 + 0] * dst[0] + a[row * 4 + 1] * dst[1] + a[row * 4 + 2] * dst[2] + a[row * 4 + 3];
	}
}

void rgd3_jointmat_to_joint_quat(const Rgd3JointMat* j, Rgd3JointQuat* out)
{
	static const int next[3] = {1, 2, 0};
	const float* m = j->mat;
	Rgd3JointQuat jq;
	float trace, s, t;
	int i, k, n;

	trace = m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2];
	if (trace > 0.0f)
	{
		t = trace + 1.0f;
		s = rgd3_math_inv_sqrt(t) * 0.5f;
		jq.q.v[3] = s * t;
		jq.q.v[0] = (m[1 * 4 + 2] - m[2 * 4 + 1]) * s;
		jq.q.v[1] = (m[2 * 4 + 0] - m[0 * 4 + 2]) * s;
		jq.q.v[2] = (m[0 * 4 + 1] - m[1 * 4 + 0]) * s;
	}
	else
	{
		i = 0;
		if (m[1 * 4 + 1] > m[0 * 4 + 0])
			i = 1;
		if (m[2 * 4 + 2] > m[i * 4 + i])
			i = 2;
		n = next[i];
		k = next[n];
		t = (m[i * 4 + i] - (m[n * 4 + n] + m[k * 4 + k])) + 1.0f;
		s = rgd3_math_inv_sqrt(t) * 0.5f;
		jq.q.v[i] = s * t;
		jq.q.v[3] = (m[n * 4 + k] - m[k * 4 + n]) * s;
		jq.q.v[n] = (m[i * 4 + n] + m[n * 4 + i]) * s;
		jq.q.v[k] = (m[i * 4 + k] + m[k * 4 + i]) * s;
	}
	jq.t.x = m[0 * 4 + 3];
	jq.t.y = m[1 * 4 + 3];
	jq.t.z = m[2 * 4 + 3];
	jq.w = 0.0f;
	*out = jq;
}

void rgd3_jointquat_to_joint_mat(const Rgd3JointQuat* jq, Rgd3JointMat* out)
{
	Rgd3Mat3 m;
	rgd3_quat_to_mat3(&jq->q, &m);
	rgd3_jointmat_set_rotation(out, &m);
	rgd3_jointmat_set_translation(out, jq->t);
}

void rgd3_joints_quats_to_mats(Rgd3JointMat* jointMats, const Rgd3JointQuat* jointQuats, int numJoints)
{
	int i;
	for (i = 0; i < numJoints; i++)
		rgd3_jointquat_to_joint_mat(&jointQuats[i], &jointMats[i]);
}

void rgd3_joints_mats_to_quats(Rgd3JointQuat* jointQuats, const Rgd3JointMat* jointMats, int numJoints)
{
	int i;
	for (i = 0; i < numJoints; i++)
		rgd3_jointmat_to_joint_quat(&jointMats[i], &jointQuats[i]);
}

void rgd3_joints_transform(Rgd3JointMat* jointMats, const int* parents, int firstJoint, int lastJoint)
{
	int i;
	for (i = firstJoint; i <= lastJoint; i++)
		rgd3_jointmat_mul_self(&jointMats[i], &jointMats[parents[i]]);
}

void rgd3_joints_untransform(Rgd3JointMat* jointMats, const int* parents, int firstJoint, int lastJoint)
{
	int i;
	for (i = lastJoint; i >= firstJoint; i--)
		rgd3_jointmat_untransform_self(&jointMats[i], &jointMats[parents[i]]);
}

void rgd3_joints_blend(Rgd3JointQuat* joints, const Rgd3JointQuat* blendJoints, float lerp, const int* index,
					   int numJoints)
{
	int i;
	for (i = 0; i < numJoints; i++)
	{
		int j = index[i];
		joints[j].q = rgd3_quat_slerp(&joints[j].q, &blendJoints[j].q, lerp);
		joints[j].t = rgd3_vec3_lerp(joints[j].t, blendJoints[j].t, lerp);
		joints[j].w = 0.0f;
	}
}

void rgd3_joints_blend_fast(Rgd3JointQuat* joints, const Rgd3JointQuat* blendJoints, float lerp, const int* index,
							int numJoints)
{
	int i;
	for (i = 0; i < numJoints; i++)
	{
		int j = index[i];
		joints[j].q = rgd3_quat_lerp(&joints[j].q, &blendJoints[j].q, lerp);
		joints[j].t = rgd3_vec3_lerp(joints[j].t, blendJoints[j].t, lerp);
		joints[j].w = 0.0f;
	}
}

/* ===================================================================== */
/* DrawVert                                                               */
/* ===================================================================== */

/* one lane of the SSE2 VertexFloatToByte: cvtps_epi32 (round to nearest
   even, 0x80000000 for NaN/out of range) then packs/packus saturation */
static u8 rgd3_vertex_round_byte(float x)
{
	float v = (x + 1.0f) * (255.0f / 2.0f) + 0.5f;
	float r;
	if (!(v >= -2147483648.0f && v < 2147483648.0f))
		return 0;
	r = nearbyintf(v);
	if (r <= 0.0f)
		return 0;
	if (r >= 255.0f)
		return 255;
	return (u8)(int)r;
}

void rgd3_vertex_float_to_byte3(float x, float y, float z, u8* bval)
{
	bval[0] = rgd3_vertex_round_byte(x);
	bval[1] = rgd3_vertex_round_byte(y);
	bval[2] = rgd3_vertex_round_byte(z);
}

void rgd3_drawvert_lerp(Rgd3DrawVert* out, const Rgd3DrawVert* a, const Rgd3DrawVert* b, float f)
{
	Rgd3DrawVert r = *out;
	Rgd3Vec2 sta = rgd3_drawvert_get_texcoord(a);
	Rgd3Vec2 stb = rgd3_drawvert_get_texcoord(b);
	Rgd3Vec2 st = rgd3_vec2_add(sta, rgd3_vec2_scale(rgd3_vec2_sub(stb, sta), f));
	r.xyz = rgd3_vec3_add(a->xyz, rgd3_vec3_scale(rgd3_vec3_sub(b->xyz, a->xyz), f));
	rgd3_drawvert_set_texcoord(&r, st.x, st.y);
	*out = r;
}

void rgd3_drawvert_lerp_all(Rgd3DrawVert* out, const Rgd3DrawVert* a, const Rgd3DrawVert* b, float f)
{
	Rgd3DrawVert r = *out;
	Rgd3Vec2 sta = rgd3_drawvert_get_texcoord(a);
	Rgd3Vec2 stb = rgd3_drawvert_get_texcoord(b);
	Rgd3Vec2 st = rgd3_vec2_add(sta, rgd3_vec2_scale(rgd3_vec2_sub(stb, sta), f));
	Rgd3Vec3 na = rgd3_drawvert_get_normal(a), nb = rgd3_drawvert_get_normal(b);
	Rgd3Vec3 ta = rgd3_drawvert_get_tangent(a), tb = rgd3_drawvert_get_tangent(b);
	Rgd3Vec3 ba = rgd3_drawvert_get_bitangent(a), bb = rgd3_drawvert_get_bitangent(b);
	Rgd3Vec3 normal = rgd3_vec3_add(na, rgd3_vec3_scale(rgd3_vec3_sub(nb, na), f));
	Rgd3Vec3 tangent = rgd3_vec3_add(ta, rgd3_vec3_scale(rgd3_vec3_sub(tb, ta), f));
	Rgd3Vec3 bitangent = rgd3_vec3_add(ba, rgd3_vec3_scale(rgd3_vec3_sub(bb, ba), f));
	int i;

	r.xyz = rgd3_vec3_add(a->xyz, rgd3_vec3_scale(rgd3_vec3_sub(b->xyz, a->xyz), f));
	rgd3_drawvert_set_texcoord(&r, st.x, st.y);

	rgd3_vec3_normalize(&normal);
	rgd3_vec3_normalize(&tangent);
	rgd3_vec3_normalize(&bitangent);
	rgd3_drawvert_set_normal(&r, normal);
	rgd3_drawvert_set_tangent(&r, tangent);
	rgd3_drawvert_set_bitangent(&r, bitangent);

	for (i = 0; i < 4; i++)
	{
		r.color[i] = (u8)(a->color[i] + f * (b->color[i] - a->color[i]));
		r.color2[i] = (u8)(a->color2[i] + f * (b->color2[i] - a->color2[i]));
	}
	*out = r;
}

/* sum of the four weighted joints (color = indices, color2 = weights) */
static void rgd3_drawvert_skin_matrix(const Rgd3DrawVert* vert, const Rgd3JointMat* joints, Rgd3JointMat* accum)
{
	rgd3_jointmat_scale(&joints[vert->color[0]], vert->color2[0] * (1.0f / 255.0f), accum);
	rgd3_jointmat_mad(accum, &joints[vert->color[1]], vert->color2[1] * (1.0f / 255.0f));
	rgd3_jointmat_mad(accum, &joints[vert->color[2]], vert->color2[2] * (1.0f / 255.0f));
	rgd3_jointmat_mad(accum, &joints[vert->color[3]], vert->color2[3] * (1.0f / 255.0f));
}

void rgd3_drawvert_skinned(const Rgd3DrawVert* vert, const Rgd3JointMat* joints, Rgd3DrawVert* out)
{
	Rgd3DrawVert src = *vert;
	Rgd3JointMat accum;
	int i;

	if (joints == NULL)
	{
		*out = src;
		return;
	}
	rgd3_drawvert_skin_matrix(&src, joints, &accum);
	out->xyz = rgd3_jointmat_mul_vec4(&accum, rgd3_vec4(src.xyz.x, src.xyz.y, src.xyz.z, 1.0f));
	rgd3_drawvert_set_texcoord_native(out, src.st[0], src.st[1]);
	rgd3_drawvert_set_normal(out, rgd3_jointmat_rotate(&accum, rgd3_drawvert_get_normal(&src)));
	rgd3_drawvert_set_tangent(out, rgd3_jointmat_rotate(&accum, rgd3_drawvert_get_tangent(&src)));
	/* idlib leaves normal[3] uninitialized; keep the source byte */
	out->normal[3] = src.normal[3];
	out->tangent[3] = src.tangent[3];
	for (i = 0; i < 4; i++)
	{
		out->color[i] = src.color[i];
		out->color2[i] = src.color2[i];
	}
}

Rgd3Vec3 rgd3_drawvert_skinned_position(const Rgd3DrawVert* vert, const Rgd3JointMat* joints)
{
	Rgd3JointMat accum;
	if (joints == NULL)
		return vert->xyz;
	rgd3_drawvert_skin_matrix(vert, joints, &accum);
	return rgd3_jointmat_mul_vec4(&accum, rgd3_vec4(vert->xyz.x, vert->xyz.y, vert->xyz.z, 1.0f));
}

int rgd3_shadowvert_create_cache(Rgd3ShadowVert* vertexCache, const Rgd3DrawVert* verts, int numVerts)
{
	int i;
	for (i = 0; i < numVerts; i++)
	{
		vertexCache[i * 2 + 0].xyzw = rgd3_vec4_from_vec3(verts[i].xyz, 1.0f);
		vertexCache[i * 2 + 1].xyzw = rgd3_vec4_from_vec3(verts[i].xyz, 0.0f);
	}
	return numVerts * 2;
}

int rgd3_shadowvert_skinned_create_cache(Rgd3ShadowVertSkinned* vertexCache, const Rgd3DrawVert* verts,
										 int numVerts)
{
	int i, k;
	for (i = 0; i < numVerts; i++)
	{
		for (k = 0; k < 2; k++)
		{
			Rgd3ShadowVertSkinned* sv = &vertexCache[i * 2 + k];
			sv->xyzw = rgd3_vec4_from_vec3(verts[i].xyz, k == 0 ? 1.0f : 0.0f);
			memcpy(sv->color, verts[i].color, 4);
			memcpy(sv->color2, verts[i].color2, 4);
		}
	}
	return numVerts * 2;
}

/* ===================================================================== */
/* self test                                                              */
/* ===================================================================== */

typedef struct Rgd3MathTest
{
	int checks;
	int failures;
} Rgd3MathTest;

static void rgd3_math_check(Rgd3MathTest* t, int ok, const char* what)
{
	t->checks++;
	if (!ok)
	{
		t->failures++;
		rgd3_printf("math: FAILED %s\n", what);
	}
}

static int rgd3_math_quat_close(Rgd3Quat a, Rgd3Quat b, float eps)
{
	return rgd3_quat_compare_eps(a, b, eps) || rgd3_quat_compare_eps(a, rgd3_quat_neg(b), eps);
}

static void rgd3_math_test_f16(Rgd3MathTest* t)
{
	static const struct
	{
		u32 bits;
		u16 half;
	} to16[] = {
		{0x00000000u, 0x0000}, /* 0 */
		{0x80000000u, 0x0000}, /* -0 loses its sign */
		{0x3f800000u, 0x3C00}, /* 1 */
		{0xbf800000u, 0xBC00}, /* -1 */
		{0x3f000000u, 0x3800}, /* 0.5 */
		{0x3dcccccdu, 0x2E66}, /* 0.1f: mantissa 0x4CCCCD >> 13 = 0x266 */
		{0xc0200000u, 0xC100}, /* -2.5 */
		{0x477fe000u, 0x7BFF}, /* 65504 */
		{0x47800000u, 0x7BFF}, /* 65536 clamps */
		{0x7f800000u, 0x7BFF}, /* +inf clamps */
		{0xff800000u, 0xFBFF}, /* -inf clamps */
		{0x38800000u, 0x0400}, /* 2^-14, smallest normal half */
		{0x38000000u, 0x0000}, /* 2^-15 would be a half denormal: flushed */
		{0x000116c2u, 0x0000}, /* float denormal (1e-40) */
		{0x40490fdbu, 0x4248}, /* pi, truncated */
		{0x449a522bu, 0x64D2}, /* 1234.5678 */
		{0xbeaaaa9fu, 0xB555}, /* -0.333333 */
	};
	static const struct
	{
		u16 half;
		u32 bits;
	} to32[] = {
		{0x3C00, 0x3f800000u}, /* 1 */
		{0x0000, 0x00000000u}, /* +0 */
		{0x8000, 0x80000000u}, /* -0 */
		{0x7BFF, 0x477fe000u}, /* 65504 */
		{0x0400, 0x38800000u}, /* 2^-14 */
		{0x0001, 0x33800000u}, /* half denormal 2^-24 */
		{0x7C00, 0x00000000u}, /* half inf -> +0 (F16toF32 quirk) */
		{0xFC00, 0x80000000u}, /* half -inf -> -0 */
		{0x7E00, 0x38000000u}, /* half NaN -> 2^-15 */
		{0x3555, 0x3eaaa000u}, /* 0.333251953125 */
	};
	int i, ok;
	u32 h;

	for (i = 0; i < (int)(sizeof(to16) / sizeof(to16[0])); i++)
	{
		u16 got = rgd3_f32_to_f16(rgd3_math_bits_float(to16[i].bits));
		rgd3_math_check(t, got == to16[i].half, rgd3_va("f32_to_f16(0x%08x) = 0x%04x, want 0x%04x", to16[i].bits,
														 got, to16[i].half));
	}
	for (i = 0; i < (int)(sizeof(to32) / sizeof(to32[0])); i++)
	{
		u32 got = rgd3_math_float_bits(rgd3_f16_to_f32(to32[i].half));
		rgd3_math_check(t, got == to32[i].bits, rgd3_va("f16_to_f32(0x%04x) = 0x%08x, want 0x%08x", to32[i].half,
														 got, to32[i].bits));
	}

	/* every normal half survives f16 -> f32 -> f16; zero/denormal halves
	   come back as +0 */
	ok = 1;
	for (h = 0; h < 65536u; h++)
	{
		int e = (int)((h >> 10) & 31);
		u16 back = rgd3_f32_to_f16(rgd3_f16_to_f32((u16)h));
		if (e > 0 && e < 31 && back != h)
			ok = 0;
		if (e == 0 && back != 0)
			ok = 0;
	}
	rgd3_math_check(t, ok, "f16 -> f32 -> f16 round trip over all halves");

	/* f32 -> f16 -> f32 within the 10-bit mantissa truncation */
	{
		static const float vals[] = {1.0f, -3.75f, 0.001f, 123.456f, -60000.0f, 0.5f};
		ok = 1;
		for (i = 0; i < (int)(sizeof(vals) / sizeof(vals[0])); i++)
		{
			float back = rgd3_f16_to_f32(rgd3_f32_to_f16(vals[i]));
			if (fabsf(back - vals[i]) > fabsf(vals[i]) * (1.0f / 1024.0f))
				ok = 0;
		}
		rgd3_math_check(t, ok, "f32 -> f16 -> f32 within 2^-10");
	}
}

static void rgd3_math_test_scalar(Rgd3MathTest* t)
{
	float s, c;
	int i, ok;

	rgd3_math_check(t, RGD3_ANGLE2SHORT(90.0f) == 16384, "ANGLE2SHORT(90)");
	rgd3_math_check(t, RGD3_ANGLE2SHORT(-90.0f) == 49152, "ANGLE2SHORT(-90)");
	rgd3_math_check(t, RGD3_SHORT2ANGLE(16384) == 90.0f, "SHORT2ANGLE(16384)");
	rgd3_math_check(t, rgd3_math_angle_normalize180(270.0f) == -90.0f, "AngleNormalize180(270)");
	rgd3_math_check(t, rgd3_math_angle_normalize360(-90.0f) == 270.0f, "AngleNormalize360(-90)");
	rgd3_math_check(t, rgd3_math_angle_delta(10.0f, 350.0f) == 20.0f, "AngleDelta(10, 350)");
	rgd3_math_check(t, rgd3_math_ftoi(-1.5f) == -1 && rgd3_math_ftoi(2.99f) == 2, "Ftoi truncates");
	rgd3_math_check(t, rgd3_math_ftob(300.0f) == 255 && rgd3_math_ftob(-3.0f) == 0 && rgd3_math_ftob(12.9f) == 12,
					"Ftob clamps");
	rgd3_math_check(t, rgd3_math_inv_sqrt(4.0f) == 0.5f && rgd3_math_inv_sqrt(0.0f) == RGD3_MATH_INFINITY,
					"InvSqrt");
	rgd3_math_check(t, rgd3_math_acos(1.5f) == 0.0f && rgd3_math_acos(-2.0f) == RGD3_MATH_PI, "ACos clamps");
	rgd3_math_check(t, rgd3_math_clamp_float(0.0f, 1.0f, 2.0f) == 1.0f && rgd3_math_clamp_int(0, 10, -5) == 0,
					"Clamp");
	rgd3_math_check(t, rgd3_math_ceil_power_of_two(33) == 64 && rgd3_math_floor_power_of_two(33) == 32 &&
						   rgd3_math_bit_count(0xff) == 8 && rgd3_math_is_power_of_two(64),
					"power of two helpers");
	rgd3_math_check(t, rgd3_math_bits_for_integer(255) == 8 && rgd3_math_ilog2(1024.0f) == 10, "ILog2");

	ok = 1;
	for (i = -20; i <= 20; i++)
	{
		float a = i * 0.37f;
		rgd3_math_sincos16(a, &s, &c);
		if (fabsf(s - sinf(a)) > 1e-5f || fabsf(c - cosf(a)) > 1e-5f || fabsf(rgd3_math_sin16(a) - s) > 0.0f ||
			fabsf(rgd3_math_atan16(a) - atanf(a)) > 1e-5f || fabsf(rgd3_math_atan2_16(a, 1.3f) - atan2f(a, 1.3f)) > 1e-5f)
			ok = 0;
	}
	rgd3_math_check(t, ok, "Sin16/Cos16/ATan16 against libm");
	rgd3_math_check(t, fabsf(rgd3_math_exp16(1.0f) - RGD3_MATH_E) < 1e-4f &&
						   fabsf(rgd3_math_log16(RGD3_MATH_E) - 1.0f) < 1e-4f &&
						   fabsf(rgd3_math_pow16(2.0f, 10.0f) - 1024.0f) < 0.1f,
					"Exp16/Log16/Pow16");
	{
		/* 6-bit exponent, 16-bit mantissa: exact for values that fit */
		static const float vals[] = {1.5f, -0.75f, 1024.0f, 0.0078125f};
		ok = 1;
		for (i = 0; i < (int)(sizeof(vals) / sizeof(vals[0])); i++)
		{
			if (rgd3_math_bits_to_float(rgd3_math_float_to_bits(vals[i], 6, 16), 6, 16) != vals[i])
				ok = 0;
		}
		rgd3_math_check(t, ok, "FloatToBits/BitsToFloat round trip");
	}
}

static void rgd3_math_test_vec_mat(Rgd3MathTest* t)
{
	static const float angleSet[][3] = {
		{0, 0, 0}, {10, 20, 30}, {-45, 135, -60}, {80, -170, 170}, {-89, 90, 0}, {30, 0, -120}, {0, 270, 45},
	};
	Rgd3Mat3 m, inv, prod, id, back;
	Rgd3Mat4 m4, inv4, prod4, id4;
	Rgd3Vec3 v;
	int i, ok;

	rgd3_mat3_identity(&id);
	rgd3_mat4_identity(&id4);

	/* zero vector normalize keeps it zero and returns 0 */
	v = rgd3_vec3_zero();
	rgd3_math_check(t, rgd3_vec3_normalize(&v) == 0.0f && rgd3_vec3_compare(v, rgd3_vec3_zero()),
					"Vec3 normalize of zero");
	v = rgd3_vec3(3.0f, 0.0f, 4.0f);
	rgd3_math_check(t, fabsf(rgd3_vec3_normalize(&v) - 5.0f) < 1e-5f && rgd3_vec3_compare_eps(v, rgd3_vec3(0.6f, 0, 0.8f), 1e-6f),
					"Vec3 normalize returns length");
	v = rgd3_vec3(0.0f, 0.0f, 0.5f);
	rgd3_math_check(t, rgd3_vec3_fix_degenerate_normal(&v) && v.z == 1.0f, "Vec3 FixDegenerateNormal");
	/* M_RAD2DEG is 180.0f / PI in float (57.2957764), so +y gives
	   89.9999924 in idlib as well */
	v = rgd3_vec3(0.0f, 1.0f, 0.0f);
	rgd3_math_check(t, fabsf(rgd3_vec3_to_yaw(&v) - 90.0f) < 1e-4f, "Vec3 ToYaw");
	v = rgd3_vec3(0.0f, -1.0f, 0.0f);
	rgd3_math_check(t, fabsf(rgd3_vec3_to_yaw(&v) - 270.0f) < 1e-4f, "Vec3 ToYaw wraps to [0, 360)");
	v = rgd3_vec3(1.0f, 0.0f, 1.0f);
	{
		Rgd3Angles a = rgd3_vec3_to_angles(&v);
		rgd3_math_check(t, fabsf(a.pitch + 45.0f) < 1e-4f && a.yaw == 0.0f && a.roll == 0.0f, "Vec3 ToAngles");
	}
	{
		Rgd3Vec3 left, down, up, fwd = rgd3_vec3_normalized(rgd3_vec3(1.0f, 2.0f, 3.0f));
		rgd3_vec3_normal_vectors(fwd, &left, &down);
		rgd3_math_check(t, fabsf(rgd3_vec3_dot(fwd, left)) < 1e-6f && fabsf(rgd3_vec3_dot(fwd, down)) < 1e-6f &&
							   fabsf(rgd3_vec3_dot(left, down)) < 1e-6f,
						"Vec3 NormalVectors");
		/* |z| > 0.7: idlib's left is only perpendicular to up */
		rgd3_vec3_ortho_basis(fwd, &left, &up);
		rgd3_math_check(t, fabsf(rgd3_vec3_dot(fwd, up)) < 1e-6f && fabsf(rgd3_vec3_dot(left, up)) < 1e-6f &&
							   fabsf(rgd3_vec3_length(up) - 1.0f) < 1e-6f,
						"Vec3 OrthogonalBasis (|z| > 0.7)");
		fwd = rgd3_vec3_normalized(rgd3_vec3(3.0f, 2.0f, 1.0f));
		rgd3_vec3_ortho_basis(fwd, &left, &up);
		rgd3_math_check(t, fabsf(rgd3_vec3_dot(fwd, left)) < 1e-6f && fabsf(rgd3_vec3_dot(fwd, up)) < 1e-6f &&
							   fabsf(rgd3_vec3_dot(left, up)) < 1e-6f,
						"Vec3 OrthogonalBasis (|z| <= 0.7)");
		rgd3_vec3_to_mat3(&fwd, &m);
		rgd3_math_check(t, fabsf(rgd3_mat3_determinant(&m) - 1.0f) < 1e-5f, "Vec3 ToMat3 is a rotation");
	}

	/* general matrix times its inverse */
	rgd3_mat3_set(&m, 2.0f, 1.0f, 0.0f, 0.5f, 3.0f, 1.0f, 1.0f, -1.0f, 4.0f);
	rgd3_math_check(t, rgd3_mat3_inverse(&m, &inv), "Mat3 inverse succeeds");
	rgd3_mat3_mul(&m, &inv, &prod);
	rgd3_math_check(t, rgd3_mat3_compare_eps(&prod, &id, 1e-5f), "Mat3 * inverse == identity");
	rgd3_mat3_mul(&inv, &m, &prod);
	rgd3_math_check(t, rgd3_mat3_compare_eps(&prod, &id, 1e-5f), "inverse * Mat3 == identity");
	rgd3_math_check(t, rgd3_mat3_inverse_fast(&m, &inv) && (rgd3_mat3_mul(&m, &inv, &prod), 1) &&
						   rgd3_mat3_compare_eps(&prod, &id, 1e-5f),
					"Mat3 inverse_fast");
	rgd3_mat3_zero(&back);
	rgd3_math_check(t, !rgd3_mat3_inverse(&back, &inv), "singular Mat3 inverse fails");

	/* mul_vec vs transpose_mul_vec on a rotation */
	{
		Rgd3Angles a = rgd3_angles(10.0f, 20.0f, 30.0f);
		Rgd3Vec3 p = rgd3_vec3(1.0f, 2.0f, 3.0f), w, l;
		rgd3_angles_to_mat3(&a, &m);
		w = rgd3_mat3_mul_vec(&m, p);
		l = rgd3_mat3_transpose_mul_vec(&m, w);
		rgd3_math_check(t, rgd3_vec3_compare_eps(l, p, 1e-5f), "Mat3 transpose_mul_vec undoes mul_vec");
		rgd3_math_check(t, rgd3_vec3_compare(rgd3_mat3_unproject_vector(&m, p), w), "Mat3 unproject == mul_vec");
		rgd3_mat3_transpose(&m, &inv);
		rgd3_mat3_mul(&m, &inv, &prod);
		rgd3_math_check(t, rgd3_mat3_compare_eps(&prod, &id, 1e-6f), "rotation * transpose == identity");
	}

	/* Mat4 inverses */
	m4.mat[0] = rgd3_vec4(2.0f, 0.5f, 0.0f, 3.0f);
	m4.mat[1] = rgd3_vec4(0.0f, 1.5f, -1.0f, 1.0f);
	m4.mat[2] = rgd3_vec4(1.0f, 0.0f, 2.5f, -2.0f);
	m4.mat[3] = rgd3_vec4(0.25f, 0.0f, 0.5f, 1.0f);
	ok = rgd3_mat4_inverse(&m4, &inv4);
	rgd3_mat4_mul(&m4, &inv4, &prod4);
	rgd3_math_check(t, ok && rgd3_mat4_compare_eps(&prod4, &id4, 1e-5f), "Mat4 * inverse == identity");
	ok = rgd3_mat4_inverse_fast(&m4, &inv4);
	rgd3_mat4_mul(&m4, &inv4, &prod4);
	rgd3_math_check(t, ok && rgd3_mat4_compare_eps(&prod4, &id4, 1e-5f), "Mat4 * inverse_fast == identity");
	rgd3_math_check(t, fabsf(rgd3_mat4_determinant(&m4) - (float)(1.0 / rgd3_mat4_determinant(&inv4))) < 1e-3f,
					"Mat4 determinant of inverse");

	/* angles -> mat3 -> angles, mat3 -> quat -> mat3, angles -> quat */
	for (i = 0; i < (int)(sizeof(angleSet) / sizeof(angleSet[0])); i++)
	{
		Rgd3Angles a = rgd3_angles(angleSet[i][0], angleSet[i][1], angleSet[i][2]);
		Rgd3Angles norm = a, back_a;
		Rgd3Quat q, q2;
		Rgd3Rotation rot;
		Rgd3Vec3 p = rgd3_vec3(3.0f, -1.0f, 2.0f), r1, r2;

		rgd3_angles_to_mat3(&a, &m);
		back_a = rgd3_mat3_to_angles(&m);
		rgd3_angles_normalize180(&norm);
		rgd3_angles_normalize180(&back_a);
		rgd3_math_check(t, rgd3_angles_compare_eps(norm, back_a, 1e-3f),
						rgd3_va("angles (%g %g %g) -> mat3 -> angles", a.pitch, a.yaw, a.roll));

		q = rgd3_mat3_to_quat(&m);
		rgd3_quat_to_mat3(&q, &back);
		rgd3_math_check(t, rgd3_mat3_compare_eps(&m, &back, 1e-5f),
						rgd3_va("mat3 -> quat -> mat3 (%g %g %g)", a.pitch, a.yaw, a.roll));

		q2 = rgd3_angles_to_quat(&a);
		rgd3_math_check(t, rgd3_math_quat_close(q, q2, 1e-5f),
						rgd3_va("angles -> quat == mat3 -> quat (%g %g %g)", a.pitch, a.yaw, a.roll));

		/* quat rotate == matrix rotate == rotation rotate */
		r1 = rgd3_mat3_mul_vec(&m, p);
		r2 = rgd3_quat_rotate(q, p);
		rgd3_math_check(t, rgd3_vec3_compare_eps(r1, r2, 1e-4f), "quat rotate == mat3 mul_vec");
		rgd3_angles_to_rotation(&a, &rot);
		rgd3_math_check(t, rgd3_mat3_compare_eps(rgd3_rotation_to_mat3(&rot), &m, 1e-4f),
						rgd3_va("angles -> rotation -> mat3 (%g %g %g)", a.pitch, a.yaw, a.roll));
		r2 = rgd3_rotation_rotate_point(&rot, &p);
		rgd3_math_check(t, rgd3_vec3_compare_eps(r1, r2, 1e-4f), "rotation rotate_point");
		rgd3_mat3_to_rotation(&m, &rot);
		rot.axisValid = 0;
		rgd3_math_check(t, rgd3_mat3_compare_eps(rgd3_rotation_to_mat3(&rot), &m, 1e-4f),
						"mat3 -> rotation -> mat3");
		rgd3_math_check(t, rgd3_math_quat_close(rgd3_rotation_to_quat(&rot), q, 1e-4f), "rotation -> quat");
	}

	/* quat -> mat3 -> quat, including the trace <= 0 branches */
	{
		static const float quats[][4] = {
			{0.1f, 0.2f, 0.3f, 0.9f}, {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f},
			{0.6f, -0.7f, 0.1f, 0.2f}, {-0.3f, 0.2f, 0.9f, -0.1f},
		};
		for (i = 0; i < (int)(sizeof(quats) / sizeof(quats[0])); i++)
		{
			Rgd3Quat q = rgd3_quat_normalize(rgd3_quat(quats[i][0], quats[i][1], quats[i][2], quats[i][3]));
			Rgd3Quat q2;
			Rgd3CQuat cq;
			rgd3_quat_to_mat3(&q, &m);
			q2 = rgd3_mat3_to_quat(&m);
			rgd3_math_check(t, rgd3_math_quat_close(q, q2, 1e-5f), rgd3_va("quat -> mat3 -> quat #%d", i));
			cq = rgd3_quat_to_cquat(q);
			rgd3_math_check(t, rgd3_math_quat_close(rgd3_cquat_to_quat(cq), q, 1e-5f),
							rgd3_va("quat -> cquat -> quat #%d", i));
		}
	}

	/* slerp: identity -> 90 degrees about z at t = 0.5 is 45 degrees */
	{
		Rgd3Angles a90 = rgd3_angles(0.0f, 90.0f, 0.0f), a45 = rgd3_angles(0.0f, 45.0f, 0.0f);
		Rgd3Quat from = rgd3_quat_identity(), to = rgd3_angles_to_quat(&a90);
		Rgd3Quat mid = rgd3_quat_slerp(&from, &to, 0.5f);
		Rgd3Quat expect = rgd3_angles_to_quat(&a45);
		rgd3_math_check(t, rgd3_math_quat_close(mid, expect, 1e-5f), "quat slerp halfway");
		mid = rgd3_quat_lerp(&from, &to, 0.5f);
		rgd3_math_check(t, rgd3_math_quat_close(mid, expect, 1e-5f), "quat nlerp halfway");
		rgd3_math_check(t, rgd3_quat_compare(rgd3_quat_slerp(&from, &to, 0.0f), from) &&
							   rgd3_quat_compare(rgd3_quat_slerp(&from, &to, 1.0f), to),
						"quat slerp endpoints");
	}

	/* ortho normalize keeps a rotation */
	{
		Rgd3Angles a = rgd3_angles(20.0f, 40.0f, 60.0f);
		rgd3_angles_to_mat3(&a, &m);
		m.mat[0] = rgd3_vec3_scale(m.mat[0], 1.01f);
		rgd3_mat3_ortho_normalize(&m, &back);
		rgd3_math_check(t, fabsf(rgd3_mat3_determinant(&back) - 1.0f) < 1e-5f, "Mat3 OrthoNormalize");
	}
}

static void rgd3_math_test_joints(Rgd3MathTest* t)
{
	static const int parents[3] = {-1, 0, 1};
	Rgd3JointQuat jq[3], back[3];
	Rgd3JointMat jm[3], orig[3], inv, prod, id;
	int i, ok;

	for (i = 0; i < 3; i++)
	{
		Rgd3Angles a = rgd3_angles(10.0f + i * 50.0f, -30.0f + i * 100.0f, 5.0f + i * 70.0f);
		jq[i].q = rgd3_angles_to_quat(&a);
		jq[i].t = rgd3_vec3(1.0f + i, -2.0f * i, 0.5f);
		jq[i].w = 0.0f;
	}
	rgd3_joints_quats_to_mats(jm, jq, 3);
	rgd3_joints_mats_to_quats(back, jm, 3);
	ok = 1;
	for (i = 0; i < 3; i++)
	{
		if (!rgd3_math_quat_close(jq[i].q, back[i].q, 1e-5f) || !rgd3_vec3_compare(jq[i].t, back[i].t) ||
			back[i].w != 0.0f)
			ok = 0;
	}
	rgd3_math_check(t, ok, "joint quat -> joint mat -> joint quat");

	/* joint mat rotation is the transposed mat3 */
	{
		Rgd3Mat3 m, m2;
		rgd3_quat_to_mat3(&jq[1].q, &m);
		rgd3_jointmat_to_mat3(&jm[1], &m2);
		rgd3_math_check(t, rgd3_mat3_compare(&m, &m2) && jm[1].mat[0 * 4 + 1] == m.mat[1].x,
						"jointmat stores the transposed mat3");
	}

	/* transform then untransform the hierarchy */
	for (i = 0; i < 3; i++)
		orig[i] = jm[i];
	rgd3_joints_transform(jm, parents, 1, 2);
	{
		Rgd3JointMat expect;
		rgd3_jointmat_multiply(&orig[0], &orig[1], &expect); /* m *= parent is parent * m */
		rgd3_math_check(t, rgd3_jointmat_compare_eps(&jm[1], &expect, 1e-5f), "TransformJoints == Multiply");
	}
	rgd3_joints_untransform(jm, parents, 1, 2);
	ok = 1;
	for (i = 0; i < 3; i++)
	{
		if (!rgd3_jointmat_compare_eps(&jm[i], &orig[i], 1e-5f))
			ok = 0;
	}
	rgd3_math_check(t, ok, "TransformJoints / UntransformJoints round trip");

	/* invert and inverse multiply */
	inv = orig[2];
	rgd3_jointmat_invert(&inv);
	rgd3_jointmat_multiply(&orig[2], &inv, &prod);
	rgd3_jointmat_identity(&id);
	rgd3_math_check(t, rgd3_jointmat_compare_eps(&prod, &id, 1e-5f), "JointMat * Invert == identity");
	rgd3_jointmat_inverse_multiply(&orig[2], &orig[2], &prod);
	rgd3_math_check(t, rgd3_jointmat_compare_eps(&prod, &id, 1e-5f), "InverseMultiply( m, m ) == identity");

	/* transform point consistency with the quaternion */
	{
		Rgd3Vec3 p = rgd3_vec3(0.3f, 0.7f, -1.1f);
		Rgd3Vec3 a = rgd3_jointmat_transform(&orig[1], p);
		Rgd3Vec3 b = rgd3_vec3_add(rgd3_quat_rotate(jq[1].q, p), jq[1].t);
		rgd3_math_check(t, rgd3_vec3_compare_eps(a, b, 1e-5f), "jointmat transform == quat rotate + t");
	}

	/* blending */
	{
		static const int index[1] = {0};
		Rgd3JointQuat a = jq[0], b = jq[1];
		rgd3_joints_blend(&a, &b, 1.0f, index, 1);
		rgd3_math_check(t, rgd3_quat_compare(a.q, b.q) && rgd3_vec3_compare(a.t, b.t), "BlendJoints at 1");
	}
}

static void rgd3_math_test_geometry(Rgd3MathTest* t)
{
	Rgd3Plane p, x1;
	Rgd3Bounds b, b2;
	Rgd3Vec3 pts[8], start, dir;
	float scale, s1, s2;
	int i, ok;

	/* plane through z = 2, normal +z */
	rgd3_math_check(t, rgd3_plane_from_points(&p, rgd3_vec3(0, 0, 2), rgd3_vec3(0, 1, 2), rgd3_vec3(1, 0, 2), 1),
					"plane from points");
	rgd3_math_check(t, rgd3_plane_compare(p, rgd3_plane(0, 0, 1, -2)), "plane normal/d sign convention");
	rgd3_math_check(t, rgd3_plane_dist(p) == 2.0f, "plane dist() == -d");
	rgd3_math_check(t, rgd3_plane_distance(p, rgd3_vec3(5, 5, 5)) == 3.0f, "plane distance");
	rgd3_math_check(t, rgd3_plane_side(p, rgd3_vec3(0, 0, 5), 0.0f) == RGD3_PLANESIDE_FRONT, "plane side front");
	rgd3_math_check(t, rgd3_plane_side(p, rgd3_vec3(0, 0, -5), 0.0f) == RGD3_PLANESIDE_BACK, "plane side back");
	rgd3_math_check(t, rgd3_plane_side(p, rgd3_vec3(3, 3, 2.05f), 0.1f) == RGD3_PLANESIDE_ON, "plane side on");
	rgd3_math_check(t, rgd3_plane_type(&p) == RGD3_PLANETYPE_Z, "plane type Z");
	rgd3_math_check(t, rgd3_plane_from_points(&x1, rgd3_vec3(0, 0, 0), rgd3_vec3(1, 0, 0), rgd3_vec3(0, 1, 0), 1) &&
						   rgd3_plane_type(&x1) == RGD3_PLANETYPE_NEGZ,
					"plane winding flips the normal");
	rgd3_math_check(t, !rgd3_plane_from_points(&x1, rgd3_vec3(0, 0, 0), rgd3_vec3(1, 1, 1), rgd3_vec3(2, 2, 2), 1),
					"degenerate plane fails");
	rgd3_math_check(t, rgd3_plane_line_intersection(p, rgd3_vec3(0, 0, 0), rgd3_vec3(0, 0, 5)) &&
						   !rgd3_plane_line_intersection(p, rgd3_vec3(0, 0, 3), rgd3_vec3(0, 0, 5)),
					"plane line intersection");
	rgd3_math_check(t, rgd3_plane_ray_intersection(p, rgd3_vec3(0, 0, 0), rgd3_vec3(0, 0, 1), &scale) &&
						   scale == 2.0f,
					"plane ray intersection");
	x1 = rgd3_plane_from_normal_dist(rgd3_vec3(1, 0, 0), 1.0f);
	rgd3_math_check(t, rgd3_plane_plane_intersection(&p, &x1, &start, &dir) &&
						   rgd3_vec3_compare_eps(start, rgd3_vec3(1, 0, 2), 1e-6f) && fabsf(fabsf(dir.y) - 1.0f) < 1e-6f,
					"plane plane intersection");
	{
		Rgd3Plane tr = rgd3_plane_translate(p, rgd3_vec3(0, 0, 3));
		rgd3_math_check(t, rgd3_plane_dist(tr) == 5.0f, "plane translate");
	}

	/* bounds */
	rgd3_bounds_clear(&b);
	rgd3_math_check(t, rgd3_bounds_is_cleared(&b), "bounds clear is inside out");
	rgd3_math_check(t, rgd3_bounds_add_point(&b, rgd3_vec3(1, 2, 3)), "bounds add first point");
	rgd3_math_check(t, rgd3_bounds_add_point(&b, rgd3_vec3(-1, 0, 5)), "bounds add second point");
	rgd3_math_check(t, !rgd3_bounds_add_point(&b, rgd3_vec3(0, 1, 4)), "bounds add inside point");
	rgd3_math_check(t, rgd3_vec3_compare(b.b[0], rgd3_vec3(-1, 0, 3)) && rgd3_vec3_compare(b.b[1], rgd3_vec3(1, 2, 5)),
					"bounds mins/maxs");
	rgd3_math_check(t, rgd3_bounds_volume(&b) == 8.0f, "bounds volume");
	rgd3_math_check(t, rgd3_vec3_compare(rgd3_bounds_get_center(&b), rgd3_vec3(0, 1, 4)), "bounds center");
	rgd3_bounds_expand(&b, 1.0f, &b2);
	rgd3_math_check(t, rgd3_vec3_compare(b2.b[0], rgd3_vec3(-2, -1, 2)) && rgd3_vec3_compare(b2.b[1], rgd3_vec3(2, 3, 6)),
					"bounds expand");
	rgd3_math_check(t, rgd3_bounds_contains_point(&b, rgd3_vec3(1, 2, 5)) &&
						   !rgd3_bounds_contains_point(&b, rgd3_vec3(1.01f, 2, 5)),
					"bounds contains point (touching counts)");
	rgd3_bounds_set(&b2, rgd3_vec3(1, 2, 5), rgd3_vec3(3, 3, 6));
	rgd3_math_check(t, rgd3_bounds_intersects_bounds(&b, &b2), "bounds intersects (touching)");
	rgd3_bounds_set(&b2, rgd3_vec3(1.5f, 2, 5), rgd3_vec3(3, 3, 6));
	rgd3_math_check(t, !rgd3_bounds_intersects_bounds(&b, &b2), "bounds disjoint");
	start = rgd3_vec3(-5, 1, 4);
	dir = rgd3_vec3(5, 1, 4);
	rgd3_math_check(t, rgd3_bounds_line_intersection(&b, &start, &dir), "bounds line hit");
	dir = rgd3_vec3(-5, 1, 10);
	rgd3_math_check(t, !rgd3_bounds_line_intersection(&b, &start, &dir), "bounds line miss");
	dir = rgd3_vec3(1, 0, 0);
	rgd3_math_check(t, rgd3_bounds_ray_intersection(&b, &start, &dir, &scale) && scale == 4.0f,
					"bounds ray intersection scale");
	rgd3_math_check(t, rgd3_bounds_plane_side(&b, &p, RGD3_ON_EPSILON) == RGD3_PLANESIDE_FRONT, "bounds plane side");
	rgd3_math_check(t, rgd3_bounds_plane_distance(&b, &p) == 1.0f, "bounds plane distance");
	rgd3_bounds_to_points(&b, pts);
	rgd3_math_check(t, rgd3_vec3_compare(pts[0], b.b[0]) && rgd3_vec3_compare(pts[6], b.b[1]) &&
						   rgd3_vec3_compare(pts[1], rgd3_vec3(1, 0, 3)) && rgd3_vec3_compare(pts[3], rgd3_vec3(-1, 2, 3)) &&
						   rgd3_vec3_compare(pts[4], rgd3_vec3(-1, 0, 5)),
					"bounds ToPoints order");
	{
		Rgd3Bounds fp;
		rgd3_bounds_from_points(&fp, pts, 8);
		rgd3_math_check(t, rgd3_bounds_compare(&fp, &b), "bounds FromPoints");
	}
	{
		/* 90 degree yaw swaps x and y extents */
		Rgd3Angles a = rgd3_angles(0, 90, 0);
		Rgd3Mat3 axis;
		Rgd3Vec3 origin = rgd3_vec3(10, 0, 0);
		Rgd3Bounds tb, sq;
		rgd3_angles_to_mat3(&a, &axis);
		rgd3_bounds_set(&sq, rgd3_vec3(-1, -2, -3), rgd3_vec3(1, 2, 3));
		rgd3_bounds_from_transformed(&tb, &sq, &origin, &axis);
		rgd3_math_check(t, rgd3_bounds_compare_eps(&tb, &(Rgd3Bounds){{{{8, -1, -3}}, {{12, 1, 3}}}}, 1e-5f),
						"bounds FromTransformedBounds");
		rgd3_math_check(t, fabsf(rgd3_bounds_radius(&sq) - sqrtf(14.0f)) < 1e-6f, "bounds radius");
	}

	/* sphere */
	{
		Rgd3Sphere s;
		Rgd3Vec3 sp[2] = {{{-1, 0, 0}}, {{3, 0, 0}}};
		rgd3_sphere_from_points(&s, sp, 2);
		rgd3_math_check(t, rgd3_vec3_compare(s.origin, rgd3_vec3(1, 0, 0)) && s.radius == 2.0f, "sphere FromPoints");
		rgd3_math_check(t, rgd3_sphere_contains_point(&s, rgd3_vec3(1, 2, 0)) &&
							   !rgd3_sphere_contains_point(&s, rgd3_vec3(1, 2.1f, 0)),
						"sphere contains point");
		start = rgd3_vec3(-5, 0, 0);
		dir = rgd3_vec3(1, 0, 0);
		rgd3_math_check(t, rgd3_sphere_ray_intersection(&s, &start, &dir, &s1, &s2) && s1 == 8.0f && s2 == 4.0f,
						"sphere ray intersection");
		rgd3_math_check(t, rgd3_sphere_plane_side(&s, &p, 0.0f) == RGD3_PLANESIDE_CROSS, "sphere touching plane is CROSS");
		rgd3_math_check(t, rgd3_sphere_plane_side(&s, &p, -0.5f) == RGD3_PLANESIDE_BACK, "sphere plane side back");
		rgd3_sphere_clear(&s);
		rgd3_sphere_add_point(&s, rgd3_vec3(0, 0, 0));
		rgd3_sphere_add_point(&s, rgd3_vec3(4, 0, 0));
		rgd3_math_check(t, rgd3_vec3_compare(s.origin, rgd3_vec3(2, 0, 0)) && s.radius == 2.0f, "sphere AddPoint");
	}

	/* box */
	{
		Rgd3Box box;
		Rgd3Vec3 bp[8];
		Rgd3Angles a = rgd3_angles(0, 45, 0);
		Rgd3Mat3 axis;
		rgd3_box_from_bounds(&box, &b);
		rgd3_box_to_points(&box, bp);
		ok = 1;
		for (i = 0; i < 8; i++)
		{
			if (!rgd3_vec3_compare_eps(bp[i], pts[i], 1e-6f))
				ok = 0;
		}
		rgd3_math_check(t, ok, "box ToPoints matches bounds ToPoints");
		rgd3_math_check(t, rgd3_box_plane_side(&box, &p, RGD3_ON_EPSILON) == RGD3_PLANESIDE_FRONT, "box plane side");
		rgd3_angles_to_mat3(&a, &axis);
		rgd3_box_rotate_self(&box, &axis);
		rgd3_math_check(t, rgd3_box_contains_point(&box, rgd3_mat3_mul_vec(&axis, rgd3_vec3(0.9f, 1.9f, 4.9f))),
						"rotated box contains rotated point");
		{
			Rgd3Box other;
			rgd3_box_from_bounds(&other, &b);
			rgd3_math_check(t, rgd3_box_intersects_box(&box, &other), "box intersects box");
			rgd3_box_translate_self(&other, rgd3_vec3(10, 0, 0));
			rgd3_math_check(t, !rgd3_box_intersects_box(&box, &other), "box disjoint");
		}
		start = rgd3_vec3(-10, 0, 0);
		dir = rgd3_vec3(1, 0, 0);
		rgd3_box_from_bounds(&box, &(Rgd3Bounds){{{{-1, -1, -1}}, {{1, 1, 1}}}});
		rgd3_math_check(t, rgd3_box_ray_intersection(&box, &start, &dir, &s1, &s2) && s1 == 9.0f && s2 == 11.0f,
						"box ray intersection");
	}

	/* pluecker: two crossing lines have a zero permuted inner product */
	{
		Rgd3Pluecker l1, l2;
		Rgd3Vec3 a, e;
		rgd3_pluecker_from_line(&l1, rgd3_vec3(-1, 0, 0), rgd3_vec3(1, 0, 0));
		rgd3_pluecker_from_line(&l2, rgd3_vec3(0, -1, 0), rgd3_vec3(0, 1, 0));
		rgd3_math_check(t, rgd3_pluecker_permuted_inner_product(&l1, &l2) == 0.0f, "pluecker crossing lines");
		rgd3_pluecker_from_line(&l2, rgd3_vec3(0, -1, 1), rgd3_vec3(0, 1, 1));
		rgd3_math_check(t, rgd3_pluecker_permuted_inner_product(&l1, &l2) != 0.0f &&
							   fabsf(rgd3_pluecker_distance3d_sqr(&l1, &l2) - 1.0f) < 1e-6f,
						"pluecker skew lines distance");
		rgd3_math_check(t, rgd3_pluecker_to_line(&l1, &a, &e) && fabsf(a.y) < 1e-6f && fabsf(a.z) < 1e-6f &&
							   fabsf(e.y) < 1e-6f && fabsf(e.z) < 1e-6f,
						"pluecker to line");
	}
}

static void rgd3_math_test_drawvert(Rgd3MathTest* t)
{
	static const float normals[][3] = {
		{0, 0, 1}, {1, 0, 0}, {0, -1, 0}, {0.6f, 0.0f, 0.8f}, {0.267261f, 0.534522f, 0.801784f}, {-0.5f, 0.5f, -0.707107f},
	};
	Rgd3DrawVert dv, dv2, mid;
	Rgd3Vec2 st;
	int i, ok;

	rgd3_drawvert_clear(&dv);
	rgd3_math_check(t, dv.normal[0] == 128 && dv.normal[1] == 128 && dv.normal[2] == 255 && dv.tangent[0] == 255 &&
						   dv.tangent[3] == 255,
					"DrawVert Clear bytes");

	/* SSE2 rounding: 0 -> 128, 1 -> 255, -1 -> 0, 0.5 -> 192, -0.5 -> 64 */
	rgd3_drawvert_set_normal(&dv, rgd3_vec3(0.5f, -0.5f, 0.0f));
	rgd3_math_check(t, dv.normal[0] == 192 && dv.normal[1] == 64 && dv.normal[2] == 128, "VertexFloatToByte rounding");
	rgd3_drawvert_set_normal(&dv, rgd3_vec3(1.0f, -1.0f, 0.0f));
	rgd3_math_check(t, dv.normal[0] == 255 && dv.normal[1] == 0 && dv.normal[2] == 128, "VertexFloatToByte range");
	rgd3_math_check(t, rgd3_vertex_float_to_byte(0.5f) == 191 && rgd3_vertex_float_to_byte(0.0f) == 128,
					"VERTEX_FLOAT_TO_BYTE truncates");

	ok = 1;
	for (i = 0; i < 256; i++)
	{
		if (rgd3_vertex_float_to_byte(rgd3_vertex_byte_to_float((u8)i)) != i)
			ok = 0;
	}
	rgd3_math_check(t, ok, "VERTEX_BYTE_TO_FLOAT -> VERTEX_FLOAT_TO_BYTE identity");

	ok = 1;
	for (i = 0; i < (int)(sizeof(normals) / sizeof(normals[0])); i++)
	{
		Rgd3Vec3 n = rgd3_vec3(normals[i][0], normals[i][1], normals[i][2]);
		rgd3_drawvert_set_normal(&dv, n);
		rgd3_drawvert_set_tangent(&dv, n);
		if (!rgd3_vec3_compare_eps(rgd3_drawvert_get_normal(&dv), n, 0.01f) ||
			!rgd3_vec3_compare_eps(rgd3_drawvert_get_tangent_raw(&dv), n, 0.01f))
			ok = 0;
	}
	rgd3_math_check(t, ok, "DrawVert normal/tangent byte round trip");

	rgd3_drawvert_clear(&dv);
	rgd3_drawvert_set_bitangent_sign(&dv, -1.0f);
	rgd3_math_check(t, rgd3_drawvert_get_bitangent_sign(&dv) == -1.0f && rgd3_drawvert_get_bitangent_sign_bit(&dv) == 1,
					"DrawVert bitangent sign");
	rgd3_drawvert_set_bitangent(&dv, rgd3_vec3(0, 1, 0));
	rgd3_math_check(t, rgd3_drawvert_get_bitangent_sign(&dv) == 1.0f &&
						   rgd3_vec3_compare_eps(rgd3_drawvert_get_bitangent(&dv), rgd3_vec3(0, 1, 0), 0.01f),
					"DrawVert SetBiTangent / GetBiTangent");

	rgd3_drawvert_set_texcoord(&dv, 0.5f, -0.25f);
	st = rgd3_drawvert_get_texcoord(&dv);
	rgd3_math_check(t, dv.st[0] == 0x3800 && st.x == 0.5f && st.y == -0.25f, "DrawVert texcoord half floats");

	rgd3_drawvert_set_color(&dv, 0x11223344u);
	rgd3_math_check(t, dv.color[0] == 0x44 && dv.color[3] == 0x11 && rgd3_drawvert_get_color(&dv) == 0x11223344u,
					"DrawVert color byte order");

	rgd3_drawvert_clear(&mid);
	dv2 = dv;
	dv2.xyz = rgd3_vec3(2, 4, 6);
	rgd3_drawvert_set_texcoord(&dv2, 1.5f, 0.75f);
	rgd3_drawvert_lerp(&mid, &dv, &dv2, 0.5f);
	st = rgd3_drawvert_get_texcoord(&mid);
	rgd3_math_check(t, rgd3_vec3_compare(mid.xyz, rgd3_vec3(1, 2, 3)) && st.x == 1.0f && st.y == 0.25f,
					"DrawVert Lerp");

	/* skinning with one identity-ish joint at full weight */
	{
		Rgd3JointMat joints[1];
		Rgd3DrawVert sk;
		rgd3_jointmat_identity(&joints[0]);
		rgd3_jointmat_set_translation(&joints[0], rgd3_vec3(10, 0, 0));
		dv.color[0] = dv.color[1] = dv.color[2] = dv.color[3] = 0;
		dv.color2[0] = 255;
		dv.color2[1] = dv.color2[2] = dv.color2[3] = 0;
		dv.xyz = rgd3_vec3(1, 2, 3);
		rgd3_drawvert_skinned(&dv, joints, &sk);
		rgd3_math_check(t, rgd3_vec3_compare_eps(sk.xyz, rgd3_vec3(11, 2, 3), 1e-5f) &&
							   rgd3_vec3_compare(sk.xyz, rgd3_drawvert_skinned_position(&dv, joints)),
						"DrawVert skinning");
	}
}

static void rgd3_math_test_random(Rgd3MathTest* t)
{
	/* seed 0: 69069 * seed + 1, low 15 bits */
	static const int expect[5] = {1, 3534, 1015, 14284, 2653};
	/* idRandom2 seed 0: 1664525 * seed + 1013904223, low 15 bits */
	static const int expect2[5] = {29535, 10546, 30441, 21300, 25859};
	Rgd3Random r;
	Rgd3Random2 r2;
	u32 seed = 0;
	int i, ok = 1, ok2 = 1, okf = 1;

	rgd3_random_init(&r, 0);
	for (i = 0; i < 5; i++)
	{
		int v = rgd3_random_int(&r);
		seed = 69069u * seed + 1u;
		if (v != expect[i] || v != (int)(seed & 0x7fff))
			ok = 0;
	}
	rgd3_math_check(t, ok, "idRandom seed 0 sequence");
	rgd3_math_check(t, rgd3_random_get_seed(&r) == (int)seed, "idRandom seed state");

	rgd3_random_init(&r, 0);
	rgd3_math_check(t, rgd3_random_float(&r) == 1.0f / 32768.0f, "idRandom RandomFloat");

	rgd3_random_init(&r, 12345);
	for (i = 0; i < 1000; i++)
	{
		float f = rgd3_random_crandom_float(&r);
		if (f < -1.0f || f >= 1.0f)
			okf = 0;
		if (rgd3_random_int_max(&r, 7) >= 7)
			okf = 0;
	}
	rgd3_math_check(t, okf, "idRandom ranges");

	rgd3_random2_init(&r2, 0);
	for (i = 0; i < 5; i++)
	{
		if (rgd3_random2_int(&r2) != expect2[i])
			ok2 = 0;
	}
	rgd3_math_check(t, ok2, "idRandom2 seed 0 sequence");
}

static void rgd3_math_test_rg(Rgd3MathTest* t)
{
	Rgd3Angles a = rgd3_angles(15.0f, 70.0f, -20.0f);
	Rgd3Mat3 axis;
	Rgd3Mat4 m4;
	Rgd3Vec3 origin = rgd3_vec3(5.0f, -3.0f, 8.0f);
	Rgd3Vec3 p = rgd3_vec3(1.0f, 2.0f, -4.0f);
	Rgd3Vec3 expect, e4;
	rg_mat4 rm, rm2;
	rg_vec4 in, out;
	rg_vec3 rv;

	rgd3_angles_to_mat3(&a, &axis);
	expect = rgd3_vec3_add(rgd3_mat3_mul_vec(&axis, p), origin);
	rgd3_mat3_origin_to_rg_mat4(&axis, &origin, &rm);
	in.x = p.x;
	in.y = p.y;
	in.z = p.z;
	in.w = 1.0f;
	rg_mat4_mulv4(&rm, &in, &out);
	rgd3_math_check(t, rgd3_vec3_compare_eps(rgd3_vec3(out.x, out.y, out.z), expect, 1e-5f) && out.w == 1.0f,
					"mat3+origin -> rg_mat4 transforms like mul_vec + origin");

	rgd3_mat4_from_mat3_origin(&m4, &axis, origin);
	e4 = rgd3_mat4_mul_vec3(&m4, p);
	rgd3_math_check(t, rgd3_vec3_compare_eps(e4, expect, 1e-5f), "idMat4( axis, origin ) * vec");
	rgd3_mat4_to_rg_mat4(&m4, &rm2);
	rg_mat4_mulv4(&rm2, &in, &out);
	rgd3_math_check(t, rgd3_vec3_compare_eps(rgd3_vec3(out.x, out.y, out.z), expect, 1e-5f),
					"Mat4 -> rg_mat4 keeps the transform");

	rv = rgd3_vec3_to_rg(&p);
	rgd3_math_check(t, rgd3_vec3_compare(rgd3_vec3_from_rg(&rv), p) && rv._pad == 0.0f, "Vec3 <-> rg_vec3");
}

int rgd3_math_self_test(void)
{
	Rgd3MathTest t = {0};

	rgd3_math_test_f16(&t);
	rgd3_math_test_scalar(&t);
	rgd3_math_test_vec_mat(&t);
	rgd3_math_test_joints(&t);
	rgd3_math_test_geometry(&t);
	rgd3_math_test_drawvert(&t);
	rgd3_math_test_random(&t);
	rgd3_math_test_rg(&t);

	rgd3_printf("math: %d checks, %d failures\n", t.checks, t.failures);
	return t.failures == 0;
}
