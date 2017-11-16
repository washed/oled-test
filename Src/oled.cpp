/*
 * oled.c
 *
 *  Created on: 07.06.2017
 *      Author: washed
 */

// clang-format off
#include "stm32l4xx_hal.h"
#include "spi.h"
#include "tim.h"
#include "dma.h"
#include "oled_data.h"

#include "fonts.h"

#include "oled.h"

// clang-format on

/*
void Display_Char_Shadow( uint8_t start_column, uint8_t start_page, UG_FONT* font, unsigned char character )
{
  if ( font->font_type == FONT_TYPE_1BPP ) return;
  if ( start_column > ( Display_Width - font->char_width ) ) return;
  if ( start_page > ( Display_Pages - 1 ) ) return;
}
*/

Display::oled oled1( Display::SH1106_128X64 );

#ifdef __cplusplus
extern "C" {
#endif

void display_test()
{
  oled1.init();

  while ( 1 )
  {
    oled1.fill_shadow_dma( 0 );
    oled1.transmit_frame();
    HAL_Delay( 100 );

    oled1.fill_shadow_dma( 255 );
    oled1.transmit_frame();
    HAL_Delay( 100 );
  }
}

void HAL_SPI_TxCpltCallback( SPI_HandleTypeDef* hspi )
{
  if ( hspi == &hspi2 )
  {
    // oled1.frame_tx_complete();
  }
}

void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef* htim )
{
  if ( htim == &htim1 )
  {
    //oled1.transmit_frame_dma();
  }
}

#ifdef __cplusplus
}
#endif /* extern "C" */

namespace Display
{
  void oled::init()
  {
    reset( 0 );
    HAL_Delay( 20 );
    reset( 1 );
    HAL_Delay( 200 );
    reset( 0 );

    HAL_Delay( 200 );

    write_instruction( 0xae );  //--turn off oled panel

    write_instruction( 0xd5 );  //--set display clock divide ratio/oscillator frequency
    write_instruction( 0x80 );  //--set divide ratio

    write_instruction( 0xa8 );  //--set multiplex ratio(1 to 64)
    // Write_Instruction( 0x1f );  //--1/32 duty for 128x32
    write_instruction( 0x3f );  //--1/64 duty for 128x64

    write_instruction( 0xd3 );  //-set display offset
    write_instruction( 0x00 );  //-not offset

    write_instruction( 0x8d );  //--set Charge Pump enable/disable
    write_instruction( 0x14 );  //--set(0x10) disable

    write_instruction( 0x40 );  //--set start line address

    write_instruction( 0xa6 );  //--set normal display

    write_instruction( 0xa4 );  // Disable Entire Display On

    write_instruction( 0xa1 );  //--set segment re-map 128 to 0

    write_instruction( 0xC8 );  //--Set COM Output Scan Direction 64 to 0

    write_instruction( 0xda );  //--set com pins hardware configuration
    // Write_Instruction( 0x42 );  // for 128x32 display!
    write_instruction( 0x12 );  // for 128x64 display

    write_instruction( 0x81 );  //--set contrast control register
    write_instruction( contrast );

    write_instruction( 0xd9 );  //--set pre-charge period
    write_instruction( 0xf1 );

    write_instruction( 0xdb );  //--set vcomh
    write_instruction( 0x40 );

    write_instruction( 0x20 );  //--set adressing mode
    write_instruction( 0x00 );

    write_instruction( 0xaf );  //--turn on oled panel

    // Set page start and end address
    set_page_address( 0, 7 );

    // Set column start and end address
    set_column_address( 2, 130 );

    fill_shadow_dma( 0 );
  }

  void oled::fill_shadow_dma( uint8_t value )
  {
    fill_value_shadow = value;
    while ( oled_tx_busy == 1 )
    {
    }
    HAL_DMA_Start_IT( &hdma_memtomem_dma2_channel1, (uint32_t)&fill_value_shadow, (uint32_t)&display_shadow[ 0 ],
                      1024 );
  }

