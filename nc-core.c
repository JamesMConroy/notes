/*
 *	ncurses additional tools
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

#include "nc-lib.h"

typedef struct { int fg, bg, id; } pair_t;
static pair_t*	pair_table = NULL;
static int		pair_count = 0;

static int to_vga[] = 
// 0  1  2  3  4  5  6  7  8   9  10  11  12  13  14  15
{  0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14,  9, 13, 11, 15 };

//#define COLOR_DGRAY		0xEC
//#define COLOR_GRAY		0xF0

// create a new color pair and store it to pair_table
int nc_createpair(int fg, int bg) {
	if ( pair_table )
		pair_table = (pair_t *) realloc(pair_table, sizeof(pair_t) * (pair_count+1));
	else
		pair_table = (pair_t *) malloc(sizeof(pair_t) * (pair_count+1));
	pair_table[pair_count].fg = fg;
	pair_table[pair_count].bg = bg;
	pair_table[pair_count].id = 0x10 + pair_count;
	init_pair(pair_table[pair_count].id, fg, bg);
	pair_count ++;
	return pair_table[pair_count - 1].id;
	}
int nc_createvgapair(int fg, int bg) { return nc_createpair(to_vga[fg & 0xf], to_vga[bg &0xf]); }

// get the pair no of fg/bg, creates a new one if not found
int nc_getpairof(int fg, int bg) {
	if ( pair_table ) {
		for ( int i = 0; i < pair_count; i ++ )
			if ( pair_table[i].fg == fg && pair_table[i].bg == bg )
				return pair_table[i].id;
		}
	return nc_createpair(fg, bg);
	}

// display a title on window 'win'
// align can be 0: left, 1: right, 2: center
void nc_wtitle(WINDOW *win, const char * title, int align) {
	wattron(win, A_BOLD);
	switch ( align ) {
	case 0:	// left
		mvwprintw(win, 0, 2, " %s ", title);
		break;
	case 1: // right
		mvwprintw(win, 0, getmaxx(win) - (strlen(title)+2) - 2, " %s ", title);
		break;
	case 2: // center
		mvwprintw(win, 0, getmaxx(win) / 2 - (strlen(title) + 2) / 2, " %s ", title);
		break;
		}
	wattroff(win, A_BOLD);
	}

// hexadecimal character to integer
static int c2dec(char c) {
	c = toupper(c);
	if ( c >= 'A' && c <= 'F' )	return 10 + (c - 'A');
	return (c - '0');
	}

static void nc_clrpairs() {
	nc_createpair(COLOR_WHITE, COLOR_BLACK);
	}

// initialize ncurses
void nc_init() {
	initscr();
	noecho(); nonl(); cbreak();
	keypad(stdscr, TRUE);
	set_tabsize(4);
	curs_set(0);

	if ( has_colors() ) {
		start_color();
		use_default_colors();
		nc_clrpairs();
		}
	}

// close ncurses
void nc_close() {
	endwin();
	if ( pair_table )
		free(pair_table);
	pair_table = NULL;
	pair_count = 0;
	}

void nc_setvgacolor(WINDOW *win, int fg, int bg)	{ wattron (win, COLOR_PAIR(nc_getpairof(to_vga[fg & 0xF], to_vga[bg & 0xF]))); }
void nc_unsetvgacolor(WINDOW *win, int fg, int bg)	{ wattroff(win, COLOR_PAIR(nc_getpairof(to_vga[fg & 0xF], to_vga[bg & 0xF]))); }

void nc_setcolor(WINDOW *win, int fg, int bg)	{ wattron (win, COLOR_PAIR(nc_getpairof(fg, bg))); }
void nc_unsetcolor(WINDOW *win, int fg, int bg)	{ wattroff(win, COLOR_PAIR(nc_getpairof(fg, bg))); }

void nc_setpair  (WINDOW *win, int pair) { wattron  (win, COLOR_PAIR(pair)); }
void nc_unsetpair(WINDOW *win, int pair) { wattroff (win, COLOR_PAIR(pair)); }

static void nc_hclr(WINDOW *win, int mode, char fg, char bg) {
	if ( mode ) 
		nc_setvgacolor(win, c2dec(fg), c2dec(bg)); 
	else
		nc_unsetvgacolor(win, c2dec(fg), c2dec(bg)); 
	}
static void nc_hpair(WINDOW *win, int mode, char d1, char d0) {
	if ( mode )
		wattron (win, COLOR_PAIR((c2dec(d1) << 4) | c2dec(d0)));
	else
		wattroff(win, COLOR_PAIR((c2dec(d1) << 4) | c2dec(d0)));
	}

// print with codes/colors
#define ccase(c,a) case (c): ( p[1] == '+' ) ? wattron(win, (a)) : wattroff(win, (a)); p ++; break;

void nc_mvwprintf(WINDOW *win, int y, int x, const char *fmt, ...) {
	char	msg[LINE_MAX], buf[LINE_MAX];
	char	*p = msg, *b = buf;

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, LINE_MAX, fmt, ap);
	va_end(ap);

	if ( y >= 0 && x >= 0 )
		wmove(win, y, x);
	else if ( y >= 0 )
		wmove(win, y, getcurx(win));
	else if ( x >= 0 )
		wmove(win, getcury(win), x);
		
	while ( *p ) {
		switch ( *p ) {
		case 27:
			p ++;
			if ( b > buf ) { *b = '\0'; waddstr(win, buf); b = buf; }
			switch ( *p ) {
			ccase('b', A_BOLD)
			ccase('r', A_REVERSE)
			ccase('d', A_DIM)
			ccase('u', A_UNDERLINE)
			case 'c': // vga color
				p ++;
				if ( isxdigit(*p) && isxdigit(p[1]) ) {
					if ( has_colors() )
						nc_hclr(win, (p[2] == '+'), p[0], p[1]);
					p += 2;
					}
				break;
			case 'C': // select pair
				p ++;
				if ( isxdigit(*p) && isxdigit(p[1]) ) {
					if ( has_colors() )
						nc_hpair(win, (p[2] == '+'), p[0], p[1]);
					p += 2;
					}
				break;
			default: // not recognized
				*b ++ = '\033';
				*b ++ = *p;
				}
			break;
		default:
			*b ++ = *p;
			}
		p ++;
		}
	if ( b > buf ) { *b = '\0'; waddstr(win, buf); b = buf; }
	}
#undef ccase

#define nc_wprintf(w,f,...)	nc_mvwprintf(w,-1,-1,f,__VA_ARGS__)
#define nc_printf(f,...)	nc_mvwprintf(stdscr,-1,-1,f,__VA_ARGS__)


