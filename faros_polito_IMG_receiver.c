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
#include <getopt.h>             /* getopt_long() */
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

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

#include "faros_polito_IMG_show_X.h"

char *prog_name;

const int debug = 0;

#define VERSION "1.1"

void update_stat(net_par_t *net_par, int delay_send_to_display) {
	sender_stat_t *s = &(net_par->stat);

	double now = get_curr_time_usec();

	if (s->time_start == 0) {
		s->time_start = now;
		s->time_last = now;
		s->enc_timeinterval_last = 0;
		s->absolute_clock_delay_usec_min = 2100000000;
		s->absolute_clock_delay_usec_max = 0;
	}

	s->frames.all += 1;
	s->frames.last += 1;

	net_par->stat.absolute_clock_delay_usec += delay_send_to_display;
	net_par->stat.absolute_clock_delay_usec_max = max(net_par->stat.absolute_clock_delay_usec_max, delay_send_to_display);
	net_par->stat.absolute_clock_delay_usec_min = min(net_par->stat.absolute_clock_delay_usec_min, delay_send_to_display);
	
	double elapsed_from_last_time = now - s->time_last;
	if (elapsed_from_last_time >= 1.00) {
		double elapsed_time = now - s->time_start;
		// bytes/s includes IP and UDP header
		printf("%9.3f STAT: delay_send_to_display(usec) min %6d  max %6d  avg %6d  fps %d  invalid %d\n",
			elapsed_time, s->absolute_clock_delay_usec_min, s->absolute_clock_delay_usec_max, s->absolute_clock_delay_usec/s->frames.last, s->frames.last, s->pck_invalid.last);
		s->frames.last = 0;
		s->time_last = now;
		s->absolute_clock_delay_usec = 0;
		s->absolute_clock_delay_usec_min = 2100000000;
		s->absolute_clock_delay_usec_max = 0;
		s->pck_invalid.last = 0;
	}
}


//convert a pixel from YUV to RGB 24 (3 bytes)
static void yuv_to_rgb_24(unsigned char y, unsigned char u, unsigned char v, unsigned char* r, unsigned char* g, unsigned char* b) {
	int amp=255;
	double R,G,B;
	
	//conversion equations
	//B=amp*(0.004565*y+0.000001*u+0.006250*v-0.872);
	//G=amp*(0.004565*y-0.001542*u-0.003183*v+0.531);
	//R=amp*(0.004565*y+0.007935*u-1.088);
	//FAROS_POLITO_IMG: B and R inverted!!
	R=amp*(0.004565*y+0.000001*u+0.006250*v-0.872);
	G=amp*(0.004565*y-0.001542*u-0.003183*v+0.531);
	B=amp*(0.004565*y+0.007935*u-1.088);

	//R, G and B must be in the range from 0 to 255    
	if (R < 0)
		R=0;
	if (G < 0)
		G=0;
	if (B < 0)
		B=0;
	
	if (R > 255)
		R=255;
	if (G > 255)
		G=255;
	if (B > 255)
		B=255;

	*r=(unsigned char)(R);
	*g=(unsigned char)(G);
	*b=(unsigned char)(B);

}

//convert a pixel from YUV to RGB 16 (2 bytes)
static void yuv_to_rgb_16(unsigned char y, unsigned char u, unsigned char v, unsigned char* rg, unsigned char* gb) {
	double R,G,B;
	
	//conversion equations
	//B=31*(0.004565*y+0.000001*u+0.006250*v-0.872);
	//G=63*(0.004565*y-0.001542*u-0.003183*v+0.531);
	//R=31*(0.004565*y+0.007935*u-1.088);
	//FAROS_POLITO_IMG: B and R inverted!!
	R=31*(0.004565*y+0.000001*u+0.006250*v-0.872);
	G=63*(0.004565*y-0.001542*u-0.003183*v+0.531);
	B=31*(0.004565*y+0.007935*u-1.088);

	//R and B must be in the range from 0 to 31, and G in the range from 0 to 63
	if (R < 0)
		R=0;
	if (G < 0)
		G=0;
	if (B < 0)
		B=0;
	
	if (R > 31)
		R=31;
	if (G > 63)
		G=63;
	if (B > 31)
		B=31;

	*rg= (((unsigned char)(R))<<3) + (((unsigned char)(G)>>3)/*&0xe0*/);
	*gb= (((unsigned char)(G)& 0x1f)<<5) +(unsigned char)(B);

}

