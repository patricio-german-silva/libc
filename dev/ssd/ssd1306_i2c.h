/*
 * ssd1306_i2c.h
 *
 *  Created on: Dec 3, 2022
 *      Author: psilva
 *
 * Version: 2
 *
 * Driver para el controlador de display OLED PLED ssd1306
 *
 *
 * El modo de operacion define como se va a comportar el display
 * Las funciones pensadas para un modo de operacion pueden no tener sentido en otro modo,
 * sinembargo eso no se controla por soft, y pueden hacer crashear la libreria
 *
 * Modos de Operacion:
 *
 * _SSD1306_MODE_TEXT
 *
 * Texto simple, el bufer de datos contiene los codigos ascii del texto a mostrar, el
 * valor de _SSD1306_DEFAULT_TEXT_ROWS y _SSD1306_DEFAULT_TEXT_COLUMNS define cuantos caracteres
 * por linea se manejan y cuantas lineas hay. Se puede hacer scrol vertical y horizontal para mostrar
 * todo el texto
 * Cada linea puede tener una fuente y espacio de interlineado diferente, tambien se puede definir como
 * linea fija, lo que implica que no se hace scroll horizontal de esa linea en particular
 *
 * _SSD1306_MODE_TEXT_AUTOSCROLL
 *
 * Igual a _SSD1306_MODE_TEXT pero no se puede hacer scroll manual. Si una linea posee mas caracteres de
 * los que se pueden mostrar horizontalmente se hace scroll automatico, Si hay mas lineas de las que se
 * pueden mostrar se hace scroll vertical, Si suceden ambos casos se hacen scroll horizontal primero y luego vertical
 * Las lineas establecidas como fijas no hacen scroll horizontal, si vertical
 *
 * _SSD1306_MODE_TEXT_CONSOLE
 *
 * Se comporta como una consola donde se puede hacer scroll vertical, el texto simplemente se añade al final
 * La fuente e interlineado es la misma para todas las lineas. Lo ideal es que sea una fuente monoespacio
 * Cada llamada a ssd1306_i2c_add_console_text agrega una nueva linea y el scroll se mantiene abajo con cada
 * inserción de texto
 *
 * _SSD1306_MODE_BITMAP
 *
 * El buffer de datos es un espejo del mapa de bits en pantalla, si el buffer es mas chico que la pantalla
 * la imagen se centra. Solo dispone de funciones para dibujar puntos individuales, lineas rectas entre
 * dos puntos indicados, rectangulos alineados o cuadrados
 *
 * _SSD1306_MODE_EXTERNAL_BITMAP
 *
 * Se setea un mapa de bits externo que sebe tener exactamente el mismo tamaño que la pantalla
 *
 */


#ifndef INC_SSD1306_I2C_H_
#define INC_SSD1306_I2C_H_

#include "stdint.h"
#include "fonts.h"


/* CONFIGURACIONES POR DEFAULT +/
 *
 * Tamaño del buffer local. Default 1024 bytes distribuido en 16 filas virtuales de 63 caracteres cada una que se pueden hacer scroll
 * El primer byte de cada fila indica:
 * 		[b7]: 1 si la fila es fija horizontalmente, 0 si hace scroll horizontal
 * 		[b6, b5, b4]: los pixeles de separacion con la linea anterior y posterior (0-7)
 * 		[b3, b2, b1, b0]: fuente a usar en dicha linea, hasta 16 fuentes
 * En modo bitmap se accede a esta matriz con una distribución diferente de subindices, pero siempre respetando el
 * tamaño. Se debe cumplir (no assert) _SSD1306_DEFAULT_TEXT_ROWS * _SSD1306_DEFAULT_TEXT_COLUMNS = _SSD1306_DEFAULT_BITMAP_PAGES *  _SSD1306_DEFAULT_BITMAP_COLUMNS
 */
#define _SSD1306_DEFAULT_TEXT_ROWS									16
#define _SSD1306_DEFAULT_TEXT_COLUMNS								64

/* Distrubucon del buffer cuando se lo maneja como mapa de bits, idealmente tiene la misma distribucion de pixeles que el display pero puede ser mas pequeño */
#define _SSD1306_DEFAULT_BITMAP_PAGES								8
#define _SSD1306_DEFAULT_BITMAP_COLUMNS								128


