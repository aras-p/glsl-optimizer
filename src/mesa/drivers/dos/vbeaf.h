/*
 *       ______             ____  ______     _____  ______ 
 *      |  ____|           |  _ \|  ____|   / / _ \|  ____|
 *      | |__ _ __ ___  ___| |_) | |__     / / |_| | |__ 
 *      |  __| '__/ _ \/ _ \  _ <|  __|   / /|  _  |  __|
 *      | |  | | |  __/  __/ |_) | |____ / / | | | | |
 *      |_|  |_|  \___|\___|____/|______/_/  |_| |_|_|
 *
 *
 *      VBE/AF structure definitions and constants.
 *
 *      See freebe.txt for copyright information.
 */


#define FREEBE_VERSION     "v1.2"


#ifndef ALLEGRO_H


#include <pc.h>


#define NULL   0

#define TRUE   1
#define FALSE  0

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))

#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))

#define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)

typedef long fixed;


#endif



/* mode attribute flags */
#define afHaveMultiBuffer        0x0001  /* multiple buffers */
#define afHaveVirtualScroll      0x0002  /* virtual scrolling */
#define afHaveBankedBuffer       0x0004  /* supports banked framebuffer */
#define afHaveLinearBuffer       0x0008  /* supports linear framebuffer */
#define afHaveAccel2D            0x0010  /* supports 2D acceleration */
#define afHaveDualBuffers        0x0020  /* uses dual buffers */
#define afHaveHWCursor           0x0040  /* supports a hardware cursor */
#define afHave8BitDAC            0x0080  /* 8 bit palette DAC */
#define afNonVGAMode             0x0100  /* not a VGA mode */
#define afHaveDoubleScan         0x0200  /* supports double scanning */
#define afHaveInterlaced         0x0400  /* supports interlacing */
#define afHaveTripleBuffer       0x0800  /* supports triple buffering */
#define afHaveStereo             0x1000  /* supports stereo LCD glasses */
#define afHaveROP2               0x2000  /* supports ROP2 mix codes */
#define afHaveHWStereoSync       0x4000  /* hardware stereo signalling */
#define afHaveEVCStereoSync      0x8000  /* HW stereo sync via EVC connector */



/* drawing modes */
typedef enum 
{ 
   AF_FORE_MIX = 0,           /* background pixels use the foreground mix */
   AF_REPLACE_MIX = 0,        /* solid drawing mode */
   AF_AND_MIX,                /* bitwise AND mode */
   AF_OR_MIX,                 /* bitwise OR mode */
   AF_XOR_MIX,                /* bitwise XOR mode */
   AF_NOP_MIX,                /* nothing is drawn */

   /* below here need only be supported if you set the afHaveROP2 flag */
   AF_R2_BLACK = 0x10, 
   AF_R2_NOTMERGESRC, 
   AF_R2_MASKNOTSRC, 
   AF_R2_NOTCOPYSRC, 
   AF_R2_MASKSRCNOT, 
   AF_R2_NOT, 
   AF_R2_XORSRC, 
   AF_R2_NOTMASKSRC, 
   AF_R2_MASKSRC, 
   AF_R2_NOTXORSRC, 
   AF_R2_NOP, 
   AF_R2_MERGENOTSRC, 
   AF_R2_COPYSRC, 
   AF_R2_MERGESRCNOT, 
   AF_R2_MERGESRC, 
   AF_R2_WHITE, 
} AF_mixModes;



/* fixed point coordinate pair */
typedef struct AF_FIX_POINT 
{
   fixed x;
   fixed y;
} AF_FIX_POINT;



/* trapezium information block */
typedef struct AF_TRAP 
{
   unsigned long y;
   unsigned long count;
   fixed x1;
   fixed x2;
   fixed slope1;
   fixed slope2;
} AF_TRAP;



/* hardware cursor description */
typedef struct AF_CURSOR 
{
   unsigned long xorMask[32];
   unsigned long andMask[32];
   unsigned long hotx;
   unsigned long hoty;
} AF_CURSOR;



/* color value */
typedef struct AF_PALETTE 
{
   unsigned char blue;
   unsigned char green;
   unsigned char red;
   unsigned char alpha;
} AF_PALETTE;



