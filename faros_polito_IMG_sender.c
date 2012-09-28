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
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */

#ifndef ACTUA
#include <linux/videodev2.h>
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>
#include <signal.h>


#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include "errlib.h"
#include "sockwrap.h"
#include "faros_polito_IMG_common.h"
#include "faros_polito_IMG_net.h"
#include "faros_polito_IMG_rtp.h"
#include "faros_polito_IMG_jpg.h"

#ifndef ACTUA
#include "faros_polito_IMG_input_v4l.h"
#endif


#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define MAX_INPUT   16
#define MAX_NORM    16


char *prog_name;

static net_par_t net_par;  // necessaria dichiararla come variabile globale per avere i dati a disposizione nella callback quando e' pronto un frame

#define VERSION "1.1"


void pack_yuyv_in_raw_and_send(int width, int height, int Bpf, unsigned char *videobuffer, net_par_t *net_par, pck_t *list) {
	int k;
	int fragment=MAX_PAYLOAD_RTP;
	int frag_num=Bpf/fragment;
	int res=Bpf-frag_num*fragment;
	int header=RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG;

	unsigned char pckbuf[header+fragment];
	int usec_equispaced = ((1000000 / net_par->st.framerate) / frag_num);
	int usec_sleep = usec_equispaced / 2;

	for (k=0; k<frag_num; k++) {
		RTP_init_header_v2_ext_faros_polito_IMG(pckbuf, RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG);
		RTP_set_seq_num(pckbuf, net_par->rtp.seqN);
		RTP_set_timestamp(pckbuf, net_par->rtp.timestamp);
		RTP_set_payload_type(pckbuf, net_par->rtp.pt);
		RTP_set_marker(pckbuf, 0);
		RTP_set_ssrc(pckbuf, net_par->rtp.ssrc);
		RTP_set_framenum(pckbuf, net_par->st.framenum);
		RTP_set_slicenum(pckbuf, k);

		//RTP_print_header(pckbuf); printf("\n");
		
		memcpy(pckbuf+header, videobuffer+k*fragment, fragment);
		if (net_par->plr == 0.00 || net_par->plr < (double)rand()/(double)RAND_MAX)
			Sendto(net_par->sockfd, pckbuf, header+fragment, 0, (struct sockaddr *) &(net_par->addr), sizeof(net_par->addr));
		
		net_par->rtp.seqN = RTP_increment_seqN(net_par->rtp.seqN);

		usleep(usec_sleep);
	}
	if (res>0) {
		RTP_init_header_v2_ext_faros_polito_IMG(pckbuf, RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG);
		RTP_set_seq_num(pckbuf, net_par->rtp.seqN);
		RTP_set_timestamp(pckbuf, net_par->rtp.timestamp);
		RTP_set_payload_type(pckbuf, net_par->rtp.pt);
		RTP_set_marker(pckbuf, 1);
		RTP_set_ssrc(pckbuf, net_par->rtp.ssrc);
		RTP_set_framenum(pckbuf, net_par->st.framenum);
		RTP_set_slicenum(pckbuf, k);

		memcpy(pckbuf+header, videobuffer+k*fragment, res);
		if (net_par->plr == 0.00 || net_par->plr < (double)rand()/(double)RAND_MAX)
			Sendto(net_par->sockfd, pckbuf, header+res, 0, (struct sockaddr *) &(net_par->addr), sizeof(net_par->addr));
		net_par->rtp.seqN = RTP_increment_seqN(net_par->rtp.seqN);
		//usleep(usec_sleep);
	}
}