/*
 * _SSD1306_DMA_BUFFER_SIZE: tamaño del Buffer de datos que se envian al i2c
 * por DMA, el minimo sin modificar el codigo es 32, que es la cantidad de
 * bytes que se envian durante la configuracion init
 * _SSD1306_DMA_BUFFER_MAX_PKG: solo afecta al modo texto, es el tamaño maximo
 * de bytes que se van a cargar en el buffer para enviar por DMA de una vez.
 * si bien el envio no es problema, el armado del paquete en modo texto puede
 * consume mucho tiempo de procesamiento, un MAX_PKG pequeño reduce el tiempo
 * ciego.
 * _SSD1306_DMA_I2C_TX_MAX_ERRORS: Si se supera esta cantidad de errores TX se
 * aborta toda comunicación con el dispositico I2C
 */
#define _SSD1306_DMA_BUFFER_SIZE									32
#define _SSD1306_DMA_BUFFER_MAX_PKG									17
#define _SSD1306_DMA_I2C_TX_MAX_ERRORS								10

/*
 * Modos de operacion
 */
#define _SSD1306_MODE_TEXT											0
#define _SSD1306_MODE_TEXT_AUTOSCROLL								1
#define _SSD1306_MODE_TEXT_CONSOLE                                  2
#define _SSD1306_MODE_BITMAP										3
#define _SSD1306_MODE_EXTERNAL_BITMAP								4

/*
 * Configuracion del autoscroll
 */
#define _SSD1306_AUTOSCROLL_TIME_MS    								500
#define _SSD1306_AUTOSCROLL_SLEEP_MS   								2000
#define _SSD1306_AUTOSCROLL_PX_VERTICAL								10
#define _SSD1306_AUTOSCROLL_PX_HORIZONTAL   						5

/*
 * Configuracion de consola
 */
#define _SSD1306_CONSOLE_FONT           							0
#define _SSD1306_CONSOLE_SPACING       								0



/*
 * sda commands
 */
#define _SSD1306_COMMAND_DISPLAY_ON									0xAF
#define _SSD1306_COMMAND_DISPLAY_OFF								0xAE
#define _SSD1306_COMMAND_ENTIRE_DISPLAY_ON							0xA4
#define _SSD1306_COMMAND_SET_CONNTRAST								0x81
#define _SSD1306_COMMAND_SET_NORMAL									0xA6
#define _SSD1306_COMMAND_SET_INVERSE								0xA7
#define _SSD1306_COMMAND_SET_MEMORY_ADDRESSING_MODE					0x20
#define _SSD1306_COMMAND_SET_MEMORY_ADDRESSING_MODE_HORIZONTAL		0x00
#define _SSD1306_COMMAND_SET_MEMORY_ADDRESSING_MODE_VERTICAL		0x01
#define _SSD1306_COMMAND_SET_MEMORY_ADDRESSING_MODE_PAGE			0x02
#define _SSD1306_COMMAND_SET_COLUMN_ADDRESS							0x21	//3 bytes command [command column_address_start column_address_end] column_address 0-127d
#define _SSD1306_COMMAND_SET_PAGE_ADDRESS							0x22	//3 bytes command [command page_address_start page_address_end] page_address 0-7d
#define _SSD1306_COMMAND_SET_DISPLAY_START_LINE						0x40
#define _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_NORMAL		0xC0	// _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_NORMAL y _SSD1306_COMMAND_SET_SEGMENT_REMAP_NORMAL: NORMAL
#define _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_REMAPED		0xC8	// _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_REMAPED y _SSD1306_COMMAND_SET_SEGMENT_REMAP_REMAPED: Rotated 180º
#define _SSD1306_COMMAND_SET_SEGMENT_REMAP_NORMAL					0xA0	// _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_REMAPED y _SSD1306_COMMAND_SET_SEGMENT_REMAP_NORMAL: Flip Vertical
#define _SSD1306_COMMAND_SET_SEGMENT_REMAP_REMAPED					0xA1	// _SSD1306_COMMAND_COM_OUTPUT_SCAN_DIRECTION_MODE_NORMAL y _SSD1306_COMMAND_SET_SEGMENT_REMAP_REMAPED: Flip Horizontal
#define _SSD1306_COMMAND_SET_MULTIPLEX_RATIO						0xA8
#define _SSD1306_COMMAND_SET_DISPLAY_OFFSET							0xD3
#define _SSD1306_COMMAND_SET_DISPLAY_CLK__OSCILLATOR_FREC			0xD5
#define _SSD1306_COMMAND_SET_PRE_CHARGE_PERIOD						0xD9
#define _SSD1306_COMMAND_SET_COM_PINS_HARDWARE_CONFIGURATION		0xDA
#define _SSD1306_COMMAND_SET_VCOMH_DESELECT_LEVEL					0xDB
#define _SSD1306_COMMAND_SET_INTERNAL_CHARGE_PUMP					0x8D
#define _SSD1306_COMMAND_SET_INTERNAL_CHARGE_PUMP_ON				0x14
#define _SSD1306_COMMAND_SET_INTERNAL_CHARGE_PUMP_OFF				0x10


