/*
   (c) Copyright 2001-2009  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH
   (c) Copyright 2011-2011  Lifia

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjälä <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.
              Andoni Zubimendi <andoni.zubimendi@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <directfb_util.h>

#include <direct/mem.h>

#include "x11.h"
#include <X11/extensions/Xrender.h>

D_DEBUG_DOMAIN( X11_Window, "X11/Window", "X11 Window handling" );

static bool use_shm = true;

static int
error_handler_shm( Display *display, XErrorEvent *event )
{
     if (use_shm) {
          D_INFO( "X11/Display: Error! Disabling XShm.\n" );

          use_shm = false;
     }

     return 0;
}


static int error_code = 0;

static int
error_handler( Display *display, XErrorEvent *event )
{
     char buf[512];

     D_DEBUG_AT( X11_Window, "%s()\n", __FUNCTION__ );

     XGetErrorText( display, event->error_code, buf, sizeof(buf) );

     D_ERROR( "X11/Window: Error! %s\n", buf );

     error_code = event->error_code;

     return 0;
}

static  int 
x11_parse_windowid_env(const char *value, unsigned long *id, int *create) {
	int create_v = 0;
	int base = 10;
	char *endptr;
	unsigned long id_r;

	if (!value || !id)
		return 0;

	if (*value == '+') {
		value++;
		create_v = 1;
	}

	if (strncasecmp(value, "0x", 2) == 0) {
		base = 16;
		value +=2;
	}

	id_r = strtoul(value, &endptr, base);
	if (*endptr != '\0')
		return 0;

        /* Only update variables if everything is OK */
	if (create)
		*create = create_v;
	*id = id_r;
	return 1;
}

static  int 
x11_parse_windowid_event(const char *value, unsigned long *id) {
	int base = 10;
	char *endptr;
	unsigned long id_r;

	if (!value || !id)
		return 0;

	if (strncasecmp(value, "0x", 2) == 0) {
		base = 16;
		value +=2;
	}

	id_r = strtoul(value, &endptr, base);
	if (*endptr != '\0')
		return 0;

	*id = id_r;
	return 1;
}

