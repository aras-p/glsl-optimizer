/******************************************************************************
** Filename       : imageSgi.cpp
**                  UNCLASSIFIED
**
** Description    : Utility to read SGI image format files.  This code was
**                  originally a SGI image loading utility provided with the
**                  Mesa 3D library @ http://www.mesa3d.org by Brain Paul.
**                  This has been extended to read all SGI image formats
**                  (e.g. INT, INTA, RGB, RGBA).
**
** Revision History:
**   Date       Name   Description
**   06/07/99   BRC    Initial Release
**
** Note:
**
** The SGI Image Data (if not RLE)
**
**   If the image is stored verbatim (without RLE), then image data directly
**   follows the 512 byte header.  The data for each scanline of the first
**   channel is written first.  If the image has more than 1 channel, all
**   the data for the first channel is written, followed by the remaining
**   channels.  If the BPC value is 1, then each scanline is written as XSIZE
**   bytes.  If the BPC value is 2, then each scanline is written as XSIZE
**   shorts.  These shorts are stored in the byte order described above.
**
******************************************************************************/
#define __IMAGESGI_CPP

#include "imagesgi.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

struct sImageSgiRaw
{
   struct sImageSgiHeader header;
   unsigned char *chan0;
   unsigned char *chan1;
   unsigned char *chan2;
   unsigned char *chan3;
   unsigned int *rowStart;
   int *rowSize;
};

// Static routines
static struct sImageSgiRaw *ImageSgiRawOpen(char const * const fileName);
static void ImageSgiRawClose(struct sImageSgiRaw *raw);
static void ImageSgiRawGetRow(struct sImageSgiRaw *raw, unsigned char *buf,
   int y, int z);
static void ImageSgiRawGetData(struct sImageSgiRaw *raw, struct sImageSgi 
*final);
static void *SwitchEndian16(void *value);
static void *SwitchEndian32(void *value);

// Static variables
FILE *mFp = NULL;
unsigned char *mChanTmp = NULL;


/*****************************************************************************/
struct sImageSgi *ImageSgiOpen(char const * const fileName)
{
   struct sImageSgiRaw *raw = NULL;
   struct sImageSgi *final = NULL;

   raw = ImageSgiRawOpen(fileName);
   final = new struct sImageSgi;

   assert(final);
   if(final)
   {
      final->header = raw->header;
      final->data = NULL;
      ImageSgiRawGetData(raw, final);
      ImageSgiRawClose(raw);
   }

   return final;
} // ImageSgiRawOpen


/*****************************************************************************/
void ImageSgiClose(struct sImageSgi *image)
{

   if(image)
   {
      if(image->data)
         delete[] image->data;
      image->data = NULL;
      delete image;
   }
   image = NULL;

   return;
} // ImageSgiClose


/*****************************************************************************/
static struct sImageSgiRaw *ImageSgiRawOpen(char const * const fileName)
{
   struct sImageSgiRaw *raw = NULL;
   int x;
   int i;
   bool swapFlag = false;
   union
   {
      int testWord;
      char testByte[4];
   } endianTest;
   endianTest.testWord = 1;

   // Determine endianess of platform.
   if(endianTest.testByte[0] == 1)
      swapFlag = true;
   else
      swapFlag = false;

   raw = new struct sImageSgiRaw;

   assert(raw);
   if(raw)
   {
      raw->chan0 = NULL;
      raw->chan1 = NULL;
      raw->chan2 = NULL;
      raw->chan3 = NULL;
      raw->rowStart = NULL;
      raw->rowSize = NULL;
      mFp = fopen(fileName, "rb");
      assert(mFp);

      fread(&raw->header, sizeof(struct sImageSgiHeader), 1, mFp);
      if(swapFlag == true)
      {
         SwitchEndian16(&raw->header.magic);
         SwitchEndian16(&raw->header.type);
         SwitchEndian16(&raw->header.dim);
         SwitchEndian16(&raw->header.xsize);
         SwitchEndian16(&raw->header.ysize);
         SwitchEndian16(&raw->header.zsize);
      }

      mChanTmp = new unsigned char[raw->header.xsize * raw->header.ysize];
      assert(mChanTmp);
      switch(raw->header.zsize)
      {
      case 4:
         raw->chan3 = new unsigned char[raw->header.xsize * 
raw->header.ysize];
         assert(raw->chan3);
      case 3:
         raw->chan2 = new unsigned char[raw->header.xsize * 
raw->header.ysize];
         assert(raw->chan2);
      case 2:
         raw->chan1 = new unsigned char[raw->header.xsize * 
raw->header.ysize];
         assert(raw->chan1);
      case 1:
         raw->chan0 = new unsigned char[raw->header.xsize * 
raw->header.ysize];
         assert(raw->chan0);
      }

      if(raw->header.type == IMAGE_SGI_TYPE_RLE)
      {
         x = raw->header.ysize * raw->header.zsize * sizeof(unsigned int);
         raw->rowStart = new unsigned int[x];
         raw->rowSize = new int[x];

         fseek(mFp, sizeof(struct sImageSgiHeader), SEEK_SET);
         fread(raw->rowStart, 1, x, mFp);
         fread(raw->rowSize, 1, x, mFp);

         if(swapFlag == true)
         {
            for(i=0; i<x/sizeof(unsigned int); i++)
               SwitchEndian32(&raw->rowStart[i]);
            for(i=0; i<x/sizeof(int); i++)
               SwitchEndian32(&raw->rowSize[i]);
         }

      }

   }