  void oled::transmit_frame()
  {
    if ( oled_tx_busy == 0 )
    {
      oled_tx_busy = 1;
      if ( this->type.horizontal_mode == true )
      {
        // Set page start and end address
        set_page_address( 0, 127 );
        // TODO: This probably doesnt work!!!
        // Set column start and end address
        set_column_address( 0, 7 );

        dc( 1 );
        cs( 1 );
        HAL_SPI_Transmit( &hspi2, (uint8_t*)&display_shadow[ 0 ], 1024, 1000 );
      }
      else
      {
        for ( uint32_t current_page = type.start_page; current_page <= type.end_page; current_page++ )
        {
          // Set current page
          set_page( current_page );

          // Set column
          set_column( type.start_column );

          dc( 1 );
          cs( 1 );
          HAL_SPI_Transmit( &hspi2, (uint8_t*)&display_shadow[ current_page * 128 ], 128, 1000 );
          cs( 0 );
        }
      }
      oled_tx_busy = 0;
    }
  }

  void oled::transmit_frame_dma()
  {
    if ( oled_tx_busy == 0 )
    {
      oled_tx_busy = 1;

      // Set page start and end address
      set_page_address( 0, 127 );

      // Set column start and end address
      set_column_address( 0, 7 );

      dc( 1 );
      cs( 1 );
      HAL_SPI_Transmit_DMA( &hspi2, (uint8_t*)&display_shadow[ 0 ], 1024 );
    }
  }

  void oled::frame_tx_complete()
  {
    cs( 0 );
    oled_tx_busy = 0;
  }

  void oled::set_column( uint8_t column )
  {
    write_instruction( column & 0x0F );
    write_instruction( 0x10 | ( column >> 4 ) );
  }

  void oled::set_page( uint8_t page )
  {
    write_instruction( 0xB0 | page );
  }

  void oled::set_page_address( uint8_t start, uint8_t end )
  {
    write_instruction( 0x21 );
    write_instruction( start );
    write_instruction( end );
  }

  void oled::set_column_address( uint8_t start, uint8_t end )
  {
    write_instruction( 0x22 );
    write_instruction( start );
    write_instruction( end );
  }

  void oled::set_contrast( uint8_t mod )
  {
    write_instruction( 0x81 );
    write_instruction( mod );
  }

  void oled::write_instruction( uint8_t cmd )
  {
    dc( 0 );
    cs( 1 );
    HAL_SPI_Transmit( &hspi2, &cmd, 1, 10 );
    cs( 0 );
  }

  void oled::write_data( uint8_t dat )
  {
    dc( 1 );
    cs( 1 );
    HAL_SPI_Transmit( &hspi2, &dat, 1, 10 );
    cs( 0 );
  }

  void oled::cs( uint8_t assert )
  {
    if ( assert == 1 )
      HAL_GPIO_WritePin( SPI2_CS2_GPIO_Port, SPI2_CS2_Pin, GPIO_PIN_RESET );
    else if ( assert == 0 )
      HAL_GPIO_WritePin( SPI2_CS2_GPIO_Port, SPI2_CS2_Pin, GPIO_PIN_SET );
  }

  void oled::dc( uint8_t data )
  {
    if ( data == 1 )
      HAL_GPIO_WritePin( OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET );
    else if ( data == 0 )
      HAL_GPIO_WritePin( OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_RESET );
  }

  void oled::reset( uint8_t reset )
  {
    if ( reset == 1 )
      HAL_GPIO_WritePin( OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_RESET );
    else if ( reset == 0 )
      HAL_GPIO_WritePin( OLED_RST_GPIO_Port, OLED_RST_Pin, GPIO_PIN_SET );
  }
}

/*
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

  // Is hardware acceleration available?

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

{
  // Not accelerated output
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
*/

/*
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
*/

/*
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
*/

/*
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
*/