/* CRTC information block for refresh rate control */
typedef struct AF_CRTCInfo
{
   unsigned short HorizontalTotal      __attribute__ ((packed));  /* horizontal total (pixels) */
   unsigned short HorizontalSyncStart  __attribute__ ((packed));  /* horizontal sync start position */
   unsigned short HorizontalSyncEnd    __attribute__ ((packed));  /* horizontal sync end position */
   unsigned short VerticalTotal        __attribute__ ((packed));  /* vertical total (lines) */
   unsigned short VerticalSyncStart    __attribute__ ((packed));  /* vertical sync start position */
   unsigned short VerticalSyncEnd      __attribute__ ((packed));  /* vertical sync end position */
   unsigned char  Flags                __attribute__ ((packed));  /* initialisation flags for mode */
   unsigned int   PixelClock           __attribute__ ((packed));  /* pixel clock in units of Hz */
   unsigned short RefreshRate          __attribute__ ((packed));  /* expected refresh rate in .01Hz */
   unsigned short NumBuffers           __attribute__ ((packed));  /* number of display buffers */
} AF_CRTCInfo;



/* definitions for CRTC information block flags */
#define afDoubleScan             0x0001  /* enable double scanned mode */
#define afInterlaced             0x0002  /* enable interlaced mode */
#define afHSyncNeg               0x0004  /* horizontal sync is negative */
#define afVSyncNeg               0x0008  /* vertical sync is negative */



typedef unsigned char AF_PATTERN;      /* pattern array elements */
typedef unsigned AF_STIPPLE;           /* 16 bit line stipple pattern */
typedef unsigned AF_COLOR;             /* packed color values */



/* mode information structure */
typedef struct AF_MODE_INFO 
{
   unsigned short Attributes              __attribute__ ((packed));
   unsigned short XResolution             __attribute__ ((packed));
   unsigned short YResolution             __attribute__ ((packed));
   unsigned short BytesPerScanLine        __attribute__ ((packed));
   unsigned short BitsPerPixel            __attribute__ ((packed));
   unsigned short MaxBuffers              __attribute__ ((packed));
   unsigned char  RedMaskSize             __attribute__ ((packed));
   unsigned char  RedFieldPosition        __attribute__ ((packed));
   unsigned char  GreenMaskSize           __attribute__ ((packed));
   unsigned char  GreenFieldPosition      __attribute__ ((packed));
   unsigned char  BlueMaskSize            __attribute__ ((packed));
   unsigned char  BlueFieldPosition       __attribute__ ((packed));
   unsigned char  RsvdMaskSize            __attribute__ ((packed));
   unsigned char  RsvdFieldPosition       __attribute__ ((packed));
   unsigned short MaxBytesPerScanLine     __attribute__ ((packed));
   unsigned short MaxScanLineWidth        __attribute__ ((packed));

   /* VBE/AF 2.0 extensions */
   unsigned short LinBytesPerScanLine     __attribute__ ((packed));
   unsigned char  BnkMaxBuffers           __attribute__ ((packed));
   unsigned char  LinMaxBuffers           __attribute__ ((packed));
   unsigned char  LinRedMaskSize          __attribute__ ((packed));
   unsigned char  LinRedFieldPosition     __attribute__ ((packed));
   unsigned char  LinGreenMaskSize        __attribute__ ((packed));
   unsigned char  LinGreenFieldPosition   __attribute__ ((packed));
   unsigned char  LinBlueMaskSize         __attribute__ ((packed));
   unsigned char  LinBlueFieldPosition    __attribute__ ((packed));
   unsigned char  LinRsvdMaskSize         __attribute__ ((packed));
   unsigned char  LinRsvdFieldPosition    __attribute__ ((packed));
   unsigned long  MaxPixelClock           __attribute__ ((packed));
   unsigned long  VideoCapabilities       __attribute__ ((packed));
   unsigned short VideoMinXScale          __attribute__ ((packed));
   unsigned short VideoMinYScale          __attribute__ ((packed));
   unsigned short VideoMaxXScale          __attribute__ ((packed));
   unsigned short VideoMaxYScale          __attribute__ ((packed));

   unsigned char  reserved[76]            __attribute__ ((packed));

} AF_MODE_INFO;



