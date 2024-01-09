#include <stdint.h>
#include <stdio.h>


uint8_t _strcmp(const char *str1, const char *str2, uint32_t max){
    while (*str1 == *str2++){
        if (*str1++ == 0) return 0;
        if (max-- == 0) return 2;
    }
    return 1;
}

int main(){
    printf("%d" , _strcmp("relalala\0", "rel4lala\0", 8));
    return 0;
}
