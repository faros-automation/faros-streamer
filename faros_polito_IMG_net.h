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

#ifndef _FAROS_POLITO_IMG_NET_H

#define _FAROS_POLITO_IMG_NET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Per il pck_t
#include "faros_polito_IMG_jpg.h"

#define IP_HEADER 20
#define UDP_HEADER 8

#define MAX_PAYLOAD (1500-IP_HEADER-UDP_HEADER)

typedef struct rtp_status_s {
	unsigned int ssrc;
	unsigned int timestamp;
	unsigned int pt;
	int seqN;	 // su 16 bit, da fare attenzione quando si incrementa
	int seqN_ext;	 // extended sequence number (su 32 bit)
} rtp_status_t;

typedef struct pck_counter_s {
	int all;
	int last;
} pck_counter_t;

typedef struct sender_stat_s {
	double time_start;
	double time_last;
	pck_counter_t pck_num;
	pck_counter_t pck_bytes;
	pck_counter_t frames;
	pck_counter_t pck_max;
	pck_counter_t pck_min;
	pck_counter_t pck_avg;
	pck_counter_t pck_invalid;
	double enc_timeinterval_last;
	int absolute_clock_count;
	int absolute_clock_delay_usec;
	int absolute_clock_delay_usec_min;
	int absolute_clock_delay_usec_max;
} sender_stat_t;

typedef struct sender_status_s {
	int framenum;
	int framerate;
	int clock;

	unsigned int curr_timestamp;
	int frame_nseq_iniz;
	int frame_shown;

	unsigned char *old_pck_udp_payload;
	int old_pck_len;
	int old_pck_present_flag;

	pck_t *pcklist;
	int pck_per_frame;
	int jpg_flag;
	int jpg_quality;
	int jpg_grayscale_flag;
	int num_frame_skip;
	int absolute_clock_flag;
	int absolute_clock_usec32;
	int width;
	int height;
	int pixel_format;	
} sender_status_t;

typedef struct net_par_s {
	int sockfd;
	char *host;
	char *port;
	double plr;
	struct addrinfo *list;
	struct sockaddr_in *solvedaddr;
	struct sockaddr_in addr;
	rtp_status_t rtp;
	int pck_class;
	sender_status_t st;
	sender_stat_t stat;
} net_par_t;


#endif

