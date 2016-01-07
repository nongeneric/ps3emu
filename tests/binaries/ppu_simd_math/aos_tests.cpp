#include <vectormath/cpp/vectormath_aos.h>
#include <vectormath/cpp/boolInVec.h>
#include <vectormath/cpp/floatInVec_math.h>
#include <stdio.h>

using namespace Vectormath;
using namespace Vectormath::Aos;

void print(Vector3 const& vec, int i) {
	printf("Vector3(%02d): %0.3f, %0.3f, %0.3f\n",
		i,
		(float)vec.getElem(0),
		(float)vec.getElem(1),
		(float)vec.getElem(2));
}

void print(Point3 const& vec, int i) {
	printf("Point3(%02d): %0.3f, %0.3f, %0.3f\n",
		i,
		(float)vec.getElem(0),
		(float)vec.getElem(1),
		(float)vec.getElem(2));
}

void print(Matrix3 const& m, int i) {
	printf("Matrix3(%02d): %0.3f, %0.3f, %0.3f, %0.3f, %0.3f, %0.3f, %0.3f, %0.3f, %0.3f\n",
		i,
		(float)m[0][0],
		(float)m[0][1],
		(float)m[0][2],
		(float)m[1][0],
		(float)m[1][1],
		(float)m[1][2],
		(float)m[2][0],
		(float)m[2][1],
		(float)m[2][2]
	);
}

void print(boolInVec b, int i) {
	printf("boolInVec(%02d): %d\n", i, (int)b);
}

void print(floatInVec f, int i) {
	printf("floatInVec(%02d): %0.3f\n", i, (float)f);
}

void print(float n, int i) {
	printf("float(%02d): %0.3f\n", i, n);
}

void aos_test_vec3() {
	Vector3 vec1(1.2);
	Vector3 vec2(3.1);
	float scalar = 4.4;
	Vector3 r1 = vec1 * scalar;
	Vector3 r2 = vec2 - vec1;
	Vector3 r3 = vec2 + vec2;
	Vector3 r4 = vec1 / scalar;

	Vector3 vec3;
	vec3.setX(1);
	vec3.setY(2);
	vec3.setZ(3);

	Vector3 r5 = vec2 + vec3;

	float r6 = r5.getX();
	float r7 = r5.getY();
	float r8 = r5.getZ();

	Vector3 r9 = vec1 * r5.getElem(2);

	print(r1, 1);
	print(r2, 2);
	print(r3, 3);
	print(r4, 4);
	print(r5, 5);
	print(r6, 6);
	print(r7, 7);
	print(r8, 8);
	print(r9, 9);
}

void aos_test_mat3() {
	Matrix3 m1(3.2);
	Matrix3 m2(4.1);
	Matrix3 r1 = m1 + m2;
	Matrix3 r2 = m1 - m2;
	Matrix3 r3 = m1.scale(Vector3(2.3, 3.2, 4));
	Matrix3 r4 = r3;
	r4.setCol0(Vector3(1.1));
	r4.setCol1(Vector3(1,2,3));
	Vector3 r5 = r4.getRow(1);
	Vector3 r6 = r4.getCol(1);

	print(r1, 1);
	print(r2, 2);
	print(r3, 3);
	print(r4, 4);
	print(r5, 5);
	print(r6, 6);
}

void aos_test_logic() {
	boolInVec b1(true);
	boolInVec b2(false);
	boolInVec r1 = b1 ^ b2;
	boolInVec r2 = b1 & b2;
	boolInVec r3 = b1 == b2;
	boolInVec r4 = b1 != b2;
	boolInVec r5 = b1 | b2;
	boolInVec r6 = select(b1, b2, b1);
	boolInVec r7 = select(b1, b2, b2);
	boolInVec r8 = select(floatInVec(1), floatInVec(2), b1);
	boolInVec r9 = select(floatInVec(1), floatInVec(2), b2);

	print(r1, 1);
	print(r2, 2);
	print(r3, 3);
	print(r4, 4);
	print(r5, 5);
	print(r6, 6);
	print(r7, 7);
	print(r8, 8);
	print(r9, 9);
}

