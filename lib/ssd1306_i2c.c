/*
 * ssd1306_i2c.c
 *
 *  Created on: Dec 3, 2022
 *      Author: psilva
 *  Updated on: Mar 19, 2023
 * Version: 16
 *
 */
#include "ssd1306_i2c.h"


/* Definicion de Fuentes en uso */
static const uint8_t *_available_fonts[] = {

#ifdef _FONTS_USE_MINI_8
        _font_mini8,
#endif

#ifdef _FONTS_USE_MINI_MONO_8
        _font_mini_mono8,
#endif

#ifdef _FONTS_USE_CALLIBRI_10
        _font_callibri10,
#endif

#ifdef _FONTS_USE_VERDANA_12
        _font_verdana12,
#endif

#ifdef _FONTS_USE_ARIAL_14
        _font_arial14,
#endif

#ifdef _FONTS_USE_TIMES_NEW_ROMAN_16
        _font_timesnewroman16,
#endif

#ifdef _FONTS_USE_COOPER_26
        _font_cooper26,
#endif
};


// Metodos estaticos
static void _set_addressing(_ssd1306_i2c *ssd, uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end);
static void _compute_widest_row(_ssd1306_i2c *ssd);
static void _compute_tallest_column(_ssd1306_i2c *ssd);
static void _draw_text(_ssd1306_i2c *ssd);
static void _draw_bitmap(_ssd1306_i2c *ssd);
static void _draw_ext_bitmap(_ssd1306_i2c *ssd);
static void _move_lines_up(_ssd1306_i2c *ssd);
static uint8_t _line_is_fixed(_ssd1306_i2c *ssd, uint8_t line);
static const uint8_t* _line_get_font(_ssd1306_i2c *ssd, uint8_t line);
static uint8_t _line_get_inline_px(_ssd1306_i2c *ssd, uint8_t line);
static uint16_t _line_get_start_at_px(_ssd1306_i2c *ssd, uint8_t line);
static uint8_t _line_get_height_px(_ssd1306_i2c *ssd, uint8_t line);
static uint16_t _line_get_total_width_px(_ssd1306_i2c *ssd, uint8_t line);
static uint8_t _line_get_column_data_px(_ssd1306_i2c *ssd, uint8_t line, uint16_t px, uint8_t offset);
static int8_t _line_get_line_by_px(_ssd1306_i2c *ssd, uint16_t px);
static uint32_t abs(int32_t a);




//---------------------------------------------------------------------------------------------------------------------------------------

