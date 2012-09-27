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


#ifndef __FAROS_POLITO_IMG_INPUT_V4L_H_
#define __FAROS_POLITO_IMG_INPUT_V4L_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include <errno.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define MAX_INPUT   16
#define MAX_NORM    16


//info needed to store one video frame in memory
typedef struct buffer_s {    
	void *start;
	size_t length;
} buffer_t;

typedef enum {      
	PIX_FMT_YUV420P,
	PIX_FMT_RGB565,
	PIX_FMT_RGB32,
	PIX_FMT_RGB3,
	PIX_FMT_YU12,
	PIX_FMT_YUYV,
} pix_fmt;

typedef int (*frame_callback)(int *fd, void *img);

void errno_exit (const char *s);

//a blocking wrapper of the ioctl function
int xioctl (int fd, int request, void *arg);

void print_pixel_format(int pixel_format);

void set_frame_callback(int *fd, frame_callback fc);

void mainloop_capturing(int *fd);

void stop_capturing (int * fd);

void start_capturing (int * fd);

void check_mmap_buf(int *fd);

//free the shared memory area
void uninit_device();

//alloc buffers and configure the shared memory area
void init_mmap (int * fd, char * dev_name, int * n_buffers);

//configure and initialize the hardware device 
void init_device (int * fd, char * dev_name, int width, int height, int pixel_format);

void close_device (int * fd);

void open_device (int * fd, char * dev_name);

//show the available devices
void enum_inputs (int * fd);

//show the available standards(norms) for capture 
void enum_standards (int * fd );

//configure the video input
void set_input(int * fd, int dev_input);

//configure the capture standard
void set_standard(int * fd, int dev_standard);

#endif

