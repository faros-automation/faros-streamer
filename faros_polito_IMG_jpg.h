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


#ifndef __FAROS_POLITO_IMG_JPG_H
#define __FAROS_POLITO_IMG_JPG_H

#define FALSE 0
#define TRUE 1

#define MAX_OUT_JPG_BUF (1024*1024)

typedef struct pck_s {
	unsigned char *buf;
	int size;
	int valid; // if >1500, size=1500 but data is truncated, so valid=0; caller function decides what to do
} pck_t;

void init_jpg_packets(int n_pck, pck_t *list, int max_size);
void free_jpg_packets(int n_pck, pck_t *list);
void reset_jpg_packets_len(int n_pck, pck_t *list, int max_size);

void create_jpg_packets(int quality, int n_pck, unsigned char *yuyv, pck_t *list, int header_size, int flag_grayscale, int width, int height);
void create_jpg_packets_from_buf(int quality, int n_pck, unsigned char *videobuf, pck_t *list, int header_size, int flag_grayscale, int width, int height, int isRGB);
void decode_jpg_packet_rgb(unsigned char *yuyv, unsigned char *jpg_data, int jpg_data_len, int image_width, int *);


#endif

