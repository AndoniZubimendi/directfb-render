#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <stdlib.h>

Display *display;
int screen;
Window root, window;
XEvent event;

int main(int argc, char *argv[])
{
	display=XOpenDisplay(NULL);
	
	Colormap colormap = 0;

	Visual *visual = 0;
	int render_event_base, render_error_base;
	int render_present=XRenderQueryExtension(display, &render_event_base, &render_error_base);
	if (!render_present) {
		fprintf(stderr, "RENDER extension missing!\n");
		abort();
	}
	
	XVisualInfo templ;
	int nvi;
	templ.screen = DefaultScreen(display);
	templ.depth = 32;
	templ.class = TrueColor;
	XVisualInfo *xvi = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask | VisualClassMask, &templ, &nvi );
	/* obtain a few parameters */

	int argbVisual = 0;
	XRenderPictFormat *fmt= 0;

	int i;
	for (i = 0;i < nvi; ++i) {
		fmt=XRenderFindVisualFormat(display, xvi[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = xvi[i].visual;
			colormap = XCreateColormap(display, DefaultRootWindow(display), visual, AllocNone);
			argbVisual = 1;
		}	
			
	}
	if (argbVisual) {
		printf("ARGB Visual found.\n");
	}

	printf("Depth: %i\n", fmt->depth);
	screen=DefaultScreen(display);
	root=DefaultRootWindow(display);
	
	/* create window and all related resources */
	int x_pos = 0;
	int y_pos = 0;
	int width = 720;
	int height = 576;
	int border_width = 0;

  XSetWindowAttributes winattr;
  winattr.background_pixel = XBlackPixel(display, screen);
  winattr.border_pixel = XBlackPixel(display, screen);
  winattr.colormap = colormap;
  long unsigned int winattr_mask = 0;
  winattr_mask |= CWBackPixel;
  winattr_mask |= CWBorderPixel;
  winattr_mask |= CWColormap;
  //winattr_mask |= CWOverrideRedirect; 

	window=XCreateWindow(display, root, x_pos, y_pos,
				 width, height, border_width,
		fmt->depth, InputOutput,
		visual, winattr_mask, &winattr);
	printf("Window Id: 0x%lx\n", window);

	XMapWindow(display, window);
	XSync(display, window);

	XSelectInput(display, window, KeyPressMask|KeyReleaseMask|ExposureMask
		|ButtonPressMask|StructureNotifyMask);
	
	char cmdGinga[4096];
	char nclApp[4096];
	strcpy(nclApp, ((argc>1)?argv[1]:"main.ncl"));
	sprintf(cmdGinga, "DIRECTFB_WINDOWID=0x%lx ginga --ncl %s", window, nclApp);
	printf("Lanzamos Ginga con el siguiente comando: %s\n", cmdGinga);
	system(cmdGinga);

	while(1) {
		XNextEvent(display, &event);
		// Process..	
	}
	
	return 0;
}
