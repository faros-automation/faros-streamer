

DIRNAME=src

CC=gcc
#CFLAGS=-Wall -O2
CFLAGS=-Wall -O0 -g
#CFLAGS=-Wall -O2 -pg
LDFLAGS=

all: faros_polito_IMG_sender_actua faros_polito_IMG_sender faros_polito_IMG_receiver

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

faros_polito_IMG_sender_actua: faros_polito_IMG_sender.c errlib.c sockwrap.c faros_polito_IMG_rtp.c faros_polito_IMG_jpg.c faros_polito_IMG_common.c 
	$(CC) $(CFLAGS) -DACTUA -ljpeg -o faros_polito_IMG_sender_actua faros_polito_IMG_sender.c errlib.c sockwrap.c faros_polito_IMG_rtp.c faros_polito_IMG_jpg.c faros_polito_IMG_common.c 

faros_polito_IMG_sender: faros_polito_IMG_sender.c errlib.c sockwrap.c faros_polito_IMG_rtp.c faros_polito_IMG_jpg.c faros_polito_IMG_common.c faros_polito_IMG_input_v4l.c
	$(CC) $(CFLAGS) -ljpeg -o faros_polito_IMG_sender faros_polito_IMG_sender.c errlib.c sockwrap.c faros_polito_IMG_rtp.c faros_polito_IMG_jpg.c faros_polito_IMG_common.c faros_polito_IMG_input_v4l.c

faros_polito_IMG_receiver: faros_polito_IMG_receiver.c errlib.c sockwrap.c faros_polito_IMG_rtp.c faros_polito_IMG_jpg.c faros_polito_IMG_common.c  faros_polito_IMG_show_X.c
	$(CC) $(CFLAGS) -ljpeg -L/usr/X11R6/lib -lX11 -lXext   -o faros_polito_IMG_receiver faros_polito_IMG_receiver.c errlib.c sockwrap.c faros_polito_IMG_rtp.c faros_polito_IMG_jpg.c faros_polito_IMG_common.c faros_polito_IMG_show_X.c

tar:
	( di=`pwd`; db=`basename $$di`; cd ..; tar cjvf faros_polito_IMG_videocommunication__$(DIRNAME)__`date +'%Y%m%d_%H%M'`.tar.bz2 "$$db" )	

clean:
	rm -f faros_polito_IMG_sender
	rm -f faros_polito_IMG_sender_actua
	rm -f faros_polito_IMG_receiver
	rm -f *.o
	rm -f *core*

