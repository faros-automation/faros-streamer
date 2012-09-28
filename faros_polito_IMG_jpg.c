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
#include <jpeglib.h>

#include <stdint.h>

#include <setjmp.h>

#include "faros_polito_IMG_rtp.h"
#include "faros_polito_IMG_jpg.h"

#define FALSE 0
#define TRUE 1

#define min(a,b) ((a)<(b)? (a) : (b))

unsigned char my_buffer[MAX_OUT_JPG_BUF];
int my_buffer_fill;

jmp_buf jmp_buffer;

void my_init_destination(j_compress_ptr cinfo)
{
	//my_buffer.resize(BLOCK_SIZE);
	my_buffer_fill=0;
	cinfo->dest->next_output_byte = &(my_buffer[0]);
	cinfo->dest->free_in_buffer = MAX_OUT_JPG_BUF-my_buffer_fill;
}

int my_empty_output_buffer(j_compress_ptr cinfo)
{
/*
	size_t oldsize = my_buffer.size();
	my_buffer.resize(oldsize + BLOCK_SIZE);
	cinfo->dest->next_output_byte = &my_buffer[oldsize];
	cinfo->dest->free_in_buffer = my_buffer.size() - oldsize;
*/
	printf("ERROR: buffer not enough for compressed jpg!\n");
	exit(1);
	return TRUE;
}

void my_term_destination(j_compress_ptr cinfo)
{
	//my_buffer.resize(my_buffer.size() - cinfo->dest->free_in_buffer);
	my_buffer_fill = MAX_OUT_JPG_BUF - cinfo->dest->free_in_buffer;
}

void init_jpg_packets(int n_pck, pck_t *list, int max_size) {
	int k;
	for (k=0; k<n_pck; k++) {
		list[k].buf = (unsigned char *)malloc(sizeof(unsigned char)*max_size);
		if (!list[k].buf) { printf("ERROR: cannot allocate list[k].buf\n"); exit(1); }
		list[k].size = max_size;
		list[k].valid = 0;
	}
}

void reset_jpg_packets_len(int n_pck, pck_t *list, int max_size) {
	int k;
	for (k=0; k<n_pck; k++) {
		list[k].size = max_size;
	}
}

void free_jpg_packets(int n_pck, pck_t *list) {
	int k;
	for (k=0; k<n_pck; k++) {
		if (list[k].buf)
			free(list[k].buf);
	}
}

//static int cnt=0;

// n_pck: numero di pacchetti da creare
// list: vettore contenente strutture, ognuna con puntatore a buffer unsigned char * di dimensione pari a  size (scritto nella struttura)
//   in  size  vengono riservati  header_size  bytes : il valore di size viene aggiornato alla fine, ed include anche l'header_size
// Se in n. di bytes prodotti e' > del n. di bytes pro
// image_height deve essere divisibile esattamente per n_pck

