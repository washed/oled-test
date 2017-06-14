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
// clang-format on

#define Start_column 0x00
#define Start_page 0x00
#define StartLine_set 0x00

static volatile uint32_t oled_tx_busy = 0;

uint8_t Contrast_level = 0xFF;

void Delay1( uint32_t n );
void Write_number( const uint8_t* n, uint8_t k, uint8_t station_dot );
void display_Contrast_level( uint8_t number );
void Write_Data( uint8_t dat );
void Write_Instruction( uint8_t cmd );
void Set_Page_Address( uint8_t add );
void Set_Column_Address( uint8_t add );
void Set_Contrast_Control_Register( uint8_t mod );
void Initial();
void Display_Chess( uint8_t value );
void Display_Chinese( uint8_t ft[] );
void Display_Chinese_Column( uint8_t ft[] );
void Display_Picture( uint8_t pic[] );
void Display_Fill_Shadow( uint8_t value );
void Display_Fill_Shadow_DMA( uint8_t value );

static void Display_CS( uint8_t assert );
static void Display_DC( uint8_t data );
static void Display_RST( uint8_t reset );
static void Display_TxFrame_DMA();

volatile uint8_t display_shadow[ 128 * 32 / 8 ];
static volatile uint8_t fill_value_shadow = 0;

void Write_number( const uint8_t* n, uint8_t k, uint8_t station_dot )
{
  uint8_t i;
  for ( i = 0; i < 8; i++ ) Write_Data( *( n + 16 * k + i ) );

  Set_Page_Address( Start_page + 1 );
  Set_Column_Address( Start_column + station_dot * 8 );
  for ( i = 8; i < 16; i++ )
  {
    Write_Data( *( n + 16 * k + i ) );
  }
}

void display_Contrast_level( uint8_t number )
{
  uint8_t number1, number2, number3;
  number1 = number / 100;
  number2 = number % 100 / 10;
  number3 = number % 100 % 10;
  Set_Column_Address( Start_column + 0 * 8 );
  Set_Page_Address( Start_page );
  Write_number( num, number1, 0 );
  Set_Column_Address( Start_column + 1 * 8 );
  Set_Page_Address( Start_page );
  Write_number( num, number2, 1 );
  Set_Column_Address( Start_column + 2 * 8 );
  Set_Page_Address( Start_page );
  Write_number( num, number3, 2 );
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

void Set_Page_Address( uint8_t add )
{
  add = 0xb0 | add;
  Write_Instruction( add );
  return;
}

void Set_Column_Address( uint8_t add )
{
  Write_Instruction( ( 0x10 | ( add >> 4 ) ) );
  Write_Instruction( ( 0x0f & add ) );
  return;
}

void Set_Contrast_Control_Register( uint8_t mod )
{
  Write_Instruction( 0x81 );
  Write_Instruction( mod );
  return;
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

  for ( uint32_t i = 0; i < 512; i++ ) display_shadow[ i ] = 0;
}

void Display_Fill( uint8_t value )
{
  for ( uint32_t i = 0; i < 512; i++ ) display_shadow[ i ] = value;

  Write_Instruction( 0x20 );
  Write_Instruction( 0x00 );

  Set_Page_Address( 0 );
  Set_Column_Address( 0x00 );

  HAL_GPIO_WritePin( OLED_DC_GPIO_Port, OLED_DC_Pin, GPIO_PIN_SET );
  HAL_GPIO_WritePin( SPI2_CS2_GPIO_Port, SPI2_CS2_Pin, GPIO_PIN_RESET );

  HAL_SPI_Transmit( &hspi2, &display_shadow, 512, 10 );

  HAL_GPIO_WritePin( SPI2_CS2_GPIO_Port, SPI2_CS2_Pin, GPIO_PIN_SET );

  Write_Instruction( 0x20 );
  Write_Instruction( 0x02 );
}

void Display_Fill_Shadow( uint8_t value )
{
  for ( uint32_t i = 0; i < 512; i++ ) display_shadow[ i ] = value;
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
    Set_Page_Address( 0 );
    Set_Column_Address( 0x00 );
    Display_DC( 1 );
    Display_CS( 1 );
    oled_tx_busy = 1;
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

void Display_Chess( uint8_t value )
{
  uint8_t i, j, k;
  for ( i = 0; i < 0x4; i++ )
  {
    Set_Page_Address( i );

    Set_Column_Address( 0x00 );

    for ( j = 0; j < 0x10; j++ )
    {
      for ( k = 0; k < 0x04; k++ ) Write_Data( value );
      for ( k = 0; k < 0x04; k++ ) Write_Data( ~value );
    }
  }
  return;
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
