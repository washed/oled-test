/*
 * oled.c
 *
 *  Created on: 07.06.2017
 *      Author: washed
 */

//----------------------------------------------------------------------
// EASTRISING TECHNOLOGY CO,.LTD.//
// Module    : ER-OLED091-1  0.91" (WHITE)
// Lanuage   : C51 Code
// Create    : JAVEN
// Date      : 2010-06-18
// Drive IC  : SSD1306
// INTERFACE : 4-wire SPI Serial Interface
// MCU 		 : AT89LV52
// VDD		 : 3V   VBAT: 3.6V
//
//----------------------------------------------------------------------

// clang-format off
#include "stm32l4xx_hal.h"
#include "spi.h"
#include "tim.h"
#include "dma.h"
#include "oled_data.h"
#include "oled.h"
#include "fonts.h"
// clang-format on

static const uint8_t Start_column = 0x00;
static const uint8_t Start_page = 0x00;
static const uint8_t StartLine_set = 0x00;
static const uint8_t Display_Width = 128;
static const uint8_t Display_Height = 32;
static const uint8_t Display_Pages = 4;

static uint8_t Contrast_level = 0xFF;

static volatile uint32_t oled_tx_busy = 0;
static volatile uint8_t display_shadow[ 512 ];
static volatile uint8_t fill_value_shadow = 0;

static void Display_SetPixel_Shadow( int16_t x, int16_t y, uint8_t set );
static void Write_Data( uint8_t dat );
static void Write_Instruction( uint8_t cmd );
static void Set_Page_Address( uint8_t add );
static void Set_Column_Address( uint8_t add );
static void Set_Contrast_Control_Register( uint8_t mod );
static void Display_CS( uint8_t assert );
static void Display_DC( uint8_t data );
static void Display_RST( uint8_t reset );
static void Display_TxFrame_DMA();

/*
void Display_Char_Shadow( uint8_t start_column, uint8_t start_page, UG_FONT* font, unsigned char character )
{
  if ( font->font_type == FONT_TYPE_1BPP ) return;
  if ( start_column > ( Display_Width - font->char_width ) ) return;
  if ( start_page > ( Display_Pages - 1 ) ) return;
}
*/

void Display_Char_Shadow( char chr, int16_t x, int16_t y, const UG_FONT* font )
{
  uint16_t i, j, k, xo, yo, c, bn, actual_char_width;
  uint8_t b, bt;
  uint32_t index;

  bt = (uint8_t)chr;

  switch ( bt )
  {
    case 0xF6:
      bt = 0x94;
      break;  // ö
    case 0xD6:
      bt = 0x99;
      break;  // Ö
    case 0xFC:
      bt = 0x81;
      break;  // ü
    case 0xDC:
      bt = 0x9A;
      break;  // Ü
    case 0xE4:
      bt = 0x84;
      break;  // ä
    case 0xC4:
      bt = 0x8E;
      break;  // Ä
    case 0xB5:
      bt = 0xE6;
      break;  // µ
    case 0xB0:
      bt = 0xF8;
      break;  // °
  }

  if ( bt < font->start_char || bt > font->end_char ) return;

  yo = y;
  bn = font->char_width;
  if ( !bn ) return;
  bn >>= 3;
  if ( font->char_width % 8 ) bn++;
  actual_char_width = ( font->widths ? font->widths[ bt - font->start_char ] : font->char_width );

  /* Is hardware acceleration available? */
  /*
  if ( gui->driver[ DRIVER_FILL_AREA ].state & DRIVER_ENABLED )
  {
    //(void(*)(UG_COLOR))
    push_pixel = ( (void* (*)( UG_S16, UG_S16, UG_S16, UG_S16 ))gui->driver[ DRIVER_FILL_AREA ].driver )(
        x, y, x + actual_char_width - 1, y + font->char_height - 1 );

    if ( font->font_type == FONT_TYPE_1BPP )
    {
      index = ( bt - font->start_char ) * font->char_height * bn;
      for ( j = 0; j < font->char_height; j++ )
      {
        c = actual_char_width;
        for ( i = 0; i < bn; i++ )
        {
          b = font->p[ index++ ];
          for ( k = 0; ( k < 8 ) && c; k++ )
          {
            if ( b & 0x01 )
            {
              push_pixel( fc );
            }
            else
            {
              push_pixel( bc );
            }
            b >>= 1;
            c--;
          }
        }
      }
    }
  }
  else
  */
  {
    /*Not accelerated output*/
    if ( font->font_type == FONT_TYPE_1BPP )
    {
      index = ( bt - font->start_char ) * font->char_height * bn;
      for ( j = 0; j < font->char_height; j++ )
      {
        xo = x;
        c = actual_char_width;
        for ( i = 0; i < bn; i++ )
        {
          b = font->p[ index++ ];
          for ( k = 0; ( k < 8 ) && c; k++ )
          {
            if ( b & 0x01 )
            {
              Display_SetPixel_Shadow( xo, yo, 1 );
            }
            else
            {
              Display_SetPixel_Shadow( xo, yo, 0 );
            }
            b >>= 1;
            xo++;
            c--;
          }
        }
        yo++;
      }
    }
  }
}

static void Display_SetPixel_Shadow( int16_t x, int16_t y, uint8_t set )
{
  uint8_t current_page, y_delta;

  if ( x >= Display_Width ) return;
  if ( y >= Display_Height ) return;

  current_page = y / 8;
  y_delta = y % 8;

  if ( set == 1 )
    display_shadow[ current_page * Display_Width + x ] |= ( 1 << y_delta );
  else if ( set == 0 )
    display_shadow[ current_page * Display_Width + x ] &= ~( 1 << y_delta );
}

