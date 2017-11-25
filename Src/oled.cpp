/*
 * oled.c
 *
 *  Created on: 07.06.2017
 *      Author: washed
 */

#include "stm32l4xx_hal.h"
#include "spi.h"
#include "tim.h"
#include "dma.h"

#include "oled.h"

Display::oled oled1( Display::SH1106_128X64, &hspi2, &htim1 );

#ifdef __cplusplus
extern "C" {
#endif

void display_test()
{
  oled1.init();
}

void HAL_SPI_TxCpltCallback( SPI_HandleTypeDef* hspi )
{
  if ( hspi == &hspi2 )
  {
    oled1.spi_tx_complete();
  }
}

void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef* htim )
{
  if ( htim == &htim1 )
  {
    oled1.update_frame_dma();
  }
}

#ifdef __cplusplus
}
#endif /* extern "C" */

namespace Display
{
  void oled::init()
  {
    reset( false );
    HAL_Delay( 20 );
    reset( true );
    HAL_Delay( 200 );
    reset( false );

    HAL_Delay( 200 );

    // set display off
    write_instruction( 0xae );

    // set display clock divide ratio/oscillator frequency
    write_instruction( 0xd5 );
    write_instruction( 0x80 );

    //set multiplex ratio
    write_instruction( 0xa8 );
    switch ( type.id )
    {
      case display_id::SSD1306_128X32:
        // 1/32 duty for 128x32
        write_instruction( 0x1f );
        break;
      case display_id::SSD1306_128X64:
      case display_id::SH1106_128X64:
        // 1/64 duty for 128x64
        write_instruction( 0x3f );
        break;

      case display_id::NONE:
      default:
        // TODO: Error!
        break;
    }
    // set display offset - no offset
    write_instruction( 0xd3 );
    write_instruction( 0x00 );

    //set Charge Pump enable/disable
    write_instruction( 0x8d );
    write_instruction( 0x14 );

    // set start line address: 0
    write_instruction( 0x40 );

    // set normal display (not inverted)
    write_instruction( 0xa6 );

    // disable entire display On
    write_instruction( 0xa4 );

    // set segment re-map 128 to 0
    write_instruction( 0xa1 );

    // set COM output scan direction 64 to 0
    write_instruction( 0xC8 );

    // set com pins hardware configuration
    write_instruction( 0xda );
    switch ( type.id )
    {
      case display_id::SSD1306_128X32:
        // for 128x32 display
        write_instruction( 0x42 );
        break;
      case display_id::SSD1306_128X64:
      case display_id::SH1106_128X64:
        // for 128x64 display
        write_instruction( 0x12 );
        break;

      case display_id::NONE:
      default:
        // TODO: Error!
        break;
    }

    // set contrast control register
    write_instruction( 0x81 );
    write_instruction( contrast );

    // set pre-charge period
    write_instruction( 0xd9 );
    write_instruction( 0xf1 );

    // set vcomh
    write_instruction( 0xdb );
    write_instruction( 0x40 );

    // set adressing mode
    write_instruction( 0x20 );
    write_instruction( 0x00 );

    // turn on oled panel
    write_instruction( 0xaf );

    // set page start and end address
    set_page_address( type.start_page, type.end_page );

    // set column start and end address
    set_column_address( type.start_column, type.end_column );

    // fill frame buffer with zero
    fill_shadow_dma( 0 );

    init_complete = true;

    // Start frame timer
    HAL_TIM_Base_Start_IT( hal_timer_handle );
  }

  void oled::fill_shadow_dma( uint8_t value )
  {
    fill_value_shadow = value;
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
        // Set command mode
        dc( false );

        // Set page start and end address
        set_page_address( 0, 127 );
        // TODO: This probably doesn't work!!!
        // Set column start and end address
        set_column_address( 0, 7 );

        // Set data mode
        dc( true );

        // Assert chip select
        cs( true );

        // Transmit
        HAL_SPI_Transmit( hal_spi_handle, (uint8_t*)&display_shadow[ 0 ], 1024, 1000 );
      }
      else
      {
        for ( uint32_t current_page = type.start_page; current_page <= type.end_page; current_page++ )
        {
          // Set command mode
          dc( false );

          // Set current page
          set_page( current_page );

          // Set column
          set_column( type.start_column );

          // Set data mode
          dc( true );

          // Assert chip select
          cs( true );

          // Transmit
          HAL_SPI_Transmit( hal_spi_handle, (uint8_t*)&display_shadow[ current_page * 128 ], 128, 1000 );

          // Deassert chip select
          cs( false );
        }
      }
      oled_tx_busy = 0;
    }
  }

  void oled::update_frame_dma()
  {
    if ( ( true == init_complete ) && ( 0 == oled_tx_busy ) )
    {
      oled_tx_busy = 1;
      dma_current_page = type.start_page;

      if ( this->type.horizontal_mode == true )
      {
        dma_pages_per_transfer = type.page_count;
        transmit_frame_dma();
      }
      else
      {
        dma_pages_per_transfer = 1;
        transmit_page_dma( type.start_page );
      }
    }
  }

  void oled::transmit_frame_dma()
  {
    // Set command mode
    dc( false );

    // Set page start and end address
    set_page( type.start_page );

    // Set column start and end address
    set_column( type.start_column );

    // Set data mode
    dc( true );

    // Assert chip select
    cs( true );

    // Begin transfer
    HAL_SPI_Transmit_DMA( hal_spi_handle, (uint8_t*)&display_shadow[ 0 ], type.width * type.page_count );
  }

  void oled::transmit_page_dma( uint8_t page )
  {
    // Set command mode
    dc( false );

    // Set page start and end address
    set_page( page );

    // Set column start and end address
    set_column( type.start_column );

    // Set data mode
    dc( true );

    // Assert chip select
    cs( true );

    // Begin transfer
    HAL_SPI_Transmit_DMA( hal_spi_handle, (uint8_t*)&display_shadow[ page * type.width ], type.width );
  }

  void oled::spi_tx_complete()
  {
    dma_current_page += dma_pages_per_transfer;
    if ( dma_current_page >= type.page_count )
    {
      dma_current_page = 0;
      frame_tx_complete();
    }
    else
    {
      transmit_page_dma( dma_current_page );
    }
  }

  void oled::frame_tx_complete()
  {
    static uint32_t column = 0;

    cs( 0 );
    oled_tx_busy = 0;

    display_shadow[ column % 1024 ] = 0;
    display_shadow[ ( column + 8 ) % 1024 ] = 255;
    column++;

    if ( column == 1024 )
    {
      column = 0;
    }
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
    dc( false );
    cs( true );
    HAL_SPI_Transmit( hal_spi_handle, &cmd, 1, 10 );
    cs( false );
  }

  void oled::write_data( uint8_t dat )
  {
    dc( true );
    cs( true );
    HAL_SPI_Transmit( hal_spi_handle, &dat, 1, 10 );
    cs( false );
  }
}

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