// NB: ---- OLD function ---- new one manages RGB etc.
void create_jpg_packets(int quality, int n_pck, unsigned char *yuyv, pck_t *list, int header_size, int flag_grayscale, int width, int height) {

	unsigned char *buffer;
	int Bpf;

	if (flag_grayscale) {
		Bpf=width*height;
	} else {
		Bpf=width*height*3;
	}

	buffer = (unsigned char *)malloc(sizeof(unsigned char)*Bpf);
	if (!buffer) { printf("ERROR: cannot allocate buffer in create_jpg_packets\n"); exit(1); }

	if (flag_grayscale) {
		int i;  // copy the Y part
		for (i=0; i<Bpf; i++) {
			buffer[i]=yuyv[i*2];
		}
	} else {
		int i;  // rearrange the Y,U,V part in YUV444
		for (i=0; i<Bpf/3; i+=2) {
			buffer[i*3]=yuyv[i*2];
			buffer[i*3+1]=yuyv[i*2+1];
			buffer[i*3+2]=yuyv[i*2+3];
			buffer[i*3+3]=yuyv[i*2+2];
			buffer[i*3+4]=yuyv[i*2+1];
			buffer[i*3+5]=yuyv[i*2+3];
		}
	}

			

	int k;
	for (k=0; k<n_pck; k++) {
		//printf("FAROS_POLITO_IMG JPG: k=%d\n", k);
		//int bufsize=Bpf/n_pck;
		int bufoff=k*Bpf/n_pck;


		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr       jerr;

		//memset(&cinfo, 0, sizeof(cinfo));

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);

		//printf("FAROS_POLITO_IMG after create_compress\n");

		struct jpeg_destination_mgr jdm;
		cinfo.dest=&jdm;
		cinfo.dest->init_destination = &my_init_destination;
		cinfo.dest->empty_output_buffer = &my_empty_output_buffer;
		cinfo.dest->term_destination = &my_term_destination;

		//printf("FAROS_POLITO_IMG: n_pck=%d\n",n_pck);

		// Da fare prima di chiamare set_defaults
		cinfo.image_width      = width;
		cinfo.image_height     = height/n_pck;
		if (flag_grayscale) {
			cinfo.input_components = 1;
			cinfo.in_color_space   = JCS_GRAYSCALE;
		} else {
			cinfo.input_components = 3;
			cinfo.in_color_space   = JCS_YCbCr;
		}

		jpeg_set_defaults(&cinfo);
		/*set the quality [0..100]  */
		//printf("FAROS_POLITO_IMG after set_defaults\n");

		jpeg_set_quality (&cinfo, quality, TRUE);




		//jpeg_stdio_dest(&cinfo, outfile);

		//cinfo.raw_data_in = TRUE;

		//unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char)*cinfo.image_width*cinfo.image_height);
		//int i,j;
		//for (j=0; j<cinfo.image_height; j++) {
		//	for (i=0; i<cinfo.image_width; i++) {
		//		//buffer[j*cinfo.image_width+i] = ( (i+j)/2 > 255 ? 255 : (i+j)/2 );
		//		buffer[j*cinfo.image_width+i] = 0x80;
		//	}
		//}

		//printf("FAROS_POLITO_IMG before set_defaults\n");


		//printf("FAROS_POLITO_IMG after set_quality\n");

		jpeg_start_compress(&cinfo, TRUE);

		JSAMPROW row_pointer;          /* pointer to a single row */

		int bits_depth;
		bits_depth=24;
		if (flag_grayscale) {
			bits_depth=8;
		}

		//printf("FAROS_POLITO_IMG before write_scanlines\n");

		while (cinfo.next_scanline < cinfo.image_height) {
			row_pointer = (JSAMPROW) ( buffer + bufoff + cinfo.next_scanline*(bits_depth>>3)*cinfo.image_width );
			jpeg_write_scanlines(&cinfo, &row_pointer, 1);
			//jpeg_write_raw_data(&cinfo, &row_pointer, 1);
		}

		//printf("FAROS_POLITO_IMG: my_buffer_fill=%d\n",my_buffer_fill);

		jpeg_finish_compress(&cinfo);

		// my_buffer_fill  e' la dimensione dei dati prodotti dalla codifica JPEG

		int max_fill = min ( my_buffer_fill , (list[k].size-header_size) );
		memcpy(list[k].buf+header_size, my_buffer, max_fill);
		list[k].size = max_fill+header_size;
		if (max_fill == my_buffer_fill) {
			list[k].valid = 1;
		} else {
			list[k].valid = 0;
		}

		//if (k<n_pck-1)
		//	jpeg_abort_compress(&cinfo);

		//char fname[1000];
		//sprintf(fname,"/tmp/test%03d.jpg",cnt);
		//FILE *fout=fopen(fname,"wb");
		//fwrite(my_buffer, my_buffer_fill, 1, fout);
		//fclose(fout);
		//printf("FAROS_POLITO_IMG: wrote file %s\n",fname);
		//cnt++;
	}
	//printf("FAROS_POLITO_IMG before finish_compress\n");
	//jpeg_finish_compress(&cinfo);
	free(buffer);
	//printf("ENCODED 1 frame\n");
}


