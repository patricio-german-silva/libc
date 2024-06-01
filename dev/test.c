#include <stdint.h>
#include <stdio.h>

/*
 * Convierte el entero en un *str
 * @param *number es el numero a convertir
 * @param *str es el resultado, terminado en '\0'
 * @result retorna la cantidad de digitos convertidos, sin el contar el '\0'
 */
uint8_t _uint32_to_str(uint32_t number, char *str){
    char result[] = "0000000000\0";
    char *ptr = &result[9];
    uint8_t i = 0;
    do {
        *ptr += number % 10;
        number /= 10;
        ptr-=1;
    } while (number > 0);

    do{
        ptr+=1;	// its ok
        *str = *ptr;
        str+=1;
        i+=1;
    } while(*ptr);
    return i-1;	// No cuento el \0
}

/*
 * Convierte el entero con signo en un *str
 * @param *number es el numero a convertir
 * @param *str es el resultado, terminado en '\0'
 * @result retorna la cantidad de digitos convertidos, contando el - si corresponde y sin el contar el '\0'
 */
uint8_t _int32_to_str(int32_t number, char *str){
	if(number < 0){
		str[0] = '-';
		return 1 + _uint32_to_str(-1*number, str+1);
	}else
		return _uint32_to_str(number, str);
}


void main(){
	char str[10];
	uint8_t i = _int32_to_str(-1, str);
	printf("%d: %s",i ,str);
}