   return raw;
} // ImageSgiRawOpen


/*****************************************************************************/
static void ImageSgiRawClose(struct sImageSgiRaw *raw)
{

   fclose(mFp);
   mFp = NULL;

   if(mChanTmp)
      delete[] mChanTmp;
   mChanTmp = NULL;

   if(raw->chan0)
      delete[] raw->chan0;
   raw->chan0 = NULL;

   if(raw->chan1)
      delete[] raw->chan1;
   raw->chan1 = NULL;

   if(raw->chan2)
      delete[] raw->chan2;
   raw->chan2 = NULL;

   if(raw->chan3)
      delete[] raw->chan3;
   raw->chan3 = NULL;

   if(raw)
      delete raw;
   raw = NULL;

   return;
} // ImageSgiRawClose


/*****************************************************************************/
static void ImageSgiRawGetRow(struct sImageSgiRaw *raw, unsigned char *buf,
   int y, int z)
{
   unsigned char *iPtr = NULL;
   unsigned char *oPtr = NULL;
   unsigned char pixel;
   int count;

   if((raw->header.type & 0xFF00) == 0x0100)
   {
      fseek(mFp, raw->rowStart[y+z*raw->header.ysize], SEEK_SET);
      fread(mChanTmp, 1, (unsigned int)raw->rowSize[y+z*raw->header.ysize], 
mFp);
      iPtr = mChanTmp;
      oPtr = buf;
      while(1)
      {
         pixel = *iPtr++;
         count = (int)(pixel & 0x7F);
         if(!count)
         {
            return;
         }
         if (pixel & 0x80)
         {
            while (count--)
            {
               *oPtr++ = *iPtr++;
            }
         }
         else
         {
            pixel = *iPtr++;
            while (count--)
            {
               *oPtr++ = pixel;
            }
         }
      }
   }
   else
   {
      fseek(mFp,
         sizeof(struct sImageSgiHeader)+(y*raw->header.xsize) +
            (z*raw->header.xsize*raw->header.ysize),
         SEEK_SET);
      fread(buf, 1, raw->header.xsize, mFp);
   }

   return;
} // ImageSgiRawGetRow


/*****************************************************************************/
static void ImageSgiRawGetData(struct sImageSgiRaw *raw, struct sImageSgi 
*final)
{
   unsigned char *ptr = NULL;
   int i, j;

   final->data =
      new unsigned 
char[raw->header.xsize*raw->header.ysize*raw->header.zsize];
   assert(final->data);

   ptr = final->data;
   for(i=0; i<raw->header.ysize; i++)
   {
      switch(raw->header.zsize)
      {
      case 1:
         ImageSgiRawGetRow(raw, raw->chan0, i, 0);
         for(j=0; j<raw->header.xsize; j++)
            *(ptr++) = raw->chan0[j];
         break;
      case 2:
         ImageSgiRawGetRow(raw, raw->chan0, i, 0);
         ImageSgiRawGetRow(raw, raw->chan1, i, 1);
         for(j=0; j<raw->header.xsize; j++)
         {
            *(ptr++) = raw->chan0[j];
            *(ptr++) = raw->chan1[j];
         }
         break;
      case 3:
         ImageSgiRawGetRow(raw, raw->chan0, i, 0);
         ImageSgiRawGetRow(raw, raw->chan1, i, 1);
         ImageSgiRawGetRow(raw, raw->chan2, i, 2);
         for(j=0; j<raw->header.xsize; j++)
         {
            *(ptr++) = raw->chan0[j];
            *(ptr++) = raw->chan1[j];
            *(ptr++) = raw->chan2[j];
         }
         break;
      case 4:
         ImageSgiRawGetRow(raw, raw->chan0, i, 0);
         ImageSgiRawGetRow(raw, raw->chan1, i, 1);
         ImageSgiRawGetRow(raw, raw->chan2, i, 2);
         ImageSgiRawGetRow(raw, raw->chan3, i, 3);
         for(j=0; j<raw->header.xsize; j++)
         {
            *(ptr++) = raw->chan0[j];
            *(ptr++) = raw->chan1[j];
            *(ptr++) = raw->chan2[j];
            *(ptr++) = raw->chan3[j];
         }
         break;
      }
   }

   return;
} // ImageSgiRawGetData


/*****************************************************************************/
static void *SwitchEndian16(void *value)
{
   short value16 = *(short *) value;
   value16 = ((value16 & 0xff00) >> 8L) +
             ((value16 & 0x00ff) << 8L);
   *(short *)value = value16;
   return value;
} // SwitchEndian16


/*****************************************************************************/
static void *SwitchEndian32(void *value)
{
   int value32 = *(int *) value;
   value32 = ((value32 & 0xff000000) >> 24L) +
             ((value32 & 0x00ff0000) >> 8)   +
             ((value32 & 0x0000ff00) << 8)   +
             ((value32 & 0x000000ff) << 24L);
   *(int *)value = value32;
   return value;
} // SwitchEndian32