//read the data from memory, converts that data to RGB, and call Put (shows the picture) 
static void process_image_yuyv (uint8_t * videoFrame,  image_context image_ctx, net_par_t *net_par, int zoom)
{
	XImage * xImage1 = image_ctx.xImage;
	uint8_t * imageLine1;
	int    x,y;
	int bpl,Bpp;
	unsigned char Y1,Y2,U,V;
	unsigned char R,G,B,RG,GB;
	imageLine1 = (uint8_t *) xImage1->data;
	bpl=xImage1->bytes_per_line;
	Bpp=xImage1->bits_per_pixel/8;

	//uint8_t *imageLine = (uint8_t *)malloc(sizeof(uint8_t *)*width*height*Bpp*2);
	uint8_t *imageLine = (uint8_t *)malloc(sizeof(uint8_t *)*net_par->st.width*net_par->st.height*Bpp*3); // puo' essere YUYV o RGB
	if (!imageLine) { printf("ERROR: cannot allocate imageLine\n"); exit(1); }

	//printf("bpl=%d Bpp=%d\n", bpl, Bpp);

	//int ret;
	//ret = read(STDIN_FILENO, videoFrame , width*height*2);
	
	int Bpf=net_par->st.width*net_par->st.height*2;

	int fragment=MAX_PAYLOAD_RTP;
	int frag_num=Bpf/fragment;
	int res=Bpf-frag_num*fragment;
	int MAXBUFL = RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG+fragment;
	int pck_per_frame = (res == 0 ? frag_num : frag_num+1);
	unsigned char pckbuf[MAXBUFL];
	if (debug) printf("pck_per_frame (YUYV) = %d\n", pck_per_frame);


	struct sockaddr_in senderaddr;
	socklen_t senderaddr_len;
	int payloadsize=0;

	// Assume IN ORDER reception of packets

	// Macchina a stati di gestione del protocollo
	while (1) {
		if (net_par->st.old_pck_present_flag) {
			memcpy(pckbuf, net_par->st.old_pck_udp_payload, net_par->st.old_pck_len);
			net_par->st.old_pck_present_flag = 0;
			payloadsize=net_par->st.old_pck_len;
			if (debug) printf("RECV: using old pck: payloadsize=%d\n", payloadsize);
		} else {
			// ricevi un nuovo pck
			senderaddr_len = sizeof(struct sockaddr_in);
			payloadsize=Recvfrom(net_par->sockfd, pckbuf, MAXBUFL, 0, (struct sockaddr *) &senderaddr, &senderaddr_len);
			if (debug) printf("RECV: payloadsize=%d\n", payloadsize);
		}
                
		net_par->rtp.seqN = RTP_get_seq_num(pckbuf);		
		net_par->rtp.timestamp = RTP_get_timestamp(pckbuf);		
		int marker = RTP_get_marker(pckbuf);

		if (debug) { RTP_print_header(pckbuf); printf("\n"); }

		if (net_par->st.framenum<0) {
			// Primo pacchetto in assoluto
			net_par->st.curr_timestamp = net_par->rtp.timestamp;
			net_par->st.framenum=0;
			net_par->rtp.seqN_ext = net_par->rtp.seqN;
		}
		int delta = net_par->rtp.seqN - (net_par->rtp.seqN_ext % 65536);
		if (delta < 65535 - MAX_REORDER_PACKETS) 
			net_par->rtp.seqN_ext += delta;
	
		// Processa il pacchetto
		if (net_par->rtp.timestamp == net_par->st.curr_timestamp) {
			// packets might be lost, put data in the correct position, assuming first seqN=0
			// an entire frame might be lost, so compute the current initial seqN value for the frame
			//net_par->st.framenum = net_par->rtp.seqN / pck_per_frame;
			//net_par->st.frame_nseq_iniz = net_par->st.framenum * pck_per_frame;
			//int pos = fragment*(net_par->rtp.seqN - net_par->st.frame_nseq_iniz);
			net_par->st.framenum = RTP_get_framenum(pckbuf);
			if (debug) printf("net_par->st.framenum = %d\n", net_par->st.framenum);
			if (debug) printf("RTP_get_slicenum() = %d\n", RTP_get_slicenum(pckbuf));
			int hdr_len=RTP_get_header_len(pckbuf);

			if (net_par->st.jpg_flag) {
				// RGB = 3*
				int pos = (3*net_par->st.width*net_par->st.height/net_par->st.pck_per_frame)*RTP_get_slicenum(pckbuf);
				if ( (pckbuf+hdr_len)[0]!=0x00 ) {
					decode_jpg_packet_rgb(videoFrame+pos, pckbuf+hdr_len, payloadsize-hdr_len);
				} else {
					// not JPG: packet was sent but content could not fit the 1500 byte packet
					net_par->stat.pck_invalid.all++;
					net_par->stat.pck_invalid.last++;
				}
			} else {
				int pos = fragment*RTP_get_slicenum(pckbuf);
				if (debug) printf("N.seq nel frame: %d\n", net_par->rtp.seqN - net_par->st.frame_nseq_iniz);
				memcpy(videoFrame+pos, pckbuf+hdr_len, payloadsize-hdr_len);
			}

			if (marker==1) {
				// mostro il frame in videoFrame a video
				// si potrebbe fare al prossimo giro ma cosi' non devo aspettare un pacchetto
				// che potrebbe non arrivare mai
				break;
			}
		} else {
			// il frame e' cambiato: aggiorno il timestamp
			// lo salvo per far finta di averlo ricevuto al prossimo giro
			net_par->st.curr_timestamp = net_par->rtp.timestamp;

			if (debug) printf("Salvo il contenuto del pck ricevuto ma non lo uso per l'immagine\n");
			if (debug) printf("payloadsize=%d\n",payloadsize);
			memcpy(net_par->st.old_pck_udp_payload, pckbuf, payloadsize);
			net_par->st.old_pck_len = payloadsize;
			net_par->st.old_pck_present_flag = 1;

			if (net_par->st.frame_shown == 0) {
				// c'e' stato un cambio senza marker bit:
				break;  // mostro il frame in videoFrame a video
			}
		}
		net_par->st.frame_shown = 0; // resetta se non e' stato mostrato il frame
	}

	net_par->st.frame_shown = 1;
	if (debug) printf("SHOWING frame\n");

	//for (y=0;y<height;y++){
	//	for (x=0;x<width;x+=2){  // red: 0x51 0x5a 0xf0 in YUV 
	//		videoFrame[width*2*y+(x*2)]=0x51;  //Y1
	//		videoFrame[width*2*y+(x*2)+1]=0x5a;  //U=Cb
	//		videoFrame[width*2*y+(x*2)+2]=0x51;  //Y2
	//		videoFrame[width*2*y+(x*2)+3]=0xf0;  //V=Cr
	//	}
	//}

	//fprintf(stderr,"FAROS_POLITO_IMG: xImage1->depth=%d\n", xImage1->depth);	
	//fprintf(stderr,"FAROS_POLITO_IMG: xImage1->bytes_per_line=%d\n", xImage1->bytes_per_line);
	//fprintf(stderr,"FAROS_POLITO_IMG: xImage1->bits_per_pixel=%d\n", xImage1->bits_per_pixel);
	
	switch (xImage1->depth)
	{
		// FIXME: non gestisce rgb come input...

		case 16://process one entire frame
			for (y = 0; y < net_par->st.height; y++)
			{
				for (x = 0; x < net_par->st.width; x+=2)
				{
					//in every loop 2 pixels are processed   
					Y1 = videoFrame[net_par->st.width*2*y+(x*2)];
					U = videoFrame[net_par->st.width*2*y+(x*2)+1];
					Y2 = videoFrame[net_par->st.width*2*y+(x*2)+2];
					V = videoFrame[net_par->st.width*2*y+(x*2)+3];

					yuv_to_rgb_16(Y1, U, V, &RG, &GB);
					imageLine[(bpl*y)+(Bpp*x)]=GB;
					imageLine[(bpl*y)+(Bpp*x)+1]=RG;

					yuv_to_rgb_16(Y2, U, V, &RG, &GB);
					imageLine[(bpl*y)+(Bpp*(x+1))]=GB;
					imageLine[(bpl*y)+(Bpp*(x+1))+1]=RG;
				}
			}
			break;
			
		case 24:
			if (!net_par->st.jpg_flag) {

				for (y = 0; y < net_par->st.height; y++) {
					for (x = 0; x < net_par->st.width; x+=2) {
						//in every loop 2 pixels are processed   
						Y1 = videoFrame[net_par->st.width*2*y+(x*2)];
						U = videoFrame[net_par->st.width*2*y+(x*2)+1];
						Y2 = videoFrame[net_par->st.width*2*y+(x*2)+2];
						V = videoFrame[net_par->st.width*2*y+(x*2)+3];
						//fprintf(stderr,"%d %d %d %d   ",Y1,U,Y2,V);

						//yuv_to_rgb_24(Y1, U, V, &R, &G, &B);
						//fprintf(stderr,"%d %d %d -> %d %d %d    ",Y1,U,V, R,G,B);

						yuv_to_rgb_24(Y1, U, V, &R, &G, &B);
						imageLine[(bpl*y)+(Bpp*x)]=B;
						imageLine[(bpl*y)+(Bpp*x)+1]=G;
						imageLine[(bpl*y)+(Bpp*x)+2]=R;

						yuv_to_rgb_24(Y2, U, V, &R, &G, &B);
						imageLine[(bpl*y)+(Bpp*(x+1))]=B;
						imageLine[(bpl*y)+(Bpp*(x+1))+1]=G;
						imageLine[(bpl*y)+(Bpp*(x+1))+2]=R;

						//imageLine[(bpl*y)+(Bpp*x)]=0x00;
						//imageLine[(bpl*y)+(Bpp*x)+1]=0x00;
						//imageLine[(bpl*y)+(Bpp*x)+2]=0x00;
						
						//imageLine[(bpl*y)+(Bpp*(x+1))]=0x00;
						//imageLine[(bpl*y)+(Bpp*(x+1))+1]=0x00;
						//imageLine[(bpl*y)+(Bpp*(x+1))+2]=0xff;
						//x=width;
						//y+=height/2;
					}
				}
			} else {
				for (y = 0; y < net_par->st.height; y++) {
					for (x = 0; x < net_par->st.width; x++) {
						R = videoFrame[net_par->st.width*3*y+(x*3)];
						G = videoFrame[net_par->st.width*3*y+(x*3)+1];
						B = videoFrame[net_par->st.width*3*y+(x*3)+2];
						imageLine[(bpl*y)+(Bpp*x)]=B;
						imageLine[(bpl*y)+(Bpp*x)+1]=G;
						imageLine[(bpl*y)+(Bpp*x)+2]=R;
					}
				}
			}
			break;
		default:
			fprintf(stderr,"\nError: Color depth not supported\n");
			exit(EXIT_FAILURE);
			break;
	}

	//printf("bpl=%d, Bpp=%d\n",bpl,Bpp);

	if (zoom == 1) {
		memcpy(imageLine1, imageLine, bpl*net_par->st.height);
	} else {
		int i,j, k,m;
		for (j=0; j<net_par->st.height; j++) {
			for (i=0; i<net_par->st.width; i++) {
				for (k=0; k<zoom; k++) {	
					for (m=0; m<zoom; m++) {	
						imageLine1[(j*zoom+m)*bpl + Bpp*(i*zoom+k)] = imageLine[j*bpl + Bpp*i];	
						imageLine1[(j*zoom+m)*bpl + Bpp*(i*zoom+k)+1] = imageLine[j*bpl + Bpp*i+1];	
						imageLine1[(j*zoom+m)*bpl + Bpp*(i*zoom+k)+2] = imageLine[j*bpl + Bpp*i+2];	
					}
				}
			}
		}
	}

	image_put(image_ctx, 0, 0, 0, 0, net_par->st.width*zoom, net_par->st.height*zoom);

	free(imageLine);

	if (XPending(image_ctx.display) > 0) {
		XNextEvent(image_ctx.display, image_ctx.event); //refresh the picture
	}

	
	//net_par->st.absolute_clock_usec32 = get_abs_clock_usec_32bit();
	int delay_send_to_display = get_abs_clock_usec_32bit() - net_par->rtp.timestamp;
	update_stat(net_par, delay_send_to_display);
}


