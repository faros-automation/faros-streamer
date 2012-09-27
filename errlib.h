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

module: errlib.h

purpose: definitions of functions in errlib.c

reference: Stevens, Unix network programming (2ed), p.922

*/

#ifndef _ERRLIB_H

#define _ERRLIB_H

#include <stdarg.h>

extern int daemon_proc;

void err_msg (const char *fmt, ...);

void err_quit (const char *fmt, ...);

void err_ret (const char *fmt, ...);

void err_sys (const char *fmt, ...);

void err_dump (const char *fmt, ...);

#endif
