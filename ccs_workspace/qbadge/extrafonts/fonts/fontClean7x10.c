//*****************************************************************************
//
// Copyright 1989 Dale Schumacher, dal@syntel.mn.org
//                399 Beacon Ave.
//                St. Paul, MN  55104-3527
// 
// Permission to use, copy, modify, and distribute this software and
// its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notice appear in all
// copies and that both that copyright notice and this permission
// notice appear in supporting documentation, and that the name of
// Dale Schumacher not be used in advertising or publicity pertaining to
// distribution of the software without specific, written prior
// permission.  Dale Schumacher makes no representations about the
// suitability of this software for any purpose.  It is provided "as
// is" without express or implied warranty.
//*****************************************************************************

//*****************************************************************************
//
// This file is generated by ftrasterize; DO NOT EDIT BY HAND!
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include "grlib/grlib.h"

//*****************************************************************************
//
// Details of this font:
//     Characters: 32 to 126 inclusive
//     Style: Clean
//     Size: 7x10
//     Bold: no
//     Italic: no
//     Memory usage: 1284 bytes
//
//*****************************************************************************

static const uint8_t g_pui8clean7x10Data[1081] =
{
      5,   7,   0,   8,  96,  10,   4,   1,  49,  49,  49,  49,
     49, 113, 176,  11,   5,   1,  17,  33,  17,  33,  17, 240,
    240, 112,  17,   6,  17,  33,  33,  33,  22,  17,  33,  33,
     33,  22,  17,  33,  33,  33, 208,  12,   6, 129,  68,  17,
     17,  67,  65,  17,  20,  65, 240,  16,   7, 129,  81,  17,
     33,  33,  33,  81,  81,  33,  33,  33,  17,  81, 240,  13,
      6, 115,  33,  97,  82,  49,  33,  18,  49,  35,  17, 192,
      8,   5,  18,  49,  49, 240, 240, 144,  12,   5,  33,  49,
     65,  49,  65,  65,  81,  65,  81, 112,  12,   5,   1,  81,
     65,  81,  65,  65,  49,  65,  49, 144,  13,   6, 225,  49,
     17,  17,  35,  33,  17,  17,  49, 240,  96,   9,   6, 225,
     81,  53,  49,  81, 240,  96,   8,   6, 240, 240, 114,  65,
     65, 176,   6,   6, 240, 150, 240, 240,   6,   5, 240, 242,
     50, 208,  13,   6,  65,  81,  65,  81,  65,  81,  65,  81,
     65,  81,  80,  17,   6,  19,  33,  49,  17,  49,  17,  49,
     17,  49,  17,  49,  17,  49,  35, 224,  11,   5,  17,  50,
     65,  65,  65,  65,  65,  65, 208,  12,   6,  19,  33,  49,
     81,  65,  65,  65,  65,  85, 208,  13,   6,  19,  33,  49,
     81,  50,  97,  81,  17,  49,  35, 224,  13,   6,  49,  66,
     66,  49,  17,  49,  17,  37,  65,  67, 208,  12,   6,   5,
     17,  81,  84,  97,  81,  17,  49,  35, 224,  14,   6,  34,
     49,  65,  84,  33,  49,  17,  49,  17,  49,  35, 224,  12,
      6,   5,  17,  49,  81,  65,  81,  65,  81,  81, 240,  16,
      6,  19,  33,  49,  17,  49,  35,  33,  49,  17,  49,  17,
     49,  35, 224,  14,   6,  19,  33,  49,  17,  49,  17,  49,
     36,  81,  65,  50, 240,   7,   5, 162,  50, 210,  50, 208,
      9,   6, 210,  66, 240,  18,  65,  65, 176,  10,   6, 240,
     18,  34,  34,  98,  98, 240,  48,   7,   6, 240,  54, 198,
    240,  48,   9,   6, 194,  98,  98,  34,  34, 240, 112,  11,
      6,  20,  17,  65,  81,  65,  65,  81, 177, 224,  15,   6,
    115,  33,  49,  17,  19,  17,  17,  17,  17,  19,  17,  99,
    224,  13,   7, 161,  97,  83,  65,  17,  53,  33,  49,  18,
     50, 224,  10,   6, 101,  17,  66,  70,  17,  66,  70, 208,
     11,   6, 131,  33,  50,  81,  81,  97,  49,  35, 208,  12,
      6, 100,  33,  49,  17,  66,  66,  66,  49,  20, 224,   9,
      6, 103,  81,  84,  33,  81,  86, 192,  10,   6, 103,  81,
     84,  33,  81,  81, 240,  32,  12,   6, 131,  33,  50,  81,
     36,  65,  17,  49,  36, 192,  10,   6,  97,  66,  66,  72,
     66,  66,  65, 192,  10,   6, 101,  49,  81,  81,  81,  81,
     53, 208,  12,   6, 131,  81,  81,  81,  17,  49,  17,  49,
     35, 224,  16,   6,  97,  49,  17,  33,  33,  17,  50,  65,
     17,  49,  33,  33,  49, 208,  10,   6,  97,  81,  81,  81,
     81,  81,  85, 208,  12,   6,  97,  67,  35,  18,  18,  66,
     66,  66,  65, 192,  13,   6,  97,  67,  50,  17,  34,  33,
     18,  51,  66,  65, 192,  13,   6, 130,  49,  33,  17,  66,
     66,  65,  17,  33,  50, 224,  11,   6, 101,  17,  66,  70,
     17,  81,  81, 240,  32,  14,   6, 130,  49,  33,  17,  66,
     66,  65,  17,  33,  50,  83,  96,  13,   6, 101,  17,  66,
     70,  17,  33,  33,  49,  17,  65, 192,  10,   6, 116,  17,
     66, 100,  98,  65,  20, 208,  10,   6, 101,  49,  81,  81,
     81,  81,  81, 240,  11,   6,  97,  66,  66,  66,  66,  66,
     65,  20, 208,  16,   7, 114,  50,  17,  49,  33,  49,  49,
     17,  65,  17,  81,  97, 240,  32,  12,   6,  97,  66,  66,
     66,  66,  18,  19,  35,  65, 192,  16,   6,  97,  49,  17,
     49,  33,  17,  65,  65,  17,  33,  49,  17,  49, 208,  13,
      6,  97,  49,  17,  49,  33,  17,  65,  81,  81,  81, 240,
     10,   6, 101,  81,  65,  65,  65,  65,  85, 208,  12,   4,
      3,  17,  49,  49,  49,  49,  49,  49,  51,  80,  12,   6,
      1,  81,  97,  81,  97,  81,  97,  81,  97,  81,  12,   6,
      3,  81,  81,  81,  81,  81,  81,  81,  51, 144,  10,   6,
     33,  65,  17,  33,  49, 240, 240, 208,   5,   6,   0,   6,
    102,   8,   5,   2,  65,  81, 240, 240, 112,  10,   6, 240,
     70,  66,  66,  50,  19,  17, 192,  11,   6,   1,  81,  81,
     85,  17,  66,  66,  70, 208,   9,   6, 240,  68,  17,  81,
     81, 100, 208,  11,   6,  81,  81,  81,  22,  66,  66,  65,
     21, 192,   8,   6, 240,  68,  17,  72, 100, 208,  12,   6,
     35,  33,  81,  68,  49,  81,  81,  81, 240,  16,  10,   6,
    240,  70,  66,  66,  65,  21,  81,  20,  12,   6,   1,  81,
     81,  85,  17,  66,  66,  66,  65, 192,  10,   6,  33,  81,
    147,  81,  81,  81,  53, 208,  12,   6,  65,  81, 147,  81,
     81,  81,  81,  81,  20,  32,  15,   6,   1,  81,  81,  81,
     49,  17,  33,  35,  49,  33,  33,  49, 208,  11,   5,   2,
     65,  65,  65,  65,  65,  65,  51, 192,  17,   6, 240,  50,
     17,  33,  17,  17,  17,  17,  17,  17,  17,  17,  17,  49,
    208,  11,   6, 240,  49,  19,  18,  50,  66,  66,  65, 192,
     10,   6, 240,  68,  17,  66,  66,  65,  20, 208,  11,   6,
    240,  53,  17,  66,  66,  70,  17,  81,  80,  10,   6, 240,
     70,  66,  66,  65,  21,  81,  81,  11,   6, 240,  49,  19,
     18,  65,  81,  81, 240,  32,   7,   6, 240,  70, 100, 102,
    208,  11,   6,  33,  81,  81,  53,  49,  81,  81,  99, 192,
     11,   6, 240,  49,  66,  66,  66,  50,  19,  17, 192,  14,
      7, 240,  98,  50,  17,  49,  49,  17,  65,  17,  81, 240,
     32,  17,   6, 240,  49,  49,  17,  17,  17,  17,  17,  17,
     17,  17,  17,  33,  17, 224,  13,   6, 240,  49,  49,  33,
     17,  65,  65,  17,  33,  49, 208,  11,   6, 240,  49,  66,
     66,  66,  65,  21,  81,  20,   9,   6, 240,  53,  65,  65,
     65,  69, 208,  12,   5,  33,  49,  65,  65,  49,  81,  65,
     65,  81, 112,  12,   4,   1,  49,  49,  49,  49,  49,  49,
     49,  49, 112,  12,   5,   1,  81,  65,  65,  81,  49,  65,
     65,  49, 144,  10,   6,  17,  65,  17,  17,  65, 240, 240,
    224,
};

const tFont g_sFontclean7x10 =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    7,

    //
    // The height of the font.
    //
    10,

    //
    // The baseline of the font.
    //
    8,

    //
    // The offset to each character in the font.
    //
    {
           0,    5,   15,   26,   43,   55,   71,   84,
          92,  104,  116,  129,  138,  146,  152,  158,
         171,  188,  199,  211,  224,  237,  249,  263,
         275,  291,  305,  312,  321,  331,  338,  347,
         358,  373,  386,  396,  407,  419,  428,  438,
         450,  460,  470,  482,  498,  508,  520,  533,
         546,  557,  571,  584,  594,  604,  615,  631,
         643,  659,  672,  682,  694,  706,  718,  728,
         733,  741,  751,  762,  771,  782,  790,  802,
         812,  824,  834,  846,  861,  872,  889,  900,
         910,  921,  931,  942,  949,  960,  971,  985,
        1002, 1015, 1026, 1035, 1047, 1059, 1071,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8clean7x10Data
};