#define DC  struct AF_DRIVER *dc



/* main VBE/AF driver structure */
typedef struct AF_DRIVER 
{
   /* header */
   char           Signature[12]           __attribute__ ((packed));
   unsigned long  Version                 __attribute__ ((packed));
   unsigned long  DriverRev               __attribute__ ((packed));
   char           OemVendorName[80]       __attribute__ ((packed));
   char           OemCopyright[80]        __attribute__ ((packed));
   short          *AvailableModes         __attribute__ ((packed));
   unsigned long  TotalMemory             __attribute__ ((packed));
   unsigned long  Attributes              __attribute__ ((packed));
   unsigned long  BankSize                __attribute__ ((packed));
   unsigned long  BankedBasePtr           __attribute__ ((packed));
   unsigned long  LinearSize              __attribute__ ((packed));
   unsigned long  LinearBasePtr           __attribute__ ((packed));
   unsigned long  LinearGranularity       __attribute__ ((packed));
   unsigned short *IOPortsTable           __attribute__ ((packed));
   unsigned long  IOMemoryBase[4]         __attribute__ ((packed));
   unsigned long  IOMemoryLen[4]          __attribute__ ((packed));
   unsigned long  LinearStridePad         __attribute__ ((packed));
   unsigned short PCIVendorID             __attribute__ ((packed));
   unsigned short PCIDeviceID             __attribute__ ((packed));
   unsigned short PCISubSysVendorID       __attribute__ ((packed));
   unsigned short PCISubSysID             __attribute__ ((packed));
   unsigned long  Checksum                __attribute__ ((packed));
   unsigned long  res2[6]                 __attribute__ ((packed));

   /* near pointers mapped by the application */
   void           *IOMemMaps[4]           __attribute__ ((packed));
   void           *BankedMem              __attribute__ ((packed));
   void           *LinearMem              __attribute__ ((packed));
   unsigned long  res3[5]                 __attribute__ ((packed));

   /* driver state variables */
   unsigned long  BufferEndX              __attribute__ ((packed));
   unsigned long  BufferEndY              __attribute__ ((packed));
   unsigned long  OriginOffset            __attribute__ ((packed));
   unsigned long  OffscreenOffset         __attribute__ ((packed));
   unsigned long  OffscreenStartY         __attribute__ ((packed));
   unsigned long  OffscreenEndY           __attribute__ ((packed));
   unsigned long  res4[10]                __attribute__ ((packed));

   /* relocatable 32 bit bank switch routine, for Windows (ugh!) */
   unsigned long  SetBank32Len            __attribute__ ((packed));
   void           *SetBank32              __attribute__ ((packed));

   /* callback functions provided by the application */
   void           *Int86                  __attribute__ ((packed));
   void           *CallRealMode           __attribute__ ((packed));

   /* main driver setup routine */
   void           *InitDriver             __attribute__ ((packed));

   /* VBE/AF 1.0 asm interface (obsolete and not supported by Allegro) */
   void           *af10Funcs[40]          __attribute__ ((packed));

   /* VBE/AF 2.0 extensions */
   void           *PlugAndPlayInit        __attribute__ ((packed));

   /* extension query function, specific to FreeBE/AF */
   void           *(*OemExt)(DC, unsigned long id);

   /* extension hook for implementing additional VESA interfaces */
   void           *SupplementalExt        __attribute__ ((packed));

   /* device driver functions */
   long  (*GetVideoModeInfo)(DC, short mode, AF_MODE_INFO *modeInfo);
   long  (*SetVideoMode)(DC, short mode, long virtualX, long virtualY, long *bytesPerLine, int numBuffers, AF_CRTCInfo *crtc);
   void  (*RestoreTextMode)(DC);
   long  (*GetClosestPixelClock)(DC, short mode, unsigned long pixelClock);
   void  (*SaveRestoreState)(DC, int subfunc, void *saveBuf);
   void  (*SetDisplayStart)(DC, long x, long y, long waitVRT);
   void  (*SetActiveBuffer)(DC, long index);
   void  (*SetVisibleBuffer)(DC, long index, long waitVRT);
   int   (*GetDisplayStartStatus)(DC);
   void  (*EnableStereoMode)(DC, int enable);
   void  (*SetPaletteData)(DC, AF_PALETTE *pal, long num, long index, long waitVRT);
   void  (*SetGammaCorrectData)(DC, AF_PALETTE *pal, long num, long index);
   void  (*SetBank)(DC, long bank);

   /* hardware cursor functions */
   void  (*SetCursor)(DC, AF_CURSOR *cursor);
   void  (*SetCursorPos)(DC, long x, long y);
   void  (*SetCursorColor)(DC, unsigned char red, unsigned char green, unsigned char blue);
   void  (*ShowCursor)(DC, long visible);

   /* 2D rendering functions */
   void  (*WaitTillIdle)(DC);
   void  (*EnableDirectAccess)(DC);
   void  (*DisableDirectAccess)(DC);
   void  (*SetMix)(DC, long foreMix, long backMix);
   void  (*Set8x8MonoPattern)(DC, unsigned char *pattern);
   void  (*Set8x8ColorPattern)(DC, int index, unsigned long *pattern);
   void  (*Use8x8ColorPattern)(DC, int index);
   void  (*SetLineStipple)(DC, unsigned short stipple);
   void  (*SetLineStippleCount)(DC, unsigned long count);
   void  (*SetClipRect)(DC, long minx, long miny, long maxx, long maxy);
   void  (*DrawScan)(DC, long color, long y, long x1, long x2);
   void  (*DrawPattScan)(DC, long foreColor, long backColor, long y, long x1, long x2);
   void  (*DrawColorPattScan)(DC, long y, long x1, long x2);
   void  (*DrawScanList)(DC, unsigned long color, long y, long length, short *scans);
   void  (*DrawPattScanList)(DC, unsigned long foreColor, unsigned long backColor, long y, long length, short *scans);
   void  (*DrawColorPattScanList)(DC, long y, long length, short *scans);
   void  (*DrawRect)(DC, unsigned long color, long left, long top, long width, long height);
   void  (*DrawPattRect)(DC, unsigned long foreColor, unsigned long backColor, long left, long top, long width, long height);
   void  (*DrawColorPattRect)(DC, long left, long top, long width, long height);
   void  (*DrawLine)(DC, unsigned long color, fixed x1, fixed y1, fixed x2, fixed y2);
   void  (*DrawStippleLine)(DC, unsigned long foreColor, unsigned long backColor, fixed x1, fixed y1, fixed x2, fixed y2);
   void  (*DrawTrap)(DC, unsigned long color, AF_TRAP *trap);
   void  (*DrawTri)(DC, unsigned long color, AF_FIX_POINT *v1, AF_FIX_POINT *v2, AF_FIX_POINT *v3, fixed xOffset, fixed yOffset);
   void  (*DrawQuad)(DC, unsigned long color, AF_FIX_POINT *v1, AF_FIX_POINT *v2, AF_FIX_POINT *v3, AF_FIX_POINT *v4, fixed xOffset, fixed yOffset);
   void  (*PutMonoImage)(DC, long foreColor, long backColor, long dstX, long dstY, long byteWidth, long srcX, long srcY, long width, long height, unsigned char *image);
   void  (*PutMonoImageLin)(DC, long foreColor, long backColor, long dstX, long dstY, long byteWidth, long srcX, long srcY, long width, long height, long imageOfs);
   void  (*PutMonoImageBM)(DC, long foreColor, long backColor, long dstX, long dstY, long byteWidth, long srcX, long srcY, long width, long height, long imagePhysAddr);
   void  (*BitBlt)(DC, long left, long top, long width, long height, long dstLeft, long dstTop, long op);
   void  (*BitBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);
   void  (*BitBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);
   void  (*BitBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);
   void  (*SrcTransBlt)(DC, long left, long top, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*SrcTransBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*SrcTransBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*SrcTransBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBlt)(DC, long left, long top, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*DstTransBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op, unsigned long transparent);
   void  (*StretchBlt)(DC, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*StretchBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*StretchBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*StretchBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op);
   void  (*SrcTransStretchBlt)(DC, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*SrcTransStretchBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*SrcTransStretchBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*SrcTransStretchBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBlt)(DC, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBltSys)(DC, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBltLin)(DC, long srcOfs, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);
   void  (*DstTransStretchBltBM)(DC, long srcPhysAddr, long srcPitch, long srcLeft, long srcTop, long srcWidth, long srcHeight, long dstLeft, long dstTop, long dstWidth, long dstHeight, long flags, long op, unsigned long transparent);

   /* hardware video functions */
   void  (*SetVideoInput)(DC, long width, long height, long format);
   void *(*SetVideoOutput)(DC, long left, long top, long width, long height);
   void  (*StartVideoFrame)(DC);
   void  (*EndVideoFrame)(DC);

} AF_DRIVER;



