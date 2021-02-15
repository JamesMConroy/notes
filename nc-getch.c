#include <stdio.h>
#include <ncurses.h>

int main(int argc, char *argv[]) {
	initscr();
	noecho(); nonl(); cbreak();
	keypad(stdscr, TRUE);
	keypad(curscr, TRUE);
	int ch = getch();
	endwin();
	printf("KEY NAME : %s - %d\n", keyname(ch), ch);
	}