// n_pck: numero di pacchetti da creare
// list: vettore contenente strutture, ognuna con puntatore a buffer unsigned char * di dimensione pari a  size (scritto nella struttura)
//   in  size  vengono riservati  header_size  bytes : il valore di size viene aggiornato alla fine, ed include anche l'header_size
// Se in n. di bytes prodotti e' > del n. di bytes pro
// image_height deve essere divisibile esattamente per n_pck
void create_jpg_packets_from_buf(int quality, int n_pck, unsigned char *videobuf, pck_t *list, int header_size, int flag_grayscale, int width, int height, int isRGB) {

	unsigned char *buffer;
	int Bpf;

	if (flag_grayscale) {
		Bpf=width*height;
	} else {
		Bpf=width*height*3;
	}

	buffer = (unsigned char *)malloc(sizeof(unsigned char)*Bpf);
	if (!buffer) { printf("ERROR: cannot allocate buffer in create_jpg_packets\n"); exit(1); }

	if (flag_grayscale) {
		int i;
		if (isRGB) {
			for (i=0; i<Bpf; i+=3) {
				uint8_t y = (uint8_t)((((uint16_t)videobuf[i+0])*6)/10 + (((uint16_t)videobuf[i+1])*3)/10 + (((uint16_t)videobuf[i+2])*1)/10 );
				buffer[i+0]=y;
				buffer[i+1]=y;
				buffer[i+2]=y;
			}
		} else {
			// copy the Y part
			for (i=0; i<Bpf; i++) {
				buffer[i]=videobuf[i*2];
			}
		}
	} else {
		int i;
		if (isRGB) {
			memcpy(buffer, videobuf, Bpf);
		} else {
			// rearrange the Y,U,V part in YUV444
			for (i=0; i<Bpf/3; i+=2) {
				buffer[i*3]=videobuf[i*2];
				buffer[i*3+1]=videobuf[i*2+1];
				buffer[i*3+2]=videobuf[i*2+3];
				buffer[i*3+3]=videobuf[i*2+2];
				buffer[i*3+4]=videobuf[i*2+1];
				buffer[i*3+5]=videobuf[i*2+3];
			}
		}
	}

			

	int k;
	for (k=0; k<n_pck; k++) {
		//printf("FAROS_POLITO_IMG JPG: k=%d\n", k);
		//int bufsize=Bpf/n_pck;
		int bufoff=k*Bpf/n_pck;


		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr       jerr;

		//memset(&cinfo, 0, sizeof(cinfo));

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);

		//printf("FAROS_POLITO_IMG after create_compress\n");

		struct jpeg_destination_mgr jdm;
		cinfo.dest=&jdm;
		cinfo.dest->init_destination = &my_init_destination;
		cinfo.dest->empty_output_buffer = &my_empty_output_buffer;
		cinfo.dest->term_destination = &my_term_destination;

		//printf("FAROS_POLITO_IMG: n_pck=%d\n",n_pck);

		// Da fare prima di chiamare set_defaults
		cinfo.image_width      = width;
		cinfo.image_height     = height/n_pck;
		if (flag_grayscale) {
			cinfo.input_components = 1;
			cinfo.in_color_space   = JCS_GRAYSCALE;
		} else {
			if (!isRGB) {
				cinfo.input_components = 3;
				cinfo.in_color_space   = JCS_YCbCr;
			} else {
				cinfo.input_components = 3;
				cinfo.in_color_space   = JCS_RGB;
			}
		}

		jpeg_set_defaults(&cinfo);
		/*set the quality [0..100]  */
		//printf("FAROS_POLITO_IMG after set_defaults\n");

		jpeg_set_quality (&cinfo, quality, TRUE);




		//jpeg_stdio_dest(&cinfo, outfile);

		//cinfo.raw_data_in = TRUE;

		//unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char)*cinfo.image_width*cinfo.image_height);
		//int i,j;
		//for (j=0; j<cinfo.image_height; j++) {
		//	for (i=0; i<cinfo.image_width; i++) {
		//		//buffer[j*cinfo.image_width+i] = ( (i+j)/2 > 255 ? 255 : (i+j)/2 );
		//		buffer[j*cinfo.image_width+i] = 0x80;
		//	}
		//}

		//printf("FAROS_POLITO_IMG before set_defaults\n");


		//printf("FAROS_POLITO_IMG after set_quality\n");

		jpeg_start_compress(&cinfo, TRUE);

		JSAMPROW row_pointer;          /* pointer to a single row */

		int bits_depth;
		bits_depth=24;
		if (flag_grayscale) {
			bits_depth=8;
		}

		//printf("FAROS_POLITO_IMG before write_scanlines\n");

		while (cinfo.next_scanline < cinfo.image_height) {
			row_pointer = (JSAMPROW) ( buffer + bufoff + cinfo.next_scanline*(bits_depth>>3)*cinfo.image_width );
			jpeg_write_scanlines(&cinfo, &row_pointer, 1);
			//jpeg_write_raw_data(&cinfo, &row_pointer, 1);
		}

		//printf("FAROS_POLITO_IMG: my_buffer_fill=%d\n",my_buffer_fill);

		jpeg_finish_compress(&cinfo);

		// my_buffer_fill  e' la dimensione dei dati prodotti dalla codifica JPEG

		int max_fill = min ( my_buffer_fill , (list[k].size-header_size) );
		memcpy(list[k].buf+header_size, my_buffer, max_fill);
		list[k].size = max_fill+header_size;
		if (max_fill == my_buffer_fill) {
			list[k].valid = 1;
		} else {
			list[k].valid = 0;
		}

		//if (k<n_pck-1)
		//	jpeg_abort_compress(&cinfo);

		//char fname[1000];
		//sprintf(fname,"/tmp/test%03d.jpg",cnt);
		//FILE *fout=fopen(fname,"wb");
		//fwrite(my_buffer, my_buffer_fill, 1, fout);
		//fclose(fout);
		//printf("FAROS_POLITO_IMG: wrote file %s\n",fname);
		//cnt++;

		jpeg_destroy_compress(&cinfo);
	}
	//printf("FAROS_POLITO_IMG before finish_compress\n");
	//jpeg_finish_compress(&cinfo);
	free(buffer);
	//printf("ENCODED 1 frame\n");
}