#undef DC



/* register data for calling real mode interrupts (DPMI format) */
typedef union 
{
   struct {
      unsigned long edi;
      unsigned long esi;
      unsigned long ebp;
      unsigned long res;
      unsigned long ebx;
      unsigned long edx;
      unsigned long ecx;
      unsigned long eax;
   } d;
   struct {
      unsigned short di, di_hi;
      unsigned short si, si_hi;
      unsigned short bp, bp_hi;
      unsigned short res, res_hi;
      unsigned short bx, bx_hi;
      unsigned short dx, dx_hi;
      unsigned short cx, cx_hi;
      unsigned short ax, ax_hi;
      unsigned short flags;
      unsigned short es;
      unsigned short ds;
      unsigned short fs;
      unsigned short gs;
      unsigned short ip;
      unsigned short cs;
      unsigned short sp;
      unsigned short ss;
   } x;
   struct {
      unsigned char edi[4];
      unsigned char esi[4];
      unsigned char ebp[4];
      unsigned char res[4];
      unsigned char bl, bh, ebx_b2, ebx_b3;
      unsigned char dl, dh, edx_b2, edx_b3;
      unsigned char cl, ch, ecx_b2, ecx_b3;
      unsigned char al, ah, eax_b2, eax_b3;
   } h;
} RM_REGS;



