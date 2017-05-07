#include <stdio.h>
#include <math.h>
#include <float.h>

float mul3(float a, float b, float c) {
    return a * b * c + 10.2;
}

float mul4vec(float* vec) {
    return vec[0] * vec[1] * vec[2] + vec[3];
}

float* mul(float* a, float* b) {
    float* r = new float[9];
    r[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    r[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
    r[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
    r[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    r[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    r[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
    r[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    r[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    r[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
    return r;
}

float f(float a, double b, float c, double d) {
    float z = a + b * c * d / a - 1.1 * b + 7 * c * d * d - (-c) * a / b / d;
    if (z < a - b) {
        z *= d - 2 * a;
    }
    if (a < b && d > 17) {
        z -= 3.3333;
    }
    if ((int)z % 3 > 1) {
        z *= 3;
    }
    return z * d / c;
}

int main(int argc, char* argv[]) {
    printf("mul3 = %e\n", mul3(1.2, 2.1, 3.3));

    printf("isnan(NAN)         = %d\n", isnan(NAN));
    printf("isnan(INFINITY)    = %d\n", isnan(INFINITY));
    printf("isnan(0.0)         = %d\n", isnan(0.0));
    printf("isnan(DBL_MIN/2.0) = %d\n", isnan(DBL_MIN/2.0));
    printf("isnan(0.0 / 0.0)   = %d\n", isnan(0.0/0.0));
    printf("isnan(Inf - Inf)   = %d\n", isnan(INFINITY - INFINITY));
    
    printf("isfinite(NAN)         = %d\n", isfinite(NAN));
    printf("isfinite(INFINITY)    = %d\n", isfinite(INFINITY));
    printf("isfinite(0.0)         = %d\n", isfinite(0.0));
    printf("isfinite(DBL_MIN/2.0) = %d\n", isfinite(DBL_MIN/2.0));
    printf("isfinite(1.0)         = %d\n", isfinite(1.0));
    printf("isfinite(exp(800))    = %d\n", isfinite(exp(800.)));
    
    printf("isinf(NAN)         = %d\n", isinf(NAN));
    printf("isinf(INFINITY)    = %d\n", isinf(INFINITY));
    printf("isinf(0.0)         = %d\n", isinf(0.0));
    printf("isinf(DBL_MIN/2.0) = %d\n", isinf(DBL_MIN/2.0));
    printf("isinf(1.0)         = %d\n", isinf(1.0));
    printf("isinf(exp(800))    = %d\n", isinf(exp(800.)));
    
    printf("isnormal(NAN)         = %d\n", isnormal(NAN));
    printf("isnormal(INFINITY)    = %d\n", isnormal(INFINITY));
    printf("isnormal(0.0)         = %d\n", isnormal(0.0));
    //printf("isnormal(DBL_MIN/2.0) = %d\n", isnormal(DBL_MIN/2.0));
    printf("isnormal(1.0)         = %d\n", isnormal(1.0));
    
    printf("isunordered(NAN,1.0) = %d\n", isunordered(NAN,1.0));
    printf("isunordered(1.0,NAN) = %d\n", isunordered(1.0,NAN));
    printf("isunordered(NAN,NAN) = %d\n", isunordered(NAN,NAN));
    printf("isunordered(1.0,0.0) = %d\n", isunordered(1.0,0.0));
    
    /*float zf = 3.23234 * 0.1292999;
    double zd = 3.23234 * 0.1292999;
    printf("18.516 = %e\n", 18.516);
    printf("float = %e\n", zf);
    printf("double = %e\n", zd);*/
    
    float* vec = new float[4];
    vec[0] = 3.33;
    vec[1] = 4.44e3;
    vec[2] = 5.4;
    vec[3] = 1.1;
    printf("mul4vec = %e\n", mul4vec(vec));
    
    float* a = new float[9];
    float* b = new float[9];
    float* c = new float[9];
    float* d = new float[9];
    
    a[0] = 0;
    a[1] = -0.141e2;
    a[2] = -0.35e2;
    a[3] = -0.50e2;
    a[4] = 0.45e2;
    a[5] = 0.65e2;
    a[6] = -0.69e2;
    a[7] = 0.2425e2;
    a[8] = 0.41e2;

    b[0] = 0.74e2;
    b[1] = -0.32e2;
    b[2] = -0.9e1;
    b[3] = 0.2e1;
    b[4] = 0.344e1;
    b[5] = 0.67e2;
    b[6] = 0.32e2;
    b[7] = -0.22e2;
    b[8] = -0.77e2;

    c[0] = 0.45e2;
    c[1] = 0.6922e2;
    c[2] = 0.42e2;
    c[3] = .0;
    c[4] = -0.46e2;
    c[5] = -0.1831e2;
    c[6] = -0.65e2;
    c[7] = -0.25e2;
    c[8] = -0.64e2;

    d[0] = -0.691e2;
    d[1] = -0.97e2;
    d[2] = 0.273e2;
    d[3] = -0.90e2;
    d[4] = -0.862e2;
    d[5] = 0.74e2;
    d[6] = 0.80e2;
    d[7] = -0.4916e2;
    d[8] = -0.18e2;

    float* r = mul(a, b);
    r = mul(r, c);
    r = mul(r, d);
    
    printf("a.b.c=\n");
    for (int i = 0; i < 9; ++i) {
        printf("%0.2f\n", r[i]);
    }
    
    printf("%0.2f\n", f(1, 2, 3, 4));
    printf("%0.2f\n", f(1.2, -0.01, 3.4, 8.0));
    printf("%0.2f\n", f(1.2, 120.2, 3.8, 44));
    printf("%s\n", isnan(f(-1.22, 0, 0, 0)) ? "some nan" : "not nan");
    printf("%0.2f\n", f(133.1391, 10176.3477, 48.2, -4.424));
}
