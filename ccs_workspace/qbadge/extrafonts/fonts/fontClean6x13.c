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
//     Size: 6x13
//     Bold: no
//     Italic: no
//     Memory usage: 1404 bytes
//
//*****************************************************************************

static const uint8_t g_pui8clean6x13Data[1203] =
{
      5,   6,   0,   9,  96,  12,   3,  49,  33,  33,  33,  33,
     33,  33,  33,  81, 128,  11,   4,  65,  17,  17,  17,  17,
     17, 240, 240, 112,  19,   5, 177,  17,  33,  17,  21,  17,
     17,  33,  17,  33,  17,  21,  17,  17,  33,  17, 176,  16,
      5, 193,  51,  17,  17,  18,  17,  51,  49,  18,  17,  17,
     19,  49, 192,  12,   5, 242,  50,  33,  49,  49,  49,  49,
     34,  50, 240,  16,   5, 179,  17,  65,  65,  81,  49,  17,
     18,  33,  17,  33,  34,  17, 160,   8,   4,  82,  33,  33,
    240, 240, 144,  14,   3,  81,  17,  33,  17,  33,  33,  33,
     33,  49,  33,  49,  48,  14,   5,  81,  81,  65,  81,  65,
     65,  65,  65,  49,  65,  49, 144,  14,   5, 240, 113,  33,
     17,  17,  19,  17,  17,  17,  33, 240, 112,  10,   5, 240,
    113,  65,  37,  33,  65, 240, 112,   9,   4, 240, 240, 114,
     34,  33,  33,  48,   6,   5, 240, 245, 240, 240,   6,   3,
    240, 194,  18, 112,  15,   6,  81,  81,  65,  81,  65,  81,
     65,  81,  65,  81,  65,  81, 176,  16,   5,  99,  17,  50,
     35,  35,  17,  18,  17,  19,  35,  34,  49,  19, 176,  13,
      4,  81,  34,  49,  49,  49,  49,  49,  49,  49,  49, 160,
     14,   5,  99,  17,  49,  65,  65,  49,  49,  49,  49,  65,
     69, 160,  14,   5,  99,  17,  49,  65,  65,  34,  81,  65,
     66,  49,  19, 176,  16,   5, 129,  65,  50,  50,  33,  17,
     33,  17,  17,  33,  21,  49,  51, 160,  12,   5,  86,  65,
     65,  68,  81,  65,  66,  49,  19, 176,  14,   5, 114,  33,
     49,  65,  68,  17,  50,  50,  50,  49,  19, 176,  13,   5,
     86,  49,  65,  65,  49,  65,  65,  49,  65,  65, 192,  15,
      5,  99,  17,  50,  50,  49,  19,  17,  50,  50,  50,  49,
     19, 176,  14,   5,  99,  17,  50,  50,  50,  49,  20,  65,
     65,  49,  34, 192,   7,   3, 194,  18, 162,  18, 112,  10,
      4, 240,  34,  34, 226,  34,  33,  33,  48,  10,   6, 240,
    210,  34,  34,  98,  98, 240, 144,   7,   5, 240, 165,  85,
    240, 160,  10,   6, 240, 146,  98,  98,  34,  34, 240, 208,
     13,   5,  99,  17,  50,  49,  65,  65,  49,  49,  65, 145,
    192,  13,   5, 240,  19,  17,  50,  20,  20,  20,  18,  17,
     83, 176,  13,   5, 193,  49,  17,  17,  50,  50,  55,  50,
     50,  49, 160,  12,   5, 164,  17,  50,  50,  53,  17,  50,
     50,  53, 176,  13,   5, 179,  17,  50,  65,  65,  65,  65,
     65,  49,  19, 176,  14,   5, 163,  33,  33,  17,  50,  50,
     50,  50,  50,  33,  19, 192,  11,   5, 166,  65,  65,  68,
     17,  65,  65,  69, 160,  11,   5, 166,  65,  65,  68,  17,
     65,  65,  65, 224,  13,   5, 179,  17,  50,  65,  65,  35,
     50,  50,  49,  20, 160,  12,   5, 161,  50,  50,  50,  55,
     50,  50,  50,  49, 160,  12,   5, 165,  33,  65,  65,  65,
     65,  65,  65,  37, 160,  12,   5, 195,  65,  65,  65,  65,
     66,  50,  49,  19, 176,  17,   5, 161,  50,  50,  33,  17,
     17,  34,  49,  17,  33,  33,  17,  50,  49, 160,  12,   5,
    161,  65,  65,  65,  65,  65,  65,  65,  69, 160,  15,   5,
    161,  50,  51,  20,  19,  17,  18,  17,  18,  50,  50,  49,
    160,  15,   5, 161,  51,  35,  34,  17,  18,  17,  18,  35,
     35,  50,  49, 160,  13,   5, 179,  17,  50,  50,  50,  50,
     50,  50,  49,  19, 176,  12,   5, 164,  17,  50,  50,  53,
     17,  65,  65,  65, 224,  14,   5, 179,  17,  50,  50,  50,
     50,  50,  50,  49,  19,  66,  80,  15,   5, 164,  17,  50,
     50,  53,  18,  49,  17,  33,  33,  17,  49, 160,  12,   5,
    179,  17,  50,  65,  83,  81,  66,  49,  19, 176,  12,   5,
    165,  33,  65,  65,  65,  65,  65,  65,  65, 192,  13,   5,
    161,  50,  50,  50,  50,  50,  50,  50,  49,  19, 176,  16,
      5, 161,  50,  50,  49,  17,  17,  33,  17,  33,  17,  49,
     65,  65, 192,  15,   5, 161,  50,  50,  50,  17,  18,  17,
     19,  20,  19,  50,  49, 160,  18,   5, 161,  50,  49,  17,
     17,  33,  17,  49,  49,  17,  33,  17,  17,  50,  49, 160,
     15,   5, 161,  50,  49,  17,  17,  33,  17,  49,  65,  65,
     65,  65, 192,  12,   5, 165,  49,  65,  49,  65,  65,  49,
     65,  53, 160,  13,   3,  52,  33,  33,  33,  33,  33,  33,
     33,  33,  35,  48,  15,   6,   1,  81,  97,  81,  97,  81,
     97,  81,  97,  81,  97,  81,  96,  14,   5,  83,  65,  65,
     65,  65,  65,  65,  65,  65,  65,  35, 112,  10,   5, 113,
     49,  17,  17,  49, 240, 240, 240,   6,   6,   0,   8,  38,
     96,   8,   4,  66,  49,  65, 240, 240, 112,  12,   5, 240,
    101,  50,  50,  50,  50,  34,  18,  17, 160,  13,   5,  81,
     65,  65,  68,  17,  50,  50,  50,  50,  53, 176,  10,   5,
    240, 101,  65,  65,  65,  65,  84, 160,  13,   5, 145,  65,
     65,  21,  50,  50,  50,  50,  49,  20, 160,  10,   5, 240,
     99,  17,  50,  55,  65,  83, 176,  13,   5, 115,  17,  65,
     52,  33,  65,  65,  65,  65,  65, 208,  12,   5, 240, 101,
     50,  50,  50,  50,  49,  20,  65,  19,  14,   5,  81,  65,
     65,  68,  17,  50,  50,  50,  50,  50,  49, 160,  12,   4,
     81,  49,  98,  49,  49,  49,  49,  49,  35, 144,  14,   5,
    129,  65, 115,  65,  65,  65,  65,  65,  65,  65,  19,  32,
     18,   5,  81,  65,  65,  65,  50,  33,  17,  17,  34,  49,
     17,  33,  33,  17,  49, 160,  13,   4,  66,  49,  49,  49,
     49,  49,  49,  49,  49,  35, 144,  17,   5, 240,  82,  17,
     17,  17,  18,  17,  18,  17,  18,  17,  18,  50,  49, 160,
     13,   5, 240,  81,  18,  18,  34,  50,  50,  50,  50,  49,
    160,  12,   5, 240,  99,  17,  50,  50,  50,  50,  49,  19,
    176,  13,   5, 240,  84,  17,  50,  50,  50,  50,  53,  17,
     65,  64,  12,   5, 240, 101,  50,  50,  50,  50,  49,  20,
     65,  65,  12,   5, 240,  81,  18,  18,  49,  65,  65,  65,
     65, 224,   9,   5, 240, 101,  65,  83,  81,  69, 176,  13,
      5, 113,  65,  65,  37,  33,  65,  65,  65,  65,  82, 160,
     13,   5, 240,  81,  50,  50,  50,  50,  50,  34,  18,  17,
    160,  14,   5, 240,  81,  50,  50,  49,  17,  17,  33,  17,
     49,  65, 192,  17,   5, 240,  81,  50,  50,  17,  18,  17,
     18,  17,  18,  17,  17,  17,  17, 176,  15,   5, 240,  81,
     50,  49,  17,  17,  49,  49,  17,  17,  50,  49, 160,  13,
      5, 240,  81,  50,  50,  50,  50,  50,  49,  20,  65,  19,
     11,   5, 240,  85,  65,  49,  49,  49,  49,  69, 160,  14,
      3,  81,  17,  33,  33,  33,  17,  49,  33,  33,  33,  49,
     48,  14,   3,  49,  33,  33,  33,  33,  33,  33,  33,  33,
     33,  33,  80,  14,   5,  81,  81,  65,  65,  65,  81,  49,
     65,  65,  65,  49, 144,  10,   5,  97,  49,  17,  17,  49,
      0,   5,  96,
};

const tFont g_sFontclean6x13 =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    6,

    //
    // The height of the font.
    //
    13,

    //
    // The baseline of the font.
    //
    11,

    //
    // The offset to each character in the font.
    //
    {
           0,    5,   17,   28,   47,   63,   75,   91,
          99,  113,  127,  141,  151,  160,  166,  172,
         187,  203,  216,  230,  244,  260,  272,  286,
         299,  314,  328,  335,  345,  355,  362,  372,
         385,  398,  411,  423,  436,  450,  461,  472,
         485,  497,  509,  521,  538,  550,  565,  580,
         593,  605,  619,  634,  646,  658,  671,  687,
         702,  720,  735,  747,  760,  775,  789,  799,
         805,  813,  825,  838,  848,  861,  871,  884,
         896,  910,  922,  936,  954,  967,  984,  997,
        1009, 1022, 1034, 1046, 1055, 1068, 1081, 1095,
        1112, 1127, 1140, 1151, 1165, 1179, 1193,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8clean6x13Data
};