static void usage (FILE *fp, int argc, char **argv) {
	fprintf (fp,
		"FarosPolitoIMGVideocommunication receiver program --- version "VERSION" Copyright (C) 2012 Enrico Masala\n"
                "FarosPolitoIMGVideocommunication comes with ABSOLUTELY NO WARRANTY; for details see the enclosed LICENSE.txt file\n"
                "This is free software, and you are welcome to redistribute it under certain conditions;\n"
                "for details see the enclosed LICENSE.txt file\n"
                "\n"
                "This software is licensed under GPL v.2.0\n"
                "Please note that you are using it at your own risk\n"
		"\n"
		"Usage: %s [options]\n\n"
		"Options:\n"
		"-w | --window-size <640*480>          (just an example) Video size\n"
		"-h | --help                           Print this message\n"
		"-H | --host                           Listen host/address\n"
		"-P | --port                           Listen port\n"
		"-J | --jpg_pck                        Use JPG, make n pck/frame (no option->raw YUYV data)\n"
		"-T | --absolute_clock                 1 = yes, use clock to measure delay (use NTP first on both PC)\n"
		"\n",
		argv[0]);
}


//needed to parse command line arguments with getopt_long
static const char short_options [] = "w:hH:P:Z:J:T:";

//also needed to parse command line arguments with getopt_long
static const struct option
long_options [] = 
{
	{ "window-size", required_argument,      NULL,           'w' },
	{ "help",        no_argument,            NULL,           'h' },
	{ "host",        required_argument,      NULL,           'H' },
	{ "port",        required_argument,      NULL,           'P' },
	{ "zoom",        required_argument,      NULL,           'Z' },
	{ "jpg_pck",     required_argument,      NULL,           'J' },
	{ "abs_clock",   required_argument,      NULL,           'T' },
	{ 0, 0, 0, 0 }
};