/* our API extensions use 32 bit magic numbers */
#define FAF_ID(a,b,c,d)    ((a<<24) | (b<<16) | (c<<8) | d)



/* ID code and magic return value for initialising the extensions */
#define FAFEXT_INIT     FAF_ID('I','N','I','T')
#define FAFEXT_MAGIC    FAF_ID('E','X', 0,  0)
#define FAFEXT_MAGIC1   FAF_ID('E','X','0','1')



/* extension providing a hardware-specific way to access video memory */
#define FAFEXT_HWPTR    FAF_ID('H','P','T','R')


#if (defined __i386__) && (!defined NO_HWPTR)


/* use seg+offset far pointers on i386 */
typedef struct FAF_HWPTR
{
   int sel;
   unsigned long offset;
} FAF_HWPTR;

#include <sys/farptr.h>
#include <sys/segments.h>

#define hwptr_init(ptr, addr)                \
   if ((addr) && (!(ptr).sel)) {             \
      (ptr).sel = _my_ds();                  \
      (ptr).offset = (unsigned long)(addr);  \
   }

#define hwptr_pokeb(ptr, off, val)     _farpokeb((ptr).sel, (ptr).offset+(off), (val))
#define hwptr_pokew(ptr, off, val)     _farpokew((ptr).sel, (ptr).offset+(off), (val))
#define hwptr_pokel(ptr, off, val)     _farpokel((ptr).sel, (ptr).offset+(off), (val))

#define hwptr_peekb(ptr, off)          _farpeekb((ptr).sel, (ptr).offset+(off))
#define hwptr_peekw(ptr, off)          _farpeekw((ptr).sel, (ptr).offset+(off))
#define hwptr_peekl(ptr, off)          _farpeekl((ptr).sel, (ptr).offset+(off))

#define hwptr_select(ptr)              _farsetsel((ptr).sel)
#define hwptr_unselect(ptr)            (ptr).sel = _fargetsel()

#define hwptr_nspokeb(ptr, off, val)   _farnspokeb((ptr).offset+(off), (val))
#define hwptr_nspokew(ptr, off, val)   _farnspokew((ptr).offset+(off), (val))
#define hwptr_nspokel(ptr, off, val)   _farnspokel((ptr).offset+(off), (val))

#define hwptr_nspeekb(ptr, off)        _farnspeekb((ptr).offset+(off))
#define hwptr_nspeekw(ptr, off)        _farnspeekw((ptr).offset+(off))
#define hwptr_nspeekl(ptr, off)        _farnspeekl((ptr).offset+(off))


