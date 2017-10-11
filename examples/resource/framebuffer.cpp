/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdint.h>
#include <iostream>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdarg>
#include <string.h>

#include <linux/fb.h>
#include <linux/videodev2.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "framebuffer.h"

namespace FBIO{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
	
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// FrameBuffer Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
FrameBuffer* FrameBuffer::Open(bool pVerbose)
{
	FrameBuffer* newFrameBuffer = NULL;

	// Open the file for reading and writing
	int File = open("/dev/fb0", O_RDWR);
	if(File)
	{
		if(pVerbose)
			std::cout << "The framebuffer device was opened successfully." << std::endl;

		// Get fixed screen information
		struct fb_fix_screeninfo finfo;
		if( ioctl(File, FBIOGET_FSCREENINFO, &finfo) )
		{
			if(pVerbose)
				std::cout << "Error reading fixed information.\n" << std::endl;
		}
		else
		{
			// Get variable screen information
			struct fb_var_screeninfo vinfo;
			if( ioctl(File, FBIOGET_VSCREENINFO, &vinfo) )
			{
				if(pVerbose)
					std::cout << "Error reading variable information." << std::endl;
			}
			else
			{
				if(pVerbose)
				{
					std::cout << "Display size: " << vinfo.xres << "x" << vinfo.yres << ", " << vinfo.bits_per_pixel << "bpp" << std::endl;

					std::cout << "Red bitfield: offset " << vinfo.red.offset << " length " << vinfo.red.length << " msb_right " << vinfo.red.msb_right << std::endl;
					std::cout << "Green bitfield: offset " << vinfo.green.offset << " length " << vinfo.green.length << " msb_right " << vinfo.green.msb_right << std::endl;
					std::cout << "Blue bitfield: offset " << vinfo.blue.offset << " length " << vinfo.blue.length << " msb_right " << vinfo.blue.msb_right << std::endl;
				}

				uint8_t* ScreenRam = (uint8_t*)mmap(0,finfo.smem_len,PROT_READ | PROT_WRITE,MAP_SHARED,File, 0);

				newFrameBuffer = new FrameBuffer(File,ScreenRam,finfo,vinfo,pVerbose);

			}
		}
	}

	if( newFrameBuffer == NULL )
	{
		close(File);
		if(pVerbose)
			std::cout << "Error: cannot open framebuffer device." << std::endl;
	}

	return newFrameBuffer;
}

FrameBuffer::FrameBuffer(int pFile,uint8_t* pFrameBuffer,struct fb_fix_screeninfo pFixInfo,struct fb_var_screeninfo pScreenInfo,bool pVerbose):
	mWidth(pScreenInfo.xres),
	mHeight(pScreenInfo.yres),
	mStride(pFixInfo.line_length),
	mPixelSize(pScreenInfo.bits_per_pixel/8),
	mFrameBufferFile(pFile),
	mFrameBufferSize(pFixInfo.smem_len),
	mVerbose(pVerbose),
	mVaribleScreenInfo(pScreenInfo),
	mFrameBuffer(pFrameBuffer)
{
}

FrameBuffer::~FrameBuffer()
{
	if(mVerbose)
		std::cout << "Free'ing frame buffer resources, frame buffer object will be invalid and not unusable." << std::endl;

	munmap((void*)mFrameBuffer,mFrameBufferSize);
	close(mFrameBufferFile);
}

void FrameBuffer::WritePixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	assert( pX >= 0 && pX < mWidth );
	assert( pY >= 0 && pY < mHeight );
	if( pX >= 0 && pX < mWidth && pY >= 0 && pY < mHeight )
	{
		// When optised by compiler these const vars will
		// all move to generate the same code as if I made it all one line and unreadable!
		// Trust your optimiser. :)
		const int RedShift = mVaribleScreenInfo.red.offset;
		const int GreenShift = mVaribleScreenInfo.green.offset;
		const int BlueShift = mVaribleScreenInfo.blue.offset;
		const int index = (pX*mPixelSize) + (pY*mStride);
		
		if( mPixelSize == 2 )
		{
			const uint16_t red = pRed >> 3;
			const uint16_t green = pGreen >> 2;
			const uint16_t blue = pBlue >> 3;

			const uint16_t pixel = (red << RedShift) | (green << GreenShift) | (blue << BlueShift);

			((uint16_t*)(mFrameBuffer + (pY*mStride)))[pX] = pixel;
		}
		else
		{
			assert( mPixelSize == 3 || mPixelSize == 4 );
			mFrameBuffer[ index + (RedShift/8) ] = pRed;
			mFrameBuffer[ index + (GreenShift/8) ] = pGreen;
			mFrameBuffer[ index + (BlueShift/8) ] = pBlue;
		}
	}
}