/*
 * Control byte, define si lo que se envia a continuacion es dato o comando
 */
#define _SSD1306_CONTROL_MULTIPLE_BYTE_DATA							0x40
#define _SSD1306_CONTROL_MULTIPLE_BYTE_COMMAND						0x00
#define _SSD1306_CONTROL_SINGLE_BYTE_DATA							0xC0
#define _SSD1306_CONTROL_SINGLE_BYTE_COMMAND						0x80


/* Definicion de punteros a funcion*/
typedef uint8_t (*transmit_to_display_def)(uint16_t i2c_address, uint8_t *pData, uint16_t Size);


/* Estructura principal
 * dma_status != 0: se esta esperando a que finalice la transferencia DMA
 * dma_status = 0: ya se envio todo al i2c y se puede sobreescribir el buffer
 * display_update = 0: No hacer nada
 * display_update = 1: solicitud de update de usuario, se sigue la cadena correspopndiente para lograr el update
 * display_update = 2: Actualizar el display con los datos de data_buffer
 * display_update = 3: limpiar el display y pasar a update = 0
 * display_update = 4: limpiar el dispay y luego pasar a display_update = 2
 * display_update = 5: Mover punteros a (0,0) y luego pasar a display_update = 2
 * display_update = 6: Mover punteros a (0,0) y luego pasar a display_update = 3
 * display_update = 7: Mover punteros a (0,0) y luego pasar a display_update = 4
 * display_curr_column: siguiente columna a dibujar (0..display_columns-1)
 * display_curr_page: siguiente pagina a dibujar (0..(display_rows/8)-1)
 * */
typedef struct{
	// Configuracion
	uint16_t i2c_address;
	uint8_t display_pages;	// rows/8
	uint8_t display_columns;
	uint8_t mode;	 // default _SSD1306_MODE_TEXT

	// Buffers
	uint8_t dma_buffer[_SSD1306_DMA_BUFFER_SIZE];
	uint8_t dma_bf_iw;
	uint8_t data_buffer[_SSD1306_DEFAULT_TEXT_ROWS][_SSD1306_DEFAULT_TEXT_COLUMNS];
	const uint8_t *extern_bitmap;	// Un mapa de bits provisto externamente, el tamaño debe ser  display_rows * display_columns

	// Estado
	uint8_t dma_status;
	uint8_t display_update;
	uint8_t display_curr_column;	// Columna actual en la que se esta escribiendo en el display
	uint8_t display_curr_page;		// Pagina actual en la que se esta escribiendo en el display
    // offset entre los datos y dispay, en modo texto representa la ubicacion
	// del display respecto a los datos, los datos son mas grandes que el display
    uint16_t vd_hoffset;
    uint16_t vd_voffset;
    uint16_t vd_hmax;
    uint16_t vd_vmax;
	uint8_t display_on;
    uint16_t scroll_cnrt;
    uint8_t console_cur_line;

	uint32_t TX_errors;
	uint32_t TX_sent;

	/* Funcion de escritura de datos fuera de la libreria. Esta funcion debe devolver 1 cuando la transmision es correcta y 0 en caso contrario
	 * de otro modo se alcanzará el timeout y se suspenderá la comunicacion con el dispositivo
	 */
	transmit_to_display_def send_data;
}_ssd1306_i2c;


/*
 * Prototipos publicos
 */

// Inicializa el display
void ssd1306_i2c_init(_ssd1306_i2c *ssd, uint16_t address, uint8_t cols, uint8_t rows);

// Cambia el modo, obliga a una limpieza de pantalla
void ssd1306_i2c_set_mode(_ssd1306_i2c *ssd, uint8_t mode);

// Envia comando de encendido al display
uint8_t ssd1306_i2c_set_display_on(_ssd1306_i2c *ssd);

// Envia comando de apagado al display
uint8_t ssd1306_i2c_set_display_off(_ssd1306_i2c *ssd);

// Retorna 1 si el display esta encendido
uint8_t ssd1306_i2c_is_display_on(_ssd1306_i2c *ssd);

// Envia comando de ajuste de contraste
uint8_t ssd1306_i2c_set_conntrast(_ssd1306_i2c *ssd, uint8_t conntrast);

