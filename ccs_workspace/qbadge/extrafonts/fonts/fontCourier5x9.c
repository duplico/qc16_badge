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
//     Style: Courier
//     Size: 5x9
//     Bold: no
//     Italic: no
//     Memory usage: 1152 bytes
//
//*****************************************************************************

static const uint8_t g_pui8courier5x9Data[950] =
{
      5,   5,   0,   6,  32,   8,   5, 161,  65,  65,  65, 145,
    224,  10,   5, 161,  17,  33,  17,  33,  17, 240, 192,  14,
      5, 193,  17,  33,  22,  17,  17,  21,  17,  17,  33,  17,
     96,  11,   5,  97,  65,  51,  34,  81,  34,  65,  65, 128,
     11,   5, 178,  18,  18,  19,  67,  18,  18,  18, 176,  11,
      5, 240,  34,  33,  66,  18,  33,  34,  17, 160,   8,   5,
     81,  65,  65, 240, 240,  64,  10,   5, 177,  65,  49,  65,
     65,  81,  65, 128,  10,   5, 161,  65,  81,  65,  65,  49,
     65, 144,   9,   5, 177,  51,  49,  49,  17, 240, 112,   9,
      5, 240,  33,  65,  37,  33,  65, 192,   8,   5, 240, 240,
     97,  65,  49,  64,   6,   5, 240, 163, 240, 112,   6,   5,
    240, 240,  81, 224,  11,   5, 209,  65,  49,  65,  49,  65,
     49,  65,  64,  13,   5, 177,  49,  17,  33,  17,  33,  17,
     33,  17,  49, 208,   9,   5, 177,  50,  65,  65,  65,  51,
    192,  10,   5, 177,  49,  17,  65,  49,  49,  67, 192,  11,
      5, 177,  49,  17,  50,  65,  33,  17,  49, 208,  10,   5,
    193,  50,  33,  17,  36,  49,  65, 192,   9,   5, 163,  33,
     66,  81,  65,  34, 208,  10,   5, 178,  33,  65,  67,  33,
     17,  34, 208,  10,   5, 163,  33,  17,  65,  49,  65,  65,
    208,  12,   5, 163,  33,  17,  49,  49,  17,  33,  17,  49,
    208,  11,   5, 178,  33,  17,  33,  17,  50,  65,  34, 208,
      6,   5, 240, 161, 145, 224,   7,   5, 240, 177, 145,  49,
    144,   9,   5, 240,  17,  49,  65,  81, 240,  48,   7,   5,
    240,  83, 115, 240,  32,   8,   5, 241,  81,  65,  49, 240,
     64,   9,   5, 177,  49,  17,  65,  49, 145, 208,  13,   5,
    178,  49,  17,  17,  18,  18,  17,  17,  20,  83,  96,  12,
      5, 240,  18,  49,  17,  35,  33,  17,  18,  18, 160,  10,
      5, 243,  49,  17,  35,  33,  17,  19, 192,  10,   5, 242,
     49,  17,  33,  65,  17,  34, 208,  11,   5, 243,  49,  17,
     33,  17,  33,  17,  19, 192,  10,   5, 244,  33,  17,  34,
     49,  17,  20, 176,   9,   5, 244,  33,  17,  34,  49,  50,
    208,  10,   5, 240,  18,  33,  67,  33,  17,  50, 192,  12,
      5, 242,  18,  17,  17,  35,  33,  17,  18,  18, 160,   8,
      5, 243,  49,  65,  65,  51, 192,  11,   5, 240,  19,  49,
     33,  17,  33,  17,  49, 208,  12,   5, 242,  33,  17,  17,
     34,  49,  17,  18,  33, 160,   9,   5, 242,  65,  65,  65,
     17,  20, 176,  12,   5, 241,  33,  20,  18,  17,  17,  33,
     17,  33, 176,  12,   5, 242,  33,  18,  17,  17,  18,  17,
     35,  33, 160,  12,   5, 240,  17,  49,  17,  33,  17,  33,
     17,  49, 208,   9,   5, 243,  49,  17,  35,  33,  50, 208,
     14,   5, 240,  17,  49,  17,  33,  17,  33,  17,  49,  49,
     82,  32,  11,   5, 243,  49,  17,  35,  33,  17,  18,  33,
    160,  10,   5, 240,  18,  33,  82,  33,  17,  34, 208,   9,
      5, 246,  17,  17,  33,  65,  51, 176,  12,   5, 242,  18,
     17,  17,  33,  17,  33,  17,  34, 192,  11,   5, 241,  33,
     17,  33,  17,  17,  49,  65, 208,  12,   5, 242,  18,  17,
     33,  18,  17,  19,  33,  17, 176,  10,   5, 241,  18,  19,
     49,  51,  33,  18, 176,  10,   5, 241,  33,  17,  33,  34,
     49,  66, 192,  10,   5, 243,  33,  17,  49,  49,  17,  35,
    192,  10,   5, 162,  49,  65,  65,  65,  65,  66, 128,  10,
      5, 161,  81,  65,  65,  81,  65,  81,  96,  10,   5, 162,
     65,  65,  65,  65,  65,  50, 128,   8,   5, 240,  18,  33,
     33, 240, 176,   6,   5, 240, 240, 165,  80,   7,   5,   1,
     81, 240, 240, 208,  10,   5, 240,  82,  66,  33,  17,  49,
     17, 176,  11,   5, 162,  65,  66,  49,  17,  33,  17,  19,
    192,   9,   5, 240,  98,  33,  65,  17,  49, 208,  12,   5,
    178,  65,  50,  33,  17,  33,  17,  49,  17, 176,   8,   5,
    240,  98,  35,  33,  82, 192,   9,   5, 178,  49,  51,  49,
     65,  51, 192,  13,   5, 240,  97,  17,  17,  17,  33,  17,
     50,  65,  34,  48,  12,   5, 162,  65,  66,  49,  17,  33,
     17,  18,  18, 160,   8,   5, 177, 130,  65,  65,  51, 192,
     10,   5, 177, 130,  65,  65,  65,  65,  50,  48,  11,   5,
    162,  65,  65,  17,  34,  51,  18,  18, 160,   9,   5, 162,
     65,  65,  65,  65,  51, 192,  12,   5, 240,  81,  17,  34,
     17,  18,  17,  18,  18, 160,  11,   5, 240,  83,  49,  17,
     33,  17,  18,  18, 160,  10,   5, 240,  97,  49,  17,  33,
     17,  49, 208,  12,   5, 240,  83,  49,  17,  33,  17,  34,
     49,  51,  32,  12,   5, 240,  97,  17,  17,  17,  33,  17,
     50,  65,  51,   8,   5, 240,  84,  33,  65,  51, 192,   8,
      5, 240,  98,  34,  81,  34, 208,  10,   5, 177,  65,  52,
     33,  65,  17,  34, 192,  12,   5, 240,  82,  17,  33,  17,
     33,  17,  34,  17, 160,  11,   5, 240,  82,  17,  33,  17,
     33,  17,  49, 192,  11,   5, 240,  81,  17,  18,  17,  21,
     33,  17, 176,  10,   5, 240,  81,  33,  34,  50,  33,  33,
    176,  13,   5, 240,  82,  17,  33,  17,  33,  17,  49,  65,
     34,  48,   9,   5, 240,  83,  49,  49,  17,  35, 192,  10,
      5, 193,  49,  65,  49,  81,  65,  81, 112,  10,   5, 161,
     65,  65,  65,  65,  65,  65, 144,  10,   5, 161,  81,  65,
     81,  49,  65,  49, 144,   9,   5, 240,  97,  17,  17,  17,
    240, 112,
};

const tFont g_sFontcourier5x9 =
{
    //
    // The format of the font.
    //
    FONT_FMT_PIXEL_RLE,

    //
    // The maximum width of the font.
    //
    5,

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
           0,    5,   13,   23,   37,   48,   59,   70,
          78,   88,   98,  107,  116,  124,  130,  136,
         147,  160,  169,  179,  190,  200,  209,  219,
         229,  241,  252,  258,  265,  274,  281,  289,
         298,  311,  323,  333,  343,  354,  364,  373,
         383,  395,  403,  414,  426,  435,  447,  459,
         471,  480,  494,  505,  515,  524,  536,  547,
         559,  569,  579,  589,  599,  609,  619,  627,
         633,  640,  650,  661,  670,  682,  690,  699,
         712,  724,  732,  742,  753,  762,  774,  785,
         795,  807,  819,  827,  835,  845,  857,  868,
         879,  889,  902,  911,  921,  931,  941,
    },

    //
    // A pointer to the actual font data
    //
    g_pui8courier5x9Data
};