void pack_frame_in_jpg_and_send(int width, int height, int Bpf, unsigned char *videobuffer, net_par_t *net_par, pck_t *list) {
	int k;
	int fragment=MAX_PAYLOAD_RTP;
	int frag_num=Bpf/fragment;
	//int res=Bpf-frag_num*fragment;
	//int header=RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG;

	// format = 0 YUYV
	// format = 1 RGB (e.g. kinect)

	int format = 0;
#ifdef KINECT
	format = 1;
#endif

	create_jpg_packets_from_buf(net_par->st.jpg_quality, net_par->st.pck_per_frame, videobuffer, list, RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG, net_par->st.jpg_grayscale_flag, net_par->st.width, net_par->st.height, format);

	//unsigned char pckbuf[header+fragment];
	int usec_equispaced = ((1000000 / net_par->st.framerate) / frag_num);
	int usec_sleep = usec_equispaced / 2;

	if (net_par->st.absolute_clock_flag) {
		net_par->rtp.timestamp = net_par->st.absolute_clock_usec32;
	}

	for (k=0; k<net_par->st.pck_per_frame; k++) {
		unsigned char *pckbuf = list[k].buf;

		RTP_init_header_v2_ext_faros_polito_IMG(pckbuf, RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG);
		RTP_set_seq_num(pckbuf, net_par->rtp.seqN);
		RTP_set_timestamp(pckbuf, net_par->rtp.timestamp);
		RTP_set_payload_type(pckbuf, net_par->rtp.pt);
		if (k == net_par->st.pck_per_frame-1) {
			RTP_set_marker(pckbuf, 1);
		} else {	
			RTP_set_marker(pckbuf, 0);
		}
		RTP_set_ssrc(pckbuf, net_par->rtp.ssrc);
		RTP_set_framenum(pckbuf, net_par->st.framenum);
		RTP_set_slicenum(pckbuf, k);  // FIXME dovrebbe essere il MB num...

		//RTP_print_header(pckbuf); printf("\n");
		
		int size = list[k].size;
		if (list[k].valid==0) {
			// Send a max packet with 0x00 at beginning, send it anyway to let receiver know, e.g., about end of frame
			// 0x00 not a valid value as start of JPG file
			pckbuf[RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG]=0x00;
		}
		//printf("framenum=%d list[k].size=%d\n", net_par->st.framenum, list[k].size);
		if (net_par->plr == 0.00 || net_par->plr < (double)rand()/(double)RAND_MAX) {
			// list[k].size gia' include RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG
			Sendto(net_par->sockfd, pckbuf, size, 0, (struct sockaddr *) &(net_par->addr), sizeof(net_par->addr));
		}
		
		net_par->rtp.seqN = RTP_increment_seqN(net_par->rtp.seqN);
		if (usec_sleep>0)
			usleep(usec_sleep);
	}
}


void update_stat(net_par_t *net_par, pck_t *list, double time_before_enc) {
	int pck_num = net_par->st.pck_per_frame;
	sender_stat_t *s = &(net_par->stat);

	double now = get_curr_time_usec();

	if (s->time_start == 0) {
		s->time_start = now;
		s->time_last = now;
		s->pck_min.all = 1000000;
		s->pck_min.last = 1000000;
		s->enc_timeinterval_last = 0;
	}

	int i;
	int tot_bytes=0;
	for (i=0; i<pck_num; i++) {
		int pckbytes;
		pckbytes = list[i].size+IP_HEADER+UDP_HEADER;
		tot_bytes += pckbytes;
		if (pckbytes > s->pck_max.all) { s->pck_max.all=pckbytes; }
		if (pckbytes > s->pck_max.last) { s->pck_max.last=pckbytes; }
		if (pckbytes < s->pck_min.all) { s->pck_min.all=pckbytes; }
		if (pckbytes < s->pck_min.last) { s->pck_min.last=pckbytes; }
		if (list[i].valid==0) {
			s->pck_invalid.all += 1;
			s->pck_invalid.last += 1;
		}
	}
	s->pck_num.all += pck_num;
	s->pck_num.last += pck_num;
	s->pck_bytes.all += tot_bytes;
	s->pck_bytes.last += tot_bytes;
	s->frames.all += 1;
	s->frames.last += 1;
	s->pck_avg.all = (s->pck_bytes.all / s->pck_num.all);
	s->pck_avg.last = (s->pck_bytes.last / s->pck_num.last);
	s->enc_timeinterval_last += (now - time_before_enc);  // tempo di codifica in YUYV o in JPG
	
	double elapsed_from_last_time = now - s->time_last;
	if (elapsed_from_last_time >= 1.00) {
		double elapsed_time = now - s->time_start;
		// bytes/s includes IP and UDP header
		printf("%9.3f STAT: pck/s %4d  kbit/s %9.3f  min %4d  max %4d  avg %4d  fps %d  f-enc(ms) %7.3f toobig %3d\n",
			elapsed_time, s->pck_num.last, (s->pck_bytes.last*8.0/1000.0), s->pck_min.last, s->pck_max.last, s->pck_avg.last, s->frames.last,  s->enc_timeinterval_last*1000.0/(double)s->frames.last, s->pck_invalid.last);
		s->pck_num.last = 0;
		s->pck_bytes.last = 0;
		s->frames.last = 0;
		s->pck_max.last = 0;
		s->pck_min.last = 1000000;
		s->pck_avg.last = 0;
		s->pck_invalid.last = 0;
		s->time_last = now;
		s->enc_timeinterval_last = 0;
	}
}


