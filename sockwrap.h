/*
 * This file is part of FarosPolitoIMGVideocommunication
 *
 *
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

#ifndef _SOCKWRAP_H

#define _SOCKWRAP_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SA struct sockaddr

int Socket (int family, int type, int protocol);

void Bind (int sockfd, const SA *myaddr, socklen_t myaddrlen);

void Listen (int sockfd, int backlog);

int Accept (int listen_sockfd, SA *cliaddr, socklen_t *addrlenp);

void Connect (int sockfd, const SA *srvaddr, socklen_t addrlen);

void Close (int fd);

void Shutdown (int fd, int howto);

ssize_t Read (int fd, void *bufptr, size_t nbytes);

void Write (int fd, void *bufptr, size_t nbytes);

ssize_t Recv (int fd, void *bufptr, size_t nbytes, int flags);

ssize_t Recvfrom (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr);

ssize_t Recvfrom_timeout (int fd, void *bufptr, size_t nbytes, int flags, SA *sa, socklen_t *salenptr, int timeout);

void Sendto (int fd, void *bufptr, size_t nbytes, int flags, const SA *sa, socklen_t salen);

void Send (int fd, void *bufptr, size_t nbytes, int flags);

/*
 * deprecated and no more supported by recent systems
 * 

void Inet_aton (const char *strptr, struct in_addr *addrptr);

*/

void Inet_pton (int af, const char *strptr, void *addrptr);

void Inet_ntop (int af, const void *addrptr, char *strptr, size_t length);

ssize_t readn (int fd, void *vptr, size_t n);

ssize_t Readn (int fd, void *ptr, size_t nbytes);

ssize_t readline (int fd, void *vptr, size_t maxlen);

ssize_t Readline (int fd, void *ptr, size_t maxlen);

ssize_t writen(int fd, const void *vptr, size_t n);

void Writen (int fd, void *ptr, size_t nbytes);

int Select (int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);

pid_t Fork (void);

struct hostent *Gethostbyname (const char *hostname);

void Getsockname (int sockfd, struct sockaddr *localaddr, socklen_t *addrp);

void Getaddrinfo ( const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

void Print_getaddrinfo_list(struct addrinfo *list_head);

int Setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

#endif
