/* Copyright (c) 2012, Texas Instruments Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

*  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

*  Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

*  Neither the name of Texas Instruments Incorporated nor the names of
   its contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/


#include "grlib.h"

static const unsigned char pixel_cocktails1BPP_UNCOMP[] =
{
0xef, 0xf9, 0xff, 
0xc7, 0x30, 0xff, 
0xc6, 0x19, 0xff, 
0xcc, 0x3f, 0xff, 
0xf8, 0x3f, 0xff, 
0xc0, 0x00, 0x03, 
0xb0, 0x7f, 0xfd, 
0xb0, 0x7f, 0xfd, 
0xa0, 0xfd, 0xfd, 
0xa0, 0xfa, 0xfd, 
0xa0, 0xf7, 0x7d, 
0xc0, 0xef, 0xbb, 
0xc0, 0x5f, 0xdb, 
0xc0, 0x6f, 0xbb, 
0xc0, 0x37, 0x43, 
0xc0, 0x02, 0x03, 
0xe0, 0x00, 0x07, 
0xe0, 0x00, 0x07, 
0xe0, 0x00, 0x07, 
0xe8, 0x00, 0x17, 
0xe8, 0x00, 0x17, 
0xf7, 0xff, 0xef, 
0xf8, 0x00, 0x1f, 
0xff, 0xff, 0xff
};

static const unsigned long palette_cocktails1BPP_UNCOMP[]=
{
	0x000000, 	0xffffff
};

const tImage  cocktails1BPP_UNCOMP=
{
	IMAGE_FMT_1BPP_UNCOMP,
	24,
	24,
	2,
	palette_cocktails1BPP_UNCOMP,
	pixel_cocktails1BPP_UNCOMP,
};