void save_ppm(char *bname, int w, int h, uint8_t *rgb, int grey) {
	//printf("NOT saving ppm\n"); return;

	struct timeval timeval_now;
	gettimeofday(&timeval_now, NULL);
	double now = timeval_now.tv_sec + (double)timeval_now.tv_usec/1000000.0;

	FILE *fp;
	char nam[1000];
	sprintf(nam,"%s_%17.6f_.ppm",bname,now);
	fp = fopen(nam, "wb");	
	if (fp==NULL) {
		printf("ERROR: cannot open file %s\n", nam); exit(1);
	}
	char buf[100];
	sprintf(buf,"P6\n%d %d %d\n",w,h,255);
	fwrite(buf, strlen(buf), 1, fp);
	unsigned char line[10000*3];
	int j,i;
	if (grey) {
		for (j=0; j<h; j++) {
			for (i=0; i<w; i++) {
				line[i*3+0]=rgb[j*w+i];
				line[i*3+1]=rgb[j*w+i];
				line[i*3+2]=rgb[j*w+i];
			}
			fwrite(line, w*3, 1, fp);
		}
	} else {
		for (j=0; j<h; j++) {
			for (i=0; i<w; i++) {
				line[i*3+0]=rgb[j*w*3+i*3+0];
				line[i*3+1]=rgb[j*w*3+i*3+1];
				line[i*3+2]=rgb[j*w*3+i*3+2];
			}
			fwrite(line, w*3, 1, fp);
		}
	}
	fclose(fp);	
}


