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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "faros_polito_IMG_show_X.h"

char *ByteOrderName(int byteOrder) {
	switch (byteOrder) {
		case LSBFirst: return ("LSBFirst");
		case MSBFirst: return ("MSBFirst");
		default:       return ("?");
	} 
}

int image_create(image_context * img_ctx, int width, int height, Bool wantShared, Bool wantSharedPixmap) {
	//int                majorVersion;
	//int                minorVersion;
	//Bool               sharedPixmapsSupported;
	XGCValues          gcValues;
	ulong              gcValuesMask;
	XWindowAttributes  windowAttributes;

	if (img_ctx->xImage != NULL) 
	{
		image_destroy(img_ctx);
	}

	gcValues.function = GXcopy;
	gcValuesMask = GCFunction;
	img_ctx->gc = XCreateGC(img_ctx->display, img_ctx->window, gcValuesMask, &gcValues);

	XGetWindowAttributes(img_ctx->display, img_ctx->window, &windowAttributes);

	img_ctx->visual = windowAttributes.visual;
	img_ctx->depth = windowAttributes.depth;

	if (wantShared && XShmQueryExtension(img_ctx->display)) {
		img_ctx->isShared = 1;
	} else {
		img_ctx->isShared = 0;
	}

	errno = 0;
	img_ctx->xImage = NULL;
	img_ctx->sharedPixmap = None;
	if (img_ctx->isShared) {
		img_ctx->shmInfo.shmid = -1;
		img_ctx->shmInfo.shmaddr = NULL;
		if ((img_ctx->xImage = XShmCreateImage(img_ctx->display, img_ctx->visual, img_ctx->depth, ZPixmap, NULL, &(img_ctx->shmInfo), width, height)) == NULL) {
			return -1;
		}
		if ((img_ctx->shmInfo.shmid = shmget(IPC_PRIVATE, img_ctx->xImage->bytes_per_line * img_ctx->xImage->height, IPC_CREAT | 0777)) < 0) { // Create segment
			return -1;
		}
		if ((img_ctx->shmInfo.shmaddr = (char *) shmat(img_ctx->shmInfo.shmid, 0, 0)) < 0) {  // We attach img_ctx->shmInfo.shmaddr = NULL;
			return -1;
		}
		img_ctx->xImage->data = img_ctx->shmInfo.shmaddr;
		img_ctx->shmInfo.readOnly = False;
		if (!XShmAttach(img_ctx->display, &(img_ctx->shmInfo))) { // X attaches
			return -1;
		}
		if (wantSharedPixmap && (XShmPixmapFormat(img_ctx->display) == ZPixmap)) {
			if ((img_ctx->sharedPixmap = XShmCreatePixmap(img_ctx->display, img_ctx->window, img_ctx->shmInfo.shmaddr, &(img_ctx->shmInfo), width, height, img_ctx->depth)) == None) {
				return -1;
			}
		}
	} else {
		if ((img_ctx->xImage = XCreateImage(img_ctx->display, img_ctx->visual, img_ctx->depth, ZPixmap, 0, NULL, width, height, 16, 0)) == NULL) {
			return -1;
		}

		img_ctx->xImage->data = (char *) malloc(img_ctx->xImage->bytes_per_line * img_ctx->xImage->height);

		if (img_ctx->xImage->data == NULL) {
			return -1;
		}
	}
	return 0;
}

int image_destroy(image_context * img_ctx) {
	if (img_ctx->xImage == NULL) return (0); // Nothing to do

	if (img_ctx->isShared) {
		if (img_ctx->shmInfo.shmid >= 0) {
			XShmDetach(img_ctx->display, &(img_ctx->shmInfo)); // X detaches
			shmdt(img_ctx->shmInfo.shmaddr); // We detach
			img_ctx->shmInfo.shmaddr = NULL;
			shmctl(img_ctx->shmInfo.shmid, IPC_RMID, 0); // Destroy segment
			img_ctx->shmInfo.shmid = -1;
		}
	} else {
		if (img_ctx->xImage->data != NULL) {
			free(img_ctx->xImage->data);
		}
	}

	img_ctx->xImage->data = NULL;

	XDestroyImage(img_ctx->xImage);

	img_ctx->xImage = NULL;

	if (img_ctx->sharedPixmap != None) {
		XFreePixmap(img_ctx->display, img_ctx->sharedPixmap);
		img_ctx->sharedPixmap = None;
	}

	if (img_ctx->display != NULL) {
		XFreeGC(img_ctx->display, img_ctx->gc);
		img_ctx->display = NULL;
	}

	return 0;
}

