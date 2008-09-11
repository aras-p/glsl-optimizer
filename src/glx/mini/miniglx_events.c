/**
 * \file miniglx_events.c
 * \brief Mini GLX client/server communication functions.
 * \author Keith Whitwell
 *
 * The Mini GLX interface is a subset of the GLX interface, plus a
 * minimal set of Xlib functions.  This file adds interfaces to
 * arbitrate a single cliprect between multiple direct rendering
 * clients.
 *
 * A fairly complete client/server non-blocking communication
 * mechanism.  Probably overkill given that none of our messages
 * currently exceed 1 byte in length and take place over the
 * relatively benign channel provided by a Unix domain socket.
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.0
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <linux/kd.h>
#include <linux/vt.h>

#include "xf86drm.h"
#include "miniglxP.h"


#define MINIGLX_FIFO_NAME "/tmp/miniglx.fifo"

/**
 * \brief Allocate an XEvent structure on the event queue.
 *
 * \param dpy the display handle.
 *
 * \return Pointer to the queued event structure or NULL on failure.
 * 
 * \internal 
 * If there is space on the XEvent queue, return a pointer
 * to the next free event and increment the eventqueue tail value.
 * Otherwise return null.
 */
static XEvent *queue_event( Display *dpy )
{
   int incr = (dpy->eventqueue.tail + 1) & MINIGLX_EVENT_QUEUE_MASK;
   if (incr == dpy->eventqueue.head) {
      return 0;
   }
   else {
      XEvent *ev = &dpy->eventqueue.queue[dpy->eventqueue.tail];
      dpy->eventqueue.tail = incr;
      return ev;
   }
}

/**
 * \brief Dequeue an XEvent and copy it into provided storage.
 *
 * \param dpy the display handle.
 * \param event_return pointer to copy the queued event to.
 *
 * \return True or False depending on success.
 * 
 * \internal 
 * If there is a queued XEvent on the queue, copy it to the provided
 * pointer and increment the eventqueue head value.  Otherwise return
 * null.
 */
static int dequeue_event( Display *dpy, XEvent *event_return )
{
   if (dpy->eventqueue.tail == dpy->eventqueue.head) {
      return False;
   }
   else {
      *event_return = dpy->eventqueue.queue[dpy->eventqueue.head];      
      dpy->eventqueue.head += 1;
      dpy->eventqueue.head &= MINIGLX_EVENT_QUEUE_MASK;
      return True;
   }
}

/**
 * \brief Shutdown a socket connection.
 *
 * \param dpy the display handle.
 * \param i the index in dpy->fd of the socket connection.
 *
 * \internal 
 * Shutdown and close the file descriptor.  If this is the special
 * connection in fd[0], issue an error message and exit - there's been
 * some sort of failure somewhere.  Otherwise, let the application
 * know about whats happened by issuing a DestroyNotify event.
 */
static void shut_fd( Display *dpy, int i )
{
   if (dpy->fd[i].fd < 0) 
      return;

   shutdown (dpy->fd[i].fd, SHUT_RDWR);
   close (dpy->fd[i].fd);
   dpy->fd[i].fd = -1;
   dpy->fd[i].readbuf_count = 0;
   dpy->fd[i].writebuf_count = 0;

   if (i == 0) {
      fprintf(stderr, "server connection lost\n");
      exit(1);
   }
   else {
      /* Pass this to the application as a DestroyNotify event.
       */
      XEvent *er = queue_event(dpy);
      if (!er) return;
      er->xdestroywindow.type = DestroyNotify;
      er->xdestroywindow.serial = 0;
      er->xdestroywindow.send_event = 0;
      er->xdestroywindow.display = dpy;
      er->xdestroywindow.window = (Window)i;

      drmGetLock(dpy->driverContext.drmFD, 1, 0);
      drmUnlock(dpy->driverContext.drmFD, 1);
   }
}

/**
 * \brief Send a message to a socket connection.
 *
 * \param dpy the display handle.
 * \param i the index in dpy->fd of the socket connection.
 * \param msg the message to send.
 * \param sz the size of the message
 *
 * \internal 
 * Copy the message to the write buffer for the nominated connection.
 * This will be actually sent to that file descriptor from
 * __miniglx_Select().
 */