void FrameBuffer::ClearScreen(uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	const int RedShift = mVaribleScreenInfo.red.offset;
	const int GreenShift = mVaribleScreenInfo.green.offset;
	const int BlueShift = mVaribleScreenInfo.blue.offset;

	if( mPixelSize == 2 )
	{
		const uint16_t red = pRed >> 3;
		const uint16_t green = pGreen >> 2;
		const uint16_t blue = pBlue >> 3;
		const uint16_t pixel = (red << RedShift) | (green << GreenShift) | (blue << BlueShift);

		uint16_t *line = (uint16_t*)mFrameBuffer;
		for( int y = 0 ; y < mHeight ; y++, line += mStride )
		{
			uint16_t *dest = line;
			for( int x = 0 ; x < mWidth ; x++ )
			{
				dest[x] = pixel;
			}
		}
	}
	else
	{
		assert( mPixelSize == 3 || mPixelSize == 4 );
		uint8_t *line = mFrameBuffer;
		for( int y = 0 ; y < mHeight ; y++, line += mStride )
		{
			uint8_t *dest = line;
			for( int x = 0 ; x < mWidth ; x++, dest += mPixelSize )
			{// Could have been done with three dst pointers to remove the (c/8) but that is not an speed up. There are indirect addressing modes on most if not all CPU's.
			// Also the (c/8) will be compiled out by the optimiser.
				dest[ (RedShift/8) ] = pRed;
				dest[ (GreenShift/8) ] = pGreen;
				dest[ (BlueShift/8) ] = pBlue;
			}
		}
	}
}

void FrameBuffer::BlitRGB24(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight)
{
	int EndX = pSourceWidth + pX; 
	int EndY = pSourceHeight + pY;
	for( int y = pY ; y < mHeight && y < EndY ; y++, pSourcePixels += pSourceWidth * 3 )
	{
		const uint8_t* scanline = pSourcePixels;
		for( int x = pX ; x < mWidth && x < EndX ; x++, scanline += 3 )
		{
			WritePixel(x,y,scanline[0],scanline[1],scanline[2]);
		}
	}
}
	
void FrameBuffer::BlitRGB24(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride)
{
	pWidth += pX; 
	pHeight += pY;
	pSourcePixels += (pSourceX*3) + (pSourceY * pSourceStride);
	for( int y = pY ; y < mHeight && y < pHeight ; y++, pSourcePixels += pSourceStride )
	{
		const uint8_t* scanline = pSourcePixels;
		for( int x = pX ; x < mWidth && x < pWidth ; x++, scanline += 3 )
		{
			WritePixel(x,y,scanline[0],scanline[1],scanline[2]);
		}
	}
}

