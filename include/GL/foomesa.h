/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * Example Foo/Mesa interface.  See src/ddsample.c for more info.
 */



#ifndef FOOMESA_H
#define FOOMESA_H



typedef struct foo_mesa_visual  *FooMesaVisual;

typedef struct foo_mesa_buffer  *FooMesaBuffer;

typedef struct foo_mesa_context *FooMesaContext;



#ifdef BEOS
#pragma export on
#endif


extern FooMesaVisual FooMesaChooseVisual( /* your params */ );

extern void FooMesaDestroyVisual( FooMesaVisual visual );


extern FooMesaBuffer FooMesaCreateBuffer( FooMesaVisual visual,
                                          void *your_window_id );

extern void FooMesaDestroyBuffer( FooMesaBuffer buffer );


extern FooMesaContext FooMesaCreateContext( FooMesaVisual visual,
                                            FooMesaContext sharelist );

extern void FooMesaDestroyContext( FooMesaContext context );


extern void FooMesaMakeCurrent( FooMesaContext context, FooMesaBuffer buffer );


extern void FooMesaSwapBuffers( FooMesaBuffer buffer );


/* Probably some more functions... */


#ifdef BEOS
#pragma export off
#endif

#endif