int send_msg( Display *dpy, int i,
		     const void *msg, size_t sz )
{
   int cnt = dpy->fd[i].writebuf_count;
   if (MINIGLX_BUF_SIZE - cnt < sz) {
      fprintf(stderr, "client %d: writebuf overflow\n", i);
      return False;
   }
   
   memcpy( dpy->fd[i].writebuf + cnt, msg, sz ); cnt += sz;
   dpy->fd[i].writebuf_count = cnt;
   return True;
}

/**
 * \brief Send a message to a socket connection.
 *
 * \param dpy the display handle.
 * \param i the index in dpy->fd of the socket connection.
 * \param msg the message to send.
 *
 * \internal 
 * Use send_msg() to send a one-byte message to a socket.
 */
int send_char_msg( Display *dpy, int i, char msg )
{
   return send_msg( dpy, i, &msg, sizeof(char));
}


/**
 * \brief Block and receive a message from a socket connection.
 *
 * \param dpy the display handle.
 * \param connection the index in dpy->fd of the socket connection.
 * \param msg storage for the received message.
 * \param msg_size the number of bytes to read.
 *
 * \internal 
 * Block and read from the connection's file descriptor
 * until msg_size bytes have been received.  
 *
 * Only called from welcome_message_part().
 */
int blocking_read( Display *dpy, int connection, 
			  char *msg, size_t msg_size )
{
   int i, r;

   for (i = 0 ; i < msg_size ; i += r) {
      r = read(dpy->fd[connection].fd, msg + i, msg_size - i);
      if (r < 1) {
	 fprintf(stderr, "blocking_read: %d %s\n", r, strerror(errno));
	 shut_fd(dpy,connection);
	 return False;
      }
   }

   return True;
}

/**
 * \brief Send/receive a part of the welcome message.
 *
 * \param dpy the display handle.
 * \param i the index in dpy->fd of the socket connection.
 * \param msg storage for the sent/received message.
 * \param sz the number of bytes to write/read.
 *
 * \return True on success, or False on failure.
 *
 * This function is called by welcome_message_part(), to either send or receive
 * (via blocking_read()) part of the welcome message, according to whether
 * Display::IsClient is set.
 *
 * Each part of the welcome message on the wire consists of a count and then the
 * actual message data with that number of bytes.
 */
static int welcome_message_part( Display *dpy, int i, void **msg, int sz )
{
   if (dpy->IsClient) {
      int sz;
      if (!blocking_read( dpy, i, (char *)&sz, sizeof(sz))) return False;
      if (!*msg) *msg = malloc(sz);
      if (!*msg) return False;
      if (!blocking_read( dpy, i, *msg, sz )) return False;
      return sz;
   }
   else {
      if (!send_msg( dpy, i, &sz, sizeof(sz))) return False;
      if (!send_msg( dpy, i, *msg, sz )) return False;
   }

   return True;
}

/**
 * \brief Send/receive the welcome message.
 *
 * \param dpy the display handle.
 * \param i the index in dpy->fd of the socket connection.
 *
 * \return True on success, or False on failure.
 *
 * Using welcome_message_part(), sends/receives the client ID, the client
 * configuration details in DRIDriverContext::shared, and the driver private
 * message in DRIDriverContext::driverClientMsg.
 */
static int welcome_message( Display *dpy, int i )
{
   void *tmp = &dpy->driverContext.shared;
   int *clientid = dpy->IsClient ? &dpy->clientID : &i;
   int size;

   if (!welcome_message_part( dpy, i, (void **)&clientid, sizeof(*clientid)))
      return False;

   if (!welcome_message_part( dpy, i, &tmp, sizeof(dpy->driverContext.shared)))
      return False;
      
   size=welcome_message_part( dpy, i,
                              (void **)&dpy->driverContext.driverClientMsg, 
			      dpy->driverContext.driverClientMsgSize );

   if (!size)
      return False;

   if (dpy->IsClient) {
     dpy->driverContext.driverClientMsgSize = size;
   }
   return True;
}


/**
 * \brief Handle a new client connection.
 *
 * \param dpy the display handle.
 *
 * \return True on success or False on failure.
 * 
 * Accepts the connection, sets it in non-blocking operation, and finds a free
 * slot in Display::fd for it.
 */
