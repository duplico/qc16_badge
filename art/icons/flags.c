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

static const unsigned char pixel_flags1BPP_UNCOMP[] =
{
0xff, 0xff, 0xff, 
0xff, 0xe0, 0xff, 
0xfe, 0x1f, 0x7f, 
0xe1, 0xff, 0xbf, 
0xcf, 0x7f, 0x8f, 
0x88, 0xbf, 0xb3, 
0x8b, 0xbf, 0xbd, 
0x8d, 0xbf, 0xbd, 
0x8e, 0x7f, 0xbd, 
0x8f, 0xff, 0xbd, 
0x8f, 0xff, 0xbd, 
0x8f, 0xff, 0x7d, 
0x8f, 0xe0, 0xfd, 
0x8e, 0x1f, 0x1d, 
0x81, 0xff, 0xe3, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0x8f, 0xff, 0xff, 
0xdf, 0xff, 0xff
};

static const unsigned long palette_flags1BPP_UNCOMP[]=
{
	0x000000, 	0xffffff
};

const tImage  flags1BPP_UNCOMP=
{
	IMAGE_FMT_1BPP_UNCOMP,
	24,
	24,
	2,
	palette_flags1BPP_UNCOMP,
	pixel_flags1BPP_UNCOMP,
};