#else


/* use regular C pointers on other platforms or if hwptr is disabled */
typedef void *FAF_HWPTR;

#define hwptr_init(ptr, addr)          ptr = (FAF_HWPTR)(addr)

#define hwptr_pokeb(ptr, off, val)     *((volatile unsigned char *)((ptr)+(off))) = (val)
#define hwptr_pokew(ptr, off, val)     *((volatile unsigned short *)((ptr)+(off))) = (val)
#define hwptr_pokel(ptr, off, val)     *((volatile unsigned long *)((ptr)+(off))) = (val)

#define hwptr_peekb(ptr, off)          (*((volatile unsigned char *)((ptr)+(off))))
#define hwptr_peekw(ptr, off)          (*((volatile unsigned short *)((ptr)+(off))))
#define hwptr_peekl(ptr, off)          (*((volatile unsigned long *)((ptr)+(off))))

#define hwptr_select(ptr)
#define hwptr_unselect(ptr)            (ptr) = NULL

#define hwptr_nspokeb(ptr, off, val)   *((volatile unsigned char *)((ptr)+(off))) = (val)
#define hwptr_nspokew(ptr, off, val)   *((volatile unsigned short *)((ptr)+(off))) = (val)
#define hwptr_nspokel(ptr, off, val)   *((volatile unsigned long *)((ptr)+(off))) = (val)

#define hwptr_nspeekb(ptr, off)        (*((volatile unsigned char *)((ptr)+(off))))
#define hwptr_nspeekw(ptr, off)        (*((volatile unsigned short *)((ptr)+(off))))
#define hwptr_nspeekl(ptr, off)        (*((volatile unsigned long *)((ptr)+(off))))


#endif      /* hwptr structure definitions */


/* interface structure containing hardware pointer data */
typedef struct FAF_HWPTR_DATA
{
   FAF_HWPTR IOMemMaps[4];
   FAF_HWPTR BankedMem;
   FAF_HWPTR LinearMem;
} FAF_HWPTR_DATA;



/* extension providing a way for the config program to set driver variables */
#define FAFEXT_CONFIG   FAF_ID('C','O','N','F')



/* config variable, so the install program can communicate with the driver */
typedef struct FAF_CONFIG_DATA
{
   unsigned long id;
   unsigned long value;
} FAF_CONFIG_DATA;



/* config variable ID used to enable/disable specific hardware functions */
#define FAF_CFG_FEATURES    FAF_ID('F','E','A','T')



/* bitfield values for the FAF_CFG_FEATURES variable */
#define fafLinear                      0x00000001
#define fafBanked                      0x00000002
#define fafHWCursor                    0x00000004
#define fafDrawScan                    0x00000008
#define fafDrawPattScan                0x00000010
#define fafDrawColorPattScan           0x00000020
#define fafDrawScanList                0x00000040
#define fafDrawPattScanList            0x00000080
#define fafDrawColorPattScanList       0x00000100
#define fafDrawRect                    0x00000200
#define fafDrawPattRect                0x00000400
#define fafDrawColorPattRect           0x00000800
#define fafDrawLine                    0x00001000
#define fafDrawStippleLine             0x00002000
#define fafDrawTrap                    0x00004000
#define fafDrawTri                     0x00008000
#define fafDrawQuad                    0x00010000
#define fafPutMonoImage                0x00020000
#define fafPutMonoImageLin             0x00040000
#define fafPutMonoImageBM              0x00080000
#define fafBitBlt                      0x00100000
#define fafBitBltSys                   0x00200000
#define fafBitBltLin                   0x00400000
#define fafBitBltBM                    0x00800000
#define fafSrcTransBlt                 0x01000000
#define fafSrcTransBltSys              0x02000000
#define fafSrcTransBltLin              0x04000000
#define fafSrcTransBltBM               0x08000000



/* helper function for enabling/disabling driver routines */
void fixup_feature_list(AF_DRIVER *af, unsigned long flags);



/* extension providing libc exports (needed for Nucleus compatibility) */
#define FAFEXT_LIBC     FAF_ID('L','I','B','C')