static int handle_new_client( Display *dpy )
{
   struct sockaddr_un client_address;
   unsigned int l = sizeof(client_address);
   int r, i;

   r = accept(dpy->fd[0].fd, (struct sockaddr *) &client_address, &l);
   if (r < 0) {
      perror ("accept()");
      shut_fd(dpy,0);
      return False;
   } 

   if (fcntl(r, F_SETFL, O_NONBLOCK) != 0) {
      perror("fcntl");
      close(r);
      return False;
   }


   /* Some rough & ready adaption of the XEvent semantics.
    */ 
   for (i = 1 ; i < dpy->nrFds ; i++) {
      if (dpy->fd[i].fd < 0) {
	 XEvent *er = queue_event(dpy);
	 if (!er) {
	    close(r);
	    return False;
	 }

	 dpy->fd[i].fd = r;
	 er->xcreatewindow.type = CreateNotify;
	 er->xcreatewindow.serial = 0;
	 er->xcreatewindow.send_event = 0;
	 er->xcreatewindow.display = dpy;
	 er->xcreatewindow.window = (Window)i;	/* fd slot == window, now? */

	 /* Send the driver client message - this is expected as the
	  * first message on a new connection.  The recpient already
	  * knows the size of the message.
	  */
	 welcome_message( dpy, i );
	 return True;
      }	    
   }


   fprintf(stderr, "[miniglx] %s: Max nr clients exceeded\n", __FUNCTION__);
   close(r);
   return False;
}

/**
 * This routine "puffs out" the very basic communications between
 * client and server to full-sized X Events that can be handled by the
 * application.
 *
 * \param dpy the display handle.
 * \param i the index in dpy->fd of the socket connection.
 *
 * \return True on success or False on failure.
 *
 * \internal
 * Interprets the message (see msg) into a XEvent and advances the file FIFO
 * buffer.
 */