void my_init_source (j_decompress_ptr cinfo) {
}

int my_fill_input_buffer (j_decompress_ptr cinfo) {
	return TRUE;
}

void my_skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
	struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) cinfo->src;
	if (num_bytes > 0) {
		src->next_input_byte += (size_t) num_bytes;
		src->bytes_in_buffer -= (size_t) num_bytes;
	}
}

void my_term_source (j_decompress_ptr cinfo) {
}

void my_error_handler (j_common_ptr cinfo) {
	longjmp(jmp_buffer, 1);  // 1 = value to be returned by setjmp
}


void decode_jpg_packet_rgb(unsigned char *rgb, unsigned char *jpg_data, int jpg_data_len) {

	//my_buffer = jpg_data
	//bytes_in_input = jpg_data_len

	struct jpeg_decompress_struct cinfo; 
	struct jpeg_error_mgr jerr;

	// Setup decompression structure
	cinfo.err = jpeg_std_error(&jerr); 
	jerr.error_exit = my_error_handler;
	jpeg_create_decompress(&cinfo); 

	struct jpeg_source_mgr jdm;
	cinfo.src=&jdm;
	cinfo.src->init_source = my_init_source;
	cinfo.src->fill_input_buffer = my_fill_input_buffer;
	cinfo.src->skip_input_data = my_skip_input_data;
	cinfo.src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	cinfo.src->term_source = my_term_source;

	cinfo.src->bytes_in_buffer = jpg_data_len;
	cinfo.src->next_input_byte = (JOCTET*)jpg_data;

	// read info from header.
	int r = jpeg_read_header(&cinfo, TRUE);
	if (r != JPEG_HEADER_OK) {
		printf("Failed to read JPEG header.\n"); exit(1);
	} else if (cinfo.num_components != 1 && cinfo.num_components != 3) {
		printf("Unsupported number of color components: %d\n", cinfo.num_components); exit(1);
	}

	jpeg_start_decompress(&cinfo); // NB: do it before using parameters output_width, output_components etc...

	//printf("cinfo.image_width=%d\n", cinfo.image_width);
	//printf("cinfo.image_height=%d\n", cinfo.image_height);
	//printf("cinfo.output_width=%d\n", cinfo.output_width);
	//printf("cinfo.output_components=%d\n", cinfo.output_components);
	//printf("cinfo.num_components=%d\n", cinfo.num_components);

	if (setjmp(jmp_buffer)==0) {
		// Not returning from jump
		
		//JSAMPARRAY imageBuffer = (*cinfo.mem->alloc_sarray)( (j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width*cinfo.output_components, 1);
		int y = 0;
		while (cinfo.output_scanline < cinfo.output_height) {
			unsigned char *ptr = rgb+y*cinfo.image_width*cinfo.output_components;
			jpeg_read_scanlines(&cinfo, (JSAMPARRAY) &ptr, 1);  // They expect the address of a pointer to the row data
			//jpeg_read_scanlines(&cinfo, imageBuffer, 1);
			//memcpy(buffer+y*cinfo.image_width, imageBuffer, cinfo.image_width*cinfo.output_components);
			y++;
		}

		//(*cinfo.mem->free_pool)( (j_common_ptr)&cinfo, JPOOL_IMAGE );

		if (cinfo.output_components==1) {
			// riporta, nello stesso buffer, i valori Y come RGB. Parte dal fondo per non sovrascrivere
			int i;
			for (i=cinfo.output_width*cinfo.image_height-1; i>=0; i--) {
				rgb[i*3+2]=rgb[i];
				rgb[i*3+1]=rgb[i];
				rgb[i*3+0]=rgb[i];
			}
		}
		jpeg_finish_decompress(&cinfo);
	} else {
		printf("Note: skipping corrupt data packet (missing a part?)\n");
		jpeg_abort((j_common_ptr) &cinfo);
	}
	     
	jpeg_destroy_decompress(&cinfo); 
}



