#ifndef D3D_TEXT_H
#define D3D_TEXT_H


#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
#define	D3DLTEXT_BITSUSED		0xFFFFFFFF
#define	MAX_VERTICES			700  // (14*40) 14 per character, 40 characters
/*===========================================================================*/
/* Macros defines.                                                           */
/*===========================================================================*/
/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
typedef struct _d3dText_metrics
{
  float		   	fntYScale,
                    fntXScale;

  int		   	fntXSpacing,
	               fntYSpacing;

  DWORD			dwColor;
  LPDIRECT3DDEVICE3 lpD3DDevice;

} D3DFONTMETRICS, *PD3DFONTMETRICS;
/*===========================================================================*/
/* Function prototypes.                                                      */
/*===========================================================================*/
extern BOOL InitD3DText( void );
extern void d3dTextDrawCharacter( char *c, int x, int y, PD3DFONTMETRICS pfntMetrics );
extern void d3dTextDrawString( char *pszString, int x, int y, PD3DFONTMETRICS pfntMetrics );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/

#ifdef __cplusplus
}
#endif


#endif