static int
handle_fifo_read( Display *dpy, int i )
{
   drm_magic_t magic;
   int err;

   while (dpy->fd[i].readbuf_count) {
      char id = dpy->fd[i].readbuf[0];
      XEvent *er;
      int count = 1;

      if (dpy->IsClient) {
	 switch (id) {
	    /* The server has called XMapWindow on a client window */
	 case _YouveGotFocus:
	    er = queue_event(dpy);
	    if (!er) return False;
	    er->xmap.type = MapNotify;
	    er->xmap.serial = 0;
	    er->xmap.send_event = False;
	    er->xmap.display = dpy;
	    er->xmap.event = dpy->TheWindow;
	    er->xmap.window = dpy->TheWindow;
	    er->xmap.override_redirect = False;
	    if (dpy->driver->notifyFocus)
	       dpy->driver->notifyFocus( 1 ); 
	    break;

	    /* The server has called XMapWindow on a client window */
	 case _RepaintPlease:
	    er = queue_event(dpy);
	    if (!er) return False;
	    er->xexpose.type = Expose;
	    er->xexpose.serial = 0;
	    er->xexpose.send_event = False;
	    er->xexpose.display = dpy;
	    er->xexpose.window = dpy->TheWindow;
	    if (dpy->rotateMode) {
	       er->xexpose.x = dpy->TheWindow->y;
	       er->xexpose.y = dpy->TheWindow->x;
	       er->xexpose.width = dpy->TheWindow->h;
	       er->xexpose.height = dpy->TheWindow->w;
	    }
	    else {
	       er->xexpose.x = dpy->TheWindow->x;
	       er->xexpose.y = dpy->TheWindow->y;
	       er->xexpose.width = dpy->TheWindow->w;
	       er->xexpose.height = dpy->TheWindow->h;
	    }
	    er->xexpose.count = 0;
	    break;

	    /* The server has called 'XUnmapWindow' on a client
	     * window.
	     */
	 case _YouveLostFocus:
	    er = queue_event(dpy);
	    if (!er) return False;
	    er->xunmap.type = UnmapNotify;
	    er->xunmap.serial = 0;
	    er->xunmap.send_event = False;
	    er->xunmap.display = dpy;
	    er->xunmap.event = dpy->TheWindow;
	    er->xunmap.window = dpy->TheWindow;
	    er->xunmap.from_configure = False;
	    if (dpy->driver->notifyFocus)
	       dpy->driver->notifyFocus( 0 ); 
	    break;
            
         case _Authorize:
            dpy->authorized = True;
            break;
	 
	 default:
	    fprintf(stderr, "Client received unhandled message type %d\n", id);
	    shut_fd(dpy, i);		/* Actually shuts down the client */
	    return False;
	 }
      }
      else {
	 switch (id) {
	    /* Lets the server know that the client is ready to render
	     * (having called 'XMapWindow' locally).
	     */
	 case _CanIHaveFocus:	 
	    er = queue_event(dpy);
	    if (!er) return False;
	    er->xmaprequest.type = MapRequest;
	    er->xmaprequest.serial = 0;
	    er->xmaprequest.send_event = False;
	    er->xmaprequest.display = dpy;
	    er->xmaprequest.parent = 0;
	    er->xmaprequest.window = (Window)i;
	    break;

	    /* Both _YouveLostFocus and _IDontWantFocus generate unmap
	     * events.  The idea is that _YouveLostFocus lets the client
	     * know that it has had focus revoked by the server, whereas
	     * _IDontWantFocus lets the server know that the client has
	     * unmapped its own window.
	     */
	 case _IDontWantFocus:
	    er = queue_event(dpy);
	    if (!er) return False;
	    er->xunmap.type = UnmapNotify;
	    er->xunmap.serial = 0;
	    er->xunmap.send_event = False;
	    er->xunmap.display = dpy;
	    er->xunmap.event = (Window)i;
	    er->xunmap.window = (Window)i;
	    er->xunmap.from_configure = False;
	    break;
            
	 case _Authorize:
            /* is full message here yet? */
	    if (dpy->fd[i].readbuf_count < count + sizeof(magic)) {
               count = 0;
               break;
	    }
            memcpy(&magic, dpy->fd[i].readbuf + count, sizeof(magic));
	    fprintf(stderr, "Authorize - magic %d\n", magic);
            
            err = drmAuthMagic(dpy->driverContext.drmFD, magic);
	    count += sizeof(magic);
            
            send_char_msg( dpy, i, _Authorize );
	    break;

	 default:
	    fprintf(stderr, "Server received unhandled message type %d\n", id);
	    shut_fd(dpy, i);		/* Generates DestroyNotify event */
	    return False;
	 }
      }

      dpy->fd[i].readbuf_count -= count;

      if (dpy->fd[i].readbuf_count) {
	 memmove(dpy->fd[i].readbuf,
		 dpy->fd[i].readbuf + count,
		 dpy->fd[i].readbuf_count);
      }
   }

   return True;
}

/**
 * Handle a VT signal
 *
 * \param dpy display handle.
 *
 * The VT switches is detected by comparing Display::haveVT and
 * Display::hwActive. When loosing the VT the hardware lock is acquired, the
 * hardware is shutdown via a call to DRIDriverRec::shutdownHardware(), and the
 * VT released. When acquiring the VT back the hardware state is restored via a
 * call to DRIDriverRec::restoreHardware() and the hardware lock released.
 */
static void __driHandleVtSignals( Display *dpy )
{
   dpy->vtSignalFlag = 0;

   fprintf(stderr, "%s: haveVT %d hwActive %d\n", __FUNCTION__,
	   dpy->haveVT, dpy->hwActive);

   if (!dpy->haveVT && dpy->hwActive) {
      /* Need to get lock and shutdown hardware */
      DRM_LIGHT_LOCK( dpy->driverContext.drmFD,
                      dpy->driverContext.pSAREA,
                      dpy->driverContext.serverContext ); 
      dpy->driver->shutdownHardware( &dpy->driverContext ); 

      /* Can now give up control of the VT */
      ioctl( dpy->ConsoleFD, VT_RELDISP, 1 ); 
      dpy->hwActive = 0;
   }
   else if (dpy->haveVT && !dpy->hwActive) {
      /* Get VT (wait??) */
      ioctl( dpy->ConsoleFD, VT_RELDISP, VT_ACTIVATE );

      /* restore HW state, release lock */
      dpy->driver->restoreHardware( &dpy->driverContext ); 
      DRM_UNLOCK( dpy->driverContext.drmFD,
                  dpy->driverContext.pSAREA,
                  dpy->driverContext.serverContext ); 
      dpy->hwActive = 1;
   }
}