// Envia comando de seteo pantalla normal, 1=white pixel, 0=black pixel y rotacion de pantalla 0 grados
uint8_t ssd1306_i2c_set_normal(_ssd1306_i2c *ssd);

// Envia comando de seteo pantalla inverse, 1=black pixel, 0=white pixel
uint8_t ssd1306_i2c_set_inverse(_ssd1306_i2c *ssd);

// Rotacion de pantalla 180 grados
uint8_t ssd1306_i2c_set_rotate_180(_ssd1306_i2c *ssd);

// Attach de una funcion que recibe una direccion i2c, un array de datos y un tamaño
void ssd1306_i2c_attach_send_data(_ssd1306_i2c *ssd, transmit_to_display_def f);

// Notifica que un llamado por callback a send_data (funcion enlazada con  ssd1306_i2c_attach) ya finalizó y se puede reescribir el buffer de envio
void ssd1306_i2c_send_complete(_ssd1306_i2c *ssd);

// Setea el texto en la linea especificada - char *data es un string terminado en '\0' - Setea fuente, fixed y spacing
void ssd1306_i2c_put_text(_ssd1306_i2c *ssd, const char *data, uint8_t at_line, uint8_t font, uint8_t fixed, uint8_t spacing);

// Setea un unico caracter al inicio de la linea especificada - Setea fuente, fixed y spacing
void ssd1306_i2c_put_char(_ssd1306_i2c *ssd, const char data, uint8_t at_line, uint8_t font, uint8_t fixed, uint8_t spacing);

// Agrega el texto a la linea especificada, al final del texto ya existente - char *data es un string terminado en '\0'
void ssd1306_i2c_add_text(_ssd1306_i2c *ssd, const char *data, uint8_t at_line);

// Agrega el caracter a la linea especificada, al final del texto ya existente
void ssd1306_i2c_add_char(_ssd1306_i2c *ssd, const char data, uint8_t at_line);

// Limpia de texto la linea especificada
void ssd1306_i2c_clean_text(_ssd1306_i2c *ssd, uint8_t at_line);

// Agrega el texto en modo consola - char *data es un string terminado en '\0'
void ssd1306_i2c_add_console_text(_ssd1306_i2c *ssd, const char *data);

// Establece un external bitmap
void ssd1306_i2c_set_exteral_bitmap(_ssd1306_i2c *ssd, const uint8_t *bitmap);

// Dibuja un pixel en el bitmap
void ssd1306_i2c_bitmap_draw_px(_ssd1306_i2c *ssd, const uint8_t x, const uint8_t y);

// Dibuja una linea que va del punto (x0, y0) al punto (x1, y1) utilizando una variante de Bresenham's Line Algorithm
void ssd1306_i2c_bitmap_draw_line(_ssd1306_i2c *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

// Dibuja un cuadrado cuyo vertice superior derecho está en (x0, y0) y su vertice superior izquierdo es el punto (x1, y1)
void ssd1306_i2c_bitmap_draw_square(_ssd1306_i2c *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

// Dibuja un rectangulo alineado a la pantalla cuyo vertice superior derecho está en (x0, y0) y su vertice inferior izquierdo es el punto (x1, y1)
void ssd1306_i2c_bitmap_draw_rectangle(_ssd1306_i2c *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

// Hace scrool down la cantidad de pixeles indicados
void ssd1306_i2c_scroll_down(_ssd1306_i2c *ssd, uint8_t px);

// Hace scrool up la cantidad de pixeles indicados
void ssd1306_i2c_scroll_up(_ssd1306_i2c *ssd, uint8_t px);

// Hace scrool rigth la cantidad de pixeles indicados
void ssd1306_i2c_scroll_rigth(_ssd1306_i2c *ssd, uint8_t px);

// Hace scrool left la cantidad de pixeles indicados
void ssd1306_i2c_scroll_left(_ssd1306_i2c *ssd, uint8_t px);

// Restet de scrool a 0
void ssd1306_i2c_scroll_reset(_ssd1306_i2c *ssd);

// Realiza las tareas de actualización, period es una aproximacion de la cantidad de tiempo entre llamados a work, para manejar el scroll automatico
void ssd1306_i2c_work(_ssd1306_i2c *ssd, uint8_t period);

// Limpia la pantalla y los buffers
void ssd1306_i2c_clear(_ssd1306_i2c *ssd);

// Fuerza un update de la pantalla
void ssd1306_i2c_update(_ssd1306_i2c *ssd);

#endif /* INC_SSD1306_I2C_H_ */
