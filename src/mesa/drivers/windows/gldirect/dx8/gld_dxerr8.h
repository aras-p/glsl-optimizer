/*==========================================================================;
 *
 *
 *  File:   dxerr8.h
 *  Content:    DirectX Error Library Include File
 *
 ****************************************************************************/

#ifndef _GLD_DXERR8_H_
#define _GLD_DXERR8_H_


#include <d3d8.h>

//
//  DXGetErrorString8
//  
//  Desc:  Converts an DirectX HRESULT to a string 
//
//  Args:  HRESULT hr   Can be any error code from
//                      DPLAY D3D8 D3DX8 DMUSIC DSOUND
//
//  Return: Converted string 
//
const char*  __stdcall DXGetErrorString8A(HRESULT hr);
const WCHAR* __stdcall DXGetErrorString8W(HRESULT hr);

#ifdef UNICODE
    #define DXGetErrorString8 DXGetErrorString8W
#else
    #define DXGetErrorString8 DXGetErrorString8A
#endif 


//
//  DXTrace
//
//  Desc:  Outputs a formatted error message to the debug stream
//
//  Args:  CHAR* strFile   The current file, typically passed in using the 
//                         __FILE__ macro.
//         DWORD dwLine    The current line number, typically passed in using the 
//                         __LINE__ macro.
//         HRESULT hr      An HRESULT that will be traced to the debug stream.
//         CHAR* strMsg    A string that will be traced to the debug stream (may be NULL)
//         BOOL bPopMsgBox If TRUE, then a message box will popup also containing the passed info.
//
//  Return: The hr that was passed in.  
//
//HRESULT __stdcall DXTraceA( char* strFile, DWORD dwLine, HRESULT hr, char* strMsg, BOOL bPopMsgBox = FALSE );
//HRESULT __stdcall DXTraceW( char* strFile, DWORD dwLine, HRESULT hr, WCHAR* strMsg, BOOL bPopMsgBox = FALSE );
HRESULT __stdcall DXTraceA( char* strFile, DWORD dwLine, HRESULT hr, char* strMsg, BOOL bPopMsgBox);
HRESULT __stdcall DXTraceW( char* strFile, DWORD dwLine, HRESULT hr, WCHAR* strMsg, BOOL bPopMsgBox);

#ifdef UNICODE
    #define DXTrace DXTraceW
#else
    #define DXTrace DXTraceA
#endif 


//
// Helper macros
//
#if defined(DEBUG) | defined(_DEBUG)
    #define DXTRACE_MSG(str)              DXTrace( __FILE__, (DWORD)__LINE__, 0, str, FALSE )
    #define DXTRACE_ERR(str,hr)           DXTrace( __FILE__, (DWORD)__LINE__, hr, str, TRUE )
    #define DXTRACE_ERR_NOMSGBOX(str,hr)  DXTrace( __FILE__, (DWORD)__LINE__, hr, str, FALSE )
#else
    #define DXTRACE_MSG(str)              (0L)
    #define DXTRACE_ERR(str,hr)           (hr)
    #define DXTRACE_ERR_NOMSGBOX(str,hr)  (hr)
#endif


#endif