#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

/**
 * Logic for the select() call.
 *
 * \param dpy display handle.
 * \param n highest fd in any set plus one.
 * \param rfds fd set to be watched for reading, or NULL to create one.
 * \param wfds fd set to be watched for writing, or NULL to create one.
 * \param xfds fd set to be watched for exceptions or error, or NULL to create one.
 * \param tv timeout value, or NULL for no timeout.
 * 
 * \return number of file descriptors contained in the sets, or a negative number on failure.
 * 
 * \note
 * This all looks pretty complex, but is necessary especially on the
 * server side to prevent a poorly-behaved client from causing the
 * server to block in a read or write and hence not service the other
 * clients.
 *
 * \sa
 * See select_tut in the Linux manual pages for more discussion.
 *
 * \internal
 * Creates and initializes the file descriptor sets by inspecting Display::fd
 * if these aren't passed in the function call. Calls select() and fulfill the
 * demands by trying to fill MiniGLXConnection::readbuf and draining
 * MiniGLXConnection::writebuf. 
 * The server fd[0] is handled specially for new connections, by calling
 * handle_new_client().
 * 
 */
int 
__miniglx_Select( Display *dpy, int n, fd_set *rfds, fd_set *wfds, fd_set *xfds,
		  struct timeval *tv )
{
   int i;
   int retval;
   fd_set my_rfds, my_wfds;
   struct timeval my_tv;

   if (!rfds) {
      rfds = &my_rfds;
      FD_ZERO(rfds);
   }

   if (!wfds) {
      wfds = &my_wfds;
      FD_ZERO(wfds);
   }

   /* Don't block if there are events queued.  Review this if the
    * flush in XMapWindow is changed to blocking.  (Test case:
    * miniglxtest).
    */
   if (dpy->eventqueue.head != dpy->eventqueue.tail) {
      my_tv.tv_sec = my_tv.tv_usec = 0;
      tv = &my_tv;
   }

   for (i = 0 ; i < dpy->nrFds; i++) {
      if (dpy->fd[i].fd < 0)
	 continue;
      
      if (dpy->fd[i].writebuf_count)
	 FD_SET(dpy->fd[i].fd, wfds);
	 
      if (dpy->fd[i].readbuf_count < MINIGLX_BUF_SIZE) 
	 FD_SET(dpy->fd[i].fd, rfds);
      
      n = max(n, dpy->fd[i].fd + 1);
   }

   if (dpy->vtSignalFlag) 
      __driHandleVtSignals( dpy ); 

   retval = select( n, rfds, wfds, xfds, tv );

   if (dpy->vtSignalFlag) {
      int tmp = errno;
      __driHandleVtSignals( dpy ); 
      errno = tmp;
   }

   if (retval < 0) {
      FD_ZERO(rfds);
      FD_ZERO(wfds);
      return retval;
   }

   /* Handle server fd[0] specially on the server - accept new client
    * connections.
    */
   if (!dpy->IsClient) {
      if (FD_ISSET(dpy->fd[0].fd, rfds)) {
	 FD_CLR(dpy->fd[0].fd, rfds);
	 handle_new_client( dpy );
      }
   }