//process frames from ACTUA camera
int read_frame_actua (void *actua_frame, int w, int h, int size, void *buf2) {

	int out_cnt = 0;
	int px_cnt = 0;
	
	uint32_t b;
	const int shift = 0;
	const int bits_shift = 2;
	int idx;

	for (idx=0; idx<size/4; idx++) {
		b = ((uint32_t *)actua_frame)[idx];
		uint8_t b1 = (uint8_t)(((b >> (20+shift)) >> bits_shift) & 0x000000FF);
		uint8_t b2 = (uint8_t)(((b >> (10+shift)) >> bits_shift) & 0x000000FF);
		uint8_t b3 = (uint8_t)(((b >> ( 0+shift)) >> bits_shift) & 0x000000FF);
		((uint8_t *)buf2)[out_cnt*2]=b3;
		out_cnt++;
		((uint8_t *)buf2)[out_cnt*2]=b2;
		out_cnt++;
		((uint8_t *)buf2)[out_cnt*2]=b1;
		out_cnt++;
	}


	net_par_t *net_par_p = &net_par;  // trasforma la variabile globale in puntatore per comodita'

	unsigned int Bpf = 0;//bytes per frame

	reset_jpg_packets_len(net_par_p->st.pck_per_frame, net_par_p->st.pcklist, MAX_PAYLOAD);
	
	// FAROS_POLITO_IMG: per calcolare ritardo fino a visualizzazione
	net_par_p->st.absolute_clock_usec32 = get_abs_clock_usec_32bit();
			
	int width=net_par_p->st.width;
	int height=net_par_p->st.height;



	// TEST per verificare se quanto ricevuto dalla kinect e' ok
	//static int cnt_frames=0;
	//if (cnt_frames%15==0) {
	//	save_ppm("test", width, height, frame, 0);
	//}
	//cnt_frames++;
	

#ifdef SCALE
	unsigned int idx1, idx2;
	uint8_t *buf3 = (uint8_t *) buf2;

	assert( (w/2) == width );
	assert( (h/2) == height );

	for(idx1=0; idx1<h; idx1+=2) {
		for(idx2=0; idx2<w; idx2+=2) {
			int idx1_s = idx1/2, idx2_s = idx2/2, width_s = w/2;
			buf3[ 2 * (idx2_s + idx1_s*width_s) +0 ] = buf3[ 2 * (idx2 + idx1*w) +0 ];
			buf3[ 2 * (idx2_s + idx1_s*width_s) +1 ] = buf3[ 2 * (idx2 + idx1*w) +1 ];
		}
	}
#endif

	//print_pixel_format(net_par_p->st.pixel_format);
	Bpf = width*height*12/8; // YUYV - YUV422

	static int cnt=0;

	//fprintf(stderr,"cnt=%d\n",cnt);
	
	
	double time_before_enc = get_curr_time_usec();

	if (net_par_p->st.framenum % (net_par_p->st.num_frame_skip+1) == 0) {
		unsigned char *videobuffer = (uint8_t *)buf2;
		if (net_par_p->st.jpg_flag) {
			pack_frame_in_jpg_and_send(width, height, Bpf, videobuffer, net_par_p, net_par_p->st.pcklist);
		} else {
			pack_yuyv_in_raw_and_send(width, height, Bpf, videobuffer, net_par_p, net_par_p->st.pcklist);
		}
		update_stat(net_par_p, net_par_p->st.pcklist, time_before_enc);
	}

	net_par_p->st.framenum++;
	net_par_p->rtp.timestamp = net_par_p->rtp.timestamp + net_par_p->st.clock / net_par_p->st.framerate;

	cnt++;

	// FIXME: secondo me non serve a niente
	//if (-1 == xioctl (*fd, VIDIOC_QBUF, &buf))
	//	errno_exit ("VIDIOC_QBUF");

	return 1;
}



//process one frame. This function is a callback for the v4l routines
int read_frame  (int * fd, void *frame) {
	net_par_t *net_par_p = &net_par;  // trasforma la variabile globale in puntatore per comodita'

	unsigned int Bpf = 0;//bytes per frame

	reset_jpg_packets_len(net_par_p->st.pck_per_frame, net_par_p->st.pcklist, MAX_PAYLOAD);
	
	// FAROS_POLITO_IMG: per calcolare ritardo fino a visualizzazione
	net_par_p->st.absolute_clock_usec32 = get_abs_clock_usec_32bit();
			
	int width=net_par_p->st.width;
	int height=net_par_p->st.height;

	// TEST per verificare se quanto ricevuto dalla kinect e' ok
	//static int cnt_frames=0;
	//if (cnt_frames%15==0) {
	//	save_ppm("test", width, height, frame, 0);
	//}
	//cnt_frames++;
	
	//printf("> W:%d H:%d\n", width, height);
	// SVICIU: WE NEED TO RESCALE THE VIDEOBUFFER FROM SNAPSHOT AREA (w x h) TO VISUALIZATION AREA (width x height)
	// YUYV format 2px 8bit per component 16 bit per pixel
	// ACTUA: gray image, write only Y
	
#ifdef SCALE
	unsigned int idx1, idx2;
	uint8_t *buf3 = (uint8_t *) frame;

	for(idx1=0; idx1<height; idx1+=2) {
		for(idx2=0; idx2<width; idx2+=2) {
			int idx1_s = idx1/2, idx2_s = idx2/2, width_s = width; // /2;
			buf3[ 2 * (idx2_s + idx1_s*width_s) +0 ] = buf3[ 2 * (idx2 + idx1*width) +0 ];
			buf3[ 2 * (idx2_s + idx1_s*width_s) +1 ] = buf3[ 2 * (idx2 + idx1*width) +1 ];
		}
	}
#endif

	//print_pixel_format(net_par_p->st.pixel_format);
	Bpf = width*height*12/8; // YUYV - YUV422

	static int cnt=0;

	//fprintf(stderr,"cnt=%d\n",cnt);

	double time_before_enc = get_curr_time_usec();

	if (net_par_p->st.framenum % (net_par_p->st.num_frame_skip+1) == 0) {
		unsigned char *videobuffer = (unsigned char *)frame;
		if (net_par_p->st.jpg_flag) {
			pack_frame_in_jpg_and_send(width, height, Bpf, videobuffer, net_par_p, net_par_p->st.pcklist);
		} else {
			pack_yuyv_in_raw_and_send(width, height, Bpf, videobuffer, net_par_p, net_par_p->st.pcklist);
		}
		update_stat(net_par_p, net_par_p->st.pcklist, time_before_enc);
	}

	net_par_p->st.framenum++;
	net_par_p->rtp.timestamp = net_par_p->rtp.timestamp + net_par_p->st.clock / net_par_p->st.framerate;

	cnt++;

	// FIXME: secondo me non serve a niente
	//if (-1 == xioctl (*fd, VIDIOC_QBUF, &buf))
	//	errno_exit ("VIDIOC_QBUF");

	return 1;
}



