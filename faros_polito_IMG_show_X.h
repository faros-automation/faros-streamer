/*
 * This file is part of FarosPolitoIMGVideocommunication
 *
 * Copyright (C) 2012  Enrico Masala
 *
 * The FarosPolitoIMGVideocommunication is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FarosPolitoIMGVideocommunication is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * This software has been developed by Enrico Masala while at the Internet Media Group (IMG) (http://media.polito.it)
 * in the context of Faros Project (http://www.faros-automation.org), funded by the Regione Piemonte
 * The project lasts from mid 2010 to mid 2012.
 *
 * Author: Enrico Masala   < masala _at-symbol_ polito dot it >
 * Date: Feb 1, 2012
 *
 * Please cite the original author if you extend or modify this program
 *
 */


#ifndef __FAROS_POLITO_IMG_SHOW_X_
#define __FAROS_POLITO_IMG_SHOW_X_

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include <X11/Xutil.h>

typedef struct 
{
	Display         *display;
	GC              gc;
	Visual          *visual;
	int             depth;
	Bool            isShared; // MITSHM
	XImage          *xImage;
	Pixmap          sharedPixmap; // None (0L) if unassigned
	XShmSegmentInfo shmInfo;

	XEvent               *event;
	Window               window;
	int	                 screenNumber;
	Atom                 atomWMDeleteWindow;
	Screen               *screen;
	Colormap             colormap;
	XWindowAttributes    windowAttributes;

} image_context;


int image_create(image_context * img_ctx, int width, int height, Bool wantShared, Bool wantSharedPixmap);
int image_put (image_context img_ctx, int srcX, int srcY, int dstX, int dstY, int width, int height);
int image_destroy(image_context * img_ctx);
int image_width(image_context img_ctx);
int image_height(image_context img_ctx);
char *ByteOrderName(int byteOrder);
int image_init_window (image_context *img_ctx, int width, int height);

#endif

