/*
 *	ncurses - additional tools
 * 
 *	Copyright (C) 2017-2021 Nicholas Christopoulos.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 */

#ifndef NDC_NCLIB_H_
#define NDC_NCLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "str.h"
#include <stdlib.h>
#include <ncurses.h>

#ifndef MAX
	#define MAX(a,b)	((a>b)?a:b)
	#define MIN(a,b)	((a<b)?a:b)
#endif

void nc_init ();
void nc_close();

void nc_color_on (WINDOW *win, int fg, int bg); // note: VGA colors
void nc_color_off(WINDOW *win, int fg, int bg); // note: VGA colors

void nc_wtitle(WINDOW *win, const char * title, int align);

//
void nc_mvwprintf(WINDOW *win, int y, int x, const char *fmt, ...);
#define nc_wprintf(w,f,...)	nc_mvwprintf(w,-1,-1,f,__VA_ARGS__)
#define nc_printf(f,...)	nc_mvwprintf(stdscr,-1,-1,f,__VA_ARGS__)

//
bool nc_mvweditstr(WINDOW *w, int y, int x, char *str, int maxlen);
bool nc_mvwreadstr(WINDOW *w, int y, int x, char *str, int maxlen);
bool nc_mveditstr(int y, int x, char *s, int m);
bool nc_mvreadstr(int y, int x, char *s, int m);
bool nc_editstr(char *s, int m);
bool nc_readstr(char *s, int m);
bool nc_weditstr(WINDOW *w, char *s, int m);
bool nc_wreadstr(WINDOW *w, char *s, int m);

//
void nc_view(const char *title, const char *body);
int  nc_listbox(const char *title, const char **items, int default_index /* = 0 */);

#ifdef __cplusplus
}
#endif

#endif
