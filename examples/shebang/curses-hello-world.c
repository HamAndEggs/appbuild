#!/usr/bin/appbuild -#
#APPBUILD_LIBS ncurses

#include <ncurses.h> // sudo apt install libncurses5-dev

int main()
{	
	initscr();			/* Start curses mode 		  */
	printw("Hello World !!!");	/* Print Hello World		  */
	refresh();			/* Print it on to the real screen */
	getch();			/* Wait for user input */
	endwin();			/* End curses mode		  */

	return 0;
}
