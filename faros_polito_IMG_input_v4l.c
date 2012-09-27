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


#include "faros_polito_IMG_input_v4l.h"

static buffer_t *buffers = NULL;
static int n_buffers;
static frame_callback fc;

void errno_exit (const char *s) {
	fprintf (stderr, "%s error %d, %s\n",s, errno, strerror (errno));
	exit (EXIT_FAILURE);
}

//a blocking wrapper of the ioctl function
int xioctl (int fd, int request, void *arg) {
	int r;

	do
		r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

void print_pixel_format(int pixel_format) {
	switch (pixel_format) {    
		case 0:
			fprintf(stderr,"V4L2_PIX_FMT_YUV420\n");           
			break;
		case 1:
			fprintf(stderr,"V4L2_PIX_FMT_RGB565\n");
			break;
		case 2:
			fprintf(stderr,"V4L2_PIX_FMT_RGB32\n");
			break;
		case 3:
			fprintf(stderr,"V4L2_PIX_FMT_RGB24\n");
			break;
		case 4:
			fprintf(stderr,"V4L2_PIX_FMT_YUV420\n");
			break;
		case 5:
			fprintf(stderr,"V4L2_PIX_FMT_YUYV\n");
			break;
		default:
			fprintf(stderr,"Pixel format Unknown\n");
			break;
	}
	//fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_SRGB;
}

void set_frame_callback(int *fd, frame_callback fr_callback) {
	fc = fr_callback;
}

void mainloop_capturing(int *fd) {
	for (;;) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO (&fds);
		FD_SET (*fd, &fds);

		// Select Timeout
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		//the classic select function, who allows to wait up to 2 seconds, 
		//until we have captured data,
		r = select (*fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;

			errno_exit ("select");
		}

		if (0 == r) {
			fprintf (stderr, "select timeout\n");
			exit (EXIT_FAILURE);
		}


		struct v4l2_buffer buf;//needed for memory mapping
		CLEAR (buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl (*fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
				case EAGAIN:
					//return 0;  // TED original value  = resource busy : wait again
					continue;

				case EIO://EIO ignored

				default:
					errno_exit ("VIDIOC_DQBUF");
			}
		}
		assert (buf.index < n_buffers);

		unsigned char *videobuffer = buffers[buf.index].start;
		fc(fd, videobuffer);

		// NECESSARIO perche' credo che scambi correttamente i buffer
		if (-1 == xioctl (*fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");

	}
}

void stop_capturing (int * fd) {
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//this call to xioctl allows to stop the stream from the capture device
	if (-1 == xioctl (*fd, VIDIOC_STREAMOFF, &type))
		errno_exit ("VIDIOC_STREAMOFF");
}

void start_capturing (int * fd ) {
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		if (-1 == xioctl (*fd, VIDIOC_QBUF, &buf))
			errno_exit ("VIDIOC_QBUF");
	}
				
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	//start the capture from the device
	if (-1 == xioctl (*fd, VIDIOC_STREAMON, &type))
		errno_exit ("VIDIOC_STREAMON");
}


//free the shared memory area
void uninit_device (void) {
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap (buffers[i].start, buffers[i].length))
			errno_exit ("munmap");
	free (buffers);
}