void FrameBuffer::DrawLineH(int pFromX,int pFromY,int pToX,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	if( pFromY < 0 || pFromY >= mHeight )
		return;

	pFromX = std::max(0,std::min(mWidth,pFromX));
	pToX = std::max(0,std::min(mWidth,pToX));

	if( pFromX == pToX )
		return;
	
	if( pFromX > pToX )
		std::swap(pFromX,pToX);

	const int RedShift = mVaribleScreenInfo.red.offset;
	const int GreenShift = mVaribleScreenInfo.green.offset;
	const int BlueShift = mVaribleScreenInfo.blue.offset;

	if( mPixelSize == 2 )
	{
		const uint16_t red = pRed >> 3;
		const uint16_t green = pGreen >> 2;
		const uint16_t blue = pBlue >> 3;
		const uint16_t pixel = (red << RedShift) | (green << GreenShift) | (blue << BlueShift);

		uint16_t *dest = (uint16_t*)(mFrameBuffer + (pFromX*2) + (pFromY*mStride) );
		for( int x = pFromX ; x < pToX && x < mWidth ; x++ )
		{
			dest[x] = pixel;
		}
	}
	else
	{
		assert( mPixelSize == 3 || mPixelSize == 4 );
		uint8_t *dest = mFrameBuffer + (pFromX*mPixelSize) + (pFromY*mStride);
		for( int x = pFromX ; x < pToX && x < mWidth ; x++, dest += mPixelSize )
		{
			dest[ (RedShift/8) ] = pRed;
			dest[ (GreenShift/8) ] = pGreen;
			dest[ (BlueShift/8) ] = pBlue;
		}
	}
}

void FrameBuffer::DrawLineV(int pFromX,int pFromY,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	if( pFromX < 0 || pFromX >= mWidth )
		return;

	pFromY = std::max(0,std::min(mHeight,pFromY));
	pToY = std::max(0,std::min(mHeight,pToY));

	if( pFromY == pToY )
		return;

	if( pFromY > pToY )
		std::swap(pFromY,pToY);

	const int RedShift = mVaribleScreenInfo.red.offset;
	const int GreenShift = mVaribleScreenInfo.green.offset;
	const int BlueShift = mVaribleScreenInfo.blue.offset;

	if( mPixelSize == 2 )
	{
		const uint16_t red = pRed >> 3;
		const uint16_t green = pGreen >> 2;
		const uint16_t blue = pBlue >> 3;
		const uint16_t pixel = (red << RedShift) | (green << GreenShift) | (blue << BlueShift);

		uint16_t *dest = (uint16_t*)(mFrameBuffer + (pFromX*2) + (pFromY*mStride) );
		for( int y = pFromY ; y < pToY && y < mHeight ; y++, dest += mStride )
		{
			*dest = pixel;
		}
	}
	else
	{
		assert( mPixelSize == 3 || mPixelSize == 4 );
		uint8_t *dest = mFrameBuffer + (pFromX*mPixelSize) + (pFromY*mStride);
		for( int y = pFromY ; y < pToY && y < mHeight ; y++, dest += mStride )
		{
			dest[ (RedShift/8) ] = pRed;
			dest[ (GreenShift/8) ] = pGreen;
			dest[ (BlueShift/8) ] = pBlue;
		}
	}
}

void FrameBuffer::DrawLine(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	if( pFromX == pToX )
		DrawLineV(pFromX,pFromY,pToY,pRed,pGreen,pBlue);
	else if( pFromY == pToY )
		DrawLineH(pFromX,pFromY,pToX,pRed,pGreen,pBlue);
	else
		DrawLineBresenham(pFromX,pFromY,pToX,pToY,pRed,pGreen,pBlue);
}