typedef struct FAF_LIBC_DATA
{
   long  size;
   void  (*abort)();
   void *(*calloc)(unsigned long num_elements, unsigned long size);
   void  (*exit)(int status);
   void  (*free)(void *ptr);
   char *(*getenv)(const char *name);
   void *(*malloc)(unsigned long size);
   void *(*realloc)(void *ptr, unsigned long size);
   int   (*system)(const char *s);
   int   (*putenv)(const char *val);
   int   (*open)(const char *file, int mode, int permissions);
   int   (*access)(const char *filename, int flags);
   int   (*close)(int fd);
   int   (*lseek)(int fd, int offset, int whence);
   int   (*read)(int fd, void *buffer, unsigned long count);
   int   (*unlink)(const char *file);
   int   (*write)(int fd, const void *buffer, unsigned long count);
   int   (*isatty)(int fd);
   int   (*remove)(const char *file);
   int   (*rename)(const char *oldname, const char *newname);
   unsigned int (*time)(unsigned int *t);
   void  (*setfileattr)(const char *filename, unsigned attrib);
   unsigned long (*getcurrentdate)();
} FAF_LIBC_DATA;



/* extension providing pmode exports (needed for Nucleus compatibility) */
#define FAFEXT_PMODE    FAF_ID('P','M','O','D')



/* It has to be said, having this many exported functions is truly insane.
 * How on earth can SciTech need this much crap just to write a simple
 * video driver? Unfortunately we have to include it all in order to 
 * support their Nucleus drivers...
 */

typedef union
{
   struct {
      unsigned long eax, ebx, ecx, edx, esi, edi, cflag;
   } e;
   struct {
      unsigned short ax, ax_hi;
      unsigned short bx, bx_hi;
      unsigned short cx, cx_hi;
      unsigned short dx, dx_hi;
      unsigned short si, si_hi;
      unsigned short di, di_hi;
      unsigned short cflag, cflag_hi;
   } x;
   struct {
      unsigned char   al, ah; unsigned short ax_hi;
      unsigned char   bl, bh; unsigned short bx_hi;
      unsigned char   cl, ch; unsigned short cx_hi;
      unsigned char   dl, dh; unsigned short dx_hi;
   } h;
} SILLY_SCITECH_REGS;



typedef struct
{
   unsigned short es, cs, ss, ds, fs, gs;
} SILLY_SCITECH_SREGS;



typedef struct FAF_PMODE_DATA
{
   long  size;
   int   (*getModeType)();
   void *(*getBIOSPointer)();
   void *(*getA0000Pointer)();
   void *(*mapPhysicalAddr)(unsigned long base, unsigned long limit);
   void *(*mallocShared)(long size);
   int   (*mapShared)(void *ptr);
   void  (*freeShared)(void *ptr);
   void *(*mapToProcess)(void *linear, unsigned long limit);
   void  (*loadDS)();
   void  (*saveDS)();
   void *(*mapRealPointer)(unsigned int r_seg, unsigned int r_off);
   void *(*allocRealSeg)(unsigned int size, unsigned int *r_seg, unsigned int *r_off);
   void  (*freeRealSeg)(void *mem);
   void *(*allocLockedMem)(unsigned int size, unsigned long *physAddr);
   void  (*freeLockedMem)(void *p);
   void  (*callRealMode)(unsigned int seg, unsigned int off, SILLY_SCITECH_REGS *regs, SILLY_SCITECH_SREGS *sregs);
   int   (*int86)(int intno, SILLY_SCITECH_REGS *in, SILLY_SCITECH_REGS *out);
   int   (*int86x)(int intno, SILLY_SCITECH_REGS *in, SILLY_SCITECH_REGS *out, SILLY_SCITECH_SREGS *sregs);
   void  (*DPMI_int86)(int intno, RM_REGS *regs);
   void  (*segread)(SILLY_SCITECH_SREGS *sregs);
   int   (*int386)(int intno, SILLY_SCITECH_REGS *in, SILLY_SCITECH_REGS *out);
   int   (*int386x)(int intno, SILLY_SCITECH_REGS *in, SILLY_SCITECH_REGS *out, SILLY_SCITECH_SREGS *sregs);
   void  (*availableMemory)(unsigned long *physical, unsigned long *total);
   void *(*getVESABuf)(unsigned int *len, unsigned int *rseg, unsigned int *roff);
   long  (*getOSType)();
   void  (*fatalError)(const char *msg);
   void  (*setBankA)(int bank);
   void  (*setBankAB)(int bank);
   const char *(*getCurrentPath)();
   const char *(*getVBEAFPath)();
   const char *(*getNucleusPath)();
   const char *(*getNucleusConfigPath)();
   const char *(*getUniqueID)();
   const char *(*getMachineName)();
   int   (*VF_available)();
   void *(*VF_init)(unsigned long baseAddr, int bankSize, int codeLen, void *bankFunc);
   void  (*VF_exit)();
   int   (*kbhit)();
   int   (*getch)();
   int   (*openConsole)();
   int   (*getConsoleStateSize)();
   void  (*saveConsoleState)(void *stateBuf, int console_id);
   void  (*restoreConsoleState)(const void *stateBuf, int console_id);
   void  (*closeConsole)(int console_id);
   void  (*setOSCursorLocation)(int x, int y);
   void  (*setOSScreenWidth)(int width, int height);
   int   (*enableWriteCombine)(unsigned long base, unsigned long length);
   void  (*backslash)(char *filename);
} FAF_PMODE_DATA;



