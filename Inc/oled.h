#ifndef __oled_H
#define __oled_H

#include "fonts.h"

void Initial();
void Display_Chess( uint8_t value );
void Display_Picture( uint8_t pic[] );
void Display_Fill_Shadow_DMA( uint8_t value );
void Write_number( const uint8_t* n, uint8_t k, uint8_t station_dot );
void display_Contrast_level( uint8_t number );
void Display_Char_Shadow( char chr, int16_t x, int16_t y, const UG_FONT* font );

#endif
