//*****************************************************************************
//
// Copyright 1984-1989, 1994 Adobe Systems Incorporated.
// Copyright 1988, 1994 Digital Equipment Corporation.
// 
// Adobe is a trademark of Adobe Systems Incorporated which may be
// registered in certain jurisdictions.
// Permission to use these trademarks is hereby granted only in
// association with the images described in this file.
// 
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose and without fee is hereby
// granted, provided that the above copyright notices appear in all
// copies and that both those copyright notices and this permission
// notice appear in supporting documentation, and that the names of
// Adobe Systems and Digital Equipment Corporation not be used in
// advertising or publicity pertaining to distribution of the software
// without specific, written prior permission.  Adobe Systems and
// Digital Equipment Corporation make no representations about the
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
//     Style: Helvetica
//     Size: 6x12
//     Bold: no
//     Italic: yes
//     Memory usage: 1568 bytes
//
//*****************************************************************************

static const uint8_t g_pui8helvetica6x12iData[1366] =
{
      5,   9,   0,  11,  32,  12,   9,  33, 129, 129, 113, 129,
    129, 240,  17, 240, 176,  11,   9,  17,  17,  97,  17,  81,
     17,   0,   8,  80,  16,   9, 193,  17,  97,  17,  70,  65,
     17,  70,  65,  17,  97,  17, 240, 128,  19,   9,  65,  99,
     81,  17,  17,  65,  17, 115,  97,  17,  65,  17,  17,  83,
     97, 240,  16,  24,   9,  19,  33,  33,  33,  17,  49,  17,
     17,  66,  17, 129,  18,  65,  17,  17,  49,  17,  33,  33,
     35, 240,  96,  19,   9,  50,  97,  33,  81,  33,  83,  81,
     17,  33,  33,  50,  49,  49,  83,  17, 240,  96,   8,   9,
      1, 129, 129,   0,   8, 112,  13,   9,  49, 113, 113, 129,
    113, 129, 129, 129, 129, 145, 112,  13,   9,  33, 145, 129,
    129, 129, 129, 113, 129, 113, 113, 128,  10,   9,  17,  17,
     98,  97,  17,   0,   8,  80,  11,   9, 240,  81, 129, 100,
     97, 129, 240, 240,  64,   8,   9,   0,   8,   1, 129, 113,
    128,   8,   9, 240, 240, 100,   0,   6,  32,   7,   9,   0,
      7, 113, 240, 176,  12,   9,  49, 129, 113, 129, 113, 129,
    113, 129, 240, 176,  18,   9,  35,  82,  33,  65,  49,  49,
     65,  49,  65,  49,  49,  65,  34,  83, 240, 128,  12,   9,
     33,  99, 113, 129, 129, 113, 129, 129, 240, 176,  13,   9,
     35,  81,  49, 129, 113,  98,  97, 113, 133, 240, 112,  14,
      9,  35,  81,  49, 129,  83, 129, 129,  65,  49,  83, 240,
    128,  15,   9,  81, 114,  97,  17,  81,  17,  81,  33,  70,
     97, 129, 240, 128,  14,   9,  36,  81, 113, 132, 145,  49,
     65,  49,  49,  83, 240, 128,  16,   9,  35,  81,  49,  65,
    116,  81,  49,  65,  49,  65,  49,  83, 240, 128,  12,   9,
      5, 129, 113, 113, 129, 113, 113, 129, 240, 176,  17,   9,
     35,  81,  49,  65,  49,  68,  81,  33,  65,  49,  65,  49,
     83, 240, 128,  16,   9,  35,  81,  49,  65,  49,  65,  49,
     84, 113,  65,  49,  83, 240, 128,   9,   9, 240,  65, 240,
    240, 209, 240, 176,  10,   9, 240,  81, 240, 240, 209, 129,
    113, 128,  11,   9, 240,  97,  98,  97, 145, 145, 240, 240,
     48,   8,   9, 240, 212, 212, 240, 240, 176,  11,   9, 240,
     65, 145, 145,  98,  97, 240, 240,  80,  13,   9,  34,  97,
     33, 129,  98,  97, 129, 240,  17, 240, 176,  24,   9,  68,
     50,  65,  18, 113,  34,  17,  17,  33,  33,  33,  17,  49,
     33,  17,  34,  18,  34,  18,  33, 148,  48,  17,   9,  81,
    114,  97,  17,  97,  17,  81,  33,  70,  49,  65,  33,  81,
    240,  80,  17,   9,  36,  81,  49,  49,  65,  53,  65,  49,
     49,  65,  49,  50,  53, 240, 112,  15,   9,  51,  81,  49,
     49, 129, 113, 129,  81,  33,  65,  68, 240, 112,  18,   9,
     36,  81,  49,  65,  49,  49,  65,  49,  49,  49,  65,  49,
     49,  68, 240, 128,  12,   9,  37,  65, 113, 133,  65, 113,
    129, 133, 240, 112,  12,   9,  37,  65, 113, 132,  81, 113,
    129, 129, 240, 176,  16,   9,  52,  65,  65,  33, 129, 113,
     51,  33,  81,  33,  65,  69, 240,  96,  19,   9,  33,  65,
     49,  65,  33,  65,  54,  49,  65,  33,  65,  49,  65,  49,
     65, 240,  96,  12,   9,  33, 129, 113, 129, 129, 113, 129,
    129, 240, 176,  14,   9,  81, 129, 113, 129, 129,  65,  33,
     81,  33,  98, 240, 144,  19,   9,  33,  49,  65,  33,  65,
     33,  83,  97,  17,  81,  49,  65,  49,  65,  65, 240,  96,
     12,   9,  33, 129, 113, 129, 129, 113, 129, 132, 240, 128,
     27,   9,  33,  65,  49,  50,  34,  50,  33,  17,  17,  17,
     33,  17,  17,  17,  17,  34,  17,  33,  33,  33,  33,  33,
     33, 240,  80,  24,   9,  33,  65,  49,  65,  33,  17,  33,
     49,  17,  33,  49,  17,  33,  33,  33,  17,  49,  50,  49,
     65, 240,  96,  18,   9,  52,  65,  65,  49,  65,  33,  65,
     49,  65,  33,  65,  49,  65,  68, 240, 112,  14,   9,  36,
     81,  49,  49,  65,  53,  65, 113, 129, 129, 240, 176,  19,
      9,  52,  65,  65,  49,  65,  33,  65,  49,  65,  33,  33,
     17,  49,  50,  69, 145, 176,  18,   9,  36,  81,  49,  49,
     65,  53,  65,  49,  49,  65,  49,  49,  65,  49, 240, 112,
     15,   9,  51,  81,  49,  49, 147, 145,  49,  65,  49,  49,
     83, 240, 128,  12,   9,   5,  97, 113, 129, 129, 113, 129,
    129, 240, 176,  19,   9,  33,  65,  49,  65,  33,  65,  49,
     65,  49,  65,  33,  65,  49,  65,  68, 240, 112,  19,   9,
      1,  65,  49,  65,  49,  49,  65,  49,  65,  33,  81,  33,
     81,  17, 113, 240, 160,  24,   9,   1,  49,  50,  49,  50,
     34,  33,  17,  34,  33,  17,  17,  17,  17,  50,  34,  49,
     49,  65,  49, 240,  96,  18,   9,  33,  65,  49,  49,  81,
     17,  98, 114,  97,  17,  81,  49,  49,  65, 240,  96,  16,
      9,   1,  81,  33,  65,  65,  33,  81,  17, 113, 113, 129,
    129, 240, 160,  12,   9,  37, 113, 113, 113, 113, 113, 113,
    133, 240, 112,  13,   9,  51,  97, 113, 129, 129, 113, 129,
    129, 113, 131,  96,  12,   9,   1, 129, 129, 129, 145, 129,
    129, 129, 240, 160,  13,   9,  51, 129, 113, 129, 129, 113,
    129, 129, 113,  99,  96,   9,   9, 177, 114,  97,  33,   0,
      7,  48,   6,   9,   0,  10,  22,  48,   7,   9,   1, 145,
      0,   9, 112,  14,   9, 240,  67, 145,  84,  65,  33,  81,
     33,  98,  17, 240, 112,  16,   9,  33, 129, 115,  97,  33,
     81,  33,  65,  49,  65,  33,  83, 240, 144,  13,   9, 240,
     82,  97,  33,  65, 129, 129,  33,  98, 240, 144,  16,   9,
     81, 129,  83,  81,  33,  65,  49,  65,  33,  81,  33,  99,
    240, 128,  13,   9, 240,  83,  81,  33,  68,  81, 129,  33,
     98, 240, 144,  12,   9,  34,  97, 115, 113, 129, 113, 129,
    129, 240, 176,  17,   9, 240,  99,  81,  33,  65,  49,  65,
     33,  81,  33,  99,  65,  33,  98,  96,  18,   9,  33, 129,
    113,  18,  82,  33,  65,  49,  49,  49,  65,  49,  65,  49,
    240, 112,  12,   9,  33, 240,  17, 129, 129, 113, 129, 129,
    240, 176,  13,   9,  49, 240,  17, 129, 129, 113, 129, 129,
    129, 113, 128,  17,   9,  33, 129, 113,  33,  81,  17,  98,
     97,  17,  97,  33,  81,  33, 240, 128,  12,   9,  33, 129,
    113, 129, 129, 113, 129, 129, 240, 176,  22,   9, 240,  67,
     18,  49,  33,  33,  33,  33,  33,  17,  33,  33,  33,  33,
     33,  33,  33,  33, 240,  80,  17,   9, 240,  65,  18,  82,
     33,  65,  49,  49,  49,  65,  49,  65,  49, 240, 112,  15,
      9, 240,  83,  81,  49,  65,  49,  49,  49,  65,  49,  83,
    240, 128,  16,   9, 240,  83,  97,  33,  81,  33,  65,  49,
     65,  33,  83,  81, 129, 128,  16,   9, 240,  83,  81,  33,
     65,  49,  65,  33,  81,  33,  99, 113, 129,  96,  12,   9,
    240,  65,  18,  82, 113, 113, 129, 129, 240, 176,  13,   9,
    240,  82,  97,  33,  97, 145,  81,  33,  98, 240, 144,  12,
      9,  33, 129, 100,  97, 129, 113, 129, 130, 240, 160,  16,
      9, 240,  65,  33,  81,  33,  65,  49,  65,  33,  81,  33,
     99, 240, 128,  15,   9, 240,  49,  49,  65,  33,  81,  33,
     81,  17, 114, 113, 240, 160,  21,   9, 240,  49,  33,  33,
     33,  33,  33,  33,  33,  33,  33,  18,  17,  65,  33,  81,
     33, 240, 112,  15,   9, 240,  81,  33,  81,  17, 113, 114,
     97,  33,  65,  49, 240, 112,  16,   9, 240,  65,  33,  81,
     33,  81,  17,  97,  17, 113, 129, 113, 113, 128,  11,   9,
    240,  68, 129, 113, 113, 113, 116, 240, 128,  13,   9,  49,
    113, 113, 129, 113, 145, 129, 113, 129, 145, 112,  13,   9,
     49, 129, 113, 129, 129, 113, 129, 129, 113, 129, 128,  13,
      9,  33, 145, 129, 113, 145, 113, 129, 129, 113, 113, 128,
     10,   9, 240, 210,  33,  49,  34,   0,   6,  16,
};

const tFont g_sFonthelvetica6x12i =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    9,

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
           0,    5,   17,   28,   44,   63,   87,  106,
         114,  127,  140,  150,  161,  169,  177,  184,
         196,  214,  226,  239,  253,  268,  282,  298,
         310,  327,  343,  352,  362,  373,  381,  392,
         405,  429,  446,  463,  478,  496,  508,  520,
         536,  555,  567,  581,  600,  612,  639,  663,
         681,  695,  714,  732,  747,  759,  778,  797,
         821,  839,  855,  867,  880,  892,  905,  914,
         920,  927,  941,  957,  970,  986,  999, 1011,
        1028, 1046, 1058, 1071, 1088, 1100, 1122, 1139,
        1154, 1170, 1186, 1198, 1211, 1223, 1239, 1254,
        1275, 1290, 1306, 1317, 1330, 1343, 1356,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8helvetica6x12iData
};