//show the usage
static void usage (FILE *fp, int argc, char **argv) {
	fprintf (fp,
		"FarosPolitoIMGVideocommunication sender program --- version "VERSION" Copyright (C) 2012 Enrico Masala\n"
                "FarosPolitoIMGVideocommunication comes with ABSOLUTELY NO WARRANTY; for details see the enclosed LICENSE.txt file\n"
                "This is free software, and you are welcome to redistribute it under certain conditions;\n"
                "for details see the enclosed LICENSE.txt file\n"
                "\n"
                "This software is licensed under GPL v.2.0\n"
                "Please note that you are using it at your own risk\n"
		"\n"
		"Usage: %s [options]\n\n"
		"Options:\n"
		"-D | --device       name               Select device name [/dev/video0]\n"
		"-d | --device-info  name               Show device info\n"
		"-i | --input        number             Video input number \n"
		"-s | --standard     number             Video standard \n"
		"-w | --window-size <640*480>           (just an example) Video size\n"
		"-h | --help                            Print this message\n"
		"-H | --host                            Destination host\n"
		"-P | --port                            Destination port\n"
		"-L | --plr                             Packet loss rate\n"
		"-J | --jpg_pck                         Use JPG, make n pck/frame (no option->raw YUYV data)\n"
		"-Q | --quality                         JPG quality [0..100]\n"
		"-G | --grayscale                       JPG in grayscale\n"
		"-S | --skip                            number of frames to skip (1 = send half frame rate)\n"
		"-T | --absolute_clock                  1 = yes, use clock to measure delay (use NTP first on both PC)\n"
		"-C | --class                           Class to set in DS (=TOS<<2) field of IP packets\n"
		"\n",
		argv[0]);
}

//used by getopt_long to know the possible inputs
static const char short_options [] = "D:d:i:s:w:hH:P:L:J:Q:G:S:T:C:";

//long version of the previous function
static const struct option long_options [] = {
	{ "device",      required_argument,      NULL,           'D' },
	{ "device-info", required_argument,      NULL,           'd' },	
	{ "input",       required_argument,      NULL,           'i' },
	{ "standard",    required_argument,      NULL,           's' },
	{ "window-size", required_argument,      NULL,           'w' },
	{ "help",        no_argument,            NULL,           'h' },
	{ "host",        required_argument,      NULL,           'H' },
	{ "port",        required_argument,      NULL,           'P' },
	{ "plr",         required_argument,      NULL,           'L' },
	{ "jpg_pck",     required_argument,      NULL,           'J' },
	{ "quality",     required_argument,      NULL,           'Q' },
	{ "grayscale",   no_argument,            NULL,           'G' },
	{ "skip",        required_argument,      NULL,           'S' },
	{ "abs_clock",   required_argument,      NULL,           'T' },
	{ "class",       required_argument,      NULL,           'C' },
	{ 0, 0, 0, 0 }
};


