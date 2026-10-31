/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_math.h - the idlib math layer: scalar helpers, vectors, matrices,
   quaternions, angles, rotations, planes, pluecker lines, bounding volumes,
   joint transforms, draw vertices and the game RNGs.
   Behaviour lifted from neo/idlib/math/Math.h/.cpp, Vector.h/.cpp,
   Matrix.h/.cpp, Quat.h/.cpp, Angles.h/.cpp, Rotation.h/.cpp, Plane.h/.cpp,
   Pluecker.h/.cpp, Random.h, Simd_Generic.cpp (joint helpers),
   neo/idlib/bv/Bounds.h/.cpp, Sphere.h/.cpp, Box.h/.cpp,
   neo/idlib/geometry/JointTransform.h/.cpp, DrawVert.h/.cpp.

   Calling convention
   ------------------
   Value types: Rgd3Vec2/3/4/5/6, Rgd3Quat, Rgd3CQuat, Rgd3Angles, Rgd3Plane.
     - RGINLINE helpers (bodies in this header) take and return them by value.
     - Out-of-line functions (prototype only, body in rgd3_math.c) take every
       struct argument of 12 bytes or more through a const pointer and still
       return value types by value.
     - Operations that modify in place and report something (normalize
       returns the old length, fix_* return 1 if anything changed) take a
       pointer to the value they modify.
   Aggregate types: Rgd3Mat2/3/4, Rgd3Rotation, Rgd3Pluecker, Rgd3Bounds,
   Rgd3Sphere, Rgd3Box, Rgd3JointQuat, Rgd3JointMat, Rgd3DrawVert.
     - Always passed by pointer, inline or not.
     - `rgd3_<type>_<verb>(self, ...)`: self comes first (set_*, from_*,
       *_self, add_point, ...).
     - Functions producing a different aggregate write to a trailing `out`
       (to_mat3, mul, transpose, inverse, ...). Unless noted, `out` may alias
       an input.
     - Array helpers (rgd3_joints_*) take the destination array first, like
       the idSIMD functions they replace.
   Booleans are `int` 1/0.

   Conventions kept from idlib
   ---------------------------
   - Rgd3Mat3 stores axes as ROWS: mat[0] = forward (x axis), mat[1] = left,
     mat[2] = up. rgd3_mat3_mul_vec(m, v) = m.mat[0]*v.x + m.mat[1]*v.y +
     m.mat[2]*v.z, i.e. local -> world. In idlib BOTH `mat * vec` and
     `vec * mat` compute exactly this (operator*(vec, mat) just calls
     mat * vec), and so does idMat3::UnprojectVector. The inverse direction
     (world -> local, dot with each row) is rgd3_mat3_transpose_mul_vec ==
     idMat3::ProjectVector. rgd3_mat3_mul(a, b) is the plain row-by-column
     product a*b of the 3x3 storage, i.e. "rotate by a, then by b" for axes.
   - Rgd3Mat4 is row-major like idMat4: mat[row].x/y/z/w, mul_vec4 is
     row-dot-vector, translation lives in mat[0..2].w.
   - Rgd3JointMat is the 3x4 row-major idJointMat; its 3x3 part is the
     TRANSPOSE of the idMat3 it was built from.
   - Rgd3Plane holds a*x + b*y + c*z + d = 0; distance(p) = n.p + d and
     rgd3_plane_dist() returns -d (idPlane::Dist).
   - Ftoi truncates (cvttss2si semantics, INT32_MIN for NaN/out of range).
   - InvSqrt is sqrtf(1/x) (the BFG code, no lookup table) and returns
     RGD3_MATH_INFINITY (1e30) for x <= FLT_MIN; Sqrt is sqrtf, 0 for x < 0.
     No tables, so there is no init function. */
#ifndef RGD3_MATH_H
#define RGD3_MATH_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

/* ===================================================================== */
/* constants and macros                                                   */
/* ===================================================================== */

#define RGD3_MATH_PI 3.14159265358979323846f
#define RGD3_MATH_TWO_PI (2.0f * RGD3_MATH_PI)
#define RGD3_MATH_HALF_PI (0.5f * RGD3_MATH_PI)
#define RGD3_MATH_ONEFOURTH_PI (0.25f * RGD3_MATH_PI)
#define RGD3_MATH_ONEOVER_PI (1.0f / RGD3_MATH_PI)
#define RGD3_MATH_ONEOVER_TWOPI (1.0f / RGD3_MATH_TWO_PI)
#define RGD3_MATH_E 2.71828182845904523536f
#define RGD3_MATH_SQRT_TWO 1.41421356237309504880f
#define RGD3_MATH_SQRT_THREE 1.73205080756887729352f
#define RGD3_MATH_SQRT_1OVER2 0.70710678118654752440f
#define RGD3_MATH_SQRT_1OVER3 0.57735026918962576450f
/* float divisions as in Math.cpp: M_RAD2DEG is 57.2957764f, slightly
   below 180/pi, so e.g. ToYaw( 0, 1, 0 ) is 89.9999924 like idlib's */
#define RGD3_MATH_M_DEG2RAD (RGD3_MATH_PI / 180.0f)
#define RGD3_MATH_M_RAD2DEG (180.0f / RGD3_MATH_PI)
#define RGD3_MATH_M_SEC2MS 1000.0f
#define RGD3_MATH_M_MS2SEC 0.001f
/* idMath::INFINITY: "huge", not IEEE inf */
#define RGD3_MATH_INFINITY 1e30f
#define RGD3_MATH_FLT_EPSILON 1.192092896e-07f
#define RGD3_MATH_FLT_SMALLEST_NON_DENORMAL 1.175494351e-38f

#define RGD3_VECTOR_EPSILON 0.001f
#define RGD3_MATRIX_INVERSE_EPSILON 1e-14
#define RGD3_MATRIX_EPSILON 1e-6

#define RGD3_DEG2RAD(a) ((a) * RGD3_MATH_M_DEG2RAD)
#define RGD3_RAD2DEG(a) ((a) * RGD3_MATH_M_RAD2DEG)
#define RGD3_SEC2MS(t) (rgd3_math_ftoi((t) * RGD3_MATH_M_SEC2MS))
#define RGD3_MS2SEC(t) ((t) * RGD3_MATH_M_MS2SEC)
#define RGD3_ANGLE2SHORT(x) (rgd3_math_ftoi((x) * 65536.0f / 360.0f) & 65535)
#define RGD3_SHORT2ANGLE(x) ((x) * (360.0f / 65536.0f))
#define RGD3_ANGLE2BYTE(x) (rgd3_math_ftoi((x) * 256.0f / 360.0f) & 255)
#define RGD3_BYTE2ANGLE(x) ((x) * (360.0f / 256.0f))

/* Angles.h index names */
#define RGD3_PITCH 0
#define RGD3_YAW 1
#define RGD3_ROLL 2

/* Plane.h */
#define RGD3_ON_EPSILON 0.1f
#define RGD3_DEGENERATE_DIST_EPSILON 1e-4f
#define RGD3_SIDE_FRONT 0
#define RGD3_SIDE_BACK 1
#define RGD3_SIDE_ON 2
#define RGD3_SIDE_CROSS 3
#define RGD3_PLANESIDE_FRONT 0
#define RGD3_PLANESIDE_BACK 1
#define RGD3_PLANESIDE_ON 2
#define RGD3_PLANESIDE_CROSS 3
#define RGD3_PLANETYPE_X 0
#define RGD3_PLANETYPE_Y 1
#define RGD3_PLANETYPE_Z 2
#define RGD3_PLANETYPE_NEGX 3
#define RGD3_PLANETYPE_NEGY 4
#define RGD3_PLANETYPE_NEGZ 5
#define RGD3_PLANETYPE_TRUEAXIAL 6 /* all types < 6 are true axial planes */
#define RGD3_PLANETYPE_ZEROX 6
#define RGD3_PLANETYPE_ZEROY 7
#define RGD3_PLANETYPE_ZEROZ 8
#define RGD3_PLANETYPE_NONAXIAL 9

/* Random.h */
#define RGD3_RANDOM_MAX_RAND 0x7fff

/* DrawVert.h: offsets the vertex layouts rely on */
#define RGD3_DRAWVERT_SIZE 32
#define RGD3_DRAWVERT_XYZ_OFFSET 0
#define RGD3_DRAWVERT_ST_OFFSET 12
#define RGD3_DRAWVERT_NORMAL_OFFSET 16
#define RGD3_DRAWVERT_TANGENT_OFFSET 20
#define RGD3_DRAWVERT_COLOR_OFFSET 24
#define RGD3_DRAWVERT_COLOR2_OFFSET 28
#define RGD3_JOINTQUAT_SIZE 32
#define RGD3_JOINTMAT_SIZE 48

/* ===================================================================== */
/* types                                                                  */
/* ===================================================================== */

typedef union Rgd3Vec2
{
	struct
	{
		float x, y;
	};
	float v[2];
} Rgd3Vec2;

typedef union Rgd3Vec3
{
	struct
	{
		float x, y, z;
	};
	float v[3];
} Rgd3Vec3;

typedef union Rgd3Vec4
{
	struct
	{
		float x, y, z, w;
	};
	float v[4];
} Rgd3Vec4;

/* idVec5: position + texcoord */
typedef union Rgd3Vec5
{
	struct
	{
		float x, y, z, s, t;
	};
	float v[5];
} Rgd3Vec5;

typedef struct Rgd3Vec6
{
	float p[6];
} Rgd3Vec6;

typedef union Rgd3Quat
{
	struct
	{
		float x, y, z, w;
	};
	float v[4];
} Rgd3Quat;

/* compressed quaternion: w is implied (>= 0) */
typedef union Rgd3CQuat
{
	struct
	{
		float x, y, z;
	};
	float v[3];
} Rgd3CQuat;

typedef union Rgd3Angles
{
	struct
	{
		float pitch, yaw, roll;
	};
	float v[3];
} Rgd3Angles;

typedef union Rgd3Plane
{
	struct
	{
		float a, b, c, d;
	};
	float v[4];
} Rgd3Plane;

typedef struct Rgd3Mat2
{
	Rgd3Vec2 mat[2];
} Rgd3Mat2;

/* rows are axes (idMat3) */
typedef struct Rgd3Mat3
{
	Rgd3Vec3 mat[3];
} Rgd3Mat3;

/* row-major (idMat4) */
typedef struct Rgd3Mat4
{
	Rgd3Vec4 mat[4];
} Rgd3Mat4;

/* idRotation: angle in degrees about vec through origin; axis caches the
   matrix while axisValid is set (set_* clear it). */
typedef struct Rgd3Rotation
{
	Rgd3Vec3 origin;
	Rgd3Vec3 vec;
	float angle;
	Rgd3Mat3 axis;
	int axisValid;
} Rgd3Rotation;

typedef struct Rgd3Pluecker
{
	float p[6];
} Rgd3Pluecker;

typedef struct Rgd3Bounds
{
	Rgd3Vec3 b[2];
} Rgd3Bounds;

typedef struct Rgd3Sphere
{
	Rgd3Vec3 origin;
	float radius;
} Rgd3Sphere;

typedef struct Rgd3Box
{
	Rgd3Vec3 center;
	Rgd3Vec3 extents;
	Rgd3Mat3 axis;
} Rgd3Box;

typedef struct Rgd3JointQuat
{
	Rgd3Quat q;
	Rgd3Vec3 t;
	float w;
} Rgd3JointQuat;

/* 3x4 row-major:
   m[0][0] m[1][0] m[2][0] t[0]
   m[0][1] m[1][1] m[2][1] t[1]
   m[0][2] m[1][2] m[2][2] t[2]   (m = the idMat3, axes as rows) */
typedef struct Rgd3JointMat
{
	float mat[12];
} Rgd3JointMat;

/* idDrawVert: st are half floats, normal/tangent are biased bytes,
   tangent[3] is the bitangent sign, color2 holds skinning weights */
typedef struct Rgd3DrawVert
{
	Rgd3Vec3 xyz;
	u16 st[2];
	u8 normal[4];
	u8 tangent[4];
	u8 color[4];
	u8 color2[4];
} Rgd3DrawVert;

typedef struct Rgd3ShadowVert
{
	Rgd3Vec4 xyzw;
} Rgd3ShadowVert;

typedef struct Rgd3ShadowVertSkinned
{
	Rgd3Vec4 xyzw;
	u8 color[4];
	u8 color2[4];
	u8 pad[8];
} Rgd3ShadowVertSkinned;

typedef struct Rgd3Random
{
	i32 seed;
} Rgd3Random;

typedef struct Rgd3Random2
{
	u32 seed;
} Rgd3Random2;

_Static_assert(sizeof(Rgd3Vec2) == 8, "Rgd3Vec2 must be 8 bytes");
_Static_assert(sizeof(Rgd3Vec3) == 12, "Rgd3Vec3 must be 12 bytes (idVec3)");
_Static_assert(sizeof(Rgd3Vec4) == 16, "Rgd3Vec4 must be 16 bytes");
_Static_assert(sizeof(Rgd3Vec5) == 20, "Rgd3Vec5 must be 20 bytes");
_Static_assert(sizeof(Rgd3Quat) == 16, "Rgd3Quat must be 16 bytes");
_Static_assert(sizeof(Rgd3CQuat) == 12, "Rgd3CQuat must be 12 bytes");
_Static_assert(sizeof(Rgd3Angles) == 12, "Rgd3Angles must be 12 bytes");
_Static_assert(sizeof(Rgd3Plane) == 16, "Rgd3Plane must be 16 bytes");
_Static_assert(sizeof(Rgd3Mat2) == 16, "Rgd3Mat2 must be 16 bytes");
_Static_assert(sizeof(Rgd3Mat3) == 36, "Rgd3Mat3 must be 36 bytes (idMat3)");
_Static_assert(sizeof(Rgd3Mat4) == 64, "Rgd3Mat4 must be 64 bytes (idMat4)");
_Static_assert(sizeof(Rgd3Bounds) == 24, "Rgd3Bounds must be 24 bytes");
_Static_assert(sizeof(Rgd3Sphere) == 16, "Rgd3Sphere must be 16 bytes");
_Static_assert(sizeof(Rgd3JointQuat) == RGD3_JOINTQUAT_SIZE, "Rgd3JointQuat must be 32 bytes");
_Static_assert(sizeof(Rgd3JointMat) == RGD3_JOINTMAT_SIZE, "Rgd3JointMat must be 48 bytes");
_Static_assert(sizeof(Rgd3DrawVert) == RGD3_DRAWVERT_SIZE, "Rgd3DrawVert must be 32 bytes");
_Static_assert(sizeof(Rgd3ShadowVert) == 16, "Rgd3ShadowVert must be 16 bytes");
_Static_assert(sizeof(Rgd3ShadowVertSkinned) == 32, "Rgd3ShadowVertSkinned must be 32 bytes");
_Static_assert(offsetof(Rgd3DrawVert, st) == RGD3_DRAWVERT_ST_OFFSET, "idDrawVert st offset");
_Static_assert(offsetof(Rgd3DrawVert, normal) == RGD3_DRAWVERT_NORMAL_OFFSET, "idDrawVert normal offset");
_Static_assert(offsetof(Rgd3DrawVert, tangent) == RGD3_DRAWVERT_TANGENT_OFFSET, "idDrawVert tangent offset");
_Static_assert(offsetof(Rgd3DrawVert, color) == RGD3_DRAWVERT_COLOR_OFFSET, "idDrawVert color offset");
_Static_assert(offsetof(Rgd3DrawVert, color2) == RGD3_DRAWVERT_COLOR2_OFFSET, "idDrawVert color2 offset");
_Static_assert(offsetof(Rgd3JointQuat, t) == 16, "idJointQuat t offset");
_Static_assert(offsetof(Rgd3ShadowVertSkinned, color) == 16, "idShadowVertSkinned color offset");
_Static_assert(offsetof(Rgd3ShadowVertSkinned, color2) == 20, "idShadowVertSkinned color2 offset");

/* ===================================================================== */
/* scalar: idMath                                                         */
/* ===================================================================== */

RGINLINE u32 rgd3_math_float_bits(float f)
{
	union
	{
		float f;
		u32 u;
	} c;
	c.f = f;
	return c.u;
}

RGINLINE float rgd3_math_bits_float(u32 u)
{
	union
	{
		float f;
		u32 u;
	} c;
	c.u = u;
	return c.f;
}

RGINLINE int rgd3_math_is_nan(float x)
{
	return x != x;
}

RGINLINE int rgd3_math_is_inf(float x)
{
	return x == x && x * 0 != x * 0;
}

RGINLINE int rgd3_math_is_inf_nan(float x)
{
	return x * 0 != x * 0;
}

RGINLINE int rgd3_math_is_denormal(float x)
{
	u32 u = rgd3_math_float_bits(x);
	return (u & 0x7f800000u) == 0 && (u & 0x007fffffu) != 0;
}

/* IsValid<float>: not NaN, inf, indefinite or denormal */
RGINLINE int rgd3_math_is_valid(float x)
{
	return !(rgd3_math_is_nan(x) || rgd3_math_is_inf(x) || rgd3_math_float_bits(x) == 0xffc00000u ||
			 rgd3_math_is_denormal(x));
}

RGINLINE float rgd3_math_inv_sqrt(float x)
{
	return (x > RGD3_MATH_FLT_SMALLEST_NON_DENORMAL) ? sqrtf(1.0f / x) : RGD3_MATH_INFINITY;
}

RGINLINE float rgd3_math_inv_sqrt16(float x)
{
	return rgd3_math_inv_sqrt(x);
}

RGINLINE float rgd3_math_sqrt(float x)
{
	return (x >= 0.0f) ? sqrtf(x) : 0.0f;
}

