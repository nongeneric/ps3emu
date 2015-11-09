#include <vectormath/cpp/vectormath_aos.h>
#include <stdio.h>

using namespace Vectormath::Aos;

void print_matrix(Matrix4 matrix) {
	float* m = (float*)&matrix;
	printf("matrix: %g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n",
		m[0], m[1], m[2], m[3], 
		m[4], m[5], m[6], m[7], 
		m[8], m[9], m[10], m[11], 
		m[12], m[13], m[14], m[15]);
	fflush(stdout);
}

void print_matrix(Matrix3 matrix) {
	float* m = (float*)&matrix;
	printf("matrix3: %g,%g,%g,%g,%g,%g,%g,%g,%g\n",
		m[0], m[1], m[2], m[3], 
		m[4], m[5], m[6], m[7], 
		m[8], m[9]);
	fflush(stdout);
}

void print_vector(Vector4 vector) {
	float* m = (float*)&vector;
	printf("vector: %g,%g,%g,%g\n", m[0], m[1], m[2], m[3]);
	fflush(stdout);
}

void print(vec_uint4 vec) {
	uint32_t* m = (uint32_t*)&vec;
	printf("vector: %x,%x,%x,%x\n", m[0], m[1], m[2], m[3]);
	fflush(stdout);
}

void print(vec_int4 vec) {
	uint32_t* m = (uint32_t*)&vec;
	printf("vector: %x,%x,%x,%x\n", m[0], m[1], m[2], m[3]);
	fflush(stdout);
}

void print(__vector float vec) {
	float* m = (float*)&vec;
	printf("vector: %g,%g,%g,%g\n", m[0], m[1], m[2], m[3]);
	fflush(stdout);
}

void print_vector(Vector3 vector) {
	float* m = (float*)&vector;
	printf("vector3: %g,%g,%g\n", m[0], m[1], m[2]);
	fflush(stdout);
}

void print_point(Point3 vector) {
	float* m = (float*)&vector;
	printf("point3: %g,%g,%g\n", m[0], m[1], m[2]);
	fflush(stdout);
}

vec_float4 custom_sinf(vec_float4 x) {
    printf("sinf\n");
    vec_float4 xl,xl2,xl3,res;
    vec_int4   q;
    vec_float4 vzero = (vec_float4)vec_xor((vec_uint4)(0), (vec_uint4)(0));
    
    print_vector(Vector4(vzero));
    
    vec_uint4 smask = (vec_uint4)(vec_int4)(-1);
    smask = vec_vslw(smask, smask);
    
    print(smask);
    
    xl = vec_madd(x, (vec_float4)(0.63661977236f),(vec_float4)vzero);
    print(xl);
    
    xl = vec_add(xl,vec_sel((vec_float4)(0.5f),xl,smask));
    print(xl);
    
    q = vec_cts(xl,0);
    print(q);
    vec_int4 offset = vec_and(q,(vec_int4)((int)0x3));
    print(offset);
    vec_float4 qf = vec_ctf(q,0);
    print(qf);
    vec_float4 p1 = vec_nmsub(qf,(vec_float4)(_SINCOS_KC1),x);
    print(p1);
    xl  = vec_nmsub(qf,(vec_float4)(_SINCOS_KC2),p1);
    xl2 = vec_madd(xl,xl,(vec_float4)vzero);
    xl3 = vec_madd(xl2,xl,(vec_float4)vzero);
    print(xl3);
    vec_float4 ct1 = vec_madd((vec_float4)(_SINCOS_CC0),xl2,(vec_float4)(_SINCOS_CC1));
    vec_float4 st1 = vec_madd((vec_float4)(_SINCOS_SC0),xl2,(vec_float4)(_SINCOS_SC1));
    print(st1);

    vec_float4 ct2 = vec_madd(ct1,xl2,(vec_float4)(_SINCOS_CC2));
    vec_float4 st2 = vec_madd(st1,xl2,(vec_float4)(_SINCOS_SC2));
    
    vec_float4 cx = vec_madd(ct2,xl2,(vec_float4)(1.0f));
    vec_float4 sx = vec_madd(st2,xl3,xl);
    
    vec_uint4 mask1 = (vec_uint4)vec_cmpeq(vec_and(offset,
                                                   (vec_int4)(0x1)),
                                           (vec_int4)vzero);
    print(mask1);
    res = vec_sel(cx,sx,mask1);
    print(res);
    vec_uint4 mask2 = (vec_uint4)vec_cmpeq(vec_and(offset,(vec_int4)(0x2)),(vec_int4)vzero);
    print(mask2);
    res = vec_sel((vec_float4)vec_xor(smask,(vec_uint4)res),res,mask2);

    return res;
}