   /* Otherwise, try and fill readbuffer and drain writebuffer:
    */
   for (i = 0 ; i < dpy->nrFds ; i++) {
      if (dpy->fd[i].fd < 0) 
	 continue;

      /* If there aren't any event slots left, don't examine
       * any more file events.  This will prevent lost events.
       */
      if (dpy->eventqueue.head == 
	  ((dpy->eventqueue.tail + 1) & MINIGLX_EVENT_QUEUE_MASK)) {
	 fprintf(stderr, "leaving event loop as event queue is full\n");
	 return retval;
      }

      if (FD_ISSET(dpy->fd[i].fd, wfds)) {
	 int r = write(dpy->fd[i].fd,
		       dpy->fd[i].writebuf,
		       dpy->fd[i].writebuf_count);
	 
	 if (r < 1) 
	    shut_fd(dpy,i);
	 else {
	    dpy->fd[i].writebuf_count -= r;
	    if (dpy->fd[i].writebuf_count) {
	       memmove(dpy->fd[i].writebuf,
		       dpy->fd[i].writebuf + r,
		       dpy->fd[i].writebuf_count);
	    }
	 }
      }

      if (FD_ISSET(dpy->fd[i].fd, rfds)) {
	 int r = read(dpy->fd[i].fd, 
		      dpy->fd[i].readbuf + dpy->fd[i].readbuf_count,
		      MINIGLX_BUF_SIZE - dpy->fd[i].readbuf_count);
	 
	 if (r < 1) 
	    shut_fd(dpy,i);
	 else {
	    dpy->fd[i].readbuf_count += r;
	 
	    handle_fifo_read( dpy, i );
	 }
      }
   }

   return retval;
}

/**
 * \brief Handle socket events.
 *
 * \param dpy the display handle.
 * \param nonblock whether to return immediately or wait for an event.
 *
 * \return True on success, False on failure. Aborts on critical error.
 *
 * \internal
 * This function is the select() main loop.
 */
int handle_fd_events( Display *dpy, int nonblock )
{
   while (1) {
      struct timeval tv = {0, 0};
      int r = __miniglx_Select( dpy, 0, 0, 0, 0, nonblock ? &tv : 0 );
      if (r >= 0) 
	 return True;
      if (errno == EINTR || errno == EAGAIN)
	 continue;
      perror ("select()");
      exit (1);
   }
}

/**
 * Initializes the connections.
 *
 * \param dpy the display handle.
 *
 * \return True on success or False on failure.
 *
 * Allocates and initializes the Display::fd array and create a Unix socket on
 * the first entry. For a server binds the socket to a filename and listen for
 * connections. For a client connects to the server and waits for a welcome
 * message. Sets the socket in nonblocking mode.
 */