RGINLINE float rgd3_math_sqrt16(float x)
{
	return rgd3_math_sqrt(x);
}

RGINLINE float rgd3_math_sin(float a)
{
	return sinf(a);
}

RGINLINE float rgd3_math_cos(float a)
{
	return cosf(a);
}

RGINLINE void rgd3_math_sincos(float a, float* s, float* c)
{
	*s = sinf(a);
	*c = cosf(a);
}

RGINLINE float rgd3_math_tan(float a)
{
	return tanf(a);
}

/* input clamped to [-1, 1] */
RGINLINE float rgd3_math_asin(float a)
{
	if (a <= -1.0f)
		return -RGD3_MATH_HALF_PI;
	if (a >= 1.0f)
		return RGD3_MATH_HALF_PI;
	return asinf(a);
}

/* input clamped to [-1, 1] */
RGINLINE float rgd3_math_acos(float a)
{
	if (a <= -1.0f)
		return RGD3_MATH_PI;
	if (a >= 1.0f)
		return 0.0f;
	return acosf(a);
}

RGINLINE float rgd3_math_atan(float a)
{
	return atanf(a);
}

/* idMath::ATan( y, x ) */
RGINLINE float rgd3_math_atan2(float y, float x)
{
	return atan2f(y, x);
}

RGINLINE float rgd3_math_pow(float x, float y)
{
	return powf(x, y);
}

RGINLINE float rgd3_math_exp(float f)
{
	return expf(f);
}

RGINLINE float rgd3_math_log(float f)
{
	return logf(f);
}

/* the 16-bit precision approximations: Quat slerp depends on sin16/atan16 */
float rgd3_math_sin16(float a);
float rgd3_math_cos16(float a);
void rgd3_math_sincos16(float a, float* s, float* c);
float rgd3_math_tan16(float a);
float rgd3_math_asin16(float a);
float rgd3_math_acos16(float a);
float rgd3_math_atan16(float a);
float rgd3_math_atan2_16(float y, float x);
float rgd3_math_exp16(float f);
float rgd3_math_log16(float f);
float rgd3_math_pow16(float x, float y);

RGINLINE int rgd3_math_ipow(int x, int y)
{
	int r;
	for (r = x; y > 1; y--)
		r *= x;
	return r;
}

RGINLINE int rgd3_math_ilog2(float f)
{
	return (int)((rgd3_math_float_bits(f) >> 23) & 0xff) - 127;
}

RGINLINE int rgd3_math_ilog2_int(int i)
{
	return rgd3_math_ilog2((float)i);
}

RGINLINE int rgd3_math_bits_for_float(float f)
{
	return rgd3_math_ilog2(f) + 1;
}

RGINLINE int rgd3_math_bits_for_integer(int i)
{
	return rgd3_math_ilog2((float)i) + 1;
}

/* 0 for f >= +0.0f, -1 (all bits) for f <= -0.0f */
RGINLINE int rgd3_math_mask_for_float_sign(float f)
{
	return (rgd3_math_float_bits(f) & 0x80000000u) ? -1 : 0;
}

RGINLINE int rgd3_math_mask_for_integer_sign(int i)
{
	return i < 0 ? -1 : 0;
}

RGINLINE int rgd3_math_floor_power_of_two(int x)
{
	u32 u = (u32)x;
	u |= u >> 1;
	u |= u >> 2;
	u |= u >> 4;
	u |= u >> 8;
	u |= u >> 16;
	u++;
	return (int)(u >> 1);
}

RGINLINE int rgd3_math_ceil_power_of_two(int x)
{
	u32 u = (u32)x;
	u--;
	u |= u >> 1;
	u |= u >> 2;
	u |= u >> 4;
	u |= u >> 8;
	u |= u >> 16;
	u++;
	return (int)u;
}

RGINLINE int rgd3_math_is_power_of_two(int x)
{
	return (x & (x - 1)) == 0 && x > 0;
}

RGINLINE int rgd3_math_bit_count(int x)
{
	u32 u = (u32)x;
	u -= ((u >> 1) & 0x55555555u);
	u = (((u >> 2) & 0x33333333u) + (u & 0x33333333u));
	u = (((u >> 4) + u) & 0x0f0f0f0fu);
	u += (u >> 8);
	return (int)((u + (u >> 16)) & 0x0000003fu);
}

RGINLINE int rgd3_math_bit_reverse(int x)
{
	u32 u = (u32)x;
	u = (((u >> 1) & 0x55555555u) | ((u & 0x55555555u) << 1));
	u = (((u >> 2) & 0x33333333u) | ((u & 0x33333333u) << 2));
	u = (((u >> 4) & 0x0f0f0f0fu) | ((u & 0x0f0f0f0fu) << 4));
	u = (((u >> 8) & 0x00ff00ffu) | ((u & 0x00ff00ffu) << 8));
	return (int)((u >> 16) | (u << 16));
}

RGINLINE float rgd3_math_fabs(float f)
{
	return fabsf(f);
}

RGINLINE float rgd3_math_floor(float f)
{
	return floorf(f);
}

RGINLINE float rgd3_math_ceil(float f)
{
	return ceilf(f);
}

/* idMath::Rint: floor( f + 0.5f ) */
RGINLINE float rgd3_math_rint(float f)
{
	return floorf(f + 0.5f);
}

RGINLINE float rgd3_math_frac(float f)
{
	return f - floorf(f);
}

/* truncating, like cvttss2si: NaN and out-of-range give INT32_MIN */
RGINLINE int rgd3_math_ftoi(float f)
{
	if (f >= -2147483648.0f && f < 2147483648.0f)
		return (int)f;
	return -2147483647 - 1;
}

/* BFG has no FtoiFast any more; older idlib callers get Ftoi */
RGINLINE int rgd3_math_ftoi_fast(float f)
{
	return rgd3_math_ftoi(f);
}

/* the SSE paths clamp the float first, then truncate; NaN -> lower bound */
RGINLINE i8 rgd3_math_ftoi8(float f)
{
	if (!(f > -128.0f))
		return -128;
	if (f > 127.0f)
		return 127;
	return (i8)(int)f;
}

RGINLINE i16 rgd3_math_ftoi16(float f)
{
	if (!(f > -32768.0f))
		return -32768;
	if (f > 32767.0f)
		return 32767;
	return (i16)(int)f;
}

RGINLINE u16 rgd3_math_ftoui16(float f)
{
	int i = rgd3_math_ftoi(f);
	if (i < 0)
		return 0;
	if (i > 65535)
		return 65535;
	return (u16)i;
}

/* float to byte, clamped to [0, 255], truncating */
RGINLINE u8 rgd3_math_ftob(float f)
{
	if (!(f > 0.0f))
		return 0;
	if (f > 255.0f)
		return 255;
	return (u8)(int)f;
}

RGINLINE i8 rgd3_math_clamp_char(int i)
{
	if (i < -128)
		return -128;
	if (i > 127)
		return 127;
	return (i8)i;
}

RGINLINE i16 rgd3_math_clamp_short(int i)
{
	if (i < -32768)
		return -32768;
	if (i > 32767)
		return 32767;
	return (i16)i;
}

RGINLINE u8 rgd3_math_clamp_byte(int x)
{
	return (u8)(x < 0 ? 0 : (x > 255 ? 255 : x));
}

