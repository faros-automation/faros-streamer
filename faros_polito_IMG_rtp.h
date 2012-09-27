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


#ifndef FAROS_POLITO_IMG_RTP_H
#define FAROS_POLITO_IMG_RTP_H

#define RTP_HEADER_LEN_BASE 12
#define RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG 20

#define MAX_PAYLOAD_RTP (1500-20-8-RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG)

// numero max di pck che possono arrivare in ritardo, fuori sequenza. Usato per capire se c'e' stato wrap di num.di sequenza, per calcolare correttamente l'extended sequence number
#define MAX_REORDER_PACKETS 3000

void RTP_print_header(unsigned char *p);

int RTP_increment_seqN(int n);

int RTP_get_header_len(unsigned char *p);
unsigned int RTP_get_seq_num(unsigned char *p);
unsigned int RTP_get_timestamp(unsigned char *p);
int RTP_get_payload_type(unsigned char *p);
unsigned int RTP_get_ssrc(unsigned char *p);
int RTP_get_marker(unsigned char *p);
int RTP_get_version(unsigned char *p);
int RTP_get_extension(unsigned char *p);
int RTP_get_extension_len(unsigned char *p);  // excluding extension header (4 bytes), returns a number of bytes
unsigned int RTP_get_extension_type(unsigned char *p); // FAROS_POLITO_IMG: 0xEEEE

unsigned int RTP_get_framenum(unsigned char *p);
unsigned int RTP_get_slicenum(unsigned char *p);

void RTP_init_header_v2(unsigned char *p, int len);
void RTP_init_header_v2_ext_faros_polito_IMG(unsigned char *p, int len);
void RTP_set_version(unsigned char *p, int v);
void RTP_set_extension(unsigned char *p, int v);
void RTP_set_marker(unsigned char *p, int v);
void RTP_set_seq_num(unsigned char *p, int v);
void RTP_set_timestamp(unsigned char *p, unsigned int v);
void RTP_set_payload_type(unsigned char *p, int v);
void RTP_set_ssrc(unsigned char *p, int v);

void RTP_set_extension_len(unsigned char *p, unsigned int v);  // excluding extension header (4 bytes), set the number of words (=4 bytes)
void RTP_set_extension_type(unsigned char *p, unsigned int v);
void RTP_set_framenum(unsigned char *p, unsigned int v);
void RTP_set_slicenum(unsigned char *p, unsigned int v);


#endif

