#include <stdio.h>
#include <stdlib.h>

// taken from http://alvinalexander.com/programming/printf-format-cheat-sheet

int main(int argc, char* argv[]) {
    printf("%5d\n", 0);
    printf("%5d\n", 123456789);
    printf("%5d\n", -10);
    printf("%5d\n", -123456789);
    
    printf("%-5d\n", 0);
    printf("%-5d\n", 123456789);
    printf("%-5d\n", -10);
    printf("%-5d\n", -123456789);
    
    printf("%03d\n", 0);
    printf("%03d\n", 1);
    printf("%03d\n", 123456789);
    printf("%03d\n", -10);
    printf("%03d\n", -123456789);
    
    printf("'%5d'\n", 10);
    printf("'%-5d'\n", 10);
    printf("'%05d'\n", 10);
    printf("'%+5d'\n", 10);
    printf("'%-+5d'\n", 10);
    
    printf("'%.1f'\n", 10.3456);
    printf("'%.2f'\n", 10.3456);
    printf("'%8.2f'\n", 10.3456);
    printf("'%8.4f'\n", 10.3456);
    printf("'%08.2f'\n", 10.3456);
    printf("'%-8.2f'\n", 10.3456);
    printf("'%-8.2f'\n", 1012.3456);
    printf("'%-8.2f'\n", 101234567.3456);

    printf("'%s'\n", "Hello");
    printf("'%10s'\n", "Hello");
    printf("'%-10s'\n", "Hello");
}
