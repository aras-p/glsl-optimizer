/******************************************************************************
** Filename       : imageSgi.h
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
**   06/08/99   BRC    Initial Release
**
******************************************************************************/

#ifndef __IMAGESGI_H
#define __IMAGESGI_H

#define IMAGE_SGI_TYPE_VERBATIM 0
#define IMAGE_SGI_TYPE_RLE      1

struct sImageSgiHeader          // 512 bytes
{
  short magic;                  // IRIS image file magic number (474)
  char type;                    // Storage format (e.g. RLE or VERBATIM)
  char numBytesPerPixelChannel; // Number of bytes per pixel channel
  unsigned short dim;           // Number of dimensions (1 to 3)
  unsigned short xsize;         // Width (in pixels)
  unsigned short ysize;         // Height (in pixels)
  unsigned short zsize;         // Number of channels (1 to 4)
  int minimumPixelValue;        // Minimum pixel value (0 to 255)
  int maximumPixelValue;        // Maximum pixel value (0 to 255)
  char padding1[4];             // (ignored)
  char imageName[80];           // Image name
  int colormap;                 // colormap ID (0=normal, 0=dithered,
                                // 2=screen, 3=colormap)
  char padding2[404];           // (ignored)
};

struct sImageSgi
{
   struct sImageSgiHeader header;
   unsigned char *data;
};

#ifndef __IMAGESGI_CPP

// RGB image load utility
extern struct sImageSgi *ImageSgiOpen(char const * const fileName);
extern void ImageSgiClose(struct sImageSgi *image);

#endif

#endif /* __IMAGESGI_H */