int __miniglx_open_connections( Display *dpy )
{
   struct sockaddr_un sa;
   int i;

   dpy->nrFds = dpy->IsClient ? 1 : MINIGLX_MAX_SERVER_FDS;
   dpy->fd = calloc(1, dpy->nrFds * sizeof(struct MiniGLXConnection));
   if (!dpy->fd)
      return False;

   for (i = 0 ; i < dpy->nrFds ; i++)
      dpy->fd[i].fd = -1;

   if (!dpy->IsClient) {
      if (unlink(MINIGLX_FIFO_NAME) != 0 && errno != ENOENT) { 
	 perror("unlink " MINIGLX_FIFO_NAME);
 	 return False; 
      } 
      
   } 

   /* Create a Unix socket -- Note this is *not* a network connection!
    */
   dpy->fd[0].fd = socket(PF_UNIX, SOCK_STREAM, 0);
   if (dpy->fd[0].fd < 0) {
      perror("socket " MINIGLX_FIFO_NAME);
      return False;
   }

   memset(&sa, 0, sizeof(sa));
   sa.sun_family = AF_UNIX;
   strcpy(sa.sun_path, MINIGLX_FIFO_NAME);

   if (dpy->IsClient) {
      /* Connect to server
       */
      if (connect(dpy->fd[0].fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
	 perror("connect");
	 shut_fd(dpy,0);
	 return False;
      }

      /* Wait for configuration messages from the server.
       */
      welcome_message( dpy, 0 );
   }
   else {
      mode_t tmp = umask( 0000 );		/* open to everybody ? */

      /* Bind socket to our filename
       */
      if (bind(dpy->fd[0].fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
	 perror("bind");
	 shut_fd(dpy,0);
	 return False;
      }
      
      umask( tmp );

      /* Listen for connections
       */
      if (listen(dpy->fd[0].fd, 5) != 0) {
	 perror("listen");
	 shut_fd(dpy,0);
	 return False;
      }
   }

   if (fcntl(dpy->fd[0].fd, F_SETFL, O_NONBLOCK) != 0) {
      perror("fcntl");
      shut_fd(dpy,0);
      return False;
   }


   return True;
}


/**
 * Frees the connections initialized by __miniglx_open_connections().
 *
 * \param dpy the display handle.
 */
void __miniglx_close_connections( Display *dpy )
{
   int i;

   for (i = 0 ; i < dpy->nrFds ; i++) {
      if (dpy->fd[i].fd >= 0) {
	 shutdown (dpy->fd[i].fd, SHUT_RDWR);
	 close (dpy->fd[i].fd);
      }
   }

   dpy->nrFds = 0;
   free(dpy->fd);
}


/**
 * Set a drawable flag.
 *
 * \param dpy the display handle.
 * \param w drawable (window).
 * \param flag flag.
 *
 * Sets the specified drawable flag in the SAREA and increment its stamp while
 * holding the light hardware lock.
 */
static void set_drawable_flag( Display *dpy, int w, int flag )
{
   if (dpy->driverContext.pSAREA) {
      if (dpy->hwActive) 
	 DRM_LIGHT_LOCK( dpy->driverContext.drmFD,
			 dpy->driverContext.pSAREA,
			 dpy->driverContext.serverContext ); 

      dpy->driverContext.pSAREA->drawableTable[w].stamp++;
      dpy->driverContext.pSAREA->drawableTable[w].flags = flag;

      if (dpy->hwActive) 
	 DRM_UNLOCK( dpy->driverContext.drmFD,
		     dpy->driverContext.pSAREA,
		     dpy->driverContext.serverContext ); 
   }
}



/**
 * \brief Map Window.
 *
 * \param dpy the display handle as returned by XOpenDisplay().
 * \param w the window handle.
 * 
 * If called by a client, sends a request for focus to the server.  If
 * called by the server, will generate a MapNotify and Expose event at
 * the client.
 * 
 */
void
XMapWindow( Display *dpy, Window w )
{
   if (dpy->IsClient) 
      send_char_msg( dpy, 0, _CanIHaveFocus );
   else {
      set_drawable_flag( dpy, (int)w, 1 );
      send_char_msg( dpy, (int)w, _YouveGotFocus );
      send_char_msg( dpy, (int)w, _RepaintPlease );
      dpy->TheWindow = w;
   }
   handle_fd_events( dpy, 0 );	/* flush write queue */
}

/**
 * \brief Unmap Window.
 *
 * \param dpy the display handle as returned by XOpenDisplay().
 * \param w the window handle.
 * 
 * Called from the client:  Lets the server know that the window won't
 * be updated anymore.
 *
 * Called from the server:  Tells the specified client that it no longer
 * holds the focus.
 */
void
XUnmapWindow( Display *dpy, Window w )
{
   if (dpy->IsClient) {
      send_char_msg( dpy, 0, _IDontWantFocus );
   } 
   else {
      dpy->TheWindow = 0;
      set_drawable_flag( dpy, (int)w, 0 );
      send_char_msg( dpy, (int)w, _YouveLostFocus );
   }
   handle_fd_events( dpy, 0 );	/* flush write queue */
}


/**
 * \brief Block and wait for next X event.
 *
 * \param dpy the display handle as returned by XOpenDisplay().
 * \param event_return a pointer to an XEvent structure for the returned data.
 *
 * Wait until there is a new XEvent pending.
 */
int XNextEvent(Display *dpy, XEvent *event_return)
{
   for (;;) {
      if ( dpy->eventqueue.head != dpy->eventqueue.tail )
	 return dequeue_event( dpy, event_return ); 
      
      handle_fd_events( dpy, 0 );
   }
}

/**
 * \brief Non-blocking check for next X event.
 *
 * \param dpy the display handle as returned by XOpenDisplay().
 * \param event_mask ignored.
 * \param event_return a pointer to an XEvent structure for the returned data.
 *
 * Check if there is a new XEvent pending.  Note that event_mask is
 * ignored and any pending event will be returned.
 */
Bool XCheckMaskEvent(Display *dpy, long event_mask, XEvent *event_return)
{
   if ( dpy->eventqueue.head != dpy->eventqueue.tail )
      return dequeue_event( dpy, event_return ); 

   handle_fd_events( dpy, 1 );
 
   return dequeue_event( dpy, event_return ); 
}
