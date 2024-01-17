/*
 * fonts.c
 *
 *  Created on: Dec 10, 2022
 *      Author: psilva
 */

#include "stdint.h"
#include "fonts.h"

/*Buffer temporal para la posición de un caracter en la fuente*/
_fontbuff buff;

// Retorna el primer caracter que maneja la fuente, usualemnte 32 (espacio en blanco)
uint8_t _fonts_get_first_char(const uint8_t *font){
	return font[4];
}

// Retorna el ultimo caracter que maneja la fuente, usualemnte 127 (delete) why??
uint8_t _fonts_get_last_char(const uint8_t *font){
	return font[4] + font[5] - 1;
}

// Cantidad de caracteres en la fuente
uint8_t _fonts_get_char_count(const uint8_t *font){
	return font[5];
}

// Altura de los caracteres en la fuente
uint8_t _fonts_get_char_height(const uint8_t *font){
	return font[3];
}

// Cantidad de bytes que tiene una columna en la fuente
uint8_t _fonts_get_bytes_per_column(const uint8_t *font){
	uint8_t bytes_on_height = font[3]/8;
	if((font[3] % 8) != 0) bytes_on_height++;
	return bytes_on_height;
}

// Ancho del caracter indicado
uint8_t _fonts_get_char_width(const uint8_t *font, char on_char){
    if((uint8_t)on_char < font[4] || (uint8_t)on_char > font[4] + font[5]) return 0;
    return font[(uint8_t)on_char - font[4] + 6];
}

// Posicion en el array font donde inicia el caracter indicado
uint16_t _fonts_get_char_starts_at(const uint8_t *font, char on_char){
    if(buff.on_char != on_char || buff.font != font){
        uint16_t pos = font[5] + 6; // Salteo header y bloque de anchos de fuente
        uint8_t bytes_per_column = _fonts_get_bytes_per_column(font);

        for(uint8_t i = 0; i < (uint8_t)on_char - font[4]; i++)
            pos+= (font[i+6]*bytes_per_column);
        buff.font = font;
        buff.on_char = on_char;
        buff.starts_at = pos;
    }
    return buff.starts_at;
}

// Un byte de datos de la fuente, columna indicada (0..whidth) y bits de offset de arriba hacia abajo (una fuente de tamaño 14 tiene offset entre 0 y 13)
// el resto de bits que no pertenece a la fuente se completa con 0
uint8_t _fonts_get_column_byte(const uint8_t *font, char on_char, uint8_t column, uint8_t offset){
    if((uint8_t)on_char < font[4] || (uint8_t)on_char > font[4] + font[5]) return 0;

    uint8_t char_width = _fonts_get_char_width(font, on_char);
    if(column >= char_width) return 0;

    if((offset) >= _fonts_get_char_height(font)) return 0;

    uint8_t bytes_per_column = _fonts_get_bytes_per_column(font);
    uint8_t retval;

    uint16_t char_pos = _fonts_get_char_starts_at(font, on_char);
    if((offset / 8) == bytes_per_column - 1){  // Solo incluye info del ultimo byte, que tiene los bits en exceso en el LSB
        retval = font[char_pos + (char_width * (offset / 8)) + column] >> ((offset % 8) + ((bytes_per_column*8) - _fonts_get_char_height(font)));
    }else{
        retval = font[char_pos + (char_width * (offset / 8)) + column] >> (offset % 8);
        if((offset / 8) + 1 == bytes_per_column - 1){  // La parte MSB corresponde al ultimo byte , que tiene los bits en exceso en el LSB
            retval |= ((font[char_pos + (char_width * ((offset / 8)+1)) + column] >> ((bytes_per_column*8) - _fonts_get_char_height(font))) << (8 - (offset % 8)));
        }else{
            retval |= font[char_pos + (char_width * ((offset / 8)+1)) + column] << (8 - (offset % 8));
        }
    }
    return retval;
}