void FrameBuffer::DrawLineBresenham(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	// Deals with all 8 quadrants.
	int deltax = pToX - pFromX;	// The difference between the x's
	int deltay = pToY - pFromY;	// The difference between the y's	
	int x = pFromX;				// Start x off at the first pixel
	int y = pFromY;				// Start y off at the first pixel

	int xinc1 = 1;
	int xinc2 = 1;
	int yinc1 = 1;
	int yinc2 = 1;

	if( deltax < 0 )
	{// The x-values are decreasing
		deltax = -deltax;
		xinc1 = -1;
		xinc2 = -1;
	}

	if( deltay < 0 )
	{// The y-values are decreasing
		deltay = -deltay;
		yinc1 = -1;
		yinc2 = -1;
	}

	int den,num,numadd,numpixels,curpixel;
	if( deltax >= deltay )
	{// There is at least one x-value for every y-value
		xinc1 = 0;	// Don't change the x when numerator >= denominator
		yinc2 = 0;	// Don't change the y for every iteration
		den = deltax;
		num = deltax>>1;
		numadd = deltay;
		numpixels = deltax;// There are more x-values than y-values
	}
	else
	{ // There is at least one y-value for every x-value
		xinc2 = 0;	// Don't change the x for every iteration
		yinc1 = 0;	// Don't change the y when numerator >= denominator
		den = deltay;
		num = deltay>>1;
		numadd = deltax;
		numpixels = deltay;// There are more y-values than x-values
	}

	for( curpixel = 0 ; curpixel <= numpixels ; curpixel++ )
	{
		if( x > -1 && x < mWidth && y > -1 && y < mHeight )
		{
			WritePixel(x,y,pRed,pGreen,pBlue);
		}
		num += numadd;	// Increase the numerator by the top of the fraction
		if (num >= den)	// Check if numerator >= denominator
		{
			num -= den;	// Calculate the new numerator value
			x += xinc1;	// Change the x as appropriate
			y += yinc1;	// Change the y as appropriate
		}
		x += xinc2;		// Change the x as appropriate
		y += yinc2;		// Change the y as appropriate
	}
}

void FrameBuffer::DrawCircle(int pX,int pY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled)
{
    int x = pRadius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (pRadius << 1);

    while (x >= y)
    {
		if(pFilled)
		{
			//WritePixel(pX + x, pY + y,pRed,pGreen,pBlue);
			//WritePixel(pX - x, pY + y,pRed,pGreen,pBlue);
			DrawLineH(pX - x, pY + y,pX + x,pRed,pGreen,pBlue);

			//WritePixel(pX - x, pY - y,pRed,pGreen,pBlue);
			//WritePixel(pX + x, pY - y,pRed,pGreen,pBlue);
			DrawLineH(pX - x, pY - y,pX + x,pRed,pGreen,pBlue);

			//WritePixel(pX + y, pY + x,pRed,pGreen,pBlue);
			//WritePixel(pX - y, pY + x,pRed,pGreen,pBlue);
			DrawLineH(pX - y, pY + x,pX + y,pRed,pGreen,pBlue);
			
			//WritePixel(pX - y, pY - x,pRed,pGreen,pBlue);
			//WritePixel(pX + y, pY - x,pRed,pGreen,pBlue);
			DrawLineH(pX - y, pY - x,pX + y,pRed,pGreen,pBlue);			
		}
		else
		{
			WritePixel(pX + x, pY + y,pRed,pGreen,pBlue);
			WritePixel(pX + y, pY + x,pRed,pGreen,pBlue);
			WritePixel(pX - y, pY + x,pRed,pGreen,pBlue);
			WritePixel(pX - x, pY + y,pRed,pGreen,pBlue);
			WritePixel(pX - x, pY - y,pRed,pGreen,pBlue);
			WritePixel(pX - y, pY - x,pRed,pGreen,pBlue);
			WritePixel(pX + y, pY - x,pRed,pGreen,pBlue);
			WritePixel(pX + x, pY - y,pRed,pGreen,pBlue);
		}

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-pRadius << 1) + dx;
        }
    }
}

void FrameBuffer::DrawRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled)
{
	if( pFilled )
	{
		for(; pFromY <= pToY ; pFromY++ )
			DrawLineH(pFromX,pFromY,pToX,pRed,pGreen,pBlue);
	}
	else
	{
		DrawLineH(pFromX,pFromY,pToX,pRed,pGreen,pBlue);
		DrawLineH(pFromX,pToY,pToX,pRed,pGreen,pBlue);

		DrawLineV(pFromX,pFromY,pToY,pRed,pGreen,pBlue);
		DrawLineV(pToX,pToY,pToY,pRed,pGreen,pBlue);
	}
}