RGINLINE int rgd3_math_clamp_int(int min, int max, int value)
{
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

/* Max( min, Min( max, value ) ) */
RGINLINE float rgd3_math_clamp_float(float min, float max, float value)
{
	float v = (max < value) ? max : value;
	return (min > v) ? min : v;
}

RGINLINE float rgd3_math_angle_normalize360(float angle)
{
	if ((angle >= 360.0f) || (angle < 0.0f))
		angle -= floorf(angle * (1.0f / 360.0f)) * 360.0f;
	return angle;
}

RGINLINE float rgd3_math_angle_normalize180(float angle)
{
	angle = rgd3_math_angle_normalize360(angle);
	if (angle > 180.0f)
		angle -= 360.0f;
	return angle;
}

RGINLINE float rgd3_math_angle_delta(float angle1, float angle2)
{
	return rgd3_math_angle_normalize180(angle1 - angle2);
}

RGINLINE int rgd3_math_float_hash(const float* array, int numFloats)
{
	u32 hash = 0;
	int i;
	for (i = 0; i < numFloats; i++)
		hash ^= rgd3_math_float_bits(array[i]);
	return (int)hash;
}

/* ::Lerp<float>: from + ( to - from ) * f, unclamped */
RGINLINE float rgd3_math_lerp(float from, float to, float f)
{
	return from + ((to - from) * f);
}

/* ::Lerp<int> */
RGINLINE int rgd3_math_lerp_int(int from, int to, float f)
{
	return rgd3_math_ftoi((float)from + (((float)to - (float)from) * f));
}

RGINLINE float rgd3_math_lerp_to_with_scale(float cur, float dest, float scale)
{
	float delta = dest - cur;
	if (delta > -1.0e-6f && delta < 1.0e-6f)
		return dest;
	return cur + (dest - cur) * scale;
}

/* idMath::FloatToBits / BitsToFloat: small custom float formats */
int rgd3_math_float_to_bits(float f, int exponentBits, int mantissaBits);
float rgd3_math_bits_to_float(int i, int exponentBits, int mantissaBits);

/* halfFloat_t, exactly DrawVert.h F16toF32 / F32toF16: f32_to_f16 truncates
   the mantissa, flushes anything below 2^-14 (and -0) to +0 and turns
   anything >= 65536 (and inf/NaN) into +-65504; f16_to_f32 maps half
   inf to signed zero and half NaN to a denormal-style value. */
RGINLINE float rgd3_f16_to_f32(u16 x)
{
	int e = (x & 32767) >> 10;
	int m = x & 1023;
	int s = (x & 32768) ? -1 : 1;
	if (0 < e && e < 31)
		return s * ldexpf(1.0f, e - 15) * (1 + m / 1024.0f);
	else if (m == 0)
		return s * 0.0f;
	return s * ldexpf(1.0f, -14) * (m / 1024.0f);
}

RGINLINE u16 rgd3_f32_to_f16(float a)
{
	u32 f = rgd3_math_float_bits(a);
	u32 signbit = (f & 0x80000000u) >> 16;
	int exponent = (int)((f & 0x7F800000u) >> 23) - 112;
	u32 mantissa = (f & 0x007FFFFFu);
	if (exponent <= 0)
		return 0;
	if (exponent > 30)
		return (u16)(signbit | 0x7BFF);
	return (u16)(signbit | ((u32)exponent << 10) | (mantissa >> 13));
}

/* ===================================================================== */
/* Rgd3Vec2 (value)                                                       */
/* ===================================================================== */

RGINLINE Rgd3Vec2 rgd3_vec2(float x, float y)
{
	Rgd3Vec2 r;
	r.x = x;
	r.y = y;
	return r;
}

RGINLINE Rgd3Vec2 rgd3_vec2_zero(void)
{
	return rgd3_vec2(0.0f, 0.0f);
}

RGINLINE Rgd3Vec2 rgd3_vec2_add(Rgd3Vec2 a, Rgd3Vec2 b)
{
	return rgd3_vec2(a.x + b.x, a.y + b.y);
}

RGINLINE Rgd3Vec2 rgd3_vec2_sub(Rgd3Vec2 a, Rgd3Vec2 b)
{
	return rgd3_vec2(a.x - b.x, a.y - b.y);
}

RGINLINE Rgd3Vec2 rgd3_vec2_scale(Rgd3Vec2 a, float s)
{
	return rgd3_vec2(a.x * s, a.y * s);
}

/* a / s, computed as a * ( 1 / s ) like idVec2::operator/ */
RGINLINE Rgd3Vec2 rgd3_vec2_div(Rgd3Vec2 a, float s)
{
	float inva = 1.0f / s;
	return rgd3_vec2(a.x * inva, a.y * inva);
}

/* component-wise product (idVec2::Scale) */
RGINLINE Rgd3Vec2 rgd3_vec2_mul(Rgd3Vec2 a, Rgd3Vec2 b)
{
	return rgd3_vec2(a.x * b.x, a.y * b.y);
}

RGINLINE Rgd3Vec2 rgd3_vec2_neg(Rgd3Vec2 a)
{
	return rgd3_vec2(-a.x, -a.y);
}

RGINLINE float rgd3_vec2_dot(Rgd3Vec2 a, Rgd3Vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

RGINLINE int rgd3_vec2_compare(Rgd3Vec2 a, Rgd3Vec2 b)
{
	return a.x == b.x && a.y == b.y;
}

RGINLINE int rgd3_vec2_compare_eps(Rgd3Vec2 a, Rgd3Vec2 b, float epsilon)
{
	if (fabsf(a.x - b.x) > epsilon)
		return 0;
	if (fabsf(a.y - b.y) > epsilon)
		return 0;
	return 1;
}

RGINLINE float rgd3_vec2_length_sqr(Rgd3Vec2 a)
{
	return a.x * a.x + a.y * a.y;
}

RGINLINE float rgd3_vec2_length(Rgd3Vec2 a)
{
	return rgd3_math_sqrt(a.x * a.x + a.y * a.y);
}

RGINLINE float rgd3_vec2_length_fast(Rgd3Vec2 a)
{
	float sqrLength = a.x * a.x + a.y * a.y;
	return sqrLength * rgd3_math_inv_sqrt(sqrLength);
}

/* returns the old length; a zero vector stays zero (0 * 1e30) */
RGINLINE float rgd3_vec2_normalize(Rgd3Vec2* a)
{
	float sqrLength = a->x * a->x + a->y * a->y;
	float invLength = rgd3_math_inv_sqrt(sqrLength);
	a->x *= invLength;
	a->y *= invLength;
	return invLength * sqrLength;
}

RGINLINE float rgd3_vec2_normalize_fast(Rgd3Vec2* a)
{
	return rgd3_vec2_normalize(a);
}

/* cap length */
RGINLINE Rgd3Vec2 rgd3_vec2_truncate(Rgd3Vec2 a, float length)
{
	if (length < RGD3_MATH_FLT_SMALLEST_NON_DENORMAL)
		return rgd3_vec2_zero();
	else
	{
		float length2 = rgd3_vec2_length_sqr(a);
		if (length2 > length * length)
		{
			float ilength = length * rgd3_math_inv_sqrt(length2);
			return rgd3_vec2_scale(a, ilength);
		}
	}
	return a;
}

RGINLINE Rgd3Vec2 rgd3_vec2_clamp(Rgd3Vec2 a, Rgd3Vec2 min, Rgd3Vec2 max)
{
	if (a.x < min.x)
		a.x = min.x;
	else if (a.x > max.x)
		a.x = max.x;
	if (a.y < min.y)
		a.y = min.y;
	else if (a.y > max.y)
		a.y = max.y;
	return a;
}

RGINLINE Rgd3Vec2 rgd3_vec2_snap(Rgd3Vec2 a)
{
	return rgd3_vec2(floorf(a.x + 0.5f), floorf(a.y + 0.5f));
}

RGINLINE Rgd3Vec2 rgd3_vec2_snap_int(Rgd3Vec2 a)
{
	return rgd3_vec2((float)(int)a.x, (float)(int)a.y);
}

/* idVec2::Lerp: clamped to v1 for l <= 0 and v2 for l >= 1 */
RGINLINE Rgd3Vec2 rgd3_vec2_lerp(Rgd3Vec2 v1, Rgd3Vec2 v2, float l)
{
	if (l <= 0.0f)
		return v1;
	if (l >= 1.0f)
		return v2;
	return rgd3_vec2_add(v1, rgd3_vec2_scale(rgd3_vec2_sub(v2, v1), l));
}

/* ===================================================================== */
/* Rgd3Vec3 (value)                                                       */
/* ===================================================================== */

RGINLINE Rgd3Vec3 rgd3_vec3(float x, float y, float z)
{
	Rgd3Vec3 r;
	r.x = x;
	r.y = y;
	r.z = z;
	return r;
}

RGINLINE Rgd3Vec3 rgd3_vec3_splat(float f)
{
	return rgd3_vec3(f, f, f);
}

RGINLINE Rgd3Vec3 rgd3_vec3_zero(void)
{
	return rgd3_vec3(0.0f, 0.0f, 0.0f);
}

RGINLINE void rgd3_vec3_set(Rgd3Vec3* v, float x, float y, float z)
{
	v->x = x;
	v->y = y;
	v->z = z;
}

RGINLINE Rgd3Vec3 rgd3_vec3_add(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return rgd3_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

RGINLINE Rgd3Vec3 rgd3_vec3_sub(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return rgd3_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

RGINLINE Rgd3Vec3 rgd3_vec3_scale(Rgd3Vec3 a, float s)
{
	return rgd3_vec3(a.x * s, a.y * s, a.z * s);
}

/* a + b * s */
RGINLINE Rgd3Vec3 rgd3_vec3_mad(Rgd3Vec3 a, Rgd3Vec3 b, float s)
{
	return rgd3_vec3(a.x + b.x * s, a.y + b.y * s, a.z + b.z * s);
}

/* a / s, computed as a * ( 1 / s ) like idVec3::operator/ */
RGINLINE Rgd3Vec3 rgd3_vec3_div(Rgd3Vec3 a, float s)
{
	float inva = 1.0f / s;
	return rgd3_vec3(a.x * inva, a.y * inva, a.z * inva);
}

/* component-wise product / quotient (idVec3::operator/=( idVec3 )) */
RGINLINE Rgd3Vec3 rgd3_vec3_mul_comp(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return rgd3_vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

RGINLINE Rgd3Vec3 rgd3_vec3_div_comp(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return rgd3_vec3(a.x / b.x, a.y / b.y, a.z / b.z);
}

RGINLINE Rgd3Vec3 rgd3_vec3_neg(Rgd3Vec3 a)
{
	return rgd3_vec3(-a.x, -a.y, -a.z);
}

RGINLINE float rgd3_vec3_dot(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

RGINLINE Rgd3Vec3 rgd3_vec3_cross(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return rgd3_vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

RGINLINE int rgd3_vec3_compare(Rgd3Vec3 a, Rgd3Vec3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

RGINLINE int rgd3_vec3_compare_eps(Rgd3Vec3 a, Rgd3Vec3 b, float epsilon)
{
	if (fabsf(a.x - b.x) > epsilon)
		return 0;
	if (fabsf(a.y - b.y) > epsilon)
		return 0;
	if (fabsf(a.z - b.z) > epsilon)
		return 0;
	return 1;
}

RGINLINE float rgd3_vec3_length_sqr(Rgd3Vec3 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z;
}

RGINLINE float rgd3_vec3_length(Rgd3Vec3 a)
{
	return rgd3_math_sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

RGINLINE float rgd3_vec3_length_fast(Rgd3Vec3 a)
{
	float sqrLength = a.x * a.x + a.y * a.y + a.z * a.z;
	return sqrLength * rgd3_math_inv_sqrt(sqrLength);
}

/* idVec3::Normalize: returns the old length. A zero (or denormal-length)
   vector is multiplied by 1e30 and so stays (near) zero; 0 is returned. */
RGINLINE float rgd3_vec3_normalize(Rgd3Vec3* a)
{
	float sqrLength = a->x * a->x + a->y * a->y + a->z * a->z;
	float invLength = rgd3_math_inv_sqrt(sqrLength);
	a->x *= invLength;
	a->y *= invLength;
	a->z *= invLength;
	return invLength * sqrLength;
}

RGINLINE float rgd3_vec3_normalize_fast(Rgd3Vec3* a)
{
	return rgd3_vec3_normalize(a);
}

/* convenience: normalized copy */
RGINLINE Rgd3Vec3 rgd3_vec3_normalized(Rgd3Vec3 a)
{
	rgd3_vec3_normalize(&a);
	return a;
}

/* snaps near-axial normals that are exactly axial in two components;
   returns 1 if the vector changed */
RGINLINE int rgd3_vec3_fix_degenerate_normal(Rgd3Vec3* v)
{
	if (v->x == 0.0f)
	{
		if (v->y == 0.0f)
		{
			if (v->z > 0.0f)
			{
				if (v->z != 1.0f)
				{
					v->z = 1.0f;
					return 1;
				}
			}
			else
			{
				if (v->z != -1.0f)
				{
					v->z = -1.0f;
					return 1;
				}
			}
			return 0;
		}
		else if (v->z == 0.0f)
		{
			if (v->y > 0.0f)
			{
				if (v->y != 1.0f)
				{
					v->y = 1.0f;
					return 1;
				}
			}
			else
			{
				if (v->y != -1.0f)
				{
					v->y = -1.0f;
					return 1;
				}
			}
			return 0;
		}
	}
	else if (v->y == 0.0f)
	{
		if (v->z == 0.0f)
		{
			if (v->x > 0.0f)
			{
				if (v->x != 1.0f)
				{
					v->x = 1.0f;
					return 1;
				}
			}
			else
			{
				if (v->x != -1.0f)
				{
					v->x = -1.0f;
					return 1;
				}
			}
			return 0;
		}
	}
	if (fabsf(v->x) == 1.0f)
	{
		if (v->y != 0.0f || v->z != 0.0f)
		{
			v->y = v->z = 0.0f;
			return 1;
		}
		return 0;
	}
	else if (fabsf(v->y) == 1.0f)
	{
		if (v->x != 0.0f || v->z != 0.0f)
		{
			v->x = v->z = 0.0f;
			return 1;
		}
		return 0;
	}
	else if (fabsf(v->z) == 1.0f)
	{
		if (v->x != 0.0f || v->y != 0.0f)
		{
			v->x = v->y = 0.0f;
			return 1;
		}
		return 0;
	}
	return 0;
}

/* zeroes components below 1e-30; returns 1 if any was */
RGINLINE int rgd3_vec3_fix_denormals(Rgd3Vec3* v)
{
	int denormal = 0;
	if (fabsf(v->x) < 1e-30f)
	{
		v->x = 0.0f;
		denormal = 1;
	}
	if (fabsf(v->y) < 1e-30f)
	{
		v->y = 0.0f;
		denormal = 1;
	}
	if (fabsf(v->z) < 1e-30f)
	{
		v->z = 0.0f;
		denormal = 1;
	}
	return denormal;
}

/* cap length */
RGINLINE Rgd3Vec3 rgd3_vec3_truncate(Rgd3Vec3 a, float length)
{
	if (length < RGD3_MATH_FLT_SMALLEST_NON_DENORMAL)
		return rgd3_vec3_zero();
	else
	{
		float length2 = rgd3_vec3_length_sqr(a);
		if (length2 > length * length)
		{
			float ilength = length * rgd3_math_inv_sqrt(length2);
			return rgd3_vec3_scale(a, ilength);
		}
	}
	return a;
}

RGINLINE Rgd3Vec3 rgd3_vec3_clamp(Rgd3Vec3 a, Rgd3Vec3 min, Rgd3Vec3 max)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		if (a.v[i] < min.v[i])
			a.v[i] = min.v[i];
		else if (a.v[i] > max.v[i])
			a.v[i] = max.v[i];
	}
	return a;
}

/* snap to closest integer value */
RGINLINE Rgd3Vec3 rgd3_vec3_snap(Rgd3Vec3 a)
{
	return rgd3_vec3(floorf(a.x + 0.5f), floorf(a.y + 0.5f), floorf(a.z + 0.5f));
}

/* snap towards zero */
RGINLINE Rgd3Vec3 rgd3_vec3_snap_int(Rgd3Vec3 a)
{
	return rgd3_vec3((float)(int)a.x, (float)(int)a.y, (float)(int)a.z);
}

/* idVec3::Lerp: clamped to v1 for l <= 0 and v2 for l >= 1 */
RGINLINE Rgd3Vec3 rgd3_vec3_lerp(Rgd3Vec3 v1, Rgd3Vec3 v2, float l)
{
	if (l <= 0.0f)
		return v1;
	if (l >= 1.0f)
		return v2;
	return rgd3_vec3_add(v1, rgd3_vec3_scale(rgd3_vec3_sub(v2, v1), l));
}

RGINLINE Rgd3Vec2 rgd3_vec3_to_vec2(Rgd3Vec3 a)
{
	return rgd3_vec2(a.x, a.y);
}

/* left/down perpendicular to a normalized vector */
RGINLINE void rgd3_vec3_normal_vectors(Rgd3Vec3 v, Rgd3Vec3* left, Rgd3Vec3* down)
{
	float d = v.x * v.x + v.y * v.y;
	if (!d)
	{
		left->x = 1;
		left->y = 0;
		left->z = 0;
	}
	else
	{
		d = rgd3_math_inv_sqrt(d);
		left->x = -v.y * d;
		left->y = v.x * d;
		left->z = 0;
	}
	*down = rgd3_vec3_cross(*left, v);
}

/* idVec3::OrthogonalBasis, quirk included: for |v.z| > 0.7 the returned
   left is perpendicular to up but NOT to v (only the other branch gives an
   orthonormal frame) */
RGINLINE void rgd3_vec3_ortho_basis(Rgd3Vec3 v, Rgd3Vec3* left, Rgd3Vec3* up)
{
	float l, s;
	if (fabsf(v.z) > 0.7f)
	{
		l = v.y * v.y + v.z * v.z;
		s = rgd3_math_inv_sqrt(l);
		up->x = 0;
		up->y = v.z * s;
		up->z = -v.y * s;
		left->x = l * s;
		left->y = -v.x * up->z;
		left->z = v.x * up->y;
	}
	else
	{
		l = v.x * v.x + v.y * v.y;
		s = rgd3_math_inv_sqrt(l);
		left->x = -v.y * s;
		left->y = v.x * s;
		left->z = 0;
		up->x = -v.z * left->y;
		up->y = v.z * left->x;
		up->z = l * s;
	}
}

/* idVec3::ProjectOntoPlane (overBounce 1.0f is the default) */
RGINLINE Rgd3Vec3 rgd3_vec3_project_onto_plane(Rgd3Vec3 v, Rgd3Vec3 normal, float overBounce)
{
	float backoff = rgd3_vec3_dot(v, normal);
	if (overBounce != 1.0)
	{
		if (backoff < 0)
			backoff *= overBounce;
		else
			backoff /= overBounce;
	}
	return rgd3_vec3_sub(v, rgd3_vec3_scale(normal, backoff));
}

/* out-of-line Vec3 helpers (inputs by pointer) */
float rgd3_vec3_to_yaw(const Rgd3Vec3* v);
float rgd3_vec3_to_pitch(const Rgd3Vec3* v);
Rgd3Angles rgd3_vec3_to_angles(const Rgd3Vec3* v);
/* forward = v (should be normalized), left horizontal, up = forward x left */
void rgd3_vec3_to_mat3(const Rgd3Vec3* v, Rgd3Mat3* out);
int rgd3_vec3_project_along_plane(Rgd3Vec3* v, const Rgd3Vec3* normal, float epsilon, float overBounce);
void rgd3_vec3_project_self_onto_sphere(Rgd3Vec3* v, float radius);
/* vectors are expected to be normalized; clamped like lerp */
Rgd3Vec3 rgd3_vec3_slerp(const Rgd3Vec3* v1, const Rgd3Vec3* v2, float t);

/* ===================================================================== */
/* Rgd3Vec4 / Vec5 / Vec6 (value)                                         */
/* ===================================================================== */

RGINLINE Rgd3Vec4 rgd3_vec4(float x, float y, float z, float w)
{
	Rgd3Vec4 r;
	r.x = x;
	r.y = y;
	r.z = z;
	r.w = w;
	return r;
}

RGINLINE Rgd3Vec4 rgd3_vec4_zero(void)
{
	return rgd3_vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

RGINLINE Rgd3Vec4 rgd3_vec4_from_vec3(Rgd3Vec3 v, float w)
{
	return rgd3_vec4(v.x, v.y, v.z, w);
}

RGINLINE Rgd3Vec4 rgd3_vec4_add(Rgd3Vec4 a, Rgd3Vec4 b)
{
	return rgd3_vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

RGINLINE Rgd3Vec4 rgd3_vec4_sub(Rgd3Vec4 a, Rgd3Vec4 b)
{
	return rgd3_vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

RGINLINE Rgd3Vec4 rgd3_vec4_scale(Rgd3Vec4 a, float s)
{
	return rgd3_vec4(a.x * s, a.y * s, a.z * s, a.w * s);
}

RGINLINE Rgd3Vec4 rgd3_vec4_div(Rgd3Vec4 a, float s)
{
	float inva = 1.0f / s;
	return rgd3_vec4(a.x * inva, a.y * inva, a.z * inva, a.w * inva);
}

/* component-wise (idVec4::Multiply) */
RGINLINE Rgd3Vec4 rgd3_vec4_mul(Rgd3Vec4 a, Rgd3Vec4 b)
{
	return rgd3_vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

RGINLINE Rgd3Vec4 rgd3_vec4_neg(Rgd3Vec4 a)
{
	return rgd3_vec4(-a.x, -a.y, -a.z, -a.w);
}

RGINLINE float rgd3_vec4_dot(Rgd3Vec4 a, Rgd3Vec4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

RGINLINE int rgd3_vec4_compare(Rgd3Vec4 a, Rgd3Vec4 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

RGINLINE int rgd3_vec4_compare_eps(Rgd3Vec4 a, Rgd3Vec4 b, float epsilon)
{
	if (fabsf(a.x - b.x) > epsilon)
		return 0;
	if (fabsf(a.y - b.y) > epsilon)
		return 0;
	if (fabsf(a.z - b.z) > epsilon)
		return 0;
	if (fabsf(a.w - b.w) > epsilon)
		return 0;
	return 1;
}

RGINLINE float rgd3_vec4_length_sqr(Rgd3Vec4 a)
{
	return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w;
}

RGINLINE float rgd3_vec4_length(Rgd3Vec4 a)
{
	return rgd3_math_sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

RGINLINE float rgd3_vec4_normalize(Rgd3Vec4* a)
{
	float sqrLength = a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w;
	float invLength = rgd3_math_inv_sqrt(sqrLength);
	a->x *= invLength;
	a->y *= invLength;
	a->z *= invLength;
	a->w *= invLength;
	return invLength * sqrLength;
}

RGINLINE float rgd3_vec4_normalize_fast(Rgd3Vec4* a)
{
	return rgd3_vec4_normalize(a);
}

RGINLINE Rgd3Vec4 rgd3_vec4_lerp(Rgd3Vec4 v1, Rgd3Vec4 v2, float l)
{
	if (l <= 0.0f)
		return v1;
	if (l >= 1.0f)
		return v2;
	return rgd3_vec4_add(v1, rgd3_vec4_scale(rgd3_vec4_sub(v2, v1), l));
}

RGINLINE Rgd3Vec3 rgd3_vec4_to_vec3(Rgd3Vec4 a)
{
	return rgd3_vec3(a.x, a.y, a.z);
}

RGINLINE Rgd3Vec5 rgd3_vec5(Rgd3Vec3 xyz, Rgd3Vec2 st)
{
	Rgd3Vec5 r;
	r.x = xyz.x;
	r.y = xyz.y;
	r.z = xyz.z;
	r.s = st.x;
	r.t = st.y;
	return r;
}

RGINLINE Rgd3Vec3 rgd3_vec5_to_vec3(Rgd3Vec5 a)
{
	return rgd3_vec3(a.x, a.y, a.z);
}

RGINLINE Rgd3Vec5 rgd3_vec5_lerp(Rgd3Vec5 v1, Rgd3Vec5 v2, float l)
{
	Rgd3Vec5 r;
	int i;
	if (l <= 0.0f)
		return v1;
	if (l >= 1.0f)
		return v2;
	for (i = 0; i < 5; i++)
		r.v[i] = v1.v[i] + l * (v2.v[i] - v1.v[i]);
	return r;
}

RGINLINE Rgd3Vec6 rgd3_vec6(float a1, float a2, float a3, float a4, float a5, float a6)
{
	Rgd3Vec6 r;
	r.p[0] = a1;
	r.p[1] = a2;
	r.p[2] = a3;
	r.p[3] = a4;
	r.p[4] = a5;
	r.p[5] = a6;
	return r;
}

RGINLINE Rgd3Vec6 rgd3_vec6_zero(void)
{
	return rgd3_vec6(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

RGINLINE Rgd3Vec6 rgd3_vec6_add(Rgd3Vec6 a, Rgd3Vec6 b)
{
	int i;
	for (i = 0; i < 6; i++)
		a.p[i] += b.p[i];
	return a;
}

RGINLINE Rgd3Vec6 rgd3_vec6_sub(Rgd3Vec6 a, Rgd3Vec6 b)
{
	int i;
	for (i = 0; i < 6; i++)
		a.p[i] -= b.p[i];
	return a;
}

RGINLINE Rgd3Vec6 rgd3_vec6_scale(Rgd3Vec6 a, float s)
{
	int i;
	for (i = 0; i < 6; i++)
		a.p[i] *= s;
	return a;
}

RGINLINE float rgd3_vec6_dot(Rgd3Vec6 a, Rgd3Vec6 b)
{
	return a.p[0] * b.p[0] + a.p[1] * b.p[1] + a.p[2] * b.p[2] + a.p[3] * b.p[3] + a.p[4] * b.p[4] +
		   a.p[5] * b.p[5];
}

RGINLINE int rgd3_vec6_compare_eps(Rgd3Vec6 a, Rgd3Vec6 b, float epsilon)
{
	int i;
	for (i = 0; i < 6; i++)
	{
		if (fabsf(a.p[i] - b.p[i]) > epsilon)
			return 0;
	}
	return 1;
}

RGINLINE float rgd3_vec6_length(Rgd3Vec6 a)
{
	return rgd3_math_sqrt(rgd3_vec6_dot(a, a));
}

RGINLINE float rgd3_vec6_normalize(Rgd3Vec6* a)
{
	float sqrLength = rgd3_vec6_dot(*a, *a);
	float invLength = rgd3_math_inv_sqrt(sqrLength);
	int i;
	for (i = 0; i < 6; i++)
		a->p[i] *= invLength;
	return invLength * sqrLength;
}

/* ===================================================================== */
/* Rgd3Mat2 (aggregate)                                                   */
/* ===================================================================== */

RGINLINE void rgd3_mat2_set(Rgd3Mat2* m, float xx, float xy, float yx, float yy)
{
	m->mat[0].x = xx;
	m->mat[0].y = xy;
	m->mat[1].x = yx;
	m->mat[1].y = yy;
}

RGINLINE void rgd3_mat2_identity(Rgd3Mat2* m)
{
	rgd3_mat2_set(m, 1.0f, 0.0f, 0.0f, 1.0f);
}

RGINLINE void rgd3_mat2_zero(Rgd3Mat2* m)
{
	rgd3_mat2_set(m, 0.0f, 0.0f, 0.0f, 0.0f);
}

RGINLINE Rgd3Vec2 rgd3_mat2_mul_vec(const Rgd3Mat2* m, Rgd3Vec2 v)
{
	return rgd3_vec2(m->mat[0].x * v.x + m->mat[0].y * v.y, m->mat[1].x * v.x + m->mat[1].y * v.y);
}

RGINLINE void rgd3_mat2_mul(const Rgd3Mat2* a, const Rgd3Mat2* b, Rgd3Mat2* out)
{
	Rgd3Mat2 r;
	r.mat[0].x = a->mat[0].x * b->mat[0].x + a->mat[0].y * b->mat[1].x;
	r.mat[0].y = a->mat[0].x * b->mat[0].y + a->mat[0].y * b->mat[1].y;
	r.mat[1].x = a->mat[1].x * b->mat[0].x + a->mat[1].y * b->mat[1].x;
	r.mat[1].y = a->mat[1].x * b->mat[0].y + a->mat[1].y * b->mat[1].y;
	*out = r;
}

RGINLINE void rgd3_mat2_transpose(const Rgd3Mat2* m, Rgd3Mat2* out)
{
	Rgd3Mat2 r;
	rgd3_mat2_set(&r, m->mat[0].x, m->mat[1].x, m->mat[0].y, m->mat[1].y);
	*out = r;
}

RGINLINE float rgd3_mat2_determinant(const Rgd3Mat2* m)
{
	return m->mat[0].x * m->mat[1].y - m->mat[0].y * m->mat[1].x;
}

RGINLINE float rgd3_mat2_trace(const Rgd3Mat2* m)
{
	return m->mat[0].x + m->mat[1].y;
}

/* idMat2::InverseSelf (double precision, fails below 1e-14) */
RGINLINE int rgd3_mat2_inverse_self(Rgd3Mat2* m)
{
	double det, invDet, a;
	det = m->mat[0].x * m->mat[1].y - m->mat[0].y * m->mat[1].x;
	if (fabsf((float)det) < RGD3_MATRIX_INVERSE_EPSILON)
		return 0;
	invDet = 1.0f / det;
	a = m->mat[0].x;
	m->mat[0].x = (float)(m->mat[1].y * invDet);
	m->mat[0].y = (float)(-m->mat[0].y * invDet);
	m->mat[1].x = (float)(-m->mat[1].x * invDet);
	m->mat[1].y = (float)(a * invDet);
	return 1;
}

RGINLINE int rgd3_mat2_inverse_fast_self(Rgd3Mat2* m)
{
	return rgd3_mat2_inverse_self(m);
}

RGINLINE int rgd3_mat2_compare_eps(const Rgd3Mat2* a, const Rgd3Mat2* b, float epsilon)
{
	return rgd3_vec2_compare_eps(a->mat[0], b->mat[0], epsilon) &&
		   rgd3_vec2_compare_eps(a->mat[1], b->mat[1], epsilon);
}

/* ===================================================================== */
/* Rgd3Mat3 (aggregate, rows are axes)                                    */
/* ===================================================================== */

RGINLINE void rgd3_mat3_set(Rgd3Mat3* m, float xx, float xy, float xz, float yx, float yy, float yz, float zx,
							float zy, float zz)
{
	m->mat[0] = rgd3_vec3(xx, xy, xz);
	m->mat[1] = rgd3_vec3(yx, yy, yz);
	m->mat[2] = rgd3_vec3(zx, zy, zz);
}

RGINLINE void rgd3_mat3_set_rows(Rgd3Mat3* m, Rgd3Vec3 x, Rgd3Vec3 y, Rgd3Vec3 z)
{
	m->mat[0] = x;
	m->mat[1] = y;
	m->mat[2] = z;
}

RGINLINE void rgd3_mat3_identity(Rgd3Mat3* m)
{
	rgd3_mat3_set(m, 1, 0, 0, 0, 1, 0, 0, 0, 1);
}

RGINLINE void rgd3_mat3_zero(Rgd3Mat3* m)
{
	rgd3_mat3_set(m, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/* m[row][col] */
RGINLINE float rgd3_mat3_get(const Rgd3Mat3* m, int row, int col)
{
	return m->mat[row].v[col];
}

/* local -> world: m.mat[0]*v.x + m.mat[1]*v.y + m.mat[2]*v.z.
   idMat3 `mat * vec`, `vec * mat`, `vec *= mat` and UnprojectVector all
   compute this. */
RGINLINE Rgd3Vec3 rgd3_mat3_mul_vec(const Rgd3Mat3* m, Rgd3Vec3 v)
{
	return rgd3_vec3(m->mat[0].x * v.x + m->mat[1].x * v.y + m->mat[2].x * v.z,
					 m->mat[0].y * v.x + m->mat[1].y * v.y + m->mat[2].y * v.z,
					 m->mat[0].z * v.x + m->mat[1].z * v.y + m->mat[2].z * v.z);
}

/* world -> local: ( v.mat[0], v.mat[1], v.mat[2] ) (idMat3::ProjectVector,
   also `vec * mat.Transpose()`) */
RGINLINE Rgd3Vec3 rgd3_mat3_transpose_mul_vec(const Rgd3Mat3* m, Rgd3Vec3 v)
{
	return rgd3_vec3(rgd3_vec3_dot(v, m->mat[0]), rgd3_vec3_dot(v, m->mat[1]), rgd3_vec3_dot(v, m->mat[2]));
}

RGINLINE Rgd3Vec3 rgd3_mat3_project_vector(const Rgd3Mat3* m, Rgd3Vec3 src)
{
	return rgd3_mat3_transpose_mul_vec(m, src);
}

/* idMat3::UnprojectVector: mat[0]*x + mat[1]*y + mat[2]*z, bit-identical
   to rgd3_mat3_mul_vec */
RGINLINE Rgd3Vec3 rgd3_mat3_unproject_vector(const Rgd3Mat3* m, Rgd3Vec3 src)
{
	return rgd3_vec3_add(rgd3_vec3_add(rgd3_vec3_scale(m->mat[0], src.x), rgd3_vec3_scale(m->mat[1], src.y)),
						 rgd3_vec3_scale(m->mat[2], src.z));
}

/* out = a * b (row-by-column on the 3x3 storage, idMat3::operator*) */
RGINLINE void rgd3_mat3_mul(const Rgd3Mat3* a, const Rgd3Mat3* b, Rgd3Mat3* out)
{
	Rgd3Mat3 r;
	int i, j;
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			r.mat[i].v[j] =
				a->mat[i].x * b->mat[0].v[j] + a->mat[i].y * b->mat[1].v[j] + a->mat[i].z * b->mat[2].v[j];
		}
	}
	*out = r;
}

RGINLINE void rgd3_mat3_scale(const Rgd3Mat3* m, float s, Rgd3Mat3* out)
{
	int i;
	for (i = 0; i < 3; i++)
		out->mat[i] = rgd3_vec3_scale(m->mat[i], s);
}

RGINLINE void rgd3_mat3_add(const Rgd3Mat3* a, const Rgd3Mat3* b, Rgd3Mat3* out)
{
	int i;
	for (i = 0; i < 3; i++)
		out->mat[i] = rgd3_vec3_add(a->mat[i], b->mat[i]);
}

RGINLINE void rgd3_mat3_sub(const Rgd3Mat3* a, const Rgd3Mat3* b, Rgd3Mat3* out)
{
	int i;
	for (i = 0; i < 3; i++)
		out->mat[i] = rgd3_vec3_sub(a->mat[i], b->mat[i]);
}

RGINLINE void rgd3_mat3_transpose(const Rgd3Mat3* m, Rgd3Mat3* out)
{
	Rgd3Mat3 r;
	rgd3_mat3_set(&r, m->mat[0].x, m->mat[1].x, m->mat[2].x, m->mat[0].y, m->mat[1].y, m->mat[2].y, m->mat[0].z,
				  m->mat[1].z, m->mat[2].z);
	*out = r;
}

RGINLINE void rgd3_mat3_transpose_self(Rgd3Mat3* m)
{
	rgd3_mat3_transpose(m, m);
}

/* out = a^T * b (idMat3::TransposeMultiply) */
RGINLINE void rgd3_mat3_transpose_multiply(const Rgd3Mat3* a, const Rgd3Mat3* b, Rgd3Mat3* out)
{
	Rgd3Mat3 r;
	int i, j;
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			r.mat[i].v[j] =
				a->mat[0].v[i] * b->mat[0].v[j] + a->mat[1].v[i] * b->mat[1].v[j] + a->mat[2].v[i] * b->mat[2].v[j];
		}
	}
	*out = r;
}

RGINLINE int rgd3_mat3_compare(const Rgd3Mat3* a, const Rgd3Mat3* b)
{
	return rgd3_vec3_compare(a->mat[0], b->mat[0]) && rgd3_vec3_compare(a->mat[1], b->mat[1]) &&
		   rgd3_vec3_compare(a->mat[2], b->mat[2]);
}

RGINLINE int rgd3_mat3_compare_eps(const Rgd3Mat3* a, const Rgd3Mat3* b, float epsilon)
{
	return rgd3_vec3_compare_eps(a->mat[0], b->mat[0], epsilon) &&
		   rgd3_vec3_compare_eps(a->mat[1], b->mat[1], epsilon) &&
		   rgd3_vec3_compare_eps(a->mat[2], b->mat[2], epsilon);
}

RGINLINE int rgd3_mat3_is_identity(const Rgd3Mat3* m, float epsilon)
{
	Rgd3Mat3 id;
	rgd3_mat3_identity(&id);
	return rgd3_mat3_compare_eps(m, &id, epsilon);
}

/* exact compare against identity */
RGINLINE int rgd3_mat3_is_rotated(const Rgd3Mat3* m)
{
	Rgd3Mat3 id;
	rgd3_mat3_identity(&id);
	return !rgd3_mat3_compare(m, &id);
}

RGINLINE float rgd3_mat3_trace(const Rgd3Mat3* m)
{
	return m->mat[0].x + m->mat[1].y + m->mat[2].z;
}

RGINLINE float rgd3_mat3_determinant(const Rgd3Mat3* m)
{
	float det2_12_01 = m->mat[1].x * m->mat[2].y - m->mat[1].y * m->mat[2].x;
	float det2_12_02 = m->mat[1].x * m->mat[2].z - m->mat[1].z * m->mat[2].x;
	float det2_12_12 = m->mat[1].y * m->mat[2].z - m->mat[1].z * m->mat[2].y;
	return m->mat[0].x * det2_12_12 - m->mat[0].y * det2_12_02 + m->mat[0].z * det2_12_01;
}

RGINLINE int rgd3_mat3_fix_degeneracies(Rgd3Mat3* m)
{
	int r = rgd3_vec3_fix_degenerate_normal(&m->mat[0]);
	r |= rgd3_vec3_fix_degenerate_normal(&m->mat[1]);
	r |= rgd3_vec3_fix_degenerate_normal(&m->mat[2]);
	return r;
}

RGINLINE int rgd3_mat3_fix_denormals(Rgd3Mat3* m)
{
	int r = rgd3_vec3_fix_denormals(&m->mat[0]);
	r |= rgd3_vec3_fix_denormals(&m->mat[1]);
	r |= rgd3_vec3_fix_denormals(&m->mat[2]);
	return r;
}

/* idMat3 SkewSymmetric */
RGINLINE void rgd3_mat3_skew_symmetric(Rgd3Vec3 src, Rgd3Mat3* out)
{
	rgd3_mat3_set(out, 0.0f, -src.z, src.y, src.z, 0.0f, -src.x, -src.y, src.x, 0.0f);
}

int rgd3_mat3_is_symmetric(const Rgd3Mat3* m, float epsilon);
int rgd3_mat3_is_diagonal(const Rgd3Mat3* m, float epsilon);
/* InverseSelf: double precision cofactors; 0 (matrix untouched) if
   |det| < 1e-14. inverse_fast uses the same code path as BFG does. */
int rgd3_mat3_inverse_self(Rgd3Mat3* m);
int rgd3_mat3_inverse_fast_self(Rgd3Mat3* m);
int rgd3_mat3_inverse(const Rgd3Mat3* m, Rgd3Mat3* out);
int rgd3_mat3_inverse_fast(const Rgd3Mat3* m, Rgd3Mat3* out);
/* OrthoNormalize (uses the original rows for the crosses) and
   OrthoNormalizeSelf (uses the already normalized rows) differ, as in idlib */
void rgd3_mat3_ortho_normalize(const Rgd3Mat3* m, Rgd3Mat3* out);
void rgd3_mat3_ortho_normalize_self(Rgd3Mat3* m);
/* pitch/yaw/roll each in [-180, 180] */
Rgd3Angles rgd3_mat3_to_angles(const Rgd3Mat3* m);
Rgd3Quat rgd3_mat3_to_quat(const Rgd3Mat3* m);
Rgd3CQuat rgd3_mat3_to_cquat(const Rgd3Mat3* m);
void rgd3_mat3_to_rotation(const Rgd3Mat3* m, Rgd3Rotation* out);
/* idMat3::ToMat4: transposed into the row-major 4x4 */
void rgd3_mat3_to_mat4(const Rgd3Mat3* m, Rgd3Mat4* out);
Rgd3Vec3 rgd3_mat3_to_angular_velocity(const Rgd3Mat3* m);

/* ===================================================================== */
/* Rgd3Mat4 (aggregate, row-major)                                        */
/* ===================================================================== */

RGINLINE void rgd3_mat4_identity(Rgd3Mat4* m)
{
	m->mat[0] = rgd3_vec4(1, 0, 0, 0);
	m->mat[1] = rgd3_vec4(0, 1, 0, 0);
	m->mat[2] = rgd3_vec4(0, 0, 1, 0);
	m->mat[3] = rgd3_vec4(0, 0, 0, 1);
}

RGINLINE void rgd3_mat4_zero(Rgd3Mat4* m)
{
	m->mat[0] = m->mat[1] = m->mat[2] = m->mat[3] = rgd3_vec4_zero();
}

/* idMat4( rotation, translation ): the idMat3 is transposed */
RGINLINE void rgd3_mat4_from_mat3_origin(Rgd3Mat4* m, const Rgd3Mat3* rotation, Rgd3Vec3 translation)
{
	m->mat[0] = rgd3_vec4(rotation->mat[0].x, rotation->mat[1].x, rotation->mat[2].x, translation.x);
	m->mat[1] = rgd3_vec4(rotation->mat[0].y, rotation->mat[1].y, rotation->mat[2].y, translation.y);
	m->mat[2] = rgd3_vec4(rotation->mat[0].z, rotation->mat[1].z, rotation->mat[2].z, translation.z);
	m->mat[3] = rgd3_vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

RGINLINE Rgd3Vec4 rgd3_mat4_mul_vec4(const Rgd3Mat4* m, Rgd3Vec4 v)
{
	return rgd3_vec4(rgd3_vec4_dot(m->mat[0], v), rgd3_vec4_dot(m->mat[1], v), rgd3_vec4_dot(m->mat[2], v),
					 rgd3_vec4_dot(m->mat[3], v));
}

/* idMat4 * idVec3: w = 1, divided by the resulting w (zero vector if 0) */
RGINLINE Rgd3Vec3 rgd3_mat4_mul_vec3(const Rgd3Mat4* m, Rgd3Vec3 v)
{
	const Rgd3Vec4* r = m->mat;
	float s = r[3].x * v.x + r[3].y * v.y + r[3].z * v.z + r[3].w;
	if (s == 0.0f)
		return rgd3_vec3_zero();
	if (s == 1.0f)
	{
		return rgd3_vec3(r[0].x * v.x + r[0].y * v.y + r[0].z * v.z + r[0].w,
						 r[1].x * v.x + r[1].y * v.y + r[1].z * v.z + r[1].w,
						 r[2].x * v.x + r[2].y * v.y + r[2].z * v.z + r[2].w);
	}
	else
	{
		float invS = 1.0f / s;
		return rgd3_vec3((r[0].x * v.x + r[0].y * v.y + r[0].z * v.z + r[0].w) * invS,
						 (r[1].x * v.x + r[1].y * v.y + r[1].z * v.z + r[1].w) * invS,
						 (r[2].x * v.x + r[2].y * v.y + r[2].z * v.z + r[2].w) * invS);
	}
}

RGINLINE void rgd3_mat4_mul(const Rgd3Mat4* a, const Rgd3Mat4* b, Rgd3Mat4* out)
{
	Rgd3Mat4 r;
	int i, j;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			r.mat[i].v[j] = a->mat[i].x * b->mat[0].v[j] + a->mat[i].y * b->mat[1].v[j] +
							a->mat[i].z * b->mat[2].v[j] + a->mat[i].w * b->mat[3].v[j];
		}
	}
	*out = r;
}

RGINLINE void rgd3_mat4_transpose(const Rgd3Mat4* m, Rgd3Mat4* out)
{
	Rgd3Mat4 r;
	int i, j;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
			r.mat[i].v[j] = m->mat[j].v[i];
	}
	*out = r;
}

RGINLINE void rgd3_mat4_transpose_self(Rgd3Mat4* m)
{
	rgd3_mat4_transpose(m, m);
}

RGINLINE float rgd3_mat4_trace(const Rgd3Mat4* m)
{
	return m->mat[0].x + m->mat[1].y + m->mat[2].z + m->mat[3].w;
}

RGINLINE int rgd3_mat4_compare(const Rgd3Mat4* a, const Rgd3Mat4* b)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if (!rgd3_vec4_compare(a->mat[i], b->mat[i]))
			return 0;
	}
	return 1;
}

RGINLINE int rgd3_mat4_compare_eps(const Rgd3Mat4* a, const Rgd3Mat4* b, float epsilon)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if (!rgd3_vec4_compare_eps(a->mat[i], b->mat[i], epsilon))
			return 0;
	}
	return 1;
}

RGINLINE int rgd3_mat4_is_identity(const Rgd3Mat4* m, float epsilon)
{
	Rgd3Mat4 id;
	rgd3_mat4_identity(&id);
	return rgd3_mat4_compare_eps(m, &id, epsilon);
}

float rgd3_mat4_determinant(const Rgd3Mat4* m);
/* InverseSelf: cofactor expansion, double invDet */
int rgd3_mat4_inverse_self(Rgd3Mat4* m);
/* InverseFastSelf: BFG's 2x2 block partition, float */
int rgd3_mat4_inverse_fast_self(Rgd3Mat4* m);
int rgd3_mat4_inverse(const Rgd3Mat4* m, Rgd3Mat4* out);
int rgd3_mat4_inverse_fast(const Rgd3Mat4* m, Rgd3Mat4* out);

/* ===================================================================== */
/* Rgd3Quat / Rgd3CQuat (value)                                           */
/* ===================================================================== */

RGINLINE Rgd3Quat rgd3_quat(float x, float y, float z, float w)
{
	Rgd3Quat r;
	r.x = x;
	r.y = y;
	r.z = z;
	r.w = w;
	return r;
}

RGINLINE Rgd3Quat rgd3_quat_identity(void)
{
	return rgd3_quat(0.0f, 0.0f, 0.0f, 1.0f);
}

RGINLINE Rgd3Quat rgd3_quat_neg(Rgd3Quat a)
{
	return rgd3_quat(-a.x, -a.y, -a.z, -a.w);
}

RGINLINE Rgd3Quat rgd3_quat_add(Rgd3Quat a, Rgd3Quat b)
{
	return rgd3_quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

RGINLINE Rgd3Quat rgd3_quat_sub(Rgd3Quat a, Rgd3Quat b)
{
	return rgd3_quat(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

RGINLINE Rgd3Quat rgd3_quat_scale(Rgd3Quat a, float s)
{
	return rgd3_quat(a.x * s, a.y * s, a.z * s, a.w * s);
}

/* idQuat::operator*( idQuat ) */
RGINLINE Rgd3Quat rgd3_quat_mul(Rgd3Quat a, Rgd3Quat b)
{
	return rgd3_quat(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,
					 a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

/* idQuat::operator*( idVec3 ) == vec * quat; same result as
   rgd3_mat3_mul_vec( quat_to_mat3( q ), v ) */
RGINLINE Rgd3Vec3 rgd3_quat_rotate(Rgd3Quat q, Rgd3Vec3 a)
{
	float xxzz = q.x * q.x - q.z * q.z;
	float wwyy = q.w * q.w - q.y * q.y;
	float xw2 = q.x * q.w * 2.0f;
	float xy2 = q.x * q.y * 2.0f;
	float xz2 = q.x * q.z * 2.0f;
	float yw2 = q.y * q.w * 2.0f;
	float yz2 = q.y * q.z * 2.0f;
	float zw2 = q.z * q.w * 2.0f;
	return rgd3_vec3((xxzz + wwyy) * a.x + (xy2 + zw2) * a.y + (xz2 - yw2) * a.z,
					 (xy2 - zw2) * a.x + (q.y * q.y + q.w * q.w - q.x * q.x - q.z * q.z) * a.y + (yz2 + xw2) * a.z,
					 (xz2 + yw2) * a.x + (yz2 - xw2) * a.y + (wwyy - xxzz) * a.z);
}

RGINLINE int rgd3_quat_compare(Rgd3Quat a, Rgd3Quat b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

RGINLINE int rgd3_quat_compare_eps(Rgd3Quat a, Rgd3Quat b, float epsilon)
{
	if (fabsf(a.x - b.x) > epsilon)
		return 0;
	if (fabsf(a.y - b.y) > epsilon)
		return 0;
	if (fabsf(a.z - b.z) > epsilon)
		return 0;
	if (fabsf(a.w - b.w) > epsilon)
		return 0;
	return 1;
}

/* conjugate */
RGINLINE Rgd3Quat rgd3_quat_inverse(Rgd3Quat a)
{
	return rgd3_quat(-a.x, -a.y, -a.z, a.w);
}

RGINLINE float rgd3_quat_length(Rgd3Quat a)
{
	return rgd3_math_sqrt(a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w);
}

/* idQuat::Normalize (zero quaternion left alone) */
RGINLINE Rgd3Quat rgd3_quat_normalize(Rgd3Quat a)
{
	float len = rgd3_quat_length(a);
	if (len)
	{
		float ilength = 1 / len;
		a.x *= ilength;
		a.y *= ilength;
		a.z *= ilength;
		a.w *= ilength;
	}
	return a;
}

/* w from x/y/z, abs() guards rounding past 1 */
RGINLINE float rgd3_quat_calc_w(Rgd3Quat a)
{
	return sqrtf(fabsf(1.0f - (a.x * a.x + a.y * a.y + a.z * a.z)));
}

RGINLINE Rgd3CQuat rgd3_cquat(float x, float y, float z)
{
	Rgd3CQuat r;
	r.x = x;
	r.y = y;
	r.z = z;
	return r;
}

/* negates the vector part when w < 0 so w can be rebuilt as >= 0 */
RGINLINE Rgd3CQuat rgd3_quat_to_cquat(Rgd3Quat a)
{
	if (a.w < 0.0f)
		return rgd3_cquat(-a.x, -a.y, -a.z);
	return rgd3_cquat(a.x, a.y, a.z);
}

RGINLINE Rgd3Quat rgd3_cquat_to_quat(Rgd3CQuat a)
{
	return rgd3_quat(a.x, a.y, a.z, sqrtf(fabsf(1.0f - (a.x * a.x + a.y * a.y + a.z * a.z))));
}

RGINLINE int rgd3_cquat_compare(Rgd3CQuat a, Rgd3CQuat b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

RGINLINE int rgd3_cquat_compare_eps(Rgd3CQuat a, Rgd3CQuat b, float epsilon)
{
	if (fabsf(a.x - b.x) > epsilon)
		return 0;
	if (fabsf(a.y - b.y) > epsilon)
		return 0;
	if (fabsf(a.z - b.z) > epsilon)
		return 0;
	return 1;
}

void rgd3_quat_to_mat3(const Rgd3Quat* q, Rgd3Mat3* out);
void rgd3_quat_to_mat4(const Rgd3Quat* q, Rgd3Mat4* out);
Rgd3Angles rgd3_quat_to_angles(const Rgd3Quat* q);
void rgd3_quat_to_rotation(const Rgd3Quat* q, Rgd3Rotation* out);
Rgd3Vec3 rgd3_quat_to_angular_velocity(const Rgd3Quat* q);
/* idQuat::Slerp (uses Sin16/ATan16 like the original) */
Rgd3Quat rgd3_quat_slerp(const Rgd3Quat* from, const Rgd3Quat* to, float t);
/* idQuat::Lerp: normalized lerp along the shorter arc */
Rgd3Quat rgd3_quat_lerp(const Rgd3Quat* from, const Rgd3Quat* to, float t);
void rgd3_cquat_to_mat3(const Rgd3CQuat* q, Rgd3Mat3* out);
Rgd3Angles rgd3_cquat_to_angles(const Rgd3CQuat* q);
void rgd3_cquat_to_rotation(const Rgd3CQuat* q, Rgd3Rotation* out);

/* ===================================================================== */
/* Rgd3Angles (value; degrees)                                            */
/* ===================================================================== */

RGINLINE Rgd3Angles rgd3_angles(float pitch, float yaw, float roll)
{
	Rgd3Angles r;
	r.pitch = pitch;
	r.yaw = yaw;
	r.roll = roll;
	return r;
}

RGINLINE Rgd3Angles rgd3_angles_zero(void)
{
	return rgd3_angles(0.0f, 0.0f, 0.0f);
}

RGINLINE Rgd3Angles rgd3_angles_from_vec3(Rgd3Vec3 v)
{
	return rgd3_angles(v.x, v.y, v.z);
}

RGINLINE Rgd3Angles rgd3_angles_add(Rgd3Angles a, Rgd3Angles b)
{
	return rgd3_angles(a.pitch + b.pitch, a.yaw + b.yaw, a.roll + b.roll);
}

RGINLINE Rgd3Angles rgd3_angles_sub(Rgd3Angles a, Rgd3Angles b)
{
	return rgd3_angles(a.pitch - b.pitch, a.yaw - b.yaw, a.roll - b.roll);
}

RGINLINE Rgd3Angles rgd3_angles_scale(Rgd3Angles a, float s)
{
	return rgd3_angles(a.pitch * s, a.yaw * s, a.roll * s);
}

/* negates the angles; in general not the inverse rotation */
RGINLINE Rgd3Angles rgd3_angles_neg(Rgd3Angles a)
{
	return rgd3_angles(-a.pitch, -a.yaw, -a.roll);
}

RGINLINE int rgd3_angles_compare(Rgd3Angles a, Rgd3Angles b)
{
	return a.pitch == b.pitch && a.yaw == b.yaw && a.roll == b.roll;
}

RGINLINE int rgd3_angles_compare_eps(Rgd3Angles a, Rgd3Angles b, float epsilon)
{
	if (fabsf(a.pitch - b.pitch) > epsilon)
		return 0;
	if (fabsf(a.yaw - b.yaw) > epsilon)
		return 0;
	if (fabsf(a.roll - b.roll) > epsilon)
		return 0;
	return 1;
}

RGINLINE Rgd3Angles rgd3_angles_clamp(Rgd3Angles a, Rgd3Angles min, Rgd3Angles max)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		if (a.v[i] < min.v[i])
			a.v[i] = min.v[i];
		else if (a.v[i] > max.v[i])
			a.v[i] = max.v[i];
	}
	return a;
}

RGINLINE Rgd3Vec3 rgd3_angles_to_vec3(Rgd3Angles a)
{
	return rgd3_vec3(a.pitch, a.yaw, a.roll);
}

/* [0, 360) */
void rgd3_angles_normalize360(Rgd3Angles* a);
/* (-180, 180] */
void rgd3_angles_normalize180(Rgd3Angles* a);
/* any of forward/right/up may be NULL */
void rgd3_angles_to_vectors(const Rgd3Angles* a, Rgd3Vec3* forward, Rgd3Vec3* right, Rgd3Vec3* up);
Rgd3Vec3 rgd3_angles_to_forward(const Rgd3Angles* a);
Rgd3Quat rgd3_angles_to_quat(const Rgd3Angles* a);
void rgd3_angles_to_rotation(const Rgd3Angles* a, Rgd3Rotation* out);
void rgd3_angles_to_mat3(const Rgd3Angles* a, Rgd3Mat3* out);
void rgd3_angles_to_mat4(const Rgd3Angles* a, Rgd3Mat4* out);
Rgd3Vec3 rgd3_angles_to_angular_velocity(const Rgd3Angles* a);

/* ===================================================================== */
/* Rgd3Rotation (aggregate)                                               */
/* ===================================================================== */

RGINLINE void rgd3_rotation_set(Rgd3Rotation* r, Rgd3Vec3 origin, Rgd3Vec3 vec, float angle)
{
	r->origin = origin;
	r->vec = vec;
	r->angle = angle;
	r->axisValid = 0;
}

RGINLINE void rgd3_rotation_set_origin(Rgd3Rotation* r, Rgd3Vec3 origin)
{
	r->origin = origin;
}

/* vec has to be normalized */
RGINLINE void rgd3_rotation_set_vec(Rgd3Rotation* r, Rgd3Vec3 vec)
{
	r->vec = vec;
	r->axisValid = 0;
}

RGINLINE void rgd3_rotation_set_angle(Rgd3Rotation* r, float angle)
{
	r->angle = angle;
	r->axisValid = 0;
}

RGINLINE void rgd3_rotation_scale(Rgd3Rotation* r, float s)
{
	r->angle *= s;
	r->axisValid = 0;
}

/* flips the rotation */
RGINLINE void rgd3_rotation_negate(Rgd3Rotation* r)
{
	r->angle = -r->angle;
	r->axisValid = 0;
}

/* computes and caches the matrix (idRotation::ToMat3's mutable axis) */
const Rgd3Mat3* rgd3_rotation_to_mat3(Rgd3Rotation* r);
/* const variant: uses the cache when valid, never writes it */
void rgd3_rotation_get_mat3(const Rgd3Rotation* r, Rgd3Mat3* out);
void rgd3_rotation_to_mat4(const Rgd3Rotation* r, Rgd3Mat4* out);
Rgd3Quat rgd3_rotation_to_quat(const Rgd3Rotation* r);
Rgd3Angles rgd3_rotation_to_angles(const Rgd3Rotation* r);
Rgd3Vec3 rgd3_rotation_to_angular_velocity(const Rgd3Rotation* r);
/* ( p - origin ) * axis + origin */
Rgd3Vec3 rgd3_rotation_rotate_point(const Rgd3Rotation* r, const Rgd3Vec3* p);
void rgd3_rotation_normalize180(Rgd3Rotation* r);
void rgd3_rotation_normalize360(Rgd3Rotation* r);

/* ===================================================================== */
/* Rgd3Plane (value)                                                      */
/* ===================================================================== */

RGINLINE Rgd3Plane rgd3_plane(float a, float b, float c, float d)
{
	Rgd3Plane r;
	r.a = a;
	r.b = b;
	r.c = c;
	r.d = d;
	return r;
}

/* idPlane( normal, dist ): d = -dist */
RGINLINE Rgd3Plane rgd3_plane_from_normal_dist(Rgd3Vec3 normal, float dist)
{
	return rgd3_plane(normal.x, normal.y, normal.z, -dist);
}

RGINLINE Rgd3Plane rgd3_plane_zero(void)
{
	return rgd3_plane(0.0f, 0.0f, 0.0f, 0.0f);
}

/* flips the plane */
RGINLINE Rgd3Plane rgd3_plane_neg(Rgd3Plane p)
{
	return rgd3_plane(-p.a, -p.b, -p.c, -p.d);
}

RGINLINE Rgd3Plane rgd3_plane_add(Rgd3Plane p, Rgd3Plane q)
{
	return rgd3_plane(p.a + q.a, p.b + q.b, p.c + q.c, p.d + q.d);
}

RGINLINE Rgd3Plane rgd3_plane_sub(Rgd3Plane p, Rgd3Plane q)
{
	return rgd3_plane(p.a - q.a, p.b - q.b, p.c - q.c, p.d - q.d);
}

RGINLINE Rgd3Plane rgd3_plane_scale(Rgd3Plane p, float s)
{
	return rgd3_plane(p.a * s, p.b * s, p.c * s, p.d * s);
}

RGINLINE Rgd3Vec3 rgd3_plane_normal(Rgd3Plane p)
{
	return rgd3_vec3(p.a, p.b, p.c);
}

RGINLINE void rgd3_plane_set_normal(Rgd3Plane* p, Rgd3Vec3 normal)
{
	p->a = normal.x;
	p->b = normal.y;
	p->c = normal.z;
}

/* idPlane::Dist: returns -d */
RGINLINE float rgd3_plane_dist(Rgd3Plane p)
{
	return -p.d;
}

/* idPlane::SetDist: d = -dist */
RGINLINE void rgd3_plane_set_dist(Rgd3Plane* p, float dist)
{
	p->d = -dist;
}

RGINLINE Rgd3Vec4 rgd3_plane_to_vec4(Rgd3Plane p)
{
	return rgd3_vec4(p.a, p.b, p.c, p.d);
}

RGINLINE int rgd3_plane_compare(Rgd3Plane p, Rgd3Plane q)
{
	return p.a == q.a && p.b == q.b && p.c == q.c && p.d == q.d;
}

RGINLINE int rgd3_plane_compare_eps(Rgd3Plane p, Rgd3Plane q, float epsilon)
{
	if (fabsf(p.a - q.a) > epsilon)
		return 0;
	if (fabsf(p.b - q.b) > epsilon)
		return 0;
	if (fabsf(p.c - q.c) > epsilon)
		return 0;
	if (fabsf(p.d - q.d) > epsilon)
		return 0;
	return 1;
}

RGINLINE int rgd3_plane_compare_eps2(Rgd3Plane p, Rgd3Plane q, float normalEps, float distEps)
{
	if (fabsf(p.d - q.d) > distEps)
		return 0;
	if (!rgd3_vec3_compare_eps(rgd3_plane_normal(p), rgd3_plane_normal(q), normalEps))
		return 0;
	return 1;
}

RGINLINE int rgd3_plane_fix_degenerate_normal(Rgd3Plane* p)
{
	Rgd3Vec3 n = rgd3_plane_normal(*p);
	int r = rgd3_vec3_fix_degenerate_normal(&n);
	rgd3_plane_set_normal(p, n);
	return r;
}

/* only normalizes the normal, does not adjust d; returns the old length
   (fixDegenerate is 1 in idlib's default) */
RGINLINE float rgd3_plane_normalize(Rgd3Plane* p, int fixDegenerate)
{
	Rgd3Vec3 n = rgd3_plane_normal(*p);
	float length = rgd3_vec3_normalize(&n);
	rgd3_plane_set_normal(p, n);
	if (fixDegenerate)
		rgd3_plane_fix_degenerate_normal(p);
	return length;
}

/* fixes d only when the normal was degenerate */
RGINLINE int rgd3_plane_fix_degeneracies(Rgd3Plane* p, float distEpsilon)
{
	int fixedNormal = rgd3_plane_fix_degenerate_normal(p);
	if (fixedNormal)
	{
		if (fabsf(p->d - rgd3_math_rint(p->d)) < distEpsilon)
			p->d = rgd3_math_rint(p->d);
	}
	return fixedNormal;
}

/* normal = ( p1 - p2 ) x ( p3 - p2 ), d through p2; 0 if degenerate */
RGINLINE int rgd3_plane_from_points(Rgd3Plane* p, Rgd3Vec3 p1, Rgd3Vec3 p2, Rgd3Vec3 p3, int fixDegenerate)
{
	rgd3_plane_set_normal(p, rgd3_vec3_cross(rgd3_vec3_sub(p1, p2), rgd3_vec3_sub(p3, p2)));
	if (rgd3_plane_normalize(p, fixDegenerate) == 0.0f)
		return 0;
	p->d = -rgd3_vec3_dot(rgd3_plane_normal(*p), p2);
	return 1;
}

RGINLINE int rgd3_plane_from_vecs(Rgd3Plane* p, Rgd3Vec3 dir1, Rgd3Vec3 dir2, Rgd3Vec3 point, int fixDegenerate)
{
	rgd3_plane_set_normal(p, rgd3_vec3_cross(dir1, dir2));
	if (rgd3_plane_normalize(p, fixDegenerate) == 0.0f)
		return 0;
	p->d = -rgd3_vec3_dot(rgd3_plane_normal(*p), point);
	return 1;
}

/* assumes the normal is valid */
RGINLINE void rgd3_plane_fit_through_point(Rgd3Plane* p, Rgd3Vec3 point)
{
	p->d = -rgd3_vec3_dot(rgd3_plane_normal(*p), point);
}

RGINLINE float rgd3_plane_distance(Rgd3Plane p, Rgd3Vec3 v)
{
	return p.a * v.x + p.b * v.y + p.c * v.z + p.d;
}

/* PLANESIDE_FRONT / BACK / ON (epsilon 0.0f is idlib's default) */
RGINLINE int rgd3_plane_side(Rgd3Plane p, Rgd3Vec3 v, float epsilon)
{
	float dist = rgd3_plane_distance(p, v);
	if (dist > epsilon)
		return RGD3_PLANESIDE_FRONT;
	else if (dist < -epsilon)
		return RGD3_PLANESIDE_BACK;
	return RGD3_PLANESIDE_ON;
}

RGINLINE Rgd3Plane rgd3_plane_translate(Rgd3Plane p, Rgd3Vec3 translation)
{
	return rgd3_plane(p.a, p.b, p.c, p.d - rgd3_vec3_dot(translation, rgd3_plane_normal(p)));
}

/* idPlane::Rotate: normal = mul_vec( axis, normal ) */
RGINLINE Rgd3Plane rgd3_plane_rotate(Rgd3Plane p, Rgd3Vec3 origin, const Rgd3Mat3* axis)
{
	Rgd3Plane r;
	Rgd3Vec3 n = rgd3_mat3_mul_vec(axis, rgd3_plane_normal(p));
	rgd3_plane_set_normal(&r, n);
	r.d = p.d + rgd3_vec3_dot(origin, rgd3_plane_normal(p)) - rgd3_vec3_dot(origin, n);
	return r;
}

/* idPlane::operator*=( idMat3 ): rotates the normal only */
RGINLINE Rgd3Plane rgd3_plane_mul_mat3(Rgd3Plane p, const Rgd3Mat3* m)
{
	rgd3_plane_set_normal(&p, rgd3_mat3_mul_vec(m, rgd3_plane_normal(p)));
	return p;
}

/* 1 if the segment crosses the plane */
RGINLINE int rgd3_plane_line_intersection(Rgd3Plane p, Rgd3Vec3 start, Rgd3Vec3 end)
{
	float d1, d2, fraction;
	d1 = rgd3_vec3_dot(rgd3_plane_normal(p), start) + p.d;
	d2 = rgd3_vec3_dot(rgd3_plane_normal(p), end) + p.d;
	if (d1 == d2)
		return 0;
	if (d1 > 0.0f && d2 > 0.0f)
		return 0;
	if (d1 < 0.0f && d2 < 0.0f)
		return 0;
	fraction = (d1 / (d1 - d2));
	return (fraction >= 0.0f && fraction <= 1.0f);
}

/* intersection point is start + dir * scale */
RGINLINE int rgd3_plane_ray_intersection(Rgd3Plane p, Rgd3Vec3 start, Rgd3Vec3 dir, float* scale)
{
	float d1, d2;
	d1 = rgd3_vec3_dot(rgd3_plane_normal(p), start) + p.d;
	d2 = rgd3_vec3_dot(rgd3_plane_normal(p), dir);
	if (d2 == 0.0f)
		return 0;
	*scale = -(d1 / d2);
	return 1;
}

int rgd3_plane_type(const Rgd3Plane* p);
/* line of intersection: start + t * dir */
int rgd3_plane_plane_intersection(const Rgd3Plane* p, const Rgd3Plane* other, Rgd3Vec3* start, Rgd3Vec3* dir);
int rgd3_plane_height_fit(Rgd3Plane* p, const Rgd3Vec3* points, int numPoints);

/* ===================================================================== */
/* Rgd3Pluecker (aggregate)                                               */
/* ===================================================================== */

RGINLINE void rgd3_pluecker_set(Rgd3Pluecker* pl, float a1, float a2, float a3, float a4, float a5, float a6)
{
	pl->p[0] = a1;
	pl->p[1] = a2;
	pl->p[2] = a3;
	pl->p[3] = a4;
	pl->p[4] = a5;
	pl->p[5] = a6;
}

RGINLINE void rgd3_pluecker_zero(Rgd3Pluecker* pl)
{
	rgd3_pluecker_set(pl, 0, 0, 0, 0, 0, 0);
}

RGINLINE void rgd3_pluecker_from_line(Rgd3Pluecker* pl, Rgd3Vec3 start, Rgd3Vec3 end)
{
	pl->p[0] = start.x * end.y - end.x * start.y;
	pl->p[1] = start.x * end.z - end.x * start.z;
	pl->p[2] = start.x - end.x;
	pl->p[3] = start.y * end.z - end.y * start.z;
	pl->p[4] = start.z - end.z;
	pl->p[5] = end.y - start.y;
}

RGINLINE void rgd3_pluecker_from_ray(Rgd3Pluecker* pl, Rgd3Vec3 start, Rgd3Vec3 dir)
{
	pl->p[0] = start.x * dir.y - dir.x * start.y;
	pl->p[1] = start.x * dir.z - dir.x * start.z;
	pl->p[2] = -dir.x;
	pl->p[3] = start.y * dir.z - dir.y * start.z;
	pl->p[4] = -dir.z;
	pl->p[5] = dir.y;
}

/* idPluecker::operator*( idPluecker ) */
RGINLINE float rgd3_pluecker_permuted_inner_product(const Rgd3Pluecker* a, const Rgd3Pluecker* b)
{
	return a->p[0] * b->p[4] + a->p[1] * b->p[5] + a->p[2] * b->p[3] + a->p[4] * b->p[0] + a->p[5] * b->p[1] +
		   a->p[3] * b->p[2];
}

RGINLINE Rgd3Vec3 rgd3_pluecker_to_dir(const Rgd3Pluecker* pl)
{
	return rgd3_vec3(-pl->p[2], pl->p[5], -pl->p[4]);
}

RGINLINE float rgd3_pluecker_length_sqr(const Rgd3Pluecker* pl)
{
	return pl->p[5] * pl->p[5] + pl->p[4] * pl->p[4] + pl->p[2] * pl->p[2];
}

RGINLINE float rgd3_pluecker_length(const Rgd3Pluecker* pl)
{
	return rgd3_math_sqrt(rgd3_pluecker_length_sqr(pl));
}

/* returns the old length, 0 (untouched) if not a line */
RGINLINE float rgd3_pluecker_normalize_self(Rgd3Pluecker* pl)
{
	float l = rgd3_pluecker_length_sqr(pl);
	float d;
	int i;
	if (l == 0.0f)
		return l;
	d = rgd3_math_inv_sqrt(l);
	for (i = 0; i < 6; i++)
		pl->p[i] *= d;
	return d * l;
}

RGINLINE int rgd3_pluecker_compare_eps(const Rgd3Pluecker* a, const Rgd3Pluecker* b, float epsilon)
{
	int i;
	for (i = 0; i < 6; i++)
	{
		if (fabsf(a->p[i] - b->p[i]) > epsilon)
			return 0;
	}
	return 1;
}

int rgd3_pluecker_from_planes(Rgd3Pluecker* pl, const Rgd3Plane* p1, const Rgd3Plane* p2);
int rgd3_pluecker_to_line(const Rgd3Pluecker* pl, Rgd3Vec3* start, Rgd3Vec3* end);
int rgd3_pluecker_to_ray(const Rgd3Pluecker* pl, Rgd3Vec3* start, Rgd3Vec3* dir);
/* squared distance between two lines, -1 for parallel lines (idlib FIXME) */
float rgd3_pluecker_distance3d_sqr(const Rgd3Pluecker* a, const Rgd3Pluecker* b);

/* ===================================================================== */
/* Rgd3Bounds (aggregate, b[0] = mins, b[1] = maxs)                       */
/* ===================================================================== */

RGINLINE void rgd3_bounds_set(Rgd3Bounds* b, Rgd3Vec3 mins, Rgd3Vec3 maxs)
{
	b->b[0] = mins;
	b->b[1] = maxs;
}

RGINLINE void rgd3_bounds_from_point(Rgd3Bounds* b, Rgd3Vec3 point)
{
	b->b[0] = point;
	b->b[1] = point;
}

/* inside out: mins = +1e30, maxs = -1e30 */
RGINLINE void rgd3_bounds_clear(Rgd3Bounds* b)
{
	b->b[0] = rgd3_vec3_splat(RGD3_MATH_INFINITY);
	b->b[1] = rgd3_vec3_splat(-RGD3_MATH_INFINITY);
}

/* single point at the origin */
RGINLINE void rgd3_bounds_zero(Rgd3Bounds* b)
{
	b->b[0] = rgd3_vec3_zero();
	b->b[1] = rgd3_vec3_zero();
}

RGINLINE int rgd3_bounds_is_cleared(const Rgd3Bounds* b)
{
	return b->b[0].x > b->b[1].x;
}

RGINLINE Rgd3Vec3 rgd3_bounds_get_center(const Rgd3Bounds* b)
{
	return rgd3_vec3((b->b[1].x + b->b[0].x) * 0.5f, (b->b[1].y + b->b[0].y) * 0.5f, (b->b[1].z + b->b[0].z) * 0.5f);
}

RGINLINE float rgd3_bounds_volume(const Rgd3Bounds* b)
{
	if (b->b[0].x >= b->b[1].x || b->b[0].y >= b->b[1].y || b->b[0].z >= b->b[1].z)
		return 0.0f;
	return (b->b[1].x - b->b[0].x) * (b->b[1].y - b->b[0].y) * (b->b[1].z - b->b[0].z);
}

/* returns 1 if the bounds expanded */
RGINLINE int rgd3_bounds_add_point(Rgd3Bounds* b, Rgd3Vec3 v)
{
	int expanded = 0;
	int i;
	for (i = 0; i < 3; i++)
	{
		if (v.v[i] < b->b[0].v[i])
		{
			b->b[0].v[i] = v.v[i];
			expanded = 1;
		}
		if (v.v[i] > b->b[1].v[i])
		{
			b->b[1].v[i] = v.v[i];
			expanded = 1;
		}
	}
	return expanded;
}

/* returns 1 if the bounds expanded */
RGINLINE int rgd3_bounds_add_bounds(Rgd3Bounds* b, const Rgd3Bounds* a)
{
	int expanded = 0;
	int i;
	for (i = 0; i < 3; i++)
	{
		if (a->b[0].v[i] < b->b[0].v[i])
		{
			b->b[0].v[i] = a->b[0].v[i];
			expanded = 1;
		}
	}
	for (i = 0; i < 3; i++)
	{
		if (a->b[1].v[i] > b->b[1].v[i])
		{
			b->b[1].v[i] = a->b[1].v[i];
			expanded = 1;
		}
	}
	return expanded;
}

RGINLINE void rgd3_bounds_intersect(const Rgd3Bounds* a, const Rgd3Bounds* b, Rgd3Bounds* out)
{
	Rgd3Bounds n;
	int i;
	for (i = 0; i < 3; i++)
	{
		n.b[0].v[i] = (b->b[0].v[i] > a->b[0].v[i]) ? b->b[0].v[i] : a->b[0].v[i];
		n.b[1].v[i] = (b->b[1].v[i] < a->b[1].v[i]) ? b->b[1].v[i] : a->b[1].v[i];
	}
	*out = n;
}

RGINLINE void rgd3_bounds_intersect_self(Rgd3Bounds* b, const Rgd3Bounds* a)
{
	rgd3_bounds_intersect(b, a, b);
}

RGINLINE void rgd3_bounds_expand(const Rgd3Bounds* b, float d, Rgd3Bounds* out)
{
	Rgd3Bounds n;
	n.b[0] = rgd3_vec3(b->b[0].x - d, b->b[0].y - d, b->b[0].z - d);
	n.b[1] = rgd3_vec3(b->b[1].x + d, b->b[1].y + d, b->b[1].z + d);
	*out = n;
}

RGINLINE void rgd3_bounds_expand_self(Rgd3Bounds* b, float d)
{
	rgd3_bounds_expand(b, d, b);
}

RGINLINE void rgd3_bounds_translate(const Rgd3Bounds* b, Rgd3Vec3 translation, Rgd3Bounds* out)
{
	Rgd3Bounds n;
	n.b[0] = rgd3_vec3_add(b->b[0], translation);
	n.b[1] = rgd3_vec3_add(b->b[1], translation);
	*out = n;
}

RGINLINE void rgd3_bounds_translate_self(Rgd3Bounds* b, Rgd3Vec3 translation)
{
	rgd3_bounds_translate(b, translation, b);
}

/* includes touching */
RGINLINE int rgd3_bounds_contains_point(const Rgd3Bounds* b, Rgd3Vec3 p)
{
	if (p.x < b->b[0].x || p.y < b->b[0].y || p.z < b->b[0].z || p.x > b->b[1].x || p.y > b->b[1].y ||
		p.z > b->b[1].z)
		return 0;
	return 1;
}

/* includes touching */
RGINLINE int rgd3_bounds_intersects_bounds(const Rgd3Bounds* b, const Rgd3Bounds* a)
{
	if (a->b[1].x < b->b[0].x || a->b[1].y < b->b[0].y || a->b[1].z < b->b[0].z || a->b[0].x > b->b[1].x ||
		a->b[0].y > b->b[1].y || a->b[0].z > b->b[1].z)
		return 0;
	return 1;
}

RGINLINE int rgd3_bounds_compare(const Rgd3Bounds* a, const Rgd3Bounds* b)
{
	return rgd3_vec3_compare(a->b[0], b->b[0]) && rgd3_vec3_compare(a->b[1], b->b[1]);
}

RGINLINE int rgd3_bounds_compare_eps(const Rgd3Bounds* a, const Rgd3Bounds* b, float epsilon)
{
	return rgd3_vec3_compare_eps(a->b[0], b->b[0], epsilon) && rgd3_vec3_compare_eps(a->b[1], b->b[1], epsilon);
}

/* projection of the bounds onto dir */
RGINLINE void rgd3_bounds_axis_projection(const Rgd3Bounds* b, Rgd3Vec3 dir, float* min, float* max)
{
	Rgd3Vec3 center = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	Rgd3Vec3 extents = rgd3_vec3_sub(b->b[1], center);
	float d1 = rgd3_vec3_dot(dir, center);
	float d2 = fabsf(extents.x * dir.x) + fabsf(extents.y * dir.y) + fabsf(extents.z * dir.z);
	*min = d1 - d2;
	*max = d1 + d2;
}

/* projection of the bounds placed at origin/axis onto dir */
RGINLINE void rgd3_bounds_axis_projection_transformed(const Rgd3Bounds* b, Rgd3Vec3 origin, const Rgd3Mat3* axis,
													  Rgd3Vec3 dir, float* min, float* max)
{
	Rgd3Vec3 center = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	Rgd3Vec3 extents = rgd3_vec3_sub(b->b[1], center);
	float d1, d2;
	center = rgd3_vec3_add(origin, rgd3_mat3_mul_vec(axis, center));
	d1 = rgd3_vec3_dot(dir, center);
	d2 = fabsf(extents.x * rgd3_vec3_dot(dir, axis->mat[0])) + fabsf(extents.y * rgd3_vec3_dot(dir, axis->mat[1])) +
		 fabsf(extents.z * rgd3_vec3_dot(dir, axis->mat[2]));
	*min = d1 - d2;
	*max = d1 + d2;
}

/* radius relative to the bounds origin / to a given center */
float rgd3_bounds_radius(const Rgd3Bounds* b);
float rgd3_bounds_radius_from(const Rgd3Bounds* b, const Rgd3Vec3* center);
float rgd3_bounds_plane_distance(const Rgd3Bounds* b, const Rgd3Plane* plane);
/* PLANESIDE_FRONT / BACK / CROSS (RGD3_ON_EPSILON is idlib's default) */
int rgd3_bounds_plane_side(const Rgd3Bounds* b, const Rgd3Plane* plane, float epsilon);
/* segment start..end against the bounds */
int rgd3_bounds_line_intersection(const Rgd3Bounds* b, const Rgd3Vec3* start, const Rgd3Vec3* end);
/* both directions from start; start inside counts with scale 0 */
int rgd3_bounds_ray_intersection(const Rgd3Bounds* b, const Rgd3Vec3* start, const Rgd3Vec3* dir, float* scale);
/* tightest bounds of `bounds` placed at origin/axis; b may alias bounds */
void rgd3_bounds_from_transformed(Rgd3Bounds* b, const Rgd3Bounds* bounds, const Rgd3Vec3* origin,
								  const Rgd3Mat3* axis);
/* idBounds::Rotate / operator*( idMat3 ) */
void rgd3_bounds_rotate(const Rgd3Bounds* b, const Rgd3Mat3* rotation, Rgd3Bounds* out);
/* cleared (1e30 inverted) bounds for numPoints == 0 */
void rgd3_bounds_from_points(Rgd3Bounds* b, const Rgd3Vec3* points, int numPoints);
void rgd3_bounds_from_point_translation(Rgd3Bounds* b, const Rgd3Vec3* point, const Rgd3Vec3* translation);
void rgd3_bounds_from_bounds_translation(Rgd3Bounds* b, const Rgd3Bounds* bounds, const Rgd3Vec3* origin,
										 const Rgd3Mat3* axis, const Rgd3Vec3* translation);
void rgd3_bounds_from_point_rotation(Rgd3Bounds* b, const Rgd3Vec3* point, const Rgd3Rotation* rotation);
void rgd3_bounds_from_bounds_rotation(Rgd3Bounds* b, const Rgd3Bounds* bounds, const Rgd3Vec3* origin,
									  const Rgd3Mat3* axis, const Rgd3Rotation* rotation);
/* corner i = ( b[(i^(i>>1))&1].x, b[(i>>1)&1].y, b[(i>>2)&1].z ) */
void rgd3_bounds_to_points(const Rgd3Bounds* b, Rgd3Vec3 points[8]);

/* ===================================================================== */
/* Rgd3Sphere (aggregate)                                                 */
/* ===================================================================== */

RGINLINE void rgd3_sphere_set(Rgd3Sphere* s, Rgd3Vec3 origin, float radius)
{
	s->origin = origin;
	s->radius = radius;
}

/* inside out: radius -1 */
RGINLINE void rgd3_sphere_clear(Rgd3Sphere* s)
{
	s->origin = rgd3_vec3_zero();
	s->radius = -1.0f;
}

RGINLINE void rgd3_sphere_zero(Rgd3Sphere* s)
{
	s->origin = rgd3_vec3_zero();
	s->radius = 0.0f;
}

RGINLINE int rgd3_sphere_is_cleared(const Rgd3Sphere* s)
{
	return s->radius < 0.0f;
}

RGINLINE int rgd3_sphere_add_point(Rgd3Sphere* s, Rgd3Vec3 p)
{
	if (s->radius < 0.0f)
	{
		s->origin = p;
		s->radius = 0.0f;
		return 1;
	}
	else
	{
		float r = rgd3_vec3_length_sqr(rgd3_vec3_sub(p, s->origin));
		if (r > s->radius * s->radius)
		{
			r = rgd3_math_sqrt(r);
			s->origin = rgd3_vec3_add(
				s->origin, rgd3_vec3_scale(rgd3_vec3_scale(rgd3_vec3_sub(p, s->origin), 0.5f), (1.0f - s->radius / r)));
			s->radius += 0.5f * (r - s->radius);
			return 1;
		}
		return 0;
	}
}

RGINLINE int rgd3_sphere_add_sphere(Rgd3Sphere* s, const Rgd3Sphere* a)
{
	if (s->radius < 0.0f)
	{
		*s = *a;
		return 1;
	}
	else
	{
		float r = rgd3_vec3_length_sqr(rgd3_vec3_sub(a->origin, s->origin));
		if (r > (s->radius + a->radius) * (s->radius + a->radius))
		{
			r = rgd3_math_sqrt(r);
			s->origin = rgd3_vec3_add(s->origin, rgd3_vec3_scale(rgd3_vec3_scale(rgd3_vec3_sub(a->origin, s->origin), 0.5f),
																	(1.0f - s->radius / (r + a->radius))));
			s->radius += 0.5f * ((r + a->radius) - s->radius);
			return 1;
		}
		return 0;
	}
}

RGINLINE void rgd3_sphere_expand_self(Rgd3Sphere* s, float d)
{
	s->radius += d;
}

RGINLINE void rgd3_sphere_translate_self(Rgd3Sphere* s, Rgd3Vec3 translation)
{
	s->origin = rgd3_vec3_add(s->origin, translation);
}

/* includes touching */
RGINLINE int rgd3_sphere_contains_point(const Rgd3Sphere* s, Rgd3Vec3 p)
{
	return !(rgd3_vec3_length_sqr(rgd3_vec3_sub(p, s->origin)) > s->radius * s->radius);
}

/* includes touching */
RGINLINE int rgd3_sphere_intersects_sphere(const Rgd3Sphere* s, const Rgd3Sphere* a)
{
	float r = a->radius + s->radius;
	return !(rgd3_vec3_length_sqr(rgd3_vec3_sub(a->origin, s->origin)) > r * r);
}

RGINLINE void rgd3_sphere_axis_projection(const Rgd3Sphere* s, Rgd3Vec3 dir, float* min, float* max)
{
	float d = rgd3_vec3_dot(dir, s->origin);
	*min = d - s->radius;
	*max = d + s->radius;
}

RGINLINE void rgd3_sphere_from_point_translation(Rgd3Sphere* s, Rgd3Vec3 point, Rgd3Vec3 translation)
{
	s->origin = rgd3_vec3_add(point, rgd3_vec3_scale(translation, 0.5f));
	s->radius = rgd3_math_sqrt(0.5f * rgd3_vec3_length_sqr(translation));
}

/* idBounds::ToSphere */
RGINLINE void rgd3_sphere_from_bounds(Rgd3Sphere* s, const Rgd3Bounds* b)
{
	s->origin = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	s->radius = rgd3_vec3_length(rgd3_vec3_sub(b->b[1], s->origin));
}

float rgd3_sphere_plane_distance(const Rgd3Sphere* s, const Rgd3Plane* plane);
int rgd3_sphere_plane_side(const Rgd3Sphere* s, const Rgd3Plane* plane, float epsilon);
int rgd3_sphere_line_intersection(const Rgd3Sphere* s, const Rgd3Vec3* start, const Rgd3Vec3* end);
/* hits are start + dir * scale1 / scale2; start inside: scale1 > 0 > scale2 */
int rgd3_sphere_ray_intersection(const Rgd3Sphere* s, const Rgd3Vec3* start, const Rgd3Vec3* dir, float* scale1,
								 float* scale2);
void rgd3_sphere_from_points(Rgd3Sphere* s, const Rgd3Vec3* points, int numPoints);
void rgd3_sphere_from_sphere_translation(Rgd3Sphere* s, const Rgd3Sphere* sphere, const Rgd3Vec3* start,
										 const Rgd3Vec3* translation);
void rgd3_sphere_from_point_rotation(Rgd3Sphere* s, const Rgd3Vec3* point, const Rgd3Rotation* rotation);
void rgd3_sphere_from_sphere_rotation(Rgd3Sphere* s, const Rgd3Sphere* sphere, const Rgd3Vec3* start,
									  const Rgd3Rotation* rotation);

/* ===================================================================== */
/* Rgd3Box (aggregate, oriented)                                          */
/* ===================================================================== */

RGINLINE void rgd3_box_set(Rgd3Box* box, Rgd3Vec3 center, Rgd3Vec3 extents, const Rgd3Mat3* axis)
{
	box->center = center;
	box->extents = extents;
	box->axis = *axis;
}

RGINLINE void rgd3_box_from_point(Rgd3Box* box, Rgd3Vec3 point)
{
	box->center = point;
	box->extents = rgd3_vec3_zero();
	rgd3_mat3_identity(&box->axis);
}

RGINLINE void rgd3_box_from_bounds(Rgd3Box* box, const Rgd3Bounds* b)
{
	box->center = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	box->extents = rgd3_vec3_sub(b->b[1], box->center);
	rgd3_mat3_identity(&box->axis);
}

RGINLINE void rgd3_box_from_bounds_transformed(Rgd3Box* box, const Rgd3Bounds* b, Rgd3Vec3 origin,
											   const Rgd3Mat3* axis)
{
	Rgd3Vec3 center = rgd3_vec3_scale(rgd3_vec3_add(b->b[0], b->b[1]), 0.5f);
	box->extents = rgd3_vec3_sub(b->b[1], center);
	box->center = rgd3_vec3_add(origin, rgd3_mat3_mul_vec(axis, center));
	box->axis = *axis;
}

/* inside out: extents -1e30 */
RGINLINE void rgd3_box_clear(Rgd3Box* box)
{
	box->center = rgd3_vec3_zero();
	box->extents = rgd3_vec3_splat(-RGD3_MATH_INFINITY);
	rgd3_mat3_identity(&box->axis);
}

RGINLINE void rgd3_box_zero(Rgd3Box* box)
{
	box->center = rgd3_vec3_zero();
	box->extents = rgd3_vec3_zero();
	rgd3_mat3_identity(&box->axis);
}

RGINLINE int rgd3_box_is_cleared(const Rgd3Box* box)
{
	return box->extents.x < 0.0f;
}

/* idBox::GetVolume as written: ( extents * 2 ).LengthSqr() */
RGINLINE float rgd3_box_volume(const Rgd3Box* box)
{
	return rgd3_vec3_length_sqr(rgd3_vec3_scale(box->extents, 2.0f));
}

RGINLINE void rgd3_box_expand_self(Rgd3Box* box, float d)
{
	box->extents.x += d;
	box->extents.y += d;
	box->extents.z += d;
}

RGINLINE void rgd3_box_translate_self(Rgd3Box* box, Rgd3Vec3 translation)
{
	box->center = rgd3_vec3_add(box->center, translation);
}

/* center *= rotation, axis *= rotation */
RGINLINE void rgd3_box_rotate_self(Rgd3Box* box, const Rgd3Mat3* rotation)
{
	box->center = rgd3_mat3_mul_vec(rotation, box->center);
	rgd3_mat3_mul(&box->axis, rotation, &box->axis);
}

/* includes touching */
RGINLINE int rgd3_box_contains_point(const Rgd3Box* box, Rgd3Vec3 p)
{
	Rgd3Vec3 lp = rgd3_vec3_sub(p, box->center);
	if (fabsf(rgd3_vec3_dot(lp, box->axis.mat[0])) > box->extents.x ||
		fabsf(rgd3_vec3_dot(lp, box->axis.mat[1])) > box->extents.y ||
		fabsf(rgd3_vec3_dot(lp, box->axis.mat[2])) > box->extents.z)
		return 0;
	return 1;
}

RGINLINE void rgd3_box_to_sphere(const Rgd3Box* box, Rgd3Sphere* out)
{
	out->origin = box->center;
	out->radius = rgd3_vec3_length(box->extents);
}

RGINLINE void rgd3_box_axis_projection(const Rgd3Box* box, Rgd3Vec3 dir, float* min, float* max)
{
	float d1 = rgd3_vec3_dot(dir, box->center);
	float d2 = fabsf(box->extents.x * rgd3_vec3_dot(dir, box->axis.mat[0])) +
			   fabsf(box->extents.y * rgd3_vec3_dot(dir, box->axis.mat[1])) +
			   fabsf(box->extents.z * rgd3_vec3_dot(dir, box->axis.mat[2]));
	*min = d1 - d2;
	*max = d1 + d2;
}

/* projections onto the three rows of ax, as bounds */
RGINLINE void rgd3_box_axis_projection_mat3(const Rgd3Box* box, const Rgd3Mat3* ax, Rgd3Bounds* out)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		float d1 = rgd3_vec3_dot(ax->mat[i], box->center);
		float d2 = fabsf(box->extents.x * rgd3_vec3_dot(ax->mat[i], box->axis.mat[0])) +
				   fabsf(box->extents.y * rgd3_vec3_dot(ax->mat[i], box->axis.mat[1])) +
				   fabsf(box->extents.z * rgd3_vec3_dot(ax->mat[i], box->axis.mat[2]));
		out->b[0].v[i] = d1 - d2;
		out->b[1].v[i] = d1 + d2;
	}
}

float rgd3_box_plane_distance(const Rgd3Box* box, const Rgd3Plane* plane);
int rgd3_box_plane_side(const Rgd3Box* box, const Rgd3Plane* plane, float epsilon);
/* separating axis test, includes touching */
int rgd3_box_intersects_box(const Rgd3Box* box, const Rgd3Box* a);
int rgd3_box_line_intersection(const Rgd3Box* box, const Rgd3Vec3* start, const Rgd3Vec3* end);
int rgd3_box_ray_intersection(const Rgd3Box* box, const Rgd3Vec3* start, const Rgd3Vec3* dir, float* scale1,
							  float* scale2);
/* idBox::ToPoints corner order */
void rgd3_box_to_points(const Rgd3Box* box, Rgd3Vec3 points[8]);

/* ===================================================================== */
/* joints (aggregate)                                                     */
/* ===================================================================== */

RGINLINE void rgd3_jointmat_identity(Rgd3JointMat* j)
{
	int i;
	for (i = 0; i < 12; i++)
		j->mat[i] = 0.0f;
	j->mat[0 * 4 + 0] = 1.0f;
	j->mat[1 * 4 + 1] = 1.0f;
	j->mat[2 * 4 + 2] = 1.0f;
}

/* stores m transposed (idJointMat::SetRotation) */
RGINLINE void rgd3_jointmat_set_rotation(Rgd3JointMat* j, const Rgd3Mat3* m)
{
	j->mat[0 * 4 + 0] = m->mat[0].x;
	j->mat[0 * 4 + 1] = m->mat[1].x;
	j->mat[0 * 4 + 2] = m->mat[2].x;
	j->mat[1 * 4 + 0] = m->mat[0].y;
	j->mat[1 * 4 + 1] = m->mat[1].y;
	j->mat[1 * 4 + 2] = m->mat[2].y;
	j->mat[2 * 4 + 0] = m->mat[0].z;
	j->mat[2 * 4 + 1] = m->mat[1].z;
	j->mat[2 * 4 + 2] = m->mat[2].z;
}

/* idJointMat::GetRotation == ToMat3 */
RGINLINE void rgd3_jointmat_to_mat3(const Rgd3JointMat* j, Rgd3Mat3* out)
{
	rgd3_mat3_set(out, j->mat[0 * 4 + 0], j->mat[1 * 4 + 0], j->mat[2 * 4 + 0], j->mat[0 * 4 + 1], j->mat[1 * 4 + 1],
				  j->mat[2 * 4 + 1], j->mat[0 * 4 + 2], j->mat[1 * 4 + 2], j->mat[2 * 4 + 2]);
}

RGINLINE void rgd3_jointmat_set_translation(Rgd3JointMat* j, Rgd3Vec3 t)
{
	j->mat[0 * 4 + 3] = t.x;
	j->mat[1 * 4 + 3] = t.y;
	j->mat[2 * 4 + 3] = t.z;
}

/* idJointMat::GetTranslation == ToVec3 */
RGINLINE Rgd3Vec3 rgd3_jointmat_to_vec3(const Rgd3JointMat* j)
{
	return rgd3_vec3(j->mat[0 * 4 + 3], j->mat[1 * 4 + 3], j->mat[2 * 4 + 3]);
}

/* rotate only (idJointMat * idVec3, Rotate) */
RGINLINE Rgd3Vec3 rgd3_jointmat_rotate(const Rgd3JointMat* j, Rgd3Vec3 v)
{
	const float* m = j->mat;
	return rgd3_vec3(m[0 * 4 + 0] * v.x + m[0 * 4 + 1] * v.y + m[0 * 4 + 2] * v.z,
					 m[1 * 4 + 0] * v.x + m[1 * 4 + 1] * v.y + m[1 * 4 + 2] * v.z,
					 m[2 * 4 + 0] * v.x + m[2 * 4 + 1] * v.y + m[2 * 4 + 2] * v.z);
}

/* rotate and translate (Transform; idJointMat * idVec4 with w = 1) */
RGINLINE Rgd3Vec3 rgd3_jointmat_transform(const Rgd3JointMat* j, Rgd3Vec3 v)
{
	const float* m = j->mat;
	return rgd3_vec3(m[0 * 4 + 0] * v.x + m[0 * 4 + 1] * v.y + m[0 * 4 + 2] * v.z + m[0 * 4 + 3],
					 m[1 * 4 + 0] * v.x + m[1 * 4 + 1] * v.y + m[1 * 4 + 2] * v.z + m[1 * 4 + 3],
					 m[2 * 4 + 0] * v.x + m[2 * 4 + 1] * v.y + m[2 * 4 + 2] * v.z + m[2 * 4 + 3]);
}

/* idJointMat * idVec4 */
RGINLINE Rgd3Vec3 rgd3_jointmat_mul_vec4(const Rgd3JointMat* j, Rgd3Vec4 v)
{
	const float* m = j->mat;
	return rgd3_vec3(m[0 * 4 + 0] * v.x + m[0 * 4 + 1] * v.y + m[0 * 4 + 2] * v.z + m[0 * 4 + 3] * v.w,
					 m[1 * 4 + 0] * v.x + m[1 * 4 + 1] * v.y + m[1 * 4 + 2] * v.z + m[1 * 4 + 3] * v.w,
					 m[2 * 4 + 0] * v.x + m[2 * 4 + 1] * v.y + m[2 * 4 + 2] * v.z + m[2 * 4 + 3] * v.w);
}

RGINLINE int rgd3_jointmat_compare(const Rgd3JointMat* a, const Rgd3JointMat* b)
{
	int i;
	for (i = 0; i < 12; i++)
	{
		if (a->mat[i] != b->mat[i])
			return 0;
	}
	return 1;
}

RGINLINE int rgd3_jointmat_compare_eps(const Rgd3JointMat* a, const Rgd3JointMat* b, float epsilon)
{
	int i;
	for (i = 0; i < 12; i++)
	{
		if (fabsf(a->mat[i] - b->mat[i]) > epsilon)
			return 0;
	}
	return 1;
}

/* idJointMat::Mul: out = j * s */
RGINLINE void rgd3_jointmat_scale(const Rgd3JointMat* j, float s, Rgd3JointMat* out)
{
	int i;
	for (i = 0; i < 12; i++)
		out->mat[i] = s * j->mat[i];
}

/* idJointMat::Mad: out += j * s */
RGINLINE void rgd3_jointmat_mad(Rgd3JointMat* out, const Rgd3JointMat* j, float s)
{
	int i;
	for (i = 0; i < 12; i++)
		out->mat[i] += s * j->mat[i];
}

/* idJointMat::FromMat4: rows 0..2 of the row-major idMat4 */
RGINLINE void rgd3_jointmat_from_mat4(Rgd3JointMat* j, const Rgd3Mat4* m)
{
	int r;
	for (r = 0; r < 3; r++)
	{
		j->mat[r * 4 + 0] = m->mat[r].x;
		j->mat[r * 4 + 1] = m->mat[r].y;
		j->mat[r * 4 + 2] = m->mat[r].z;
		j->mat[r * 4 + 3] = m->mat[r].w;
	}
}

RGINLINE void rgd3_jointmat_to_mat4(const Rgd3JointMat* j, Rgd3Mat4* out)
{
	int r;
	for (r = 0; r < 3; r++)
		out->mat[r] = rgd3_vec4(j->mat[r * 4 + 0], j->mat[r * 4 + 1], j->mat[r * 4 + 2], j->mat[r * 4 + 3]);
	out->mat[3] = rgd3_vec4(0.0f, 0.0f, 0.0f, 1.0f);
}

/* idJointMat::operator*=: transform j by parent a (child-to-parent) */
void rgd3_jointmat_mul_self(Rgd3JointMat* j, const Rgd3JointMat* a);
/* idJointMat::operator/=: untransform j by parent a */
void rgd3_jointmat_untransform_self(Rgd3JointMat* j, const Rgd3JointMat* a);
/* idJointMat::Invert (rigid transforms only) */
void rgd3_jointmat_invert(Rgd3JointMat* j);
/* idJointMat::Multiply / InverseMultiply; out must not alias m1 or m2 */
void rgd3_jointmat_multiply(const Rgd3JointMat* m1, const Rgd3JointMat* m2, Rgd3JointMat* out);
void rgd3_jointmat_inverse_multiply(const Rgd3JointMat* m1, const Rgd3JointMat* m2, Rgd3JointMat* out);
/* idJointMat::ToJointQuat (w = 0) */
void rgd3_jointmat_to_joint_quat(const Rgd3JointMat* j, Rgd3JointQuat* out);
/* SetRotation( q.ToMat3() ) + SetTranslation( t ) */
void rgd3_jointquat_to_joint_mat(const Rgd3JointQuat* jq, Rgd3JointMat* out);

/* the idSIMD_Generic joint loops: destination first */
void rgd3_joints_quats_to_mats(Rgd3JointMat* jointMats, const Rgd3JointQuat* jointQuats, int numJoints);
void rgd3_joints_mats_to_quats(Rgd3JointQuat* jointQuats, const Rgd3JointMat* jointMats, int numJoints);
/* jointMats[i] *= jointMats[parents[i]] for i = first..last, parents[i] < i */
void rgd3_joints_transform(Rgd3JointMat* jointMats, const int* parents, int firstJoint, int lastJoint);
/* jointMats[i] /= jointMats[parents[i]] for i = last..first */
void rgd3_joints_untransform(Rgd3JointMat* jointMats, const int* parents, int firstJoint, int lastJoint);
/* joints[index[i]] = slerp/lerp( joints, blendJoints, lerp ), w = 0 */
void rgd3_joints_blend(Rgd3JointQuat* joints, const Rgd3JointQuat* blendJoints, float lerp, const int* index,
					   int numJoints);
void rgd3_joints_blend_fast(Rgd3JointQuat* joints, const Rgd3JointQuat* blendJoints, float lerp, const int* index,
							int numJoints);

/* ===================================================================== */
/* Rgd3DrawVert (aggregate)                                               */
/* ===================================================================== */

/* VERTEX_BYTE_TO_FLOAT: [0, 255] -> [-1, 1] */
RGINLINE float rgd3_vertex_byte_to_float(u8 x)
{
	return x * (2.0f / 255.0f) - 1.0f;
}

/* VERTEX_FLOAT_TO_BYTE macro: Ftob( ( x + 1 ) * 127.5 + 0.5 ), truncating */
RGINLINE u8 rgd3_vertex_float_to_byte(float x)
{
	return rgd3_math_ftob((x + 1.0f) * (255.0f / 2.0f) + 0.5f);
}

/* VertexFloatToByte as the shipped PC build ran it (SSE2 path):
   ( x + 1 ) * 127.5 + 0.5 is ROUNDED to nearest-even (_mm_cvtps_epi32) and
   saturated, so it can differ by one from the macro above. This is what
   SetNormal/SetTangent produced in BFG's own binary caches. bval[3] is left
   untouched. */
void rgd3_vertex_float_to_byte3(float x, float y, float z, u8* bval);

RGINLINE u32 rgd3_drawvert_pack_bytes(const u8* b)
{
	return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
}

RGINLINE void rgd3_drawvert_unpack_bytes(u8* b, u32 v)
{
	b[0] = (u8)(v & 0xff);
	b[1] = (u8)((v >> 8) & 0xff);
	b[2] = (u8)((v >> 16) & 0xff);
	b[3] = (u8)(v >> 24);
}

/* xyz/st/colors 0, normal (0,0,1), tangent (1,0,0) with sign byte 0xff */
RGINLINE void rgd3_drawvert_clear(Rgd3DrawVert* dv)
{
	dv->xyz = rgd3_vec3_zero();
	dv->st[0] = 0;
	dv->st[1] = 0;
	rgd3_drawvert_unpack_bytes(dv->normal, 0x00FF8080u);
	rgd3_drawvert_unpack_bytes(dv->tangent, 0xFF8080FFu);
	rgd3_drawvert_unpack_bytes(dv->color, 0);
	rgd3_drawvert_unpack_bytes(dv->color2, 0);
}

/* not re-normalized (as the vertex programs see it) */
RGINLINE Rgd3Vec3 rgd3_drawvert_get_normal_raw(const Rgd3DrawVert* dv)
{
	return rgd3_vec3(rgd3_vertex_byte_to_float(dv->normal[0]), rgd3_vertex_byte_to_float(dv->normal[1]),
					 rgd3_vertex_byte_to_float(dv->normal[2]));
}

RGINLINE Rgd3Vec3 rgd3_drawvert_get_normal(const Rgd3DrawVert* dv)
{
	return rgd3_vec3_normalized(rgd3_drawvert_get_normal_raw(dv));
}

/* n must be normalized already */
RGINLINE void rgd3_drawvert_set_normal(Rgd3DrawVert* dv, Rgd3Vec3 n)
{
	rgd3_vertex_float_to_byte3(n.x, n.y, n.z, dv->normal);
}

RGINLINE Rgd3Vec3 rgd3_drawvert_get_tangent_raw(const Rgd3DrawVert* dv)
{
	return rgd3_vec3(rgd3_vertex_byte_to_float(dv->tangent[0]), rgd3_vertex_byte_to_float(dv->tangent[1]),
					 rgd3_vertex_byte_to_float(dv->tangent[2]));
}

RGINLINE Rgd3Vec3 rgd3_drawvert_get_tangent(const Rgd3DrawVert* dv)
{
	return rgd3_vec3_normalized(rgd3_drawvert_get_tangent_raw(dv));
}

RGINLINE void rgd3_drawvert_set_tangent(Rgd3DrawVert* dv, Rgd3Vec3 t)
{
	rgd3_vertex_float_to_byte3(t.x, t.y, t.z, dv->tangent);
}

/* tangent[3] < 128 means -1 */
RGINLINE float rgd3_drawvert_get_bitangent_sign(const Rgd3DrawVert* dv)
{
	return (dv->tangent[3] < 128) ? -1.0f : 1.0f;
}

RGINLINE u8 rgd3_drawvert_get_bitangent_sign_bit(const Rgd3DrawVert* dv)
{
	return (dv->tangent[3] < 128) ? 1 : 0;
}

RGINLINE void rgd3_drawvert_set_bitangent_sign(Rgd3DrawVert* dv, float sign)
{
	dv->tangent[3] = (sign < 0.0f) ? 0 : 255;
}

RGINLINE void rgd3_drawvert_set_bitangent_sign_bit(Rgd3DrawVert* dv, u8 bit)
{
	dv->tangent[3] = bit ? 0 : 255;
}

/* derived from normal, tangent and the sign */
RGINLINE Rgd3Vec3 rgd3_drawvert_get_bitangent(const Rgd3DrawVert* dv)
{
	Rgd3Vec3 b = rgd3_vec3_cross(rgd3_drawvert_get_normal(dv), rgd3_drawvert_get_tangent(dv));
	return rgd3_vec3_scale(b, rgd3_drawvert_get_bitangent_sign(dv));
}

RGINLINE Rgd3Vec3 rgd3_drawvert_get_bitangent_raw(const Rgd3DrawVert* dv)
{
	Rgd3Vec3 b = rgd3_vec3_cross(rgd3_drawvert_get_normal_raw(dv), rgd3_drawvert_get_tangent_raw(dv));
	return rgd3_vec3_scale(b, rgd3_drawvert_get_bitangent_sign(dv));
}

/* only stores the sign of ( normal x tangent ) . t */
RGINLINE void rgd3_drawvert_set_bitangent(Rgd3DrawVert* dv, Rgd3Vec3 t)
{
	Rgd3Vec3 b = rgd3_vec3_cross(rgd3_drawvert_get_normal(dv), rgd3_drawvert_get_tangent(dv));
	rgd3_drawvert_set_bitangent_sign(dv, rgd3_vec3_dot(b, t));
}

RGINLINE void rgd3_drawvert_set_texcoord_native(Rgd3DrawVert* dv, u16 s, u16 t)
{
	dv->st[0] = s;
	dv->st[1] = t;
}

RGINLINE void rgd3_drawvert_set_texcoord_s(Rgd3DrawVert* dv, float s)
{
	dv->st[0] = rgd3_f32_to_f16(s);
}

RGINLINE void rgd3_drawvert_set_texcoord_t(Rgd3DrawVert* dv, float t)
{
	dv->st[1] = rgd3_f32_to_f16(t);
}

RGINLINE void rgd3_drawvert_set_texcoord(Rgd3DrawVert* dv, float s, float t)
{
	rgd3_drawvert_set_texcoord_s(dv, s);
	rgd3_drawvert_set_texcoord_t(dv, t);
}

RGINLINE Rgd3Vec2 rgd3_drawvert_get_texcoord(const Rgd3DrawVert* dv)
{
	return rgd3_vec2(rgd3_f16_to_f32(dv->st[0]), rgd3_f16_to_f32(dv->st[1]));
}

/* colors as little-endian dwords (color[0] is the low byte) */
RGINLINE void rgd3_drawvert_set_color(Rgd3DrawVert* dv, u32 color)
{
	rgd3_drawvert_unpack_bytes(dv->color, color);
}

RGINLINE u32 rgd3_drawvert_get_color(const Rgd3DrawVert* dv)
{
	return rgd3_drawvert_pack_bytes(dv->color);
}

RGINLINE void rgd3_drawvert_set_color2(Rgd3DrawVert* dv, u32 color2)
{
	rgd3_drawvert_unpack_bytes(dv->color2, color2);
}

RGINLINE u32 rgd3_drawvert_get_color2(const Rgd3DrawVert* dv)
{
	return rgd3_drawvert_pack_bytes(dv->color2);
}

RGINLINE void rgd3_drawvert_clear_color2(Rgd3DrawVert* dv)
{
	rgd3_drawvert_unpack_bytes(dv->color2, 0x80808080u);
}

RGINLINE void rgd3_shadowvert_clear(Rgd3ShadowVert* sv)
{
	sv->xyzw = rgd3_vec4_zero();
}

RGINLINE void rgd3_shadowvert_skinned_clear(Rgd3ShadowVertSkinned* sv)
{
	sv->xyzw = rgd3_vec4_zero();
}

/* idDrawVert::Lerp: position and texcoord only; out may alias a or b */
void rgd3_drawvert_lerp(Rgd3DrawVert* out, const Rgd3DrawVert* a, const Rgd3DrawVert* b, float f);
/* idDrawVert::LerpAll: also normal/tangent/bitangent sign and colors */
void rgd3_drawvert_lerp_all(Rgd3DrawVert* out, const Rgd3DrawVert* a, const Rgd3DrawVert* b, float f);
/* idDrawVert::GetSkinnedDrawVert (joints NULL: copy) */
void rgd3_drawvert_skinned(const Rgd3DrawVert* vert, const Rgd3JointMat* joints, Rgd3DrawVert* out);
Rgd3Vec3 rgd3_drawvert_skinned_position(const Rgd3DrawVert* vert, const Rgd3JointMat* joints);
/* CreateShadowCache: two shadow verts (w = 1, w = 0) per vertex; returns
   numVerts * 2 */
int rgd3_shadowvert_create_cache(Rgd3ShadowVert* vertexCache, const Rgd3DrawVert* verts, int numVerts);
int rgd3_shadowvert_skinned_create_cache(Rgd3ShadowVertSkinned* vertexCache, const Rgd3DrawVert* verts,
										 int numVerts);

/* ===================================================================== */
/* Rgd3Random / Rgd3Random2                                               */
/* ===================================================================== */

RGINLINE void rgd3_random_init(Rgd3Random* r, int seed)
{
	r->seed = seed;
}

RGINLINE void rgd3_random_set_seed(Rgd3Random* r, int seed)
{
	r->seed = seed;
}

RGINLINE int rgd3_random_get_seed(const Rgd3Random* r)
{
	return r->seed;
}

/* seed = 69069 * seed + 1 (32-bit wrap); [0, 0x7fff] */
RGINLINE int rgd3_random_int(Rgd3Random* r)
{
	r->seed = (i32)(69069u * (u32)r->seed + 1u);
	return r->seed & RGD3_RANDOM_MAX_RAND;
}

/* [0, max[; 0 for max == 0 */
RGINLINE int rgd3_random_int_max(Rgd3Random* r, int max)
{
	if (max == 0)
		return 0;
	return rgd3_random_int(r) % max;
}

/* [0, 1) in steps of 1/32768 */
RGINLINE float rgd3_random_float(Rgd3Random* r)
{
	return rgd3_random_int(r) / (float)(RGD3_RANDOM_MAX_RAND + 1);
}

/* [-1, 1) */
RGINLINE float rgd3_random_crandom_float(Rgd3Random* r)
{
	return 2.0f * (rgd3_random_float(r) - 0.5f);
}

RGINLINE void rgd3_random2_init(Rgd3Random2* r, u32 seed)
{
	r->seed = seed;
}

RGINLINE void rgd3_random2_set_seed(Rgd3Random2* r, u32 seed)
{
	r->seed = seed;
}

RGINLINE u32 rgd3_random2_get_seed(const Rgd3Random2* r)
{
	return r->seed;
}

/* seed = 1664525 * seed + 1013904223 */
RGINLINE int rgd3_random2_int(Rgd3Random2* r)
{
	r->seed = 1664525u * r->seed + 1013904223u;
	return (int)(r->seed & RGD3_RANDOM_MAX_RAND);
}

/* ( RandomInt() >> ( 16 - BitsForInteger( max ) ) ) % max */
RGINLINE int rgd3_random2_int_max(Rgd3Random2* r, int max)
{
	if (max == 0)
		return 0;
	return (rgd3_random2_int(r) >> (16 - rgd3_math_bits_for_integer(max))) % max;
}

/* [0, 1): 23 random mantissa bits of a float in [1, 2), minus 1 */
RGINLINE float rgd3_random2_float(Rgd3Random2* r)
{
	r->seed = 1664525u * r->seed + 1013904223u;
	return rgd3_math_bits_float(0x3f800000u | (r->seed & 0x007fffffu)) - 1.0f;
}

/* [-1, 1) */
RGINLINE float rgd3_random2_crandom_float(Rgd3Random2* r)
{
	r->seed = 1664525u * r->seed + 1013904223u;
	return 2.0f * rgd3_math_bits_float(0x3f800000u | (r->seed & 0x007fffffu)) - 3.0f;
}

/* ===================================================================== */
/* rg_math interop                                                        */
/* ===================================================================== */

/* rg_vec3 is 16 bytes (padded); never memcpy between the two */
RGINLINE rg_vec3 rgd3_vec3_to_rg(const Rgd3Vec3* v)
{
	rg_vec3 r;
	r.x = v->x;
	r.y = v->y;
	r.z = v->z;
	r._pad = 0.0f;
	return r;
}

RGINLINE Rgd3Vec3 rgd3_vec3_from_rg(const rg_vec3* v)
{
	return rgd3_vec3(v->x, v->y, v->z);
}

/* column-major model matrix: columns 0..2 = axis rows 0..2, column 3 =
   origin, so out * (p, 1) == mul_vec( axis, p ) + origin */
RGINLINE void rgd3_mat3_origin_to_rg_mat4(const Rgd3Mat3* axis, const Rgd3Vec3* origin, rg_mat4* out)
{
	int c;
	for (c = 0; c < 3; c++)
	{
		out->m[c * 4 + 0] = axis->mat[c].x;
		out->m[c * 4 + 1] = axis->mat[c].y;
		out->m[c * 4 + 2] = axis->mat[c].z;
		out->m[c * 4 + 3] = 0.0f;
	}
	out->m[12] = origin->x;
	out->m[13] = origin->y;
	out->m[14] = origin->z;
	out->m[15] = 1.0f;
}

/* row-major idMat4 -> column-major rg_mat4 (same matrix) */
RGINLINE void rgd3_mat4_to_rg_mat4(const Rgd3Mat4* m, rg_mat4* out)
{
	int r, c;
	for (r = 0; r < 4; r++)
	{
		for (c = 0; c < 4; c++)
			out->m[c * 4 + r] = m->mat[r].v[c];
	}
}

/* runs identity/round-trip checks; prints failures and a summary line */
int rgd3_math_self_test(void);

#endif
