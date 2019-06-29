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

static const unsigned char pixel_cameras1BPP_UNCOMP[] =
{
0xff, 0xff, 0xff, 
0xff, 0xff, 0xff, 
0xff, 0xff, 0xcf, 
0xff, 0xfe, 0x07, 
0xff, 0xf0, 0x07, 
0xff, 0x00, 0x03, 
0xf8, 0x00, 0x03, 
0xe0, 0x00, 0x03, 
0xc0, 0x00, 0x03, 
0xe0, 0x00, 0x0b, 
0xd0, 0x00, 0x23, 
0xc8, 0x00, 0x83, 
0xa5, 0x55, 0x07, 
0xa0, 0x00, 0x3f, 
0xa0, 0x01, 0xff, 
0xc0, 0x1f, 0xff, 
0xf1, 0xf1, 0xff, 
0xff, 0xf8, 0xff, 
0xff, 0xfc, 0x7f, 
0xff, 0xfe, 0x3f, 
0xff, 0xff, 0x1f, 
0xff, 0xff, 0x83, 
0xff, 0xff, 0xc3, 
0xff, 0xff, 0xff
};

static const unsigned long palette_cameras1BPP_UNCOMP[]=
{
	0x000000, 	0xffffff
};

const tImage  cameras1BPP_UNCOMP=
{
	IMAGE_FMT_1BPP_UNCOMP,
	24,
	24,
	2,
	palette_cameras1BPP_UNCOMP,
	pixel_cameras1BPP_UNCOMP,
};
