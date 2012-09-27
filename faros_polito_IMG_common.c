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
#include <sys/time.h>
#include <inttypes.h>

#include "faros_polito_IMG_net.h"
#include "faros_polito_IMG_common.h"


double get_curr_time_usec() {
	struct timeval timeval_now;
	struct timezone timezone_now;
	gettimeofday(&timeval_now, &timezone_now);
	double now = timeval_now.tv_sec + (double)timeval_now.tv_usec/1000000.0;
	return now;
}

void set_pck_per_frame_for_yuyv(net_par_t *net_par) {
	if (net_par->st.jpg_flag == 0) {
		int Bpf = net_par->st.width*net_par->st.height*2;  // NB: assumes YUYV
		int fragment=MAX_PAYLOAD_RTP;
		int frag_num=Bpf/fragment;
		int res=Bpf-frag_num*fragment;
		if (res==0) {
			net_par->st.pck_per_frame = frag_num;
		} else {
			net_par->st.pck_per_frame = frag_num+1;
		}
	}
}

uint32_t get_abs_clock_usec_32bit() {
	struct timeval timeval_now;
	struct timezone timezone_now;
	gettimeofday(&timeval_now, &timezone_now);
	uint32_t now_sec = (uint32_t) timeval_now.tv_sec;
	uint32_t now_usec = (uint32_t) timeval_now.tv_usec;
	uint64_t now_ll = now_sec*1000000LL+now_usec;
	uint64_t now_usec_ll32 = now_ll & 0xFFFFFFFF;
	//printf("TIME 32bit: now_sec = %d  now_usec = %06d now_ll=%"PRId64" now_usec_ll32=%"PRId64" now32=%d\n", now_sec, now_usec, now_ll, now_usec_ll32, (unsigned int)now_usec_ll32);
	return (uint32_t)now_usec_ll32;
}

