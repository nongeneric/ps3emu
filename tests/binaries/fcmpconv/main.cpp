#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

double b() {
    return 0.79;
}

double c() {
    return b() + 1;
}

double _314() { return 3.14; }
double _n6() { return -6.; }
double _n66() { return -6.6; }
int _7() { return 7; }
int _n100() { return -100; }

int main(int argc, char* argv[]) {
    printf("b > c: %d\n", b() > c());
    printf("b < c: %d\n", b() < c());
    printf("b == c: %d\n", b() == c());
    printf("b > 0: %d\n", b() > 0);
    printf("b < 0: %d\n", b() < 0);
    printf("c > 0: %d\n", c() > 0);
    printf("c < 0: %d\n", c() < 0);
    
    printf("(int)3.14: %d\n", (int)_314());
    printf("(int)-6: %d\n", (int)_n6());
    printf("(int)-6.6: %d\n", (int)_n66());
    
    printf("(float)7: %f\n", (float)_7());
    printf("(float)-100: %f\n", (float)_n100());
    
    printf("%f\n", 1.19);
}
