#include <wchar.h>
#include <stdlib.h>
#include "nc-lib.h"

//
bool nc_mvweditstr(WINDOW *w, int y, int x, char *u8str, int maxlen) {
	int		pos = 0, len, i, ocurs, key;
	bool	insert = true;
	wchar_t *str, *wcs, wch;
	char	*s;

	wcs = u8towcs(u8str);
	str = (wchar_t *) malloc(sizeof(wchar_t) * LINE_MAX);
	wcscpy(str, wcs);
	free(wcs);
	len = wcslen(str);
	ocurs = curs_set(1);
	while ( true ) {
		// print
		
		s = wcstou8(str);
		if ( s ) {
			mvwprintw(w, y, x, "%s", s);
			free(s);
			}
		mvwhline (w, y, x + len, ' ', maxlen - len);
		
		// get key
		key = mvwgetch(w, y, x + pos);
		if ( key > 0x7f && key < 0xff && (key & 0xE0) == 0xC0 ) {
			char mbs[5];
			int keylen = 1;
			mbs[0] = key;
			if ( (key & 0xF8) == 0xF0 ) keylen = 4;
			else if ( (key & 0xF0) == 0xE0 ) keylen = 3;
			else if ( (key & 0xE0) == 0xC0 ) keylen = 2;
			for ( i = 1; i < keylen; i ++ )
				mbs[i] = wgetch(w);
			mbs[keylen] = '\0';
			mbtowc(&wch, mbs, keylen);
			}
		else
			wch = (wchar_t) key;
		
		// execute
		switch ( key ) {
		case KEY_LEFT:	if ( pos ) pos --; break;
		case KEY_RIGHT:	if ( str[pos] ) pos ++; break;
		case KEY_HOME:	pos = 0; break;
		case KEY_END:	pos = len; break;
		case '\010': case '\x7f':
		case KEY_BACKSPACE:
			if ( pos ) {
				pos --; len --;
				for ( i = pos; str[i]; i ++ )
					str[i] = str[i+1];
				str[len] = L'\0';
				}
			break;
		case KEY_DC:
			if ( str[pos] ) {
				for ( i = pos; str[i]; i ++ )
					str[i] = str[i+1];
				len --;
				}
			break;
		case KEY_IC:	
			insert = !insert;
			curs_set((insert)?1:2);
			break;
		case 27: case '': case '': case '':
		case KEY_CANCEL:
			curs_set(ocurs);
			free(str);
			return false; // canceled
		case '\r': case '\n':
		case KEY_ENTER: 
			mvwprintw(w, y, x, "%ls", str);
			mvwhline (w, y, x + len, ' ', maxlen - len);
			curs_set(ocurs);
			s = wcstou8(str); strcpy(u8str, s); free(s);
			free(str);
			return true;
		default:
			if ( len < maxlen ) {
				if ( insert ) {
					for ( i = len; i >= pos; i -- )
						str[i+1] = str[i];
					str[pos] = wch;
					str[++ len] = L'\0';
					}
				else {
					str[pos] = key;
					if ( pos == len )
						str[++ len] = L'\0';
					}
				pos ++;
				}
			}
		}
	return false; // never comes here
	}

//
bool nc_mvwreadstr(WINDOW *w, int y, int x, char *str, int maxlen) {
	*str = '\0';
	return nc_mvweditstr(w, y, x, str, maxlen);
	}

/////
bool nc_mveditstr(int y, int x, char *s, int m)	{ return nc_mvweditstr(stdscr,y,x,s,m); }
bool nc_mvreadstr(int y, int x, char *s, int m)	{ return nc_mvwreadstr(stdscr,y,x,s,m); }
bool nc_editstr(char *s, int m)					{ return nc_mvweditstr(stdscr,getcury(stdscr),getcurx(stdscr),s,m); }
bool nc_readstr(char *s, int m)					{ return nc_mvwreadstr(stdscr,getcury(stdscr),getcurx(stdscr),s,m); }
bool nc_weditstr(WINDOW *w, char *s, int m)		{ return nc_mvweditstr(w,getcury(w),getcurx(w),s,m); }
bool nc_wreadstr(WINDOW *w, char *s, int m)		{ return nc_mvwreadstr(w,getcury(w),getcurx(w),s,m); }