static int
dfb_x11_init_render(XWindow *xw) {
	int nvi, i;
	XVisualInfo templ;
	templ.screen = DefaultScreen(xw->display);
	templ.depth = 32;
	templ.class = TrueColor;

	XVisualInfo *xvi = XGetVisualInfo(xw->display, VisualScreenMask | VisualDepthMask| VisualClassMask, &templ, &nvi);

	int depth;
	int argbVisual = 0;
	XRenderPictFormat *fmt= 0;
	Visual *visual = 0;
	Colormap colormap = 0;

	for (i = 0;i < nvi; i++) {
		fmt=XRenderFindVisualFormat(xw->display, xvi[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = xvi[i].visual;
			colormap = XCreateColormap(xw->display, DefaultRootWindow(xw->display), visual, AllocNone);
			depth = xvi[i].depth;
			argbVisual = 1;
			break;
		}	
	}

	if (argbVisual) {
     		D_INFO( "X11/Display: XRender ARGB Visual found.\n");
		xw->render_visual = visual;
		xw->render_fmt = fmt;
		xw->render_colormap = colormap;
		xw->render_depth = depth;
	} else {
     		D_INFO( "X11/Display: XRender ARGB Visual not found.\n");
	}

	return argbVisual;
}

Bool
dfb_x11_open_window( DFBX11 *x11, XWindow** ppXW, int iXPos, int iYPos, int iWidth, int iHeight, DFBSurfacePixelFormat format )
{
     XWindow              *xw;
     XSetWindowAttributes  attr = { .background_pixmap = 0 };
     void                 *old_error_handler = 0;

     D_DEBUG_AT( X11_Window, "Creating %4dx%4d %s window...\n", iWidth, iHeight, dfb_pixelformat_name(format) );


     xw = D_CALLOC( 1, sizeof(XWindow) );

     /* We set the structure as needed for our window */
     xw->width   = iWidth;
     xw->height  = iHeight;
     xw->display = x11->display;

     if (x11->use_render) {
        if (!dfb_x11_init_render(xw)) {
            D_FREE( xw );
            return False;
        }
     }
 
     xw->screenptr = DefaultScreenOfDisplay(xw->display);
     xw->screennum = DefaultScreen(xw->display);
     if (x11->use_render) 
         xw->depth     = xw->render_depth;
     else
         xw->depth     = DefaultDepthOfScreen(xw->screenptr);
     xw->visual    = DefaultVisualOfScreen(xw->screenptr);

     attr.colormap = xw->render_colormap;
     attr.event_mask =
            ButtonPressMask
          | ButtonReleaseMask
          | PointerMotionMask
          | KeyPressMask
          | KeyReleaseMask
          | ExposureMask
          | StructureNotifyMask;

     //attr.background_pixel  = 0x80 <<24 | BlackPixelOfScreen(xw->screenptr);
     XLockDisplay( x11->display );

     old_error_handler = XSetErrorHandler( error_handler );

     error_code = 0;
     long unsigned int attr_mask = CWEventMask |  CWBackPixel |
                                   CWBorderPixel | CWColormap;
     attr.background_pixel = XBlackPixel(xw->display, x11->screennum);
     attr.border_pixel = XBlackPixel(xw->display, x11->screennum);

     char *window_id_env = getenv("DIRECTFB_WINDOWID");
     char *window_id_event = getenv("DIRECTFB_WINDOWEVENT");
     unsigned long int rootwin = RootWindowOfScreen(xw->screenptr);
     unsigned long int eventwin = 0;
     int create_window = 1;

     if (window_id_env) {
     	if (x11_parse_windowid_env(window_id_env, &rootwin, &create_window)) {
            D_INFO( "X11/Display: Using window id 0x%lx as our %swindow\n", rootwin,
	              (create_window)?"parent ":"");
  	} else {
            D_INFO( "X11/Display: Error parsing DIRECTFB_WINDOWID\n" );
	}

     }
     

     if (window_id_event) {
     	if (x11_parse_windowid_event(window_id_event, &eventwin)) {
            D_INFO( "X11/Display: Using window id 0x%lx as our event replay events,\n", rootwin);
  	} else {
            D_INFO( "X11/Display: Error parsing DIRECTFB_WINDOWEVENT\n" );
	}

     }
     x11->event_window = eventwin;

     if (create_window) {
       xw->window = XCreateWindow( xw->display,
                                   rootwin,
                                   iXPos, iYPos, iWidth, iHeight, 0, xw->depth, InputOutput,
                                   xw->render_visual, attr_mask, &attr );
     } else {
       xw->window = rootwin;
     }

     if (x11->use_render) {
         xw->render_drawable = XCreatePixmap(xw->display, xw->window, iWidth, iHeight, xw->depth);
     }
     XSync( xw->display, False );
     if (!xw->window || error_code) {
          D_FREE( xw );
          XUnlockDisplay( x11->display );
          return False;
     }

     if (create_window) {
         XSizeHints Hints;

        /*
         * Here we inform the function of what we are going to change for the
         * window (there's also PPosition but it's obsolete)
         */
         Hints.flags    =    PSize | PMinSize | PMaxSize;

         /*
          * Now we set the structure to the values we need for width & height.
          * For esthetic reasons we set Width=MinWidth=MaxWidth.
          * The same goes for Height. You can try whith differents values, or
          * let's use Hints.flags=Psize; and resize your window..
          */
          Hints.min_width          =    Hints.max_width          =    Hints.base_width    =    xw->width;
          Hints.min_height    =    Hints.max_height    =    Hints.base_height   =    xw->height;

          /* Now we can set the size hints for the specified window */
          XSetWMNormalHints(xw->display,xw->window,&Hints);

          /* We change the title of the window (default:Untitled) */
          XStoreName(xw->display,xw->window,"DFB X11 system window");
     }

     XGCValues gcvalues;
     gcvalues.function = GXcopy;
     gcvalues.subwindow_mode = IncludeInferiors;
     unsigned long gcactive = 0; //GCFunction | GCSubwindowMode;
     
    
     xw->gc = XCreateGC(xw->display, xw->window, gcactive, &gcvalues);
   
#if 0
     // Create a null cursor
     Pixmap  pixmp1;
     Pixmap  pixmp2;
     XColor  fore;
     XColor  back;
     char    zero = 0;

     pixmp1 = XCreateBitmapFromData( xw->display, xw->window, &zero, 1, 1 );
     pixmp2 = XCreateBitmapFromData( xw->display, xw->window, &zero, 1, 1 );

     xw->NullCursor = XCreatePixmapCursor( xw->display, pixmp1, pixmp2, &fore, &back, 0, 0 );

     XFreePixmap ( xw->display, pixmp1 );
     XFreePixmap ( xw->display, pixmp2 );

     XDefineCursor( xw->display, xw->window, xw->NullCursor );
#endif

     /* maps the window and raises it to the top of the stack */
     XMapRaised( xw->display, xw->window );


     if (x11->use_shm) {
          // Shared memory
          xw->shmseginfo=(XShmSegmentInfo *)D_CALLOC(1, sizeof(XShmSegmentInfo));
          if (!xw->shmseginfo) {
               x11->use_shm = false;
               goto no_shm;
          }

          xw->ximage=XShmCreateImage(xw->display, xw->visual, xw->depth, ZPixmap,
                                     NULL,xw->shmseginfo, xw->width, xw->height * 2);
          XSync( xw->display, False );
          if (!xw->ximage || error_code) {
               D_ERROR("X11: Error creating shared image (XShmCreateImage) \n");
               x11->use_shm = false;
               D_FREE(xw->shmseginfo);
               error_code = 0;
               goto no_shm;
          }

          xw->bpp = (xw->ximage->bits_per_pixel + 7) / 8;

          /* we firstly create our shared memory segment with the size we need, and
          correct permissions for the owner, the group and the world --> 0777 */
          xw->shmseginfo->shmid=shmget(IPC_PRIVATE,
                                       xw->ximage->bytes_per_line * xw->ximage->height * 2,
                                       IPC_CREAT|0777);

          if (xw->shmseginfo->shmid<0) {
               x11->use_shm = false;
               XDestroyImage(xw->ximage);
               D_FREE(xw->shmseginfo);
               goto no_shm;
          }

          /* Then, we have to attach the segment to our process, and we let the
          function search the correct memory place --> NULL. It's safest ! */
          xw->shmseginfo->shmaddr = shmat( xw->shmseginfo->shmid, NULL, 0 );
          if (!xw->shmseginfo->shmaddr) {
               x11->use_shm = false;
               shmctl(xw->shmseginfo->shmid,IPC_RMID,NULL);
               XDestroyImage(xw->ximage);
               D_FREE(xw->shmseginfo);
               goto no_shm;
          }

          /* We set the buffer in Read and Write mode */
          xw->shmseginfo->readOnly=False;

          xw->virtualscreen= xw->ximage->data = xw->shmseginfo->shmaddr;


          XSetErrorHandler( error_handler_shm );

          XShmAttach(x11->display,xw->shmseginfo);

          XShmPutImage(x11->display, xw->window, xw->gc, xw->ximage,
                       0, 0, 0, 0, 1, 1, False);

          XSync(x11->display, False);

          XSetErrorHandler( error_handler );

          if (!x11->use_shm) {
               shmdt(xw->shmseginfo->shmaddr);
               shmctl(xw->shmseginfo->shmid,IPC_RMID,NULL);
               XDestroyImage(xw->ximage);
               D_FREE(xw->shmseginfo);
          }
     }

no_shm:
     if (!x11->use_shm) {
          int pitch;

          xw->bpp = (xw->depth > 16) ? 4 :
                    (xw->depth >  8) ? 2 : 1;

          pitch = (xw->bpp * xw->width + 3) & ~3;

          /* Use malloc(), not D_MALLOC() here, because XCreateImage()
           * will call free() on this data.
           */
          xw->virtualscreen = malloc ( 2 * xw->height * pitch );

          xw->ximage = XCreateImage( xw->display, xw->visual, xw->depth, ZPixmap, 0,
                                     xw->virtualscreen, xw->width, xw->height * 2, 32, pitch );
          XSync( xw->display, False );
          if (!xw->ximage || error_code) {
               D_ERROR( "X11/Window: XCreateImage( Visual %02lu, depth %d, size %dx%d, buffer %p [%d] ) failed!\n",
                        xw->visual->visualid, xw->depth, xw->width, xw->height * 2, xw->virtualscreen, pitch );
               XFreeGC(xw->display,xw->gc);
               XDestroyWindow(xw->display,xw->window);
               XSetErrorHandler( old_error_handler );
               XUnlockDisplay( x11->display );
               D_FREE( xw );
               return False;
          }
     }

     XSetErrorHandler( old_error_handler );

     XUnlockDisplay( x11->display );

     D_INFO( "X11/Display: %ssing XShm.\n", x11->use_shm ? "U" : "Not u" );

     (*ppXW) = xw;

     XRenderPictureAttributes pict_attr;
     pict_attr.poly_edge=PolyEdgeSmooth;
     pict_attr.poly_mode=PolyModeImprecise;
     unsigned long int pict_mask = CPPolyEdge|CPPolyMode;
     xw->render_picture = XRenderCreatePicture (xw->display, xw->window, xw->render_fmt, pict_mask, &pict_attr);

     return True;
}

void
dfb_x11_close_window( DFBX11 *x11, XWindow* xw )
{
     if (x11->use_shm) {
          XShmDetach( xw->display, xw->shmseginfo );
          shmdt( xw->shmseginfo->shmaddr );
          shmctl( xw->shmseginfo->shmid, IPC_RMID, NULL );
          D_FREE( xw->shmseginfo );
}

     XDestroyImage( xw->ximage );

     XFreeGC( xw->display, xw->gc );
     XDestroyWindow( xw->display, xw->window );
#if 0
     XFreeCursor( xw->display, xw->NullCursor );
#endif

     D_FREE( xw );
}

