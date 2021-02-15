/*
 *	ncurses listbox
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

#include <stdbool.h>
#include <ctype.h>
#include "nc-lib.h"

/*
 * returns the selected item index or -1
 */
int nc_listbox(const char *title, const char **items, int defidx) {
	int		x, y, dx, dy, c, exf = 0;
	int		i, lines, wlines, offset;
	int		maxlen = 0, pos = 0;
	bool	found;
	
	for ( lines = 0; items[lines]; lines ++ )
		maxlen = MAX(maxlen, strlen(items[lines])); 
	if ( title )
		maxlen = MAX(maxlen, strlen(title)); 
	if ( defidx < 0 || defidx >= lines )
		defidx = 0;
	
	// outer window
	dx = maxlen + 6;
	dy = MIN(LINES - 4, lines + 2);
	x = COLS / 2 - dx / 2;
	y = LINES / 2 - dy / 2;
	WINDOW	*wout = newwin(dy, dx, y, x);
	keypad(wout, TRUE);
	
	WINDOW	*w = subwin(wout, dy - 2, dx - 4, y + 1, x + 2);
	keypad(w, TRUE);
	wlines = dy - 4;

	offset = 0;
	pos = defidx;
	do {
		if ( offset > pos )	offset = pos;
		if ( offset + lines < pos )	offset = pos - lines;
		if ( offset < 0 ) offset = 0;
		werase(wout);
		box(wout, 0, 0);
		if ( title ) nc_wtitle(wout, title, 2);
		wrefresh(wout);
		werase(w);
		for ( i = offset, y = 0; i < lines; i ++, y ++ ) {
			if ( i == pos ) wattron(w, A_REVERSE);
			mvwprintw(w, y, 0, " %-*s ", maxlen, items[i]);
			if ( i == pos ) wattroff(w, A_REVERSE);
			}
		wrefresh(w);
		
		c = wgetch(w);
		switch ( c ) {
		case 'q':		pos = -1; exf ++; break;
		case '': case '':
		case '\033':	pos = -1; exf ++; break;
		case '\r': case '\n': exf ++; break;
		case KEY_UP:
			if ( pos )
				pos --;
			break;
		case KEY_DOWN:
			if ( pos < (lines - 1) )
				pos ++;
			break;
		case KEY_PPAGE:
			pos -= wlines;
			if ( pos < 0 )
				pos = 0;
			break;
		case KEY_NPAGE:
			pos += wlines;
			if ( pos > (lines - 1) )
				pos = lines - 1;
			break;
		case KEY_HOME:	offset = pos = 0; break;
		case KEY_END:	pos = lines - 1; break;
		default:
			found = false;
			if ( toupper(c) >= toupper(items[pos][0]) ) { // find next
				for ( i = pos + 1; i < lines; i ++ ) {
					if ( toupper(items[i][0]) == toupper(c) ) {
						pos = i;
						found = true;
						break;
						}
					}
				}
			if ( !found ) { // search from the beginning
				for ( i = 0; i < lines; i ++ ) {
					if ( toupper(items[i][0]) == toupper(c) ) {
						pos = i;
						found = true;
						break;
						}
					}
				}
			}
		} while ( !exf );

	// cleanup
	delwin(w);
	delwin(wout);
	refresh();
	return pos;
	}

