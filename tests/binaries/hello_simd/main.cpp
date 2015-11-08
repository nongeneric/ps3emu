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

void print_vector(Vector4 vector) {
	float* m = (float*)&vector;
	printf("vector: %g,%g,%g,%g\n", m[0], m[1], m[2], m[3]);
	fflush(stdout);
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
}
