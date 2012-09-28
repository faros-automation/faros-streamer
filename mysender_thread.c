#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <termios.h>

#define MSG_SIZE	10

void *control_thread_main()
{
   int sockfd;
   struct hostent *hostinfo;
   struct sockaddr_in address;
   char hostname[] = "localhost";
   const int  port = 5555;

   int fd;
   fd_set testfds, clientfds;
   char kb_msg[MSG_SIZE + 10];
   int key;

   struct termios old_tio, new_tio;

	printf("Hello World! It's me, %s thread.\n", "control_send");

	/* Create a socket for the client */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Name the socket, as agreed with the server */
	hostinfo = gethostbyname(hostname);  /* look for host's name */
	address.sin_addr = *(struct in_addr *)*hostinfo -> h_addr_list;
	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	/* Connect the socket to the server's socket */
	if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0){
		perror("connecting");
		exit(1);
	}

	fflush(stdout);

#define STDIN_FILENO 0

	/* get the terminal settings for stdin */
	tcgetattr(STDIN_FILENO,&old_tio);

	/* we want to keep the old setting to restore them a the end */
	new_tio=old_tio;

	/* disable canonical mode (buffered i/o) and local echo */
	new_tio.c_lflag &=(~ICANON & ~ECHO);

	/* set the new settings immediately */
	tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);


	FD_ZERO(&clientfds);
	FD_SET(sockfd,&clientfds);
	FD_SET(0,&clientfds);//stdin

	/*  Now wait for messages from the server */
	while (1) {
		testfds=clientfds;
		select(FD_SETSIZE,&testfds,NULL,NULL,NULL);

		for(fd=0;fd<FD_SETSIZE;fd++){
			if(FD_ISSET(fd,&testfds)){
				if(fd == 0){ 
					//fgets(kb_msg, MSG_SIZE+1, stdin);
					//write(sockfd, kb_msg, strlen(kb_msg));
					key = fgetc(stdin);
					sprintf(kb_msg, "%c\n", key);
					fprintf(stdout,"%s",kb_msg);
					fflush(stdout);
					write(sockfd, kb_msg, strlen(kb_msg));
				}
			}
		}
	}

	/* restore the former settings */
	tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);



	pthread_exit(NULL);
}
