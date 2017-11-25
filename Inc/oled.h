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
    oled( display_type _type, SPI_HandleTypeDef* _hal_spi_handle, TIM_HandleTypeDef* _hal_timer_handle )
    {
      type = _type;
      hal_spi_handle = _hal_spi_handle;
      hal_timer_handle = _hal_timer_handle;
    }

    void init();

    // Frame buffer manipulation functions
    void fill_shadow_dma( uint8_t value );
    void set_pixel_shadow( int16_t x, int16_t y, bool set );

    // TX related functions
    void spi_tx_complete();
    void frame_tx_complete();
    void update_frame_dma();
    void transmit_frame();
    void transmit_frame_dma();
    void transmit_page_dma( uint8_t page );

   private:
    display_type type;
    SPI_HandleTypeDef* hal_spi_handle;
    TIM_HandleTypeDef* hal_timer_handle;

    uint8_t contrast = 0xFF;

    volatile uint8_t display_shadow[ 1024 ];
    volatile uint32_t oled_tx_busy = 0;
    volatile uint8_t fill_value_shadow = 0;
    volatile uint8_t dma_current_page = 0;
    volatile uint8_t dma_pages_per_transfer = 1;
    volatile bool init_complete = false;

    // Inline member functions
    inline void set_contrast( uint8_t number );
    inline void set_page_address( uint8_t start, uint8_t end );
    inline void set_column_address( uint8_t start, uint8_t end );
    inline void set_column( uint8_t column );
    inline void set_page( uint8_t page );
    inline void write_data( uint8_t dat );
    inline void write_instruction( uint8_t cmd );
    inline void cs( bool assert );
    inline void dc( bool assert_data );
    inline void reset( bool assert_reset );
  };

  extern oled oled1;
}

#endif
