#include "fpconv.h"
#include <stdio.h>

int main() {
    char buf[24 + 1]; /* reserve space for null terminator */
    
    printf("3.13 = ");
    int str_len = fpconv_dtoa(3.13, buf);
    buf[str_len] = '\0';
    printf("%s\n", buf);
    
    printf("0.02380113 = ");
    str_len = fpconv_dtoa(0.02380113, buf);
    buf[str_len] = '\0';
    printf("%s\n", buf);
    
    printf("3.23234 * 0.1292999 = ");
    str_len = fpconv_dtoa(3.23234 * 0.1292999, buf);
    buf[str_len] = '\0';
    printf("%s\n", buf);
    
    printf("-493893848 = ");
    str_len = fpconv_dtoa(-493893848, buf);
    buf[str_len] = '\0';
    printf("%s\n", buf);
    
    printf("1.322828e300 = ");
    str_len = fpconv_dtoa(1.322828e300, buf);
    buf[str_len] = '\0';
    printf("%s\n", buf);

    printf("0.0000182919575748888 = ");
    str_len = fpconv_dtoa(0.0000182919575748888, buf);
    buf[str_len] = '\0';
    printf("%s\n", buf);
}