void aos_test_scalar_ops() {
	floatInVec f1(0.76);
	floatInVec f2(-0.5);
	floatInVec f3(0.2);

	floatInVec r1 = acosf(f1);
	floatInVec r2 = asinf(f1);
	floatInVec r3 = atan2f(f1, f2);
	floatInVec r4 = atanf(f1);
	floatInVec r5 = cbrtf(f1);
	floatInVec r6 = ceilf(f1);
	floatInVec r7 = copysignf(f1, f2);
	floatInVec r8 = cosf(f1);
	floatInVec r9 = divf(f1, f2);
	floatInVec r10 = exp2f(f1);
	floatInVec r11 = expm1f(f1);
	floatInVec r12 = fabsf(f1);
	floatInVec r13 = fdimf(f1, f2);
	floatInVec r14 = floorf(f1);
	floatInVec r15 = fmaf(f1, f2, f3);
	floatInVec r16 = fmaxf(f1, f2);
	floatInVec r17 = fminf(f1, f2);
	floatInVec r18 = fmodf(f1, f2);
	floatInVec r19 = hypotf(f1, f2);
	floatInVec r20 = log10f(f1);
	floatInVec r21 = log1pf(f1);
	floatInVec r22 = log2f(f1);
	floatInVec r23 = logbf(f1);
	floatInVec r24 = logf(f1);
	floatInVec r25 = r3;
	floatInVec r26 = modff(f1, &r25);
	floatInVec r27 = negatef(f1);
	floatInVec r28 = powf(f1, f2);
	floatInVec r29 = recipf(f1);
	floatInVec r30 = remainderf(f1, f2);
	floatInVec r31 = rsqrtf(f1);
	floatInVec r32 = r1;
	floatInVec r33 = r2;
	sincosf(r1, &r32, &r33);
	floatInVec r34 = sinf(f1);
	floatInVec r35 = sqrtf(f1);
	floatInVec r36 = tanf(f1);
	floatInVec r37 = truncf(f1);
	floatInVec r38 = expf(f1);

	print(r1, 1);
	print(r2, 2);
	print(r3, 3);
	print(r4, 4);
	print(r5, 5);
	print(r6, 6);
	print(r7, 7);
	print(r8, 8);
	print(r9, 9);
	print(r10, 10);
	print(r11, 11);
	print(r12, 12);
	print(r13, 13);
	print(r14, 14);
	print(r15, 15);
	print(r16, 16);
	print(r17, 17);
	print(r18, 18);
	print(r19, 19);
	print(r20, 20);
	print(r21, 21);
	print(r22, 22);
	print(r23, 23);
	print(r24, 24);
	print(r25, 25);
	print(r26, 26);
	print(r27, 27);
	print(r28, 28);
	print(r29, 29);
	print(r30, 30);
	print(r31, 31);
	print(r32, 32);
	print(r33, 33);
	print(r34, 34);
	print(r35, 35);
	print(r36, 36);
	print(r37, 37);
	print(r38, 38);
}

void aos_test_vec_ops() {
	Vector3 vec1(1, -1.2, 1.4);
	Vector3 vec2(2, 4, -6);
	floatInVec f1(3.4);

	Vector3 r1 = absPerElem(vec1);
	Vector3 r2 = copySignPerElem(vec1, vec2);
	Vector3 r3 = cross(vec1, vec2);
	Matrix3 r4 = crossMatrix(vec1);
	Matrix3 r5 = crossMatrixMul(vec2, r4);
	Vector3 r6 = divPerElem(vec1, vec2);
	floatInVec r7 = dot(vec1, vec2);
	floatInVec r8 = length(vec1);
	floatInVec r9 = lengthSqr(vec1);
	Vector3 r10 = lerp(f1, vec1, vec2);
	floatInVec r11 = maxElem(vec1);
	Vector3 r12 = maxPerElem(vec1, vec2);
	floatInVec r13 = minElem(vec1);
	Vector3 r14 = mulPerElem(vec1, vec2);
	Vector3 r15 = normalize(vec1);
	Matrix3 r16 = outer(vec1, vec2);
	Vector3 r17 = recipPerElem(vec1);
	Vector3 r18 = rsqrtPerElem(r1);
	Vector3 r19 = select(vec1, vec2, true);
	Vector3 r20 = select(vec1, vec2, false);
	Vector3 r21 = slerp(0.7, Vector3(1,0,0), Vector3(0,1,0));
	Vector3 r22 = sqrtPerElem(r1);
	floatInVec r23 = sum(vec1);

	Point3 p1(-2.2, -2.4, 4.6);
	Point3 p2(1.2, -3.4, 7.6);

	Point3 r24 = copySignPerElem(p1, p2);
	floatInVec r25 = dist(p1, p2);
	floatInVec r26 = distFromOrigin(p1);
	floatInVec r27 = distSqr(p1, p2);
	floatInVec r28 = projection(p1, Vector3(0, 1, 0));

	Matrix3 m1;
	m1.setCol0(Vector3(1.2, 1.3, 1.7));
	m1.setCol1(Vector3(2, 3, 4));
	m1.setCol2(Vector3(-10, 0, 7));

	floatInVec r29 = determinant(m1);
	Matrix3 r30 = inverse(m1);

	float fs1[4];
	storeXYZ(vec1, fs1);
	Vector3 r31;
	loadXYZ(r31, fs1);

	print(r1, 1);
	print(r2, 2);
	print(r3, 3);
	print(r4, 4);
	print(r5, 5);
	print(r6, 6);
	print(r7, 7);
	print(r8, 8);
	print(r9, 9);
	print(r10, 10);
	print(r11, 11);
	print(r12, 12);
	print(r13, 13);
	print(r14, 14);
	print(r15, 15);
	print(r16, 16);
	print(r17, 17);
	print(r18, 18);
	print(r19, 19);
	print(r20, 20);
	print(r21, 21);
	print(r22, 22);
	print(r23, 23);
	print(r24, 24);
	print(r25, 25);
	print(r26, 26);
	print(r27, 27);
	print(r28, 28);
	print(r29, 29);
	print(r30, 30);
	print(r31, 31);
}

void aos_tests() {
	aos_test_vec3();
	aos_test_mat3();
	aos_test_logic();
	aos_test_scalar_ops();
	aos_test_vec_ops();
}