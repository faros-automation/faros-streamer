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
#include <string.h>
#include "faros_polito_IMG_rtp.h"

void RTP_print_header(unsigned char *p) {
	int i;
	for (i=0; i<RTP_get_header_len(p); i++) {
		printf("%02x ", p[i]);
	}
	printf(" seqN=%d timestamp=%d marker=%d", RTP_get_seq_num(p), RTP_get_timestamp(p), RTP_get_marker(p));
}

int RTP_increment_seqN(int n) {
	n++;
	if (n > 65535) {
		n = n - 65536;
	}
	return n;
}
		

int RTP_get_header_len(unsigned char *p) {
	int ext = RTP_get_extension(p);
	if (ext == 0) {
		return RTP_HEADER_LEN_BASE;
	} else {
		int extlen = RTP_get_extension_len(p);
		return ( RTP_HEADER_LEN_BASE + 4 + extlen );
	}
}

int RTP_get_extension_len(unsigned char *p) {
	return 4 * ( (((unsigned int)p[14])<<8) + (unsigned int)p[15] );
}

unsigned int RTP_get_extension_type(unsigned char *p) {
	return (((unsigned int)p[12])<<8) + (unsigned int)p[13];
}

unsigned int RTP_get_framenum(unsigned char *p) {
	return (((unsigned int)p[16])<<8) + (unsigned int)p[17];
}

unsigned int RTP_get_slicenum(unsigned char *p) {
	return (((unsigned int)p[18])<<8) + (unsigned int)p[19];
}

unsigned int RTP_get_seq_num(unsigned char *p) {
	return (((unsigned int)p[2])<<8) + (unsigned int)p[3];
}

unsigned int RTP_get_timestamp(unsigned char *p) {
	return (((unsigned int)p[4])<<24) + (((unsigned int)p[5])<<16) + (((unsigned int)p[6])<<8) + (unsigned int)p[7];
}

int RTP_get_payload_type(unsigned char *p) {
	return (int)(p[2] & 0x7f);	
}

unsigned int RTP_get_ssrc(unsigned char *p) {
	return (((unsigned int)p[8])<<24) + (((unsigned int)p[9])<<16) + (((unsigned int)p[10])<<8) + (unsigned int)p[11];
}

int RTP_get_marker(unsigned char *p) {
	return ((unsigned int)p[1])>>7;
}

int RTP_get_version(unsigned char *p) {
	return ((unsigned int)p[0])>>6;
}

int RTP_get_extension(unsigned char *p) {
	return ( (unsigned int) ( p[0]>>4 & 0x01 ) );
}

void RTP_init_header_v2(unsigned char *p, int len) {
	memset(p, 0, len);
	RTP_set_version(p, 0x2);
}

void RTP_init_header_v2_ext_faros_polito_IMG(unsigned char *p, int len) {
	memset(p, 0, len);
	RTP_set_version(p, 0x2);
	RTP_set_extension(p, 0x1);
	RTP_set_extension_type(p, 0xEEEE);
	RTP_set_extension_len(p, 1);  // 1 = n.of words = 4 bytes
}

void RTP_set_version(unsigned char *p, int v) {
	p[0] = ( p[0] & 0x3f ) | (((unsigned char)v)<<6 );
}

void RTP_set_extension(unsigned char *p, int v) {
	p[0] = ( p[0] & 0xef ) | (((unsigned char)v)<<4 );
}

void RTP_set_marker(unsigned char *p, int v) {
	p[1] = ( p[1] & 0x7f ) | (((unsigned char)v)<<7 );
}

void RTP_set_seq_num(unsigned char *p, int v) {
	p[2] = (unsigned char)((unsigned int)(0x00ff & v>>8));
	p[3] = (unsigned char)(0xff & v);
}

void RTP_set_timestamp(unsigned char *p, unsigned int v) {
	p[4] = (unsigned char)((unsigned int)(0x000000ff & v>>24));
	p[5] = (unsigned char)((unsigned int)(0x000000ff & v>>16));
	p[6] = (unsigned char)((unsigned int)(0x000000ff & v>>8));
	p[7] = (unsigned char)(0x000000ff & v);
}

void RTP_set_payload_type(unsigned char *p, int v) {
	p[1] = ( p[1] & 0x80 ) | (unsigned char)(v & 0x0000007f);
}

void RTP_set_ssrc(unsigned char *p, int v) {
	p[8] = (unsigned char)((unsigned int)(0xff & v>>24));
	p[9] = (unsigned char)((unsigned int)(0xff & v>>16));
	p[10] = (unsigned char)((unsigned int)(0xff & v>>8));
	p[11] = (unsigned char)(0xff & v);
}

void RTP_set_extension_len(unsigned char *p, unsigned int v) {
	p[14] = (unsigned char)((unsigned int)(0xff & v>>8));
	p[15] = (unsigned char)(0xff & v);
}

void RTP_set_extension_type(unsigned char *p, unsigned int v) {
	p[12] = (unsigned char)((unsigned int)(0xff & v>>8));
	p[13] = (unsigned char)(0xff & v);
}

void RTP_set_framenum(unsigned char *p, unsigned int v) {
	p[16] = (unsigned char)((unsigned int)(0xff & v>>8));
	p[17] = (unsigned char)(0xff & v);
}

void RTP_set_slicenum(unsigned char *p, unsigned int v) {
	p[18] = (unsigned char)((unsigned int)(0xff & v>>8));
	p[19] = (unsigned char)(0xff & v);
}


