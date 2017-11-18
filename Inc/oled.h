#ifndef __oled_H
#define __oled_H

namespace Display
{
  enum class display_id : uint32_t
  {
    NONE = 0,
    SSD1306_128X32 = 1,
    SSD1306_128X64 = 2,
    SH1106_128X64 = 3
  };

  struct display_type
  {
    display_id id;
    uint8_t start_column;
    uint8_t end_column;
    uint8_t start_page;
    uint8_t end_page;
    uint8_t width;
    uint8_t height;
    uint8_t page_count;
    bool horizontal_mode;
  };

  constexpr display_type SSD1306_128X32 = {
    display_id::SSD1306_128X32,  // ID
    0,                           // start column
    128,                         // end column
    0,                           // start page
    3,                           // end page
    128,                         // width
    32,                          // height
    4,                           // page count
    true                         // horizontal_mode
  };

  constexpr display_type SSD1306_128X64 = {
    display_id::SSD1306_128X64,  // ID
    0,                           // start column
    128,                         // end column
    0,                           // start page
    7,                           // end page
    128,                         // width
    64,                          // height
    8,                           // page count
    true                         // horizontal_mode
  };

  constexpr display_type SH1106_128X64 = {
    display_id::SH1106_128X64,  // ID
    2,                          // start column
    130,                        // end column
    0,                          // start page
    7,                          // end page
    128,                        // width
    64,                         // height
    8,                          // page count
    false                       // horizontal_mode
  };

  class oled
  {
   public:
    oled( display_type _type )
    {
      type = _type;
    }

    void init();

    void fill_shadow_dma( uint8_t value );
    void putc_shadow_dma( char chr, int16_t x, int16_t y, const UG_FONT* font );
    void set_contrast( uint8_t number );
    void spi_tx_complete();
    void frame_tx_complete();

    void update_frame_dma();

    void transmit_frame();
    void transmit_frame_dma();
    void transmit_page_dma( uint8_t page );

    // void Write_number( const uint8_t* n, uint8_t k, uint8_t station_dot );
    // void Display_Picture( uint8_t pic[] );

   private:
    display_type type;

    uint8_t contrast = 0xFF;

    volatile uint8_t active_buffer = 0;
    volatile uint8_t display_shadow[ 2 ][ 1024 ];
    volatile uint32_t oled_tx_busy = 0;
    volatile uint8_t fill_value_shadow = 0;
    volatile uint8_t dma_current_page = 0;
    volatile uint8_t dma_pages_per_transfer = 1;
    volatile bool init_complete = false;

    void Display_SetPixel_Shadow( int16_t x, int16_t y, uint8_t set );

    void Display_TxFrame_DMA();

    void set_page_address( uint8_t start, uint8_t end );
    void set_column_address( uint8_t start, uint8_t end );
    void set_column( uint8_t column );
    void set_page( uint8_t page );
    void write_data( uint8_t dat );
    void write_instruction( uint8_t cmd );

    void cs( uint8_t assert );
    void dc( uint8_t data );
    void reset( uint8_t reset );
  };

  extern oled oled1;
}

#endif
