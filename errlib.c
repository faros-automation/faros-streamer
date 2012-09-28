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

/*

module: errlib.c

purpose: library of error functions

reference: Stevens, Unix network programming (2ed), p.922

*/

#include <stdio.h>
#include <stdlib.h> /* exit() */
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#define MAXLINE 4095

int daemon_proc = 0; /* set to 0 if stdout/stderr available, else set to 1 */

static void err_doit (int errnoflag, int level, const char *fmt, va_list ap) {
	int errno_save = errno;
	size_t n;
	char buf[MAXLINE+1];

	vsnprintf (buf, MAXLINE, fmt, ap);
	n = strlen(buf);
	if (errnoflag)
		snprintf (buf+n, MAXLINE-n, ": %s", strerror(errno_save));
	strcat (buf, "\n");

	if (daemon_proc)
		syslog (level, "%s", buf);
	else {
		fflush (stdout);
		fputs (buf, stderr);
		fflush (stderr);
	}
}

void err_ret (const char *fmt, ...) {
	va_list ap;

	va_start (ap, fmt);
	err_doit (1, LOG_INFO, fmt, ap);
	va_end (ap);
	return;
}

void err_sys (const char *fmt, ...) {
	va_list ap;

	va_start (ap, fmt);
	err_doit (1, LOG_ERR, fmt, ap);
	va_end (ap);
	exit (1);
}

void err_msg (const char *fmt, ...) {
	va_list ap;

	va_start (ap, fmt);
	err_doit (0, LOG_INFO, fmt, ap);
	va_end (ap);
	return;
}

void err_quit (const char *fmt, ...) {
	va_list ap;

	va_start (ap, fmt);
	err_doit (0, LOG_ERR, fmt, ap);
	va_end (ap);
	exit (1);
}