int image_put (image_context img_ctx, int srcX, int srcY, int dstX, int dstY, int width, int height) {

	if (img_ctx.xImage == NULL) return (-1);

	if (width < 0) width = image_width(img_ctx);
	if (height < 0) height = image_height(img_ctx);

	if (img_ctx.isShared) {
		XShmPutImage(img_ctx.display, img_ctx.window, img_ctx.gc, img_ctx.xImage, srcX, srcY, dstX, dstY, width, height, False);
	} else {
		XPutImage(img_ctx.display, img_ctx.window, img_ctx.gc, img_ctx.xImage, srcX, srcY, dstX, dstY, width, height);
	}

	return 0;
}

int image_width(image_context img_ctx)  {
	return ((img_ctx.xImage != NULL) ? img_ctx.xImage->width : 0);
}

int image_height(image_context img_ctx) {
	return ((img_ctx.xImage != NULL) ? img_ctx.xImage->height : 0);
}

int image_init_window (image_context *img_ctx, int width, int height) {

	//****************** configure window and display  ****************************
	//try to open the display
	if ((img_ctx->display = XOpenDisplay(NULL)) == NULL) { 
		printf("Error: fail XOpenDisplay() \n");
		return -1;
	}

	//get default display number
	img_ctx->screenNumber = DefaultScreen(img_ctx->display);
	//associate screen with the default display
	img_ctx->screen = XScreenOfDisplay(img_ctx->display, img_ctx->screenNumber);

	//create the window
	img_ctx->window = XCreateSimpleWindow(
			img_ctx->display,
			RootWindowOfScreen(img_ctx->screen),
			0, // x
			0, // y
			width, // width
			height, // height
			0,                          // border width
			BlackPixelOfScreen(img_ctx->screen), // border
			BlackPixelOfScreen(img_ctx->screen)  // background
			);

	img_ctx->xImage = NULL;//xImage is not allocated yet

	if (image_create(img_ctx, width, height, True, False) < 0) {
		printf("Error: image_create() failed\n");
		exit(1);
	}

	XMapRaised(img_ctx->display, img_ctx->window);

	XStoreName(img_ctx->display, img_ctx->window, "Viewer");

	XGetWindowAttributes(img_ctx->display, img_ctx->window, &(img_ctx->windowAttributes));
	/******************* Show info about display, window and image ************************/

	fprintf(stderr,"\nDisplay:\n");
	fprintf(stderr,"Image byte order = %s\n", ByteOrderName(ImageByteOrder(img_ctx->display)));
	fprintf(stderr,"Bitmap unit      = %i\n", BitmapUnit(img_ctx->display));
	fprintf(stderr,"Bitmap bit order = %s\n", ByteOrderName(BitmapBitOrder(img_ctx->display)));
	fprintf(stderr,"Bitmap pad       = %i\n", BitmapPad(img_ctx->display));

	fprintf(stderr,"\nWindow:\n");
	fprintf(stderr,"Depth            = %i\n", img_ctx->windowAttributes.depth);
	//fprintf(stderr,"Visual ID        = 0x%02x\n", img_ctx->windowAttributes.visual->visualid);
	//fprintf(stderr,"Visual class     = %s\n", 
	//				VisualClassName(img_ctx->windowAttributes.visual->c_class));
	fprintf(stderr,"Red mask         = 0x%08lx\n", img_ctx->windowAttributes.visual->red_mask);
	fprintf(stderr,"Green mask       = 0x%08lx\n", img_ctx->windowAttributes.visual->green_mask);
	fprintf(stderr,"Blue mask        = 0x%08lx\n", img_ctx->windowAttributes.visual->blue_mask);
	fprintf(stderr,"Bits per R/G/B   = %i\n", img_ctx->windowAttributes.visual->bits_per_rgb);

	fprintf(stderr,"Image byte order = %s\n", ByteOrderName((img_ctx->xImage)->byte_order));
	fprintf(stderr,"Bitmap unit      = %i\n", img_ctx->xImage->bitmap_unit);
	fprintf(stderr,"Bitmap bit order = %s\n", ByteOrderName(img_ctx->xImage->bitmap_bit_order));
	fprintf(stderr,"Bitmap pad       = %i\n", img_ctx->xImage->bitmap_pad);
	fprintf(stderr,"Depth            = %i\n", img_ctx->xImage->depth);
	fprintf(stderr,"Red mask         = 0x%08lx\n", img_ctx->xImage->red_mask);
	fprintf(stderr,"Green mask       = 0x%08lx\n", img_ctx->xImage->green_mask);
	fprintf(stderr,"Blue mask        = 0x%08lx\n", img_ctx->xImage->blue_mask);
	fprintf(stderr,"Bits per pixel   = %i\n", img_ctx->xImage->bits_per_pixel);
	fprintf(stderr,"Bytes per line   = %i\n", img_ctx->xImage->bytes_per_line);
	fprintf(stderr,"IsShared         = %s\n", img_ctx->isShared ? "True" : "False");
	//fprintf(stderr,"HasSharedPixmap  = %s\n", img_ctx->HasSharedPixmap() ? "True" : "False");    

	/***************************************************************************/

	return 0;
}