int main(int argc, char* argv[]) {
    Vector4 vector = Vector4::Vector4(2);
	print_vector(vector);
    
    printf("x: %g\n", (float)vector[0]);
    printf("y: %g\n", (float)vector[1]);
    printf("z: %g\n", (float)vector[2]);
    printf("w: %g\n", (float)vector[3]);

	Vector4 yaxis = Vector4::yAxis();
	print_vector(yaxis);

	Matrix4 mat = Matrix4::identity();
	print_matrix(mat);
    print_matrix(2 * mat);
    print_matrix(mat * mat);
    
    mat[3][1] = 0.43344;
    mat[2][2] = 0.32333;
    mat[3][3] = 0.43344;
    mat[1][0] = 0.23222;
    mat[0][2] = 0.49994;
    mat[0][3] = 0.42352;
    
    print_matrix(2 * mat);
    print_matrix(mat * mat * mat * mat);
    
    Matrix4 m4(
        Vector4(0, 1, 2, 3),
        Vector4(4, 5, 6, 7),
        Vector4(8, 9, 10, 11),
        Vector4(12, 13, 14, 15)
    );
    
    print_matrix(m4);
    print_matrix(transpose(m4));
    
    print_matrix(Matrix4::lookAt(
        Point3(0.2, 0.3, 0.4),
        Point3(1, 2, 3),
        Vector3(-0.1, 2, 10)
    ));
    
    Matrix4 xrot = Matrix4::rotationX(1.3f);
    Matrix4 yrot = Matrix4::rotationY(0.f);
    Matrix4 pos = Matrix4::translation(-Vector3(10, 20, 30));
    Matrix4 m = xrot * yrot * pos;
    print_matrix(m);
    Matrix4 inv = -pos * transpose(xrot) * transpose(yrot);
    print_matrix(inv);
    
    Matrix3 xrot3 = Matrix3::rotationX(-1.3f);
    print_matrix(xrot3);
    Matrix3 yrot3 = Matrix3::rotationY(-2.7f);
    print_matrix(yrot3);
    
    Vector3 zvec3 = (xrot3 * yrot3) * Vector3::zAxis();
    
    print_vector(zvec3);
    
    Point3 target3 = Point3(1, 2, 3);
    float dist3 = 17.17f;
    Point3 pos3 = target3 + zvec3 * dist3;
    
    print_point(pos3);
    
    Vector3	zAxis = normalize(target3 - pos3);
	Vector3 xAxis = normalize(cross(zAxis, Vector3(0.f, 1.f, 0.f)));
	Vector3 yAxis = normalize(cross(xAxis, zAxis));
    
    print_vector(zAxis);
    print_vector(xAxis);
    print_vector(yAxis);
    
	Matrix4	rotate;
	rotate.setCol0(Vector4(xAxis, 0.f));
	rotate.setCol1(Vector4(yAxis, 0.f));
	rotate.setCol2(-Vector4(zAxis, 0.f));
	rotate.setCol3(Vector4(0.f, 0.f, 0.f, 1.f));
    
    print_matrix(rotate);
    
	Matrix4	translate = Matrix4::translation(-Vector3(pos3));
	Matrix4 mm4 = transpose(rotate) * translate;
	Matrix4 imm4 = (-translate) * rotate;
    print_matrix(mm4);
    print_matrix(imm4);
    
    print_vector( Vector3(Vectormath::floatInVec(-1.3f)) );
    print_vector( Vector4(Vectormath::floatInVec(-1.3f)) );
    
    print_matrix(Matrix3(
        Vector3::xAxis(),
        Vector3( Vectormath::floatInVec(-1.3f) ),
        Vector3( Vectormath::floatInVec(0.8f) )
    ));
    
    vec_float4 s, c, res1, res2;
    vec_uint4 select_y, select_z;
    vec_float4 zero;
    select_y = _VECTORMATH_MASK_0x0F00;
    select_z = _VECTORMATH_MASK_0x00F0;
    zero = (vec_float4)(0.0f);
    
    print_vector(Vector3(zero));
    
    sincosf4( Vectormath::floatInVec(-1.3f).get128(), &s, &c );
    
    print_vector(Vector3(s));
    print_vector(Vector3(c));
    
    res1 = vec_sel( zero, c, select_y );
    
    print_vector(Vector4(res1));
    print_vector(Vector3(res1));
    
    res1 = vec_sel( res1, s, select_z );
    
    print_vector(Vector3(res1));
    
    res2 = vec_sel( zero, negatef4(s), select_y );
    
    print_vector(Vector3(res2));
    
    res2 = vec_sel( res2, c, select_z );
    
    print_vector(Vector3(res2));
    
    print_matrix(Matrix3(
        Vector3::xAxis( ),
        Vector3( res1 ),
        Vector3( res2 )
    ));
    
    print_vector(Vector4(sinf4((vec_float4)(-1.3f))));
    print_vector(Vector4(custom_sinf((vec_float4)(-1.3f))));
    
    return 0;
}
