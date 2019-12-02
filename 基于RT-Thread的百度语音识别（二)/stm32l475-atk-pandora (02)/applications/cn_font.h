#include <rtthread.h>
#include <string.h>
#include <drv_lcd.h>


void get_hz_mat(unsigned char *code, unsigned char *mat, uint8_t size);

void show_str(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *str, uint8_t size);	

void show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size);