typedef enum 
{      
	PIX_FMT_YUV420P,
	PIX_FMT_RGB565,
	PIX_FMT_RGB32,
	PIX_FMT_n3,
	PIX_FMT_n4,
	PIX_FMT_YUYV,
} pix_fmt;  


int main (int argc, char ** argv) 
{
	image_context        img_ctx;
	uint8_t              *videoFrame;
	
	//int                 MaxImageWidth        = 640*2; // For 3D video
	//int                 MaxImageHeight       = 480;
	int                 index;
	int                 c;
	pix_fmt             pixel_format = PIX_FMT_YUV420P;

	printf("FarosPolitoIMGVideocommunication receiver program --- version "VERSION" Copyright (C) 2012 Enrico Masala\n");

	int zoom = 1;
	net_par_t net_par;
	net_par.host=NULL;
	net_par.port=NULL;
	net_par.st.pck_per_frame=0;
	net_par.st.jpg_flag=0;
	net_par.st.absolute_clock_flag=0;  // 1 = use timestamp for absolute clock... (sync both pc with NTP, then measure delay...)
	net_par.st.width=640;
	net_par.st.height=480;
	memset(&net_par.stat, 0, sizeof(net_par.stat));

	pixel_format = 5; // YUYV - YUV422
	
	for (;;) {
		c = getopt_long (argc, argv, short_options, long_options, &index);

		if (-1 == c)
			break; //no more arguments

		switch (c) {
			case 0: // getopt_long() flag
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

			case 'h':
				usage (stdout, argc, argv);
				exit (EXIT_SUCCESS);

			case 'H':
				net_par.host=strdup(optarg);
				break;
			case 'P':
				net_par.port=strdup(optarg);
				break;
			case 'Z':
				zoom=atoi(optarg);
				break;
			case 'J':
				net_par.st.pck_per_frame=atof(optarg);
				net_par.st.jpg_flag=1;
				break;
			case 'T':
				net_par.st.absolute_clock_flag=1;
				break;

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

	/* for errlib to know the program name */
	prog_name = argv[0];
	net_par.rtp.seqN = 0;

	net_par.sockfd = Socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	/* specify address to bind to */
	memset(&net_par.addr, 0, sizeof(net_par.addr));
	net_par.addr.sin_family = AF_INET;
	net_par.addr.sin_port = htons((uint16_t)(unsigned int)atoi(net_par.port));
	net_par.addr.sin_addr.s_addr = htonl (INADDR_ANY);

	Bind(net_par.sockfd, (SA*) &(net_par.addr), sizeof(net_par.addr));

	printf("host=%s port=%s\n", net_par.host, net_par.port);


	if (image_init_window(&img_ctx, net_par.st.width*zoom,  net_par.st.height*zoom  )<0) {
		exit(1);
	}
   
    videoFrame = (uint8_t*) calloc (1, net_par.st.width*net_par.st.height*4);  // rgb32????

    printf("INFO: always remember to TURN OFF the FIREWALL if any is present\n");

    net_par.st.framenum=-1;

    net_par.st.old_pck_udp_payload = (unsigned char *)malloc((MAX_PAYLOAD_RTP+RTP_HEADER_LEN_EXT_FAROS_POLITO_IMG)*sizeof(unsigned char));
    if (!net_par.st.old_pck_udp_payload) { printf("ERROR: cannot allocate memory\n"); exit(1); }
    net_par.st.old_pck_len = 0;
    net_par.st.old_pck_present_flag = 0;
    net_par.st.frame_shown = 0;

    net_par.st.framerate=30;
    net_par.st.clock=90000;  // 90 KHz
    
    while(1) {
	process_image_yuyv  (videoFrame, img_ctx, &net_par, zoom);
    }
    exit (1);
}
