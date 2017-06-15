/*
 * fonts.h
 *
 *  Created on: 15.06.2017
 *      Author: washed
 */

#ifndef FONTS_H_
#define FONTS_H_

#define __UG_FONT_DATA

#define USE_FONT_4X6
#define USE_FONT_16X26

typedef enum { FONT_TYPE_1BPP, FONT_TYPE_8BPP } FONT_TYPE;

typedef struct
{
  unsigned char* p;
  FONT_TYPE font_type;
  int16_t char_width;
  int16_t char_height;
  uint16_t start_char;
  uint16_t end_char;
  uint8_t* widths;
} UG_FONT;

#ifdef USE_FONT_4X6
extern const UG_FONT FONT_4X6;
#endif
#ifdef USE_FONT_5X8
extern const UG_FONT FONT_5X8;
#endif
#ifdef USE_FONT_5X12
extern const UG_FONT FONT_5X12;
#endif
#ifdef USE_FONT_6X8
extern const UG_FONT FONT_6X8;
#endif
#ifdef USE_FONT_6X10
extern const UG_FONT FONT_6X10;
#endif
#ifdef USE_FONT_7X12
extern const UG_FONT FONT_7X12;
#endif
#ifdef USE_FONT_8X8
extern const UG_FONT FONT_8X8;
#endif
#ifdef USE_FONT_8X12
extern const UG_FONT FONT_8X12;
#endif
#ifdef USE_FONT_8X14
extern const UG_FONT FONT_8X14;
#endif
#ifdef USE_FONT_10X16
extern const UG_FONT FONT_10X16;
#endif
#ifdef USE_FONT_12X16
extern const UG_FONT FONT_12X16;
#endif
#ifdef USE_FONT_12X20
extern const UG_FONT FONT_12X20;
#endif
#ifdef USE_FONT_16X26
extern const UG_FONT FONT_16X26;
#endif

#endif /* FONTS_H_ */