void FrameBuffer::HSV2RGB(float H,float S, float V,uint8_t &rRed,uint8_t &rGreen, uint8_t &rBlue)const
{
	float R;
	float G;
	float B;
	if(S <= 0.0f)
	{
		R = V;
		G = V;
		B = V;
	}
	else
	{
		float hh, p, q, t, ff;
		long i;

		hh = H;
		if(hh >= 360.0) hh = 0.0;
		hh /= 60.0;
		i = (long)hh;
		ff = hh - i;
		p = V * (1.0 - S);
		q = V * (1.0 - (S * ff));
		t = V * (1.0 - (S * (1.0 - ff)));

		switch(i)
		{
		case 0:
			R = V;
			G = t;
			B = p;
			break;
		
		case 1:
			R = q;
			G = V;
			B = p;
			break;
		
		case 2:
			R = p;
			G = V;
			B = t;
			break;

		
		case 3:
			R = p;
			G = q;
			B = V;
			break;
		
		case 4:
			R = t;
			G = p;
			B = V;
			break;

		case 5:
		default:
			R = V;
			G = p;
			B = q;
			break;
		}
	}

	rRed = (uint8_t)(R * 255.0f);
	rGreen = (uint8_t)(G * 255.0f);
	rBlue = (uint8_t)(B * 255.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Font Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static const uint8_t font8x13[] =
{
    0x00, 0x00, 0xaa, 0x00, 0x82, 0x00, 0x82, 0x00, 0x82, 0x00, 0xaa, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x00, 
    0x00, 0x00, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 
    0xaa, 0x55, 0xaa, 0x00, 0x00, 0xa0, 0xa0, 0xe0, 0xa0, 0xae, 0x04, 0x04, 
    0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x80, 0xc0, 0x80, 0x8e, 0x08, 
    0x0c, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x80, 0x80, 0x6c, 
    0x0a, 0x0c, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 
    0xee, 0x08, 0x0c, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x24, 
    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x10, 0x7c, 0x10, 0x10, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 
    0xa0, 0xa0, 0xa0, 0xa8, 0x08, 0x08, 0x08, 0x0e, 0x00, 0x00, 0x00, 0x00, 
    0x88, 0x88, 0x50, 0x50, 0x2e, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x30, 0xc0, 0x30, 0x0e, 0x00, 
    0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x18, 0x06, 0x18, 0xe0, 
    0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x44, 0x44, 
    0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x7e, 0x08, 
    0x10, 0x7e, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x70, 
    0x20, 0x20, 0x20, 0x62, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 
    0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x24, 0x24, 0x7e, 0x24, 0x7e, 0x24, 0x24, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x3c, 0x50, 0x50, 0x38, 0x14, 0x14, 0x78, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x22, 0x52, 0x24, 0x08, 0x08, 0x10, 0x24, 0x2a, 0x44, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x48, 0x48, 0x30, 0x4a, 0x44, 
    0x3a, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x08, 0x10, 0x10, 0x10, 
    0x08, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 0x10, 0x08, 0x08, 
    0x08, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x18, 
    0x7e, 0x18, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x30, 0x40, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 
    0x00, 0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 
    0x00, 0x00, 0x00, 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 
    0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x02, 0x04, 0x18, 0x20, 0x40, 
    0x7e, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 0x08, 0x1c, 0x02, 0x02, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0c, 0x14, 0x24, 0x44, 0x44, 
    0x7e, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x40, 0x40, 0x5c, 0x62, 
    0x02, 0x02, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x20, 0x40, 0x40, 
    0x5c, 0x62, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 
    0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 
    0x42, 0x42, 0x3c, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x3c, 
    0x42, 0x42, 0x46, 0x3a, 0x02, 0x02, 0x04, 0x38, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 0x38, 0x30, 0x40, 0x00, 
    0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 
    0x40, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x02, 0x04, 0x08, 0x08, 
    0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x4e, 0x52, 0x56, 
    0x4a, 0x40, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 
    0x7e, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 0x42, 0x44, 
    0x78, 0x44, 0x42, 0x44, 0x78, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 
    0x40, 0x40, 0x40, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 
    0x42, 0x42, 0x42, 0x42, 0x42, 0x44, 0x78, 0x00, 0x00, 0x00, 0x00, 0x7e, 
    0x40, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x00, 
    0x7e, 0x40, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 
    0x00, 0x3c, 0x42, 0x40, 0x40, 0x40, 0x4e, 0x42, 0x46, 0x3a, 0x00, 0x00, 
    0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x00, 
    0x00, 0x00, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 
    0x00, 0x00, 0x00, 0x00, 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x44, 
    0x38, 0x00, 0x00, 0x00, 0x00, 0x42, 0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 
    0x44, 0x42, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 
    0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0xc6, 0xaa, 0x92, 
    0x92, 0x82, 0x82, 0x82, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x62, 0x52, 
    0x4a, 0x46, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 
    0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x42, 
    0x42, 0x42, 0x7c, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x3c, 
    0x42, 0x42, 0x42, 0x42, 0x42, 0x52, 0x4a, 0x3c, 0x02, 0x00, 0x00, 0x00, 
    0x7c, 0x42, 0x42, 0x42, 0x7c, 0x50, 0x48, 0x44, 0x42, 0x00, 0x00, 0x00, 
    0x00, 0x3c, 0x42, 0x40, 0x40, 0x3c, 0x02, 0x02, 0x42, 0x3c, 0x00, 0x00, 
    0x00, 0x00, 0xfe, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 
    0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x44, 0x44, 0x44, 0x28, 0x28, 0x28, 
    0x10, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x82, 0x82, 0x92, 0x92, 0x92, 
    0xaa, 0x44, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x44, 0x28, 0x10, 0x28, 
    0x44, 0x82, 0x82, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x44, 0x28, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 0x08, 
    0x10, 0x20, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 
    0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x78, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 
    0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x5c, 0x62, 0x42, 0x42, 0x62, 
    0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x3a, 0x46, 0x42, 
    0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 
    0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x20, 
    0x7c, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x3a, 0x44, 0x44, 0x38, 0x40, 0x3c, 0x42, 0x3c, 0x00, 0x00, 0x40, 0x40, 
    0x40, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00, 
    0x00, 0x40, 0x40, 0x40, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00, 0x00, 
    0x00, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x92, 0x92, 0x92, 0x92, 0x82, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x62, 0x42, 0x42, 0x42, 
    0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x42, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x62, 0x42, 
    0x62, 0x5c, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x46, 
    0x42, 0x46, 0x3a, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 
    0x22, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x3c, 0x42, 0x30, 0x0c, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 
    0x20, 0x7c, 0x20, 0x20, 0x20, 0x22, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x92, 0x92, 0xaa, 0x44, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x02, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x04, 0x08, 0x10, 0x20, 
    0x7e, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x10, 0x10, 0x08, 0x30, 0x08, 0x10, 
    0x10, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x70, 0x08, 0x08, 0x10, 0x0c, 
    0x10, 0x08, 0x08, 0x70, 0x00, 0x00, 0x00, 0x00, 0x24, 0x54, 0x48, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x54, 0x50, 
    0x50, 0x54, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 
    0x70, 0x20, 0x20, 0x20, 0x62, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x42, 0x3c, 0x24, 0x24, 0x3c, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 
    0x82, 0x44, 0x28, 0x7c, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 
    0x18, 0x24, 0x20, 0x18, 0x24, 0x24, 0x18, 0x04, 0x24, 0x18, 0x00, 0x00, 
    0x00, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x38, 0x44, 0x92, 0xaa, 0xa2, 0xaa, 0x92, 0x44, 0x38, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3c, 0x00, 0x7c, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x24, 0x48, 0x90, 0x48, 0x24, 
    0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 
    0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x92, 0xaa, 0xaa, 
    0xb2, 0xaa, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 
    0x24, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x30, 
    0x48, 0x08, 0x30, 0x40, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x30, 0x48, 0x10, 0x08, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x66, 0x5a, 
    0x40, 0x00, 0x00, 0x00, 0x3e, 0x74, 0x74, 0x74, 0x34, 0x14, 0x14, 0x14, 
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x08, 0x18, 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x70, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x48, 0x48, 0x30, 
    0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x48, 
    0x24, 0x12, 0x24, 0x48, 0x90, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc0, 0x40, 
    0x40, 0x42, 0xe6, 0x0a, 0x12, 0x1a, 0x06, 0x00, 0x00, 0x00, 0x40, 0xc0, 
    0x40, 0x40, 0x4c, 0xf2, 0x02, 0x0c, 0x10, 0x1e, 0x00, 0x00, 0x00, 0x60, 
    0x90, 0x20, 0x10, 0x92, 0x66, 0x0a, 0x12, 0x1a, 0x06, 0x00, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x10, 0x10, 0x20, 0x40, 0x42, 0x42, 0x3c, 0x00, 0x00, 
    0x00, 0x10, 0x08, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00, 
    0x00, 0x00, 0x08, 0x10, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 
    0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 
    0x42, 0x00, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 
    0x42, 0x42, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x18, 0x24, 0x42, 0x42, 
    0x7e, 0x42, 0x42, 0x00, 0x00, 0x00, 0x18, 0x24, 0x18, 0x18, 0x24, 0x42, 
    0x42, 0x7e, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x90, 0x90, 0x90, 
    0x9c, 0xf0, 0x90, 0x90, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 
    0x40, 0x40, 0x40, 0x40, 0x42, 0x3c, 0x08, 0x10, 0x00, 0x10, 0x08, 0x00, 
    0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x08, 0x10, 
    0x00, 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x18, 
    0x24, 0x00, 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 
    0x24, 0x24, 0x00, 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 
    0x00, 0x20, 0x10, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 
    0x00, 0x00, 0x08, 0x10, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 
    0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x7c, 0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 0x42, 0x42, 0xe2, 0x42, 
    0x42, 0x44, 0x78, 0x00, 0x00, 0x00, 0x64, 0x98, 0x00, 0x82, 0xc2, 0xa2, 
    0x92, 0x8a, 0x86, 0x82, 0x00, 0x00, 0x00, 0x20, 0x10, 0x00, 0x7c, 0x82, 
    0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x7c, 
    0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 
    0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x64, 0x98, 
    0x00, 0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x44, 
    0x44, 0x00, 0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, 0x00, 0x00, 
    0x00, 0x02, 0x3c, 0x46, 0x4a, 0x4a, 0x52, 0x52, 0x52, 0x62, 0x3c, 0x40, 
    0x00, 0x00, 0x20, 0x10, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 
    0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 
    0x3c, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x42, 0x42, 0x42, 0x42, 
    0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x44, 0x44, 0x28, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x40, 0x7c, 0x42, 0x42, 
    0x42, 0x7c, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 
    0x48, 0x50, 0x4c, 0x42, 0x42, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 
    0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x04, 
    0x08, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 0x00, 
    0x18, 0x24, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 
    0x00, 0x32, 0x4c, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 
    0x00, 0x00, 0x24, 0x24, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 
    0x00, 0x00, 0x18, 0x24, 0x18, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x12, 0x7c, 0x90, 0x92, 
    0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 
    0x42, 0x3c, 0x08, 0x10, 0x00, 0x00, 0x10, 0x08, 0x00, 0x3c, 0x42, 0x7e, 
    0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x3c, 0x42, 
    0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x3c, 
    0x42, 0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 
    0x3c, 0x42, 0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 
    0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x20, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 
    0x30, 0x48, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 
    0x00, 0x48, 0x48, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 
    0x00, 0x24, 0x18, 0x28, 0x04, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 
    0x00, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x42, 
    0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x42, 
    0x3c, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x3c, 0x42, 0x42, 0x42, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x3c, 0x42, 0x42, 
    0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x3c, 0x42, 
    0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x3c, 
    0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 
    0x00, 0x7c, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x3c, 0x46, 0x4a, 0x52, 0x62, 0x3c, 0x40, 0x00, 0x00, 0x00, 0x20, 
    0x10, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 0x00, 0x00, 
    0x08, 0x10, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 0x00, 
    0x00, 0x18, 0x24, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 
    0x00, 0x00, 0x28, 0x28, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 
    0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x02, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x40, 0x40, 0x5c, 0x62, 0x42, 0x42, 0x62, 
    0x5c, 0x40, 0x40, 0x00, 0x00, 0x24, 0x24, 0x00, 0x42, 0x42, 0x42, 0x46, 
    0x3a, 0x02, 0x42, 0x3c
};

Font::Font(int pPixelSize):mPixelSize(1)
{
	SetPenColour(255,255,255);
	SetPixelSize(pPixelSize);
}

Font::~Font()
{
}

void Font::DrawChar(FrameBuffer* pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,int pChar)const
{
	const uint8_t* d = font8x13 + pChar * 13;
	if( mPixelSize == 1 )
	{
		for(int y = 0 ; y < 13 ; y++ , d++)
		{
			int x = pX;
			const uint8_t bits = *d;
			if( bits != 0 )
			{
				for( int bit = 7 ; bit > -1 ; bit-- , x++ )
				{
					if( bits&(1<<bit) )
					{
						pDest->WritePixel(x,pY + y,pGreen,pGreen,pBlue);
					}
				}
			}
		}
	}
	else
	{
		for(int y = 0 ; y < 13 ; y++ , d++ , pY += mPixelSize )
		{
			int x = pX;
			const uint8_t bits = *d;
			if( bits != 0 )
			{
				for( int bit = 7 ; bit > -1 ; bit-- , x+=mPixelSize )
				{
					if( bits&(1<<bit) )
					{
						for(int i = 0 ; i < mPixelSize ; i++ )
							pDest->DrawLineH(x,pY+i,x+mPixelSize,pGreen,pGreen,pBlue);
					}
				}
			}
		}
	}

}

void Font::Print(FrameBuffer* pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pText)const
{
	while(*pText)
	{
		DrawChar(pDest,pX,pY,pGreen,pGreen,pBlue,*pText);
		pX += 8*mPixelSize;
		pText++;
	};
}

void Font::Printf(FrameBuffer* pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pFmt,...)const
{
	char buf[1024];	
	va_list args;
	va_start(args, pFmt);
	vsnprintf(buf, sizeof(buf), pFmt, args);
	va_end(args);
	Print(pDest, pX,pY,pGreen,pGreen,pBlue, buf);
}

void Font::Print(FrameBuffer* pDest,int pX,int pY,const char* pText)const
{
	Print(pDest,pX,pY,mPenColour.r,mPenColour.g,mPenColour.b,pText);
}

void Font::Printf(FrameBuffer* pDest,int pX,int pY,const char* pFmt,...)const
{
	char buf[1024];	
	va_list args;
	va_start(args, pFmt);
	vsnprintf(buf, sizeof(buf), pFmt, args);
	va_end(args);
	Print(pDest, pX,pY,mPenColour.r,mPenColour.g,mPenColour.b, buf);
}

void Font::SetPenColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	mPenColour.r = pRed;
	mPenColour.g = pGreen;
	mPenColour.b = pBlue;
}

void Font::SetPixelSize(int pPixelSize)
{
	assert( pPixelSize > 0 );
	if( pPixelSize > 0 )
		mPixelSize = pPixelSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace FBIO
   