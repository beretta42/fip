/* 

A vt52 / Fuzix VT driver

*/

#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include "snazzy.h"


int def_ypad = 0;
int def_xpad = 1;         

struct termios prev;
struct termios new;
struct winsize sizes;


/* clear screen */
void cls( void )
{
    printf("\x1bH\x1bJ");
}

/* set cursor coords */
void sgoto( int x, int y)
{
    printf("\x1bY%c%c", ' '+y, ' '+x);
}

/* set color */
void color( int fd, int bg )
{
    printf("\x1b""b%c\x1b""c%c", '0'+fd, '0'+bg);
}


void hint_label( struct widget *w )
{
    w->w = strlen(w->text);
    w->h = 1;
    if (w->stat & FOCUSABLE)
	w->w += 2;
}

void draw_label( struct widget *w )
{
    char *f = "";
    sgoto(w->x, w->y);
    if (w->stat & FOCUSABLE)
	f = w->stat & FOCUSED ? "->" : "  ";
    printf("%s%s", f, w->text);
    fflush(stdout);
}

/* set height and width of button */
void hint_button( struct widget *w )
{
    w->w = strlen(w->text) + 4;
    w->h = 1;
}

void draw_button( struct widget *w )
{
    sgoto(w->x, w->y);
    printf("%s[%s]", 
	   w->stat & FOCUSED ? "->" : "  ", 
	   w->text);
    fflush(stdout);
}

void hint_check( struct widget *w )
{
    w->w = strlen(w->text) + 6;
    w->h = 1;
}

void draw_check( struct widget *w )
{
    sgoto(w->x, w->y);
    printf("%s<%c> %s", 
	   w->stat & FOCUSED ? "->" : "  ",
	   w->stat & SELECTED ? '*' : ' ',
	   w->text);
    fflush(stdout);
}


void draw_input( struct widget *w )
{
    sgoto(w->x, w->y);
    printf("%s%s: %s",
	   w->stat & FOCUSED ? "->" : "  ",
	   w->text,
	   w->text);
    fflush(stdout);
}

void clear_area( struct widget *w )
{
    static char *lb[256];
    int h = w->h;
    int x = w->x;
    int y = w->y;
    memset( &lb, ' ', w->w );
    lb[w->w] = 0;
    while (h--){
	sgoto(x, y++);
	puts(lb);
    }
}

void ll_deinit( void )
{
    /* reset tty to normal */
    tcsetattr(1, TCSANOW, &prev);
}


int ll_init( void )
{
    /* save screen settings */
    tcgetattr(1, &prev);
    tcgetattr(1, &new);
    /* set up terminal */
    cfmakeraw( &new );
    tcsetattr(1, TCSANOW, &new);
    /* get screen size */
    if (ioctl(1,TIOCGWINSZ, &sizes) < 0){
	ll_deinit();
	perror("ioctl");
	exit(-1);
    }
    /* fixme: fuzix doesn't really do this */
    sizes.ws_row = 25; sizes.ws_col = 80;
    /* fixme: make the root window here */
}