#ifndef ACTUA
// Per poter premere CTRL+C e avere il gmon.out --- e' necessario che si chiami la exit() !
static void sig_chld(int signo) {
	exit(2);
}
#endif


#ifndef ACTUA
int main (int argc, char ** argv) {
#else
int main_sender (int argc, char ** argv) {
#endif

	int                 dev_standard = -1; 
	int                 dev_input = -1;
	int                 set_inp              = 0;
	int                 set_std              = 0;
	char                *dev_name            = "/dev/video0";
	int                 index;
	int                 c;

	printf("FarosPolitoIMGVideocommunication sender program --- version "VERSION" Copyright (C) 2012 Enrico Masala\n");

#ifndef ACTUA
	int                 fd                   = -1;
	// Per poter premere CTRL+C e avere il gmon.out --- e' necessario che si chiami la exit() !
	struct sigaction action;
	memset(&action, 0, sizeof (action));
	action.sa_handler = sig_chld;
	int sigact_res = sigaction(SIGINT, &action, NULL);
	if (sigact_res == -1)
		err_quit("(%s) sigaction() failed", prog_name);
#endif
	


	/* for errlib to know the program name */
	prog_name = argv[0];
	net_par.host=NULL;
	net_par.port=NULL;
	net_par.pck_class=0; // TOS or DS class
	net_par.plr=0.00;

	net_par.st.width=640;
	net_par.st.height=480;
	net_par.st.pixel_format=0;
	net_par.st.pck_per_frame=0;
	net_par.st.jpg_flag=0;
	net_par.st.jpg_quality=30;
	net_par.st.jpg_grayscale_flag=0;
	net_par.st.num_frame_skip=0;
	net_par.st.absolute_clock_flag=0;  // 1 = use timestamp for absolute clock... (sync both pc with NTP, then measure delay...)
	memset(&net_par.stat, 0, sizeof(net_par.stat));
	srand ( time(NULL) );

	net_par.st.pixel_format=5; // YUYV

	//process all the command line arguments
	for (;;) {
		c = getopt_long (argc, argv,short_options, long_options,&index);

		if (-1 == c)
			break;//no more arguments (quit from for)

		switch (c) {
			case 0: // getopt_long() flag
				break;

			case 'D':
				dev_name = optarg;
				break;
		
			case 'd':
				dev_name = optarg;
#ifndef ACTUA
				open_device (&fd,dev_name);
				printf("\n");
				printf("Device info: %s\n\n",dev_name);
				enum_inputs(&fd);
				printf("\n");
				enum_standards(&fd);
				printf("\n");
				close_device (&fd);
				exit (EXIT_SUCCESS);
#endif
				//break;
				
			case 'i':  
				dev_input = atoi(optarg);              
				set_inp=1;
				break;

			case 's':
				dev_standard = atoi(optarg);
				set_std=1;
				break;

			case 'w':
				{ int i;
					for (i=0; i<strlen(optarg); i++) {
						if(optarg[i]=='*') {
							optarg[i]=' ';
							break;
						}
					}
				}
				sscanf(optarg, "%d%d", &net_par.st.width, &net_par.st.height);
				break;

			case 'H':
				net_par.host=strdup(optarg);
				break;
			case 'P':
				net_par.port=strdup(optarg);
				break;
			case 'L':
				net_par.plr=atof(optarg);
				break;
			case 'J':
				net_par.st.pck_per_frame=atof(optarg);
				net_par.st.jpg_flag=1;
				break;
			case 'Q':
				net_par.st.jpg_quality=atoi(optarg);
				break;
			case 'G':
				net_par.st.jpg_grayscale_flag=1;
				break;
			case 'S':
				net_par.st.num_frame_skip=atoi(optarg);
				break;
			case 'T':
				net_par.st.absolute_clock_flag=1;
				break;
			case 'C':
				net_par.pck_class=atoi(optarg);
				if (net_par.pck_class<0 || net_par.pck_class>63) {
					usage (stderr, argc, argv);
					printf("\nError: class value must be >0 and <63\n\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'h':
				usage (stdout, argc, argv);
				exit (EXIT_SUCCESS);

			default:
				usage (stderr, argc, argv);
				exit (EXIT_FAILURE);
		}
	}

	set_pck_per_frame_for_yuyv(&net_par);

	if (net_par.host==NULL || net_par.port==NULL) {
		usage (stderr, argc, argv);
		fprintf(stderr,"ERROR: missing host and/or port\n\n");
		exit (EXIT_FAILURE);
	}

#ifndef ACTUA
	open_device (&fd, dev_name);
	
	//set the input if needed
	if (set_inp==1) {
		set_input(&fd, dev_input);
	}

	//set the standard if needed
	if (set_std==1)
		set_standard(&fd, dev_standard);
#endif


        Getaddrinfo(net_par.host, net_par.port, NULL, &net_par.list);
        net_par.solvedaddr = (struct sockaddr_in *)net_par.list->ai_addr;
        //solvedaddr = (struct sockaddr_in *)net_par.list->next->ai_addr;
	Print_getaddrinfo_list(net_par.list);

        /* create socket */
        net_par.sockfd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        /* specify address to bind to */
        memset(&net_par.addr, 0, sizeof(net_par.addr));
        net_par.addr.sin_family = AF_INET;
        net_par.addr.sin_port = net_par.solvedaddr->sin_port;
        net_par.addr.sin_addr.s_addr = net_par.solvedaddr->sin_addr.s_addr;
	//  Set TOS or DS field
	u_int8_t tclass = ((uint8_t)net_par.pck_class)<<2;  // 000000 00 (DS+2bit)
	Setsockopt(net_par.sockfd, IPPROTO_IP, IP_TOS, &tclass, sizeof(tclass));

	printf("host=%s port=%s TOSclass=%d\n", net_par.host, net_par.port, net_par.pck_class);

	net_par.st.framenum=0;
	net_par.st.framerate=15;
	net_par.st.clock=90000;  // 90 KHz

	net_par.rtp.seqN = 0;
	net_par.rtp.timestamp = 0;
	net_par.rtp.pt = 0x7C;
	net_par.rtp.ssrc = 0xABCD0123;

#ifndef ACTUA
	print_pixel_format(net_par.st.pixel_format);

	init_device (&fd, dev_name, net_par.st.width, net_par.st.height, net_par.st.pixel_format);

	set_frame_callback(&fd, read_frame);

	start_capturing (&fd);
#endif

	net_par.st.pcklist = (pck_t *)malloc(sizeof(pck_t)*net_par.st.pck_per_frame);
	if (net_par.st.pcklist == NULL) { printf("ERROR: cannot allocate pcklist\n"); exit(1); }

	init_jpg_packets(net_par.st.pck_per_frame, net_par.st.pcklist, MAX_PAYLOAD);

#ifndef ACTUA
	mainloop_capturing (&fd);

	//TODO: main loop never exits, a break method must be implemented to execute the following code
	
	free_jpg_packets(net_par.st.pck_per_frame, net_par.st.pcklist);
	free(net_par.st.pcklist);

	stop_capturing (&fd);
	uninit_device ();
	close_device (&fd);

	exit (EXIT_SUCCESS);
#else
	return 0;
#endif


}

#ifdef ACTUA
int main(int argc, char **argv) {
	main_sender(argc, argv);

	FILE *fp=fopen("pippo000.raw","rb");
	void *frame_actua=malloc(1747628);	
	int v = fread(frame_actua, 1747624, 1, fp);
	if (v!=1) {printf("ERROR: cannot read file\n"); exit(1); }
	fclose(fp);
	void *buf2=malloc(1280*1024*2);
	
	while (1) { 
		//read_frame_actua(frame_actua, 1747624, buf2);
		read_frame_actua(frame_actua, 1280, 1024, 1747620, buf2);
	}
}
#endif