//alloc buffers and configure the shared memory area
void init_mmap (int * fd, char * dev_name, int * n_buffers) {
	struct v4l2_requestbuffers req;
	//buffers is an array of n_buffers length, and every element store a frame. Global variable
	CLEAR (req);

	//TED req.count               = 2;
	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (*fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support memory mapping\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",dev_name);
		exit (EXIT_FAILURE);
	}
	buffers = calloc (req.count, sizeof (buffer_t));
	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
	//map every element of the array buffers to the shared memory
	for (*n_buffers = 0; *n_buffers < req.count; ++*n_buffers) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = *n_buffers;

		if (-1 == xioctl (*fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		buffers[*n_buffers].length = buf.length;
		buffers[*n_buffers].start = mmap (NULL,// start anywhere
							buf.length,
							PROT_READ | PROT_WRITE, // required
							MAP_SHARED, // recommended
							*fd, buf.m.offset);

		if (MAP_FAILED == buffers[*n_buffers].start)
			errno_exit ("mmap");
	}
}

//configure and initialize the hardware device 
void init_device (int * fd, char * dev_name, int width, int height, int pixel_format) {
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	memset(&cropcap, 0, sizeof(cropcap));

	if (-1 == xioctl (*fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",dev_name);
		exit (EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf (stderr, "%s does not support streaming i/o\n",dev_name);
		exit (EXIT_FAILURE);
	}

	// Select video input, video standard and tune here.
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (*fd, VIDIOC_CROPCAP, &cropcap)) {
				// Errors ignored.
	}

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c = cropcap.defrect; // reset to default

	if (-1 == xioctl (*fd, VIDIOC_S_CROP, &crop)) {
		switch (errno) {
			case EINVAL:
				// Cropping not supported.
			break;
			default:
				// Errors ignored.
			break;
		}
	}

	CLEAR (fmt);
	//set image properties
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width;
	fmt.fmt.pix.height      = height;
		
	switch (pixel_format) {    
		case 0:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;           
			break;
		case 1:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
			break;
		case 2:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
			break;
		case 3:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
			break;
		case 4:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
			break;
		case 5:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
			break;
	}
	//fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_SRGB;
	//fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	//TED commented
	if (-1 == xioctl (*fd, VIDIOC_S_FMT, &fmt))
		errno_exit ("\nError: pixel format not supported\n");

	// Note VIDIOC_S_FMT may change width and height.

	//check the configuration data
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	fprintf(stderr, "Video bytespreline = %d\n",fmt.fmt.pix.bytesperline);

	init_mmap (fd, dev_name, &n_buffers);
}

void close_device (int * fd) {
	if (-1 == close (*fd))
		errno_exit ("close");

	*fd = -1;
}

void open_device (int * fd, char * dev_name) {
	struct stat st; 

	if (-1 == stat (dev_name, &st)) {
		fprintf (stderr, "Cannot identify '%s': %d, %s\n",dev_name, errno, strerror (errno));
			exit (EXIT_FAILURE);
	}

	if (!S_ISCHR (st.st_mode)) {
		fprintf (stderr, "%s is no device\n", dev_name);
			exit (EXIT_FAILURE);
	}

	*fd = open (dev_name, O_RDWR // required 
		 | O_NONBLOCK, 0);

	if (-1 == *fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n",dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
}

//show the available devices
void enum_inputs (int * fd) {
	int  ninputs;
	struct v4l2_input  inp[MAX_INPUT];
	printf("Available Inputs:\n");
	for (ninputs = 0; ninputs < MAX_INPUT; ninputs++) {
		inp[ninputs].index = ninputs;
		if (-1 == ioctl(*fd, VIDIOC_ENUMINPUT, &inp[ninputs]))
			break;
		printf("number = %d      description = %s\n",ninputs,inp[ninputs].name); 
	}
}

//show the available standards(norms) for capture 
void enum_standards (int * fd ) {
	struct v4l2_standard  std[MAX_NORM];
	int  nstds;
	printf("Available Standards:\n");
	for (nstds = 0; nstds < MAX_NORM; nstds++) {
		std[nstds].index = nstds;
		if (-1 == ioctl(*fd, VIDIOC_ENUMSTD, &std[nstds]))
			break;
		printf("number = %d     name = %s\n",nstds,std[nstds].name);
	}
}

//configure the video input
void set_input(int * fd, int dev_input) {
	struct v4l2_input input;
	int index = dev_input;
	//set the input
	if (-1 == ioctl (*fd, VIDIOC_S_INPUT, &index)) {
		perror ("VIDIOC_S_INPUT");
		exit (EXIT_FAILURE);
	}
	//check the input
	if (-1 == ioctl (*fd, VIDIOC_G_INPUT, &index)) {
		perror ("VIDIOC_G_INPUT");
		exit (EXIT_FAILURE);
	}
	memset (&input, 0, sizeof (input));
	input.index = index;
	if (-1 == ioctl (*fd, VIDIOC_ENUMINPUT, &input)) {
		perror ("VIDIOC_ENUMINPUT");
		exit (EXIT_FAILURE);
	}
	fprintf (stderr,"input: %s\n", input.name); 
}

//configure the capture standard
void set_standard(int * fd, int dev_standard) {
	struct v4l2_standard standard;
	v4l2_std_id st;
	standard.index = dev_standard;;
	if (-1 == ioctl (*fd, VIDIOC_ENUMSTD, &standard)) {
		perror ("VIDIOC_ENUMSTD");
	}
	st=standard.id;
	
	if (-1 == ioctl (*fd, VIDIOC_S_STD, &st)) {
		perror ("VIDIOC_S_STD");
	}
	fprintf (stderr,"standard: %s\n", standard.name); 
}