void Display_Number_Shadow( uint8_t start_column, uint8_t start_page, uint8_t number )
{
  if ( start_column > ( Display_Width - 16 ) ) return;
  if ( start_page > ( Display_Pages - 1 ) ) return;

  for ( uint32_t i = 0; i < 8; i++ )
  {
    display_shadow[ i + start_column + ( start_page * Display_Width ) ] = num[ i + ( number * 16 ) ];
    display_shadow[ i + Display_Width + start_column + ( start_page * Display_Width ) ] =
        num[ i + 8 + ( number * 16 ) ];
  }
}

void Write_Instruction( uint8_t cmd )
{
  Display_DC( 0 );
  Display_CS( 1 );
  HAL_SPI_Transmit( &hspi2, &cmd, 1, 10 );
  Display_CS( 0 );
}

void Write_Data( uint8_t dat )
{
  Display_DC( 1 );
  Display_CS( 1 );
  HAL_SPI_Transmit( &hspi2, &dat, 1, 10 );
  Display_CS( 0 );
}

void Set_Page_Address( uint8_t address )
{
  address = 0xb0 | address;
  Write_Instruction( address );
}

void Set_Column_Address( uint8_t address )
{
  Write_Instruction( ( 0x10 | ( address >> 4 ) ) );
  Write_Instruction( ( 0x0f & address ) );
}

void Set_Contrast_Control_Register( uint8_t mod )
{
  Write_Instruction( 0x81 );
  Write_Instruction( mod );
}

void Initial()
{
  Display_RST( 0 );
  HAL_Delay( 20 );
  Display_RST( 1 );
  HAL_Delay( 200 );
  Display_RST( 0 );

  HAL_Delay( 200 );

  Write_Instruction( 0xae );  //--turn off oled panel

  Write_Instruction( 0xd5 );  //--set display clock divide ratio/oscillator frequency
  Write_Instruction( 0x80 );  //--set divide ratio

  Write_Instruction( 0xa8 );  //--set multiplex ratio(1 to 64)
  Write_Instruction( 0x1f );  //--1/32 duty

  Write_Instruction( 0xd3 );  //-set display offset
  Write_Instruction( 0x00 );  //-not offset

  Write_Instruction( 0x8d );  //--set Charge Pump enable/disable
  Write_Instruction( 0x14 );  //--set(0x10) disable

  Write_Instruction( 0x40 );  //--set start line address

  Write_Instruction( 0xa6 );  //--set normal display

  Write_Instruction( 0xa4 );  // Disable Entire Display On

  Write_Instruction( 0xa1 );  //--set segment re-map 128 to 0

  Write_Instruction( 0xC8 );  //--Set COM Output Scan Direction 64 to 0

  Write_Instruction( 0xda );  //--set com pins hardware configuration
  Write_Instruction( 0x42 );

  Write_Instruction( 0x81 );  //--set contrast control register
  Write_Instruction( Contrast_level );

  Write_Instruction( 0xd9 );  //--set pre-charge period
  Write_Instruction( 0xf1 );

  Write_Instruction( 0xdb );  //--set vcomh
  Write_Instruction( 0x40 );

  Write_Instruction( 0x20 );  //--set adressing mode
  Write_Instruction( 0x00 );

  Write_Instruction( 0xaf );  //--turn on oled panel

  Display_Fill_Shadow_DMA( 0 );
}

void Display_Fill_Shadow_DMA( uint8_t value )
{
  fill_value_shadow = value;
  while ( oled_tx_busy == 1 )
  {
  }
  HAL_DMA_Start_IT( &hdma_memtomem_dma2_channel1, (uint32_t)&fill_value_shadow, (uint32_t)&display_shadow[ 0 ], 512 );
}

static void Display_TxFrame_DMA()
{
  if ( oled_tx_busy == 0 )
  {
    oled_tx_busy = 1;
    Set_Page_Address( 0 );
    Set_Column_Address( 0x00 );
    Display_DC( 1 );
    Display_CS( 1 );
    HAL_SPI_Transmit_DMA( &hspi2, (uint8_t*)&display_shadow[ 0 ], 512 );
  }
}

static void Display_CS( uint8_t assert )
{
  if ( assert == 1 )
    HAL_GPIO_WritePin( SPI2_CS2_GPIO_Port, SPI2_CS2_Pin, GPIO_PIN_RESET );
  else if ( assert == 0 )
    HAL_GPIO_WritePin( SPI2_CS2_GPIO_Port, SPI2_CS2_Pin, GPIO_PIN_SET );
}

static void Display_DC( uint8_t data )
{
  if ( data == 1 )
    HAL_GPIO_WritePin( OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET );
  else if ( data == 0 )
    HAL_GPIO_WritePin( OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET );
}

static void Display_RST( uint8_t reset )
{
  if ( reset == 1 )
    HAL_GPIO_WritePin( OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET );
  else if ( reset == 0 )
    HAL_GPIO_WritePin( OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET );
}

void Display_Picture( uint8_t pic[] )
{
  uint8_t i, j, num = 0;
  for ( i = 0; i < 0x04; i++ )
  {
    Set_Page_Address( i );
    Set_Column_Address( 0x00 );
    for ( j = 0; j < 0x80; j++ )
    {
      Write_Data( pic[ i * 0x80 + j ] );
    }
  }
  return;
}

void HAL_SPI_TxCpltCallback( SPI_HandleTypeDef* hspi )
{
  if ( hspi == &hspi2 )
  {
    Display_CS( 0 );
    oled_tx_busy = 0;
  }
}

void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef* htim )
{
  if ( htim == &htim1 )
  {
    Display_TxFrame_DMA();
  }
}
