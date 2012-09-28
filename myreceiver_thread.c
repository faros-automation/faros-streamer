#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MSG_SIZE	10

extern int ctrl_jpg_quality;

void *control_thread_main()
{
   int server_sockfd, client_sockfd;
   struct sockaddr_in server_address;
   const int  port = 5555;

   int fd;
   fd_set readfds, testfds;
   char msg[MSG_SIZE + 1];
   int result;  

	printf("Hello World! It's me, %s thread.\n", "control_recv");

	/* Create and name a socket for the server */
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(port);

	bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address));

	/* Create a connection queue and initialize a file descriptor set */
	listen(server_sockfd, 1);

	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);
	FD_SET(0, &readfds);  /* Add keyboard to file descriptor set */

	/*  Now wait for clients and requests */
	while (1) {
		testfds = readfds;
		select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

		/* If there is activity, find which descriptor it's on using FD_ISSET */
		for (fd = 0; fd < FD_SETSIZE; fd++) {
			if (FD_ISSET(fd, &testfds)) {
				if (fd == server_sockfd) { /* Accept a new connection request */
					client_sockfd = accept(server_sockfd, NULL, NULL);
					/*printf("client_sockfd: %d\n",client_sockfd);*/

					FD_SET(client_sockfd, &readfds);
					/*Client ID*/
					printf("Client joined\n");
					fflush(stdout);

					sprintf(msg,"M%2d",client_sockfd);
					/*write 2 byte clientID */
					send(client_sockfd,msg,strlen(msg),0);
				} else if(fd) {
					result = read(fd, msg, MSG_SIZE);
					if(result==-1) perror("read()");
					else if(result>0){
						msg[result]='\0';
						printf("[%c]|%s|",msg[0],msg);
						if(msg[0]=='+') { ctrl_jpg_quality+=1; printf(">> ctrl_jpg_quality: %d\n", ctrl_jpg_quality); }
						else if(msg[0]=='-') { ctrl_jpg_quality-=1; printf(">> ctrl_jpg_quality: %d\n", ctrl_jpg_quality); }

					}
				}
			}//if
		}//for
	}//while




	fflush(stdout);

	pthread_exit(NULL);
}
