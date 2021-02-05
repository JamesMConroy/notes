#include "nc-lib.h"

//
bool nc_mvweditstr(WINDOW *w, int y, int x, char *str, int maxlen) {
	int		pos = 0, len, i, key, ocurs;
	bool	insert = true;

	len = strlen(str);
	ocurs = curs_set(1);
	while ( true ) {
		// print
		mvwprintw(w, y, x, "%s", str);
		mvwhline (w, y, x + len, ' ', maxlen - len);
		
		// get key
		key = mvwgetch(w, y, x + pos);
		
		// execute
		switch ( key ) {
		case KEY_LEFT:	if ( pos ) pos --; break;
		case KEY_RIGHT:	if ( str[pos] ) pos ++; break;
		case KEY_HOME:	pos = 0; break;
		case KEY_END:	pos = len; break;
		case KEY_BACKSPACE: case 8: case 127:
			if ( pos ) {
				pos --; len --;
				for ( i = pos; str[i]; i ++ )
					str[i] = str[i+1];
				str[len] = '\0';
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
		case KEY_CANCEL: case ERR:
		case 27: case '': case '': case '':
			curs_set(ocurs);
			return false; // canceled
		case KEY_ENTER: case '\r': case '\n':
			mvwprintw(w, y, x, "%s", str);
			mvwhline (w, y, x + len, ' ', maxlen - len);
			curs_set(ocurs);
			return true;
		default:
			if ( len < maxlen ) {
				if ( insert ) {
					for ( i = len; i >= pos; i -- )
						str[i+1] = str[i];
					str[pos] = key;
					str[++ len] = '\0';
					}
				else {
					str[pos] = key;
					if ( pos == len )
						str[++ len] = '\0';
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