// Inicializa el display
// cols: cantidad real de columnas del display
// rowa: cantidad real de filas del display
void ssd1306_i2c_init(_ssd1306_i2c *ssd, uint16_t address, uint8_t cols, uint8_t rows){
    ssd->i2c_address = address;
    ssd->mode = _SSD1306_MODE_TEXT;
    ssd->dma_status = 10;	// Tiempo de espera
    ssd->display_update = 3;	// limpio display
    ssd->display_freeze = 0;
    ssd->display_curr_column = 0;
    ssd->display_curr_page = 0;
    ssd->display_columns = cols;
    ssd->display_pages = rows/8;
    ssd->vd_hoffset = 0;
    ssd->vd_voffset = 0;
    ssd->vd_hmax = 0;
    ssd->vd_vmax = 0;
    ssd->curr_conntrast = 0xFF;
    ssd->set_options= 0;

    ssd->display_on = 0;
    ssd->dma_bf_iw = 0;
    ssd->TX_errors = 0;
    ssd->TX_sent = 0;
    ssd->console_cur_line = 0;

    /* Carga los comandos en el buffer para ser enviados al display cuando dma_status sea 0 */
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_MULTIPLE_BYTE_COMMAND;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_DISPLAY_OFF;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_MEMORY_ADDRESSING_MODE;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_MEMORY_ADDRESSING_MODE_HORIZONTAL;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_COLUMN_ADDRESS;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x00;	// Column address start 0
    ssd->dma_buffer[ssd->dma_bf_iw++] = ssd->display_columns-1;	// Column address end, default 127d
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_PAGE_ADDRESS;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x00;	// Page address start 0
    ssd->dma_buffer[ssd->dma_bf_iw++] = ssd->display_pages-1;	// Page address end, default 7d
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_NORMAL;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_SEGMENT_REMAP_NORMAL;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_CONNTRAST;
    ssd->dma_buffer[ssd->dma_bf_iw++] = ssd->curr_conntrast ;	// MAX contrast
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_NORMAL;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_MULTIPLEX_RATIO;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x3F;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_ENTIRE_DISPLAY_ON;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_DISPLAY_START_LINE; // Start 0
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_DISPLAY_OFFSET;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x00;	// No offset
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_DISPLAY_CLK__OSCILLATOR_FREC;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0xF0;	// Reset display clock divide ratio/oscillator frequency
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_PRE_CHARGE_PERIOD;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x22;	// Reset Pre-charge Period
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_COM_PINS_HARDWARE_CONFIGURATION;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x12;	// Reset COM Pins Hardware Configuration
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_VCOMH_DESELECT_LEVEL;
    ssd->dma_buffer[ssd->dma_bf_iw++] = 0x20;	// Set V COMH Deselect Level 0.77xVcc
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_INTERNAL_CHARGE_PUMP;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_INTERNAL_CHARGE_PUMP_ON;	// Enable internal charge pump
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_DISPLAY_ON;
    ssd->display_on = 1;

    // Inicializo el buffer local en 0
    for(uint16_t i = 0; i < _SSD1306_DEFAULT_TEXT_ROWS; i++)
        for(uint16_t j = 0; j < _SSD1306_DEFAULT_TEXT_COLUMNS; j++)
            ssd->data_buffer[i][j] = 0x00;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Cambia el modo, obliga a una limpiueza de pantalla e inicializa el display de foma a decuada para este modo
void ssd1306_i2c_set_mode(_ssd1306_i2c *ssd, uint8_t mode){
    ssd->mode = mode;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Envia comando de encendido al display
uint8_t ssd1306_i2c_set_display_on(_ssd1306_i2c *ssd){
    if(ssd->dma_status != 0) return 1;
    ssd->display_on = 1;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_SINGLE_BYTE_COMMAND;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_DISPLAY_ON;
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Envia comando de apagado al display
uint8_t ssd1306_i2c_set_display_off(_ssd1306_i2c *ssd){
    if(ssd->dma_status != 0) return 1;
    ssd->display_on = 0;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_SINGLE_BYTE_COMMAND;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_DISPLAY_OFF;
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Retorna 1 si el display esta encendido
uint8_t ssd1306_i2c_is_display_on(_ssd1306_i2c *ssd){
    return ssd->display_on;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Envia comando de ajuste de contraste
void ssd1306_i2c_set_conntrast(_ssd1306_i2c *ssd, uint8_t conntrast){
    ssd->curr_conntrast = conntrast;
    ssd->set_options |= 0b00001000;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Envia comando de seteo pantalla normal
void ssd1306_i2c_set_normal(_ssd1306_i2c *ssd){
    ssd->set_options |= 0b10000000;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Envia comando de seteo pantalla inverse
void ssd1306_i2c_set_inverse(_ssd1306_i2c *ssd){
    ssd->set_options |= 0b01000000;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Rotacion de pantalla 0 grados
void ssd1306_i2c_set_rotate_0(_ssd1306_i2c *ssd){
    ssd->set_options |= 0b00100000;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Rotacion de pantalla 180 grados
void ssd1306_i2c_set_rotate_180(_ssd1306_i2c *ssd){
    ssd->set_options |= 0b00010000;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Desabilitar update automatico del display
void ssd1306_i2c_freeze(_ssd1306_i2c *ssd){
    ssd->display_freeze = 1;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Rehabilitar update automatico del display
void ssd1306_i2c_unfreeze(_ssd1306_i2c *ssd){
    ssd->display_freeze = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Attach de una funcion que recibe una direccion i2c, un array de datos y un tamaño
void ssd1306_i2c_attach_send_data(_ssd1306_i2c *ssd, transmit_to_display_def f){
    ssd->send_data = f;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Notifica que un llamado por callback a send_data (funcion enlazada con  ssd1306_i2c_attach) ya finalizó y se puede reescribir el buffer de envio
void ssd1306_i2c_send_complete(_ssd1306_i2c *ssd){
    ssd->TX_sent++;
    ssd->dma_status = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Agrega el texto a la linea especificada - char *data es un string terminado en '\0' - Setea fuente, fixed y spacing
void ssd1306_i2c_put_text(_ssd1306_i2c *ssd, const char *data, uint8_t at_line, uint8_t font, uint8_t fixed, uint8_t spacing){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    /* Seteo byte 0
     * 		[b7]: 1 si la fila es fija, 0 si hace escroll
     * 		[b6, b5, b4]: los pixeles de separacion entre lineas (0-7)
     * 		[b3, b2, b1, b0]: fuente a usar en dicha linea, hasta 16 fuentes
     */
    ssd->data_buffer[at_line][0] = 0;
    ssd->data_buffer[at_line][0] |= ((fixed != 0)<<7);
    ssd->data_buffer[at_line][0] |= ((spacing & 0b00000111)<<4);
    ssd->data_buffer[at_line][0] |= (font & 0b00001111);
    uint8_t i = 1;
    while(i < (_SSD1306_DEFAULT_TEXT_COLUMNS - 1) && *data)
        ssd->data_buffer[at_line][i++] = *(data++);
    ssd->data_buffer[at_line][i] = '\0';
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Setea un unico caracter al inicio de la linea especificada - Setea fuente, fixed y spacing
void ssd1306_i2c_put_char(_ssd1306_i2c *ssd, const char data, uint8_t at_line, uint8_t font, uint8_t fixed, uint8_t spacing){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    /* Seteo byte 0
     * 		[b7]: 1 si la fila es fija, 0 si hace escroll
     * 		[b6, b5, b4]: los pixeles de separacion entre lineas (0-7)
     * 		[b3, b2, b1, b0]: fuente a usar en dicha linea, hasta 16 fuentes
     */
    ssd->data_buffer[at_line][0] = 0;
    ssd->data_buffer[at_line][0] |= ((fixed != 0)<<7);
    ssd->data_buffer[at_line][0] |= ((spacing & 0b00000111)<<4);
    ssd->data_buffer[at_line][0] |= (font & 0b00001111);
    uint8_t i = 1;
    if(i < (_SSD1306_DEFAULT_TEXT_COLUMNS - 1) && data)
        ssd->data_buffer[at_line][i++] = data;
    ssd->data_buffer[at_line][i] = '\0';
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Agrega el texto a la linea especificada, al final del texto ya existente - char *data es un string terminado en '\0'
void ssd1306_i2c_add_text(_ssd1306_i2c *ssd, const char *data, uint8_t at_line){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    uint16_t i = 1;
    while(ssd->data_buffer[at_line][i])	// Busco el marcador '\0'
        i++;
    while(i < (_SSD1306_DEFAULT_TEXT_COLUMNS - 1) && *data)
        ssd->data_buffer[at_line][i++] = *(data++);
    ssd->data_buffer[at_line][i] = '\0';
}


//---------------------------------------------------------------------------------------------------------------------------------------
// Agrega el caracter a la linea especificada, al final del texto ya existente
void ssd1306_i2c_add_char(_ssd1306_i2c *ssd, const char data, uint8_t at_line){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    uint16_t i = 1;
    while(ssd->data_buffer[at_line][i])	// Busco el marcador '\0'
        i++;
    if(i < (_SSD1306_DEFAULT_TEXT_COLUMNS -1) && data)
        ssd->data_buffer[at_line][i++] = data;
    ssd->data_buffer[at_line][i] = '\0';
}


//---------------------------------------------------------------------------------------------------------------------------------------
// Setea la fuente de la linea especificada
void ssd1306_i2c_set_font(_ssd1306_i2c *ssd, uint8_t at_line, uint8_t font){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    ssd->data_buffer[at_line][0] &= 0b11110000;
    ssd->data_buffer[at_line][0] |= (font & 0b00001111);
}


//---------------------------------------------------------------------------------------------------------------------------------------
// Setea el espaciado de la linea especificada
void ssd1306_i2c_set_spacing(_ssd1306_i2c *ssd, uint8_t at_line, uint8_t spacing){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    ssd->data_buffer[at_line][0] &= 0b10001111;
    ssd->data_buffer[at_line][0] |= ((spacing & 0b00000111)<<4);
}


//---------------------------------------------------------------------------------------------------------------------------------------
// Setea si la linea especificada es fija o no
void ssd1306_i2c_set_fixed(_ssd1306_i2c *ssd, uint8_t at_line, uint8_t fixed){
    if(at_line >= _SSD1306_DEFAULT_TEXT_ROWS) return;
    ssd->data_buffer[at_line][0] &= 0b01111111;
    ssd->data_buffer[at_line][0] |= ((fixed != 0)<<7);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Limpia de texto la linea especificada
void ssd1306_i2c_clean_text(_ssd1306_i2c *ssd, uint8_t at_line){
    for(uint8_t i = 0; i < _SSD1306_DEFAULT_TEXT_COLUMNS ; i++)
        ssd->data_buffer[at_line][i] = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Agrega el texto en modo consola - char *data es un string terminado en '\0'
void ssd1306_i2c_add_console_text(_ssd1306_i2c *ssd, const char *data){
    if(ssd->console_cur_line == _SSD1306_DEFAULT_TEXT_ROWS){
        _move_lines_up(ssd);
        ssd->console_cur_line--;
    }
    ssd->data_buffer[ssd->console_cur_line][0] = 0;
    ssd->data_buffer[ssd->console_cur_line][0] |= (1<<7);
    ssd->data_buffer[ssd->console_cur_line][0] |= ((_SSD1306_CONSOLE_SPACING & 0b00000111)<<4);
    ssd->data_buffer[ssd->console_cur_line][0] |= (_SSD1306_CONSOLE_FONT & 0b00001111);
    uint8_t i = 1;
    while(i < (_SSD1306_DEFAULT_TEXT_COLUMNS - 1) && *data)
        ssd->data_buffer[ssd->console_cur_line][i++] = *(data++);
    ssd->data_buffer[ssd->console_cur_line][i] = '\0';
    ssd->console_cur_line++;

    _compute_tallest_column(ssd);
    if(ssd->vd_vmax > (ssd->display_pages*8))
        ssd->vd_voffset = ssd->vd_vmax-(ssd->display_pages*8);
    else
        ssd->vd_voffset = 0;

}


//---------------------------------------------------------------------------------------------------------------------------------------
// Establece un external bitmap
void ssd1306_i2c_set_exteral_bitmap(_ssd1306_i2c *ssd, const uint8_t *bitmap){
    ssd->extern_bitmap = bitmap;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Dibuja un pixel en el bitmap
void ssd1306_i2c_bitmap_draw_px(_ssd1306_i2c *ssd, const uint8_t x, const uint8_t y){
    if(x >= _SSD1306_DEFAULT_BITMAP_COLUMNS || y >= (_SSD1306_DEFAULT_BITMAP_PAGES * 8)) return;
    uint16_t pos = ((y/8) * _SSD1306_DEFAULT_BITMAP_COLUMNS) + x; // Ubicacion lineal del byte
    ssd->data_buffer[pos/_SSD1306_DEFAULT_TEXT_COLUMNS][pos%_SSD1306_DEFAULT_TEXT_COLUMNS] |= 1<<(y % 8);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Dibuja una linea que va del punto (x0, y0) al punto (x1, y1) utilizando EFLA by Po-Han Lin (http://www.edepot.com/algorithm.html)
void ssd1306_i2c_bitmap_draw_line(_ssd1306_i2c *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1){
    uint8_t yLonger = 0;
    int16_t incrementVal, endVal;
    int16_t shortLen = y1 - y0;
    int16_t longLen = x1 - x0;
    if (abs(shortLen)>abs(longLen)) {
        int16_t swap = shortLen;
        shortLen = longLen;
        longLen = swap;
        yLonger = 1;
    }

    endVal=longLen;
    if(longLen < 0){
        incrementVal = -1;
        longLen = -longLen;
    }else{
        incrementVal=1;
    }

    int16_t decInc;
    if(longLen == 0) decInc=0;
    else decInc = (shortLen << 8) / longLen;
    int16_t j = 0;
    if (yLonger) {
        for(int16_t i = 0; i != endVal ; i += incrementVal){
            ssd1306_i2c_bitmap_draw_px(ssd, x0 + (j >> 8), y0 + i);
            j+=decInc;
        }
    }else{
        for(int16_t i = 0; i != endVal ; i += incrementVal){
            ssd1306_i2c_bitmap_draw_px(ssd, x0 + i, y0 + (j >> 8));
            j+=decInc;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Dibuja un cuadrado cuyo vertice superior derecho está en (x0, y0) y su vertice superior izquierdo es el punto (x1, y1)
void ssd1306_i2c_bitmap_draw_square(_ssd1306_i2c *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1){
    ssd1306_i2c_bitmap_draw_line(ssd, x0, y0, x1, y1);
    ssd1306_i2c_bitmap_draw_line(ssd, x1, y1, x1+(y0-y1), y1+(x1-x0));
    ssd1306_i2c_bitmap_draw_line(ssd, x0, y0, x0+(y0-y1), y0+(x1-x0));
    ssd1306_i2c_bitmap_draw_line(ssd, x0+(y0-y1), y0+(x1-x0), x1+(y0-y1), y1+(x1-x0));
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Dibuja un rectangulo alineado a la pantalla cuyo vertice superior derecho está en (x0, y0) y su vertice inferior izquierdo es el punto (x1, y1)
void ssd1306_i2c_bitmap_draw_rectangle(_ssd1306_i2c *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1){
    ssd1306_i2c_bitmap_draw_line(ssd, x0, y0, x1, y0);
    ssd1306_i2c_bitmap_draw_line(ssd, x1, y0, x1, y1);
    ssd1306_i2c_bitmap_draw_line(ssd, x1, y1, x0, y1);
    ssd1306_i2c_bitmap_draw_line(ssd, x0, y1, x0, y0);
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hace scrool down la cantidad de pixeles indicados
void ssd1306_i2c_scroll_down(_ssd1306_i2c *ssd, uint8_t px){
    ssd->vd_voffset += px;
    if(ssd->vd_voffset > ssd->vd_vmax)
        ssd->vd_voffset = ssd->vd_vmax;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hace scrool up la cantidad de pixeles indicados
void ssd1306_i2c_scroll_up(_ssd1306_i2c *ssd, uint8_t px){
    if(ssd->vd_voffset < px)
        ssd->vd_voffset = 0;
    else
        ssd->vd_voffset -=px;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hace scrool rigth la cantidad de pixeles indicados
void ssd1306_i2c_scroll_rigth(_ssd1306_i2c *ssd, uint8_t px){
    ssd->vd_hoffset += px;
    if(ssd->vd_hoffset > ssd->vd_hmax)
        ssd->vd_hoffset = ssd->vd_hmax;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Hace scrool left la cantidad de pixeles indicados
void ssd1306_i2c_scroll_left(_ssd1306_i2c *ssd, uint8_t px){
    if(ssd->vd_hoffset < px)
        ssd->vd_hoffset = 0;
    else
        ssd->vd_hoffset -=px;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Restet de scrool a 0
void ssd1306_i2c_scroll_reset(_ssd1306_i2c *ssd){
    ssd->vd_hoffset = 0;
    ssd->vd_voffset = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Realiza las tareas de envio por DMA y actualización, period es una aproximacion de la cantidad de tiempo entre llamados a work, para manejar el scroll automatico
void ssd1306_i2c_work(_ssd1306_i2c *ssd, uint8_t period){

    if(ssd->TX_errors > _SSD1306_DMA_I2C_TX_MAX_ERRORS || (ssd->display_freeze == 1 && ssd->display_update == 0))	return;

    // Autoscroll, si corresponde
    if(ssd->mode == _SSD1306_MODE_TEXT_AUTOSCROLL){
        ssd->scroll_cnrt += period;
        if(ssd->display_update == 0 && ssd->scroll_cnrt >= _SSD1306_AUTOSCROLL_TIME_MS &&
            (ssd->vd_hoffset != 0 || ssd->scroll_cnrt > _SSD1306_AUTOSCROLL_SLEEP_MS)){     // Espera en el inicio de linea antes de hacer scroll
            ssd->scroll_cnrt = 0;
            if(ssd->vd_hmax > ssd->display_columns){
                ssd->vd_hoffset += _SSD1306_AUTOSCROLL_PX_HORIZONTAL;
                if(ssd->vd_hoffset > ssd->vd_hmax - ssd->display_columns + _SSD1306_AUTOSCROLL_PX_HORIZONTAL)
                    ssd->vd_hoffset = 0;
            }
            if(ssd->vd_vmax > ssd->display_pages*8 && ssd->vd_hoffset == 0){
                ssd->vd_voffset += _SSD1306_AUTOSCROLL_PX_VERTICAL;
                if(ssd->vd_voffset > ssd->vd_vmax - (ssd->display_pages*8) + _SSD1306_AUTOSCROLL_PX_VERTICAL)
                    ssd->vd_voffset = 0;
            }
            ssd->display_update = 1;
        }
    }

    // dma_status utilizado para asignar tiempo de espera
    if(ssd->dma_status > 0){
        ssd->dma_status--;
        return;
    }

    // Si hay datos en el buffer se envian
    if(ssd->dma_bf_iw != 0){
        if(ssd->send_data(ssd->i2c_address, ssd->dma_buffer, ssd->dma_bf_iw) == 1){
            ssd->dma_bf_iw = 0;
        }else{
            ssd->TX_errors++;
        }
        ssd->dma_status = 2;
        return;
    }

    // Hay un pedido de actualizar el display, si llegue aqui dma_bf_iw es 0
    if(ssd->display_update != 0){
        if(ssd->display_update > 4){
            // Primero debo asegurar los indices del display el principio de la grilla
            _set_addressing(ssd, 0, ssd->display_columns, 0, ssd->display_pages);

            // Aplico cambios si corresponde
            if(ssd->set_options){
                if(ssd->set_options & 0b10000000){
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_NORMAL;
                }else if(ssd->set_options & 0b01000000){
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_INVERSE;
                }

                if(ssd->set_options & 0b00100000){
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_NORMAL;
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_SEGMENT_REMAP_NORMAL;
                }else if(ssd->set_options & 0b00010000){
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_REMAPED;
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_SEGMENT_REMAP_REMAPED;
                }

                if(ssd->set_options & 0b00001000){
                    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_CONNTRAST;
                    ssd->dma_buffer[ssd->dma_bf_iw++] = ssd->curr_conntrast;
                }
                ssd->set_options = 0;
            }

            ssd->display_curr_column = 0;
            ssd->display_curr_page = 0;
            ssd->display_update -= 3;
            return;
        }

        if(ssd->display_update > 2){	// Limpieza del display
            ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_MULTIPLE_BYTE_DATA;
            while(ssd->dma_bf_iw < _SSD1306_DMA_BUFFER_SIZE){
                ssd->dma_buffer[ssd->dma_bf_iw++] = 0x00;  // Punto de test
                ssd->display_curr_column++;
                if(ssd->display_curr_column == ssd->display_columns){
                    ssd->display_curr_column = 0;
                    ssd->display_curr_page++;
                    if(ssd->display_curr_page == ssd->display_pages){
                        ssd->display_curr_page = 0;
                        ssd->display_update -=3;
                        return;
                    }
                }
            }
            return;
        }

        switch(ssd->mode){
            case _SSD1306_MODE_TEXT:
            case _SSD1306_MODE_TEXT_AUTOSCROLL:
            case _SSD1306_MODE_TEXT_CONSOLE:
                _draw_text(ssd);
                break;
            case _SSD1306_MODE_BITMAP:
                _draw_bitmap(ssd);
                break;
            case _SSD1306_MODE_EXTERNAL_BITMAP:
                _draw_ext_bitmap(ssd);
                break;
        }
        // Se cargaron nuevos datos en el buffer, se envian
        if(ssd->dma_bf_iw != 0){
            if(ssd->send_data(ssd->i2c_address, ssd->dma_buffer, ssd->dma_bf_iw) == 1){
                ssd->dma_bf_iw = 0;
            }else{
                ssd->TX_errors++;
            }
            ssd->dma_status = 2;
            return;
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------------------------
// Limpia la pantalla y los buffers
void ssd1306_i2c_clear(_ssd1306_i2c *ssd){
    // Inicializo el buffer local en 0
    for(uint16_t i = 0; i < _SSD1306_DEFAULT_TEXT_ROWS; i++)
        for(uint16_t j = 0; j < _SSD1306_DEFAULT_TEXT_COLUMNS; j++)
            ssd->data_buffer[i][j] = 0x00;
    ssd->display_update = 6;
    ssd->vd_hmax = 0;
    ssd->vd_vmax = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Fuerza un update de la pantalla
void ssd1306_i2c_update(_ssd1306_i2c *ssd){
    ssd->display_update = 1;
    switch (ssd->mode) {
    case _SSD1306_MODE_TEXT_AUTOSCROLL:
        if(_SSD1306_AUTOSCROLL_ON_UPDATE_RESUME == 0){
            ssd->scroll_cnrt = 0;
            ssd->vd_voffset = 0;
            ssd->vd_hoffset = 0;
        }
    case _SSD1306_MODE_TEXT_CONSOLE:    // no break, is ok
    case _SSD1306_MODE_TEXT:
        _compute_tallest_column(ssd);
        _compute_widest_row(ssd);
        break;
    }
}




//---------------------------------------------------------------------------------------------------------------------------------------
// PRIVADO
//---------------------------------------------------------------------------------------------------------------------------------------
static void _set_addressing(_ssd1306_i2c *ssd, uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end){
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_MULTIPLE_BYTE_COMMAND;
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_COLUMN_ADDRESS;
    ssd->dma_buffer[ssd->dma_bf_iw++] = col_start;	// Column address start, default 0
    ssd->dma_buffer[ssd->dma_bf_iw++] = col_end-1;	// Column address end, default 127d
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_COMMAND_SET_PAGE_ADDRESS;
    ssd->dma_buffer[ssd->dma_bf_iw++] = page_start;	// Page address start, default 0
    ssd->dma_buffer[ssd->dma_bf_iw++] = page_end-1;	// Page address end, default 7d
}

//---------------------------------------------------------------------------------------------------------------------------------------
static void _draw_text(_ssd1306_i2c *ssd){
    if(ssd->display_update == 1){	// Actualizo display_update y punteros
        ssd->display_update = 5;
        ssd->display_curr_page = 0;
        ssd->display_curr_column = 0;
        return;
    }

    // comando de inicio de envio de datos
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_MULTIPLE_BYTE_DATA;
    uint8_t data;
    while(ssd->dma_bf_iw < _SSD1306_DMA_BUFFER_MAX_PKG){
        // Magic goes here
        data = 0;
        int8_t line_starts_curr_page = _line_get_line_by_px(ssd, (ssd->display_curr_page * 8) + ssd->vd_voffset);
        int8_t line_ends_curr_page = _line_get_line_by_px(ssd, ((ssd->display_curr_page+1) * 8) + ssd->vd_voffset - 1);
        if(line_starts_curr_page >= 0){
            if(_line_is_fixed(ssd, line_starts_curr_page))
                data = _line_get_column_data_px(ssd, line_starts_curr_page, ssd->display_curr_column, ((ssd->display_curr_page * 8) + ssd->vd_voffset) - _line_get_start_at_px(ssd, line_starts_curr_page));
            else
                data = _line_get_column_data_px(ssd, line_starts_curr_page, (ssd->display_curr_column + ssd->vd_hoffset), ((ssd->display_curr_page * 8) + ssd->vd_voffset) - _line_get_start_at_px(ssd, line_starts_curr_page));

            // La pagina no esta compuesto solo por bits de una linea y la siguiente linea existe
            if(line_ends_curr_page >= 0 && line_starts_curr_page != line_ends_curr_page){
                if(_line_is_fixed(ssd, line_ends_curr_page))
                    data |= _line_get_column_data_px(ssd, line_ends_curr_page, ssd->display_curr_column, 0) << (_line_get_start_at_px(ssd, line_ends_curr_page) - ((ssd->display_curr_page * 8) + ssd->vd_voffset));
                else
                    data |= _line_get_column_data_px(ssd, line_ends_curr_page, (ssd->display_curr_column + ssd->vd_hoffset), 0) << (_line_get_start_at_px(ssd, line_ends_curr_page) - ((ssd->display_curr_page * 8) + ssd->vd_voffset));
            }
        }   // Else data = 0;

        ssd->dma_buffer[ssd->dma_bf_iw++] = data;
        ssd->display_curr_column++;

        if(ssd->display_curr_column == ssd->display_columns){
            ssd->display_curr_column = 0;
            ssd->display_curr_page++;
            if(ssd->display_curr_page == ssd->display_pages){	// Termine de dibujar
                ssd->display_update = 0;
                ssd->display_curr_page = 0;
                return;
            }
        }
    }

}

//---------------------------------------------------------------------------------------------------------------------------------------
static void _draw_bitmap(_ssd1306_i2c *ssd){
    // Determino la ubicacion del bitmap, si es mas pequeño que el mapa de bits del display
    uint8_t i = (ssd->display_pages - _SSD1306_DEFAULT_BITMAP_PAGES)/2;
    uint8_t j = (ssd->display_columns - _SSD1306_DEFAULT_BITMAP_COLUMNS)/2;

    if(ssd->display_update == 1){	// Actualizo display_update y punteros
        ssd->display_update = 5;
        ssd->display_curr_page = 0;
        ssd->display_curr_column = 0;
        return;
    }

    // comando de inicio de envio de datos
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_MULTIPLE_BYTE_DATA;

    while(ssd->dma_bf_iw < _SSD1306_DMA_BUFFER_SIZE){
        if(ssd->display_curr_page < i || ssd->display_curr_column < j || ssd->display_curr_page >= (i + _SSD1306_DEFAULT_BITMAP_PAGES) || ssd->display_curr_column >= (j + _SSD1306_DEFAULT_BITMAP_COLUMNS))
            ssd->dma_buffer[ssd->dma_bf_iw++] = 0;
        else{
            uint16_t pos = ((ssd->display_curr_page - i) * _SSD1306_DEFAULT_BITMAP_COLUMNS) + (ssd->display_curr_column - j); // Ubicacion lineal del byte
            ssd->dma_buffer[ssd->dma_bf_iw++] = ssd->data_buffer[pos/_SSD1306_DEFAULT_TEXT_COLUMNS][pos%_SSD1306_DEFAULT_TEXT_COLUMNS];
        }
        ssd->display_curr_column++;
        if(ssd->display_curr_column == ssd->display_columns){
            ssd->display_curr_column = 0;
            ssd->display_curr_page++;
            if(ssd->display_curr_page == ssd->display_pages){	// Termine de dibujar
                ssd->display_update = 0;
                ssd->display_curr_page = 0;
                return;
            }
        }
    }
}



//---------------------------------------------------------------------------------------------------------------------------------------
static void _draw_ext_bitmap(_ssd1306_i2c *ssd){
    if(ssd->display_update == 1){	// Actualizo display_update y punteros
        ssd->display_update = 5;
        ssd->display_curr_page = 0;
        ssd->display_curr_column = 0;
        return;
    }

    // comando de inicio de envio de datos
    ssd->dma_buffer[ssd->dma_bf_iw++] = _SSD1306_CONTROL_MULTIPLE_BYTE_DATA;

    while(ssd->dma_bf_iw < _SSD1306_DMA_BUFFER_SIZE){
        ssd->dma_buffer[ssd->dma_bf_iw++] = ssd->extern_bitmap[(ssd->display_curr_page * ssd->display_columns) + ssd->display_curr_column];
        ssd->display_curr_column++;
        if(ssd->display_curr_column == ssd->display_columns){
            ssd->display_curr_column = 0;
            ssd->display_curr_page++;
            if(ssd->display_curr_page == ssd->display_pages){	// Termine de dibujar
                ssd->display_update = 0;
                ssd->display_curr_page = 0;
                return;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
static void _move_lines_up(_ssd1306_i2c *ssd){
    for(uint8_t i = 1; i < _SSD1306_DEFAULT_TEXT_ROWS; i++){
        for(uint8_t j = 0; j < _SSD1306_DEFAULT_TEXT_COLUMNS; j++){
            ssd->data_buffer[i-1][j] = ssd->data_buffer[i][j];
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
static void _compute_widest_row(_ssd1306_i2c *ssd){
    uint16_t tmp = 0;
    ssd->vd_hmax = 0;
    for(uint8_t i = 0; i < _SSD1306_DEFAULT_TEXT_ROWS; i++){
        tmp = _line_get_total_width_px(ssd, i);
        if(tmp > ssd->vd_hmax && !_line_is_fixed(ssd, i))   // No me interesa el ancho de las lineas fijas
            ssd->vd_hmax = tmp;
    }
    if(ssd->vd_hmax <= ssd->display_columns) // Me aseguro qeu el display no quede desplazado si el ancho maximo se ha reducido
       ssd->vd_hoffset = 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// No hay una columna mas alta, todas son iguales, pero para mantener la uniforidad... Esto es privado despues de todo ¿que hace usted acá?
// en _line_get_height_px: la altura de una linea no seteada es 0
static void _compute_tallest_column(_ssd1306_i2c *ssd){
    ssd->vd_vmax = 0;
    for(uint8_t i = 0; i < _SSD1306_DEFAULT_TEXT_ROWS; i++)
            ssd->vd_vmax += _line_get_height_px(ssd, i);
    if(ssd->vd_vmax <= ssd->display_pages*8) // Me aseguro qeu el display no quede desplazado si la altura total se ha reducido
       ssd->vd_voffset = 0;
}



//---------------------------------------------------------------------------------------------------------------------------------------
static uint8_t _line_is_fixed(_ssd1306_i2c *ssd, uint8_t line){
    return (ssd->data_buffer[line][0] & 0b10000000) == 0b10000000;
}

//---------------------------------------------------------------------------------------------------------------------------------------
static const uint8_t* _line_get_font(_ssd1306_i2c *ssd, uint8_t line){
    return _available_fonts[ssd->data_buffer[line][0] & 0b00001111];
}

//---------------------------------------------------------------------------------------------------------------------------------------
static uint8_t _line_get_inline_px(_ssd1306_i2c *ssd, uint8_t line){
    return (ssd->data_buffer[line][0] & 0b01110000)>>4;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Retorna la altura en pixels de la linea line segun su fuente y espaciado, 0 si la linea no esta seteada
static uint8_t _line_get_height_px(_ssd1306_i2c *ssd, uint8_t line){
    if(ssd->data_buffer[line][1])  // Si no empieza por un caracter no nulo la altura es 0
        return _fonts_get_char_height(_line_get_font(ssd, line)) + 2*_line_get_inline_px(ssd, line);
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
static uint16_t _line_get_start_at_px(_ssd1306_i2c *ssd, uint8_t line){
    uint16_t retval = 0;
    for(uint8_t i = 0; i < line ; i++)
        retval += _line_get_height_px(ssd, i);
    return retval;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Ancho total de la linea en px
static uint16_t _line_get_total_width_px(_ssd1306_i2c *ssd, uint8_t line){
    uint8_t i = 1;
    const uint8_t *f = _line_get_font(ssd, line);
    uint16_t retval = 0;
    while(ssd->data_buffer[line][i] && i < _SSD1306_DEFAULT_TEXT_COLUMNS)
        retval += _fonts_get_char_width(f, ssd->data_buffer[line][i++]) + 1;	// Espacio entre caracteres
    return retval;
}

//---------------------------------------------------------------------------------------------------------------------------------------
//Retorna la columna de bits correspondiente a la columna px de la linea line con un offset up-down offset (offset tambien cuenta para los bits de interlineado)
static uint8_t _line_get_column_data_px(_ssd1306_i2c *ssd, uint8_t line, uint16_t px, uint8_t offset){
    uint8_t i = 1;
    uint8_t inline_px;
    uint16_t cur = 0;
    const uint8_t *f = _line_get_font(ssd, line);
    while(ssd->data_buffer[line][i] && i < _SSD1306_DEFAULT_TEXT_COLUMNS){
        if(cur + _fonts_get_char_width(f, ssd->data_buffer[line][i]) > px){
            inline_px =  _line_get_inline_px(ssd, line);
            if(offset < inline_px)
                return (_fonts_get_column_byte(f, ssd->data_buffer[line][i],  px - cur, 0) << (inline_px - offset));
            else
                return _fonts_get_column_byte(f, ssd->data_buffer[line][i],  px - cur, offset - inline_px);
        }else if(cur + _fonts_get_char_width(f, ssd->data_buffer[line][i]) == px){
            return 0;
        }
        cur += _fonts_get_char_width(f, ssd->data_buffer[line][i]) + 1;	// Espacio entre caracteres
        i++;
    }
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// Retorna la linea a la que corresponde el px dado, -1 si se recorrio todas las lineas y no se alcanza dicho pixel
static int8_t _line_get_line_by_px(_ssd1306_i2c *ssd, uint16_t px){
    uint8_t line = 0;
    uint16_t i = 0;
    while(line < _SSD1306_DEFAULT_TEXT_ROWS){
        i += _line_get_height_px(ssd, line);
        if( i > px)
            return line;
        line++;
    }
    return -1;
}


//---------------------------------------------------------------------------------------------------------------------------------------
// Retorna el valor absoluto
static uint32_t abs(int32_t a){
    if(a < 0) return -a;
    return a;
}
