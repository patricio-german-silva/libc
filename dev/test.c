#include <stdint.h>
#include <stdio.h>


uint8_t _strcmp(const char *str1, const char *str2, uint32_t max){
    while (*str1 == *str2++){
        if (*str1++ == 0) return 0;
        if (max-- == 0) return 2;
    }
    return 1;
}

uint8_t _touppercase(const char *strin, char *strout, uint32_t max){
  const uint8_t d = ('A'-'a');
  while(*strin != 0){
    if(*strin >= 'a' && *strin <= 'z')
      *strout++ = *strin + d;
    else
      *strout++ = *strin;
    if (--max == 0){
      *strout = 0;
      return 2;
    }
    strin++;
  }
  return 0;
}

uint8_t _tolowercase(const char *strin, char *strout, uint32_t max){
  const uint8_t d = ('A'-'a');
  while(*strin != 0){
    if(*strin >= 'A' && *strin <= 'Z')
      *strout++ = *strin - d;
    else
      *strout++ = *strin;
    if (--max == 0){
      *strout = 0;
      return 2;
    }
    strin++;
  }
  return 0;
}


int main(){
  char i[] = "RelalAla\0";
  char o[20];
  uint8_t r= _tolowercase(i, i, 2324234236);
   printf("%s: %d\n" , i, r);
   return 0;
}
