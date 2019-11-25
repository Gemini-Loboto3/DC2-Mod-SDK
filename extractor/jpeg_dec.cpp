#define _CRT_SECURE_NO_WARNINGS

#include <stdafx.h>
#include "memstream.h"
#include "tjpgd.h"
#include "Bitmap.h"

#define MODE	0	/* Test mode: 0:JPEG-BMP conversion, 1:Report about each files but output */
#define SCALE	0	/* Output scaling 0:1/1, 1:1/2, 2:1/4 or 3:1/8 */

typedef struct {
	MEM_STREAM hin;			/* Handle to input stream */
	uint8_t* frmbuf;	/* Pointer to the frame buffer */
	uint32_t wbyte;		/* Number of bytes a line of the frame buffer */
} IODEV;

uint16_t input_func(
	JDEC* jd,		/* Decompression object */
	uint8_t* buff,	/* Pointer to the read buffer (0:skip) */
	uint16_t ndata	/* Number of bytes to read/skip */
)
{
	uint32_t rb;
	IODEV* dev = (IODEV*)jd->device;


	if (buff)
	{
		rb = MemStreamRead(&dev->hin, buff, ndata);
		//ReadFile(dev->hin, buff, ndata, (LPDWORD)&rb, 0);
		return (uint16_t)rb;
	}
	else
	{
		//rb = SetFilePointer(dev->hin, ndata, 0, FILE_CURRENT);
		rb = MemStreamSeek(&dev->hin, ndata, SEEK_CUR);
		return rb == 0xFFFFFFFF ? 0 : ndata;
	}
}

/* User defined output function */

uint16_t output_func(JDEC* jd, void* bitmap, JRECT* rect)
{
	uint32_t ny, nbx, xc, wd;
	uint8_t* src, * dst;
	IODEV* dev = (IODEV*)jd->device;

	nbx = (rect->right - rect->left + 1) * 3;	/* Number of bytes a line of the rectangular */
	ny = rect->bottom - rect->top + 1;			/* Number of lines of the rectangular */
	src = (uint8_t*)bitmap;						/* RGB bitmap to be output */

	wd = dev->wbyte;							/* Number of bytes a line of the frame buffer */
	dst = dev->frmbuf + rect->top * wd + rect->left * 3;	/* Left-top of the destination rectangular in the frame buffer */

	do
	{	/* Copy the rectangular to the frame buffer */
		xc = nbx;
		do { *dst++ = *src++; } while (--xc);
		dst += wd - nbx;
	} while (--ny);

	return 1;	/* Continue to decompress */
}

__inline u8 c5(u32 col)
{
	return col;
	//col >>= 3;
	//return (col >> 3) << 3;
	//return (col << 3) | (col >> 2);
}

void write_bmp(CBitmap& dst, int w, int h, u8* data)
{
	dst.Create(w, h);

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++, data += 3)
		{
			u32 c = c5(data[2]) | (c5(data[1]) << 8) | (c5(data[0]) << 16) | 0xff000000;
			dst.setPixel(x, y, c);
		}
	}
}

void jpegtest(u8* data, size_t size, CBitmap &dst)
{
	const size_t sz_work = 4096;	/* Size of working buffer for TJpgDec module */
	void* jdwork;	/* Pointer to TJpgDec work area */
	JDEC jd;		/* TJpgDec decompression object */
	IODEV iodev;	/* Identifier of the decompression session (depends on application) */
	uint32_t xb, xs, ys;

	/* Open JPEG file */
	MemStreamOpen(&iodev.hin, data, size);
	jdwork = malloc(sz_work);	/* Allocate a work area for TJpgDec */

	JRESULT rc = jd_prepare(&jd, input_func, jdwork, (uint16_t)sz_work, &iodev);		/* Prepare to decompress the file */

	if (!rc)
	{
		xs = jd.width >> SCALE;		/* Output size */
		ys = jd.height >> SCALE;
		xb = (xs * 3 + 3) & ~3;		/* Byte width of the frame buffer */
		iodev.wbyte = xb;
		iodev.frmbuf = (uint8_t*)malloc(xb * ys);	/* Allocate an output frame buffer */
		rc = jd_decomp(&jd, output_func, SCALE);	/* Start to decompress */
		if (!rc)
		{
			/* Save the decompressed picture as a png file */
			write_bmp(dst, xs, ys, (uint8_t*)iodev.frmbuf);
		}
		else
		{
			/* Error occured on decompress */
			
		}
		free(iodev.frmbuf);	/* Discard frame buffer */
	}
	else
	{
		/* Error occured on prepare */
	}

	free(jdwork);	/* Discard work area */
}