/* assorted helper functions */
void trace_putc(char c);
void trace_printf(char *msg, ...);

void rm_int(int num, RM_REGS *regs);

int allocate_dos_memory(int size, int *sel);
void free_dos_memory(int sel);

int allocate_selector(int addr, int size);
void free_selector(int sel);

int get_vesa_info(int *vram_size, unsigned long *linear_addr, void (*callback)(int vesa_num, int linear, int w, int h, int bpp, int bytes_per_scanline, int redsize, int redpos, int greensize, int greenpos, int bluesize, int bluepos, int rsvdsize, int rsvdpos));



/* read_vga_register:
 *  Reads the contents of a VGA hardware register.
 */
extern inline int read_vga_register(int port, int index)
{
   if (port==0x3C0)
      inportb(0x3DA); 

   outportb(port, index);
   return inportb(port+1);
}



/* write_vga_register:
 *  Writes a byte to a VGA hardware register.
 */
extern inline void write_vga_register(int port, int index, int v) 
{
   if (port==0x3C0) {
      inportb(0x3DA);
      outportb(port, index);
      outportb(port, v);
   }
   else {
      outportb(port, index);
      outportb(port+1, v);
   }
}



/* alter_vga_register:
 *  Alters specific bits of a VGA hardware register.
 */
extern inline void alter_vga_register(int port, int index, int mask, int v)
{
   int temp;
   temp = read_vga_register(port, index);
   temp &= (~mask);
   temp |= (v & mask);
   write_vga_register(port, index, temp);
}



/* test_vga_register:
 *  Tests whether specific bits of a VGA hardware register can be changed.
 */
extern inline int test_vga_register(int port, int index, int mask)
{
   int old, nw1, nw2;

   old = read_vga_register(port, index);
   write_vga_register(port, index, old & (~mask));
   nw1 = read_vga_register(port, index) & mask;
   write_vga_register(port, index, old | mask);
   nw2 = read_vga_register(port, index) & mask;
   write_vga_register(port, index, old);

   return ((nw1==0) && (nw2==mask));
}



/* test_register:
 *  Tests whether specific bits of a hardware register can be changed.
 */
extern inline int test_register(int port, int mask)
{
   int old, nw1, nw2;

   old = inportb(port);
   outportb(port, old & (~mask));
   nw1 = inportb(port) & mask;
   outportb(port, old | mask);
   nw2 = inportb(port) & mask;
   outportb(port, old);

   return ((nw1==0) && (nw2==mask));
}



/* PCI routines added by SET */
#define PCIConfigurationAddress  0xCF8
#define PCIConfigurationData     0xCFC
#define PCIEnable                0x80000000

extern int FindPCIDevice(int deviceID, int vendorID, int deviceIndex, int *handle);

extern inline unsigned PCIReadLong(int handle, int index)
{
   outportl(PCIConfigurationAddress, PCIEnable | handle | index);
   return inportl(PCIConfigurationData);
}
