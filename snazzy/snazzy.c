/*
A dialog type of thing
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "snazzy.h"
#include "ll.h"

/* these defs don't go here */
#define BLACK    0
#define BLUE     1
#define RED      2
#define MAGENTA  3
#define GREEN    4
#define CYAN     5
#define YELLOW   6
#define WHITE    7


struct widget *root = NULL;

/* for iterator functions */
struct widget *obj = NULL;
struct widget *iobj = NULL;

#define LISTZ 64
struct widget *list[LISTZ];
int cur = 0;
int sptr = 0;


/* (re)start iterating over tree of widgets */
void istart( struct widget *w )
{
    iobj = obj = w;
}

/* get next widget in tree (depth first pre) */
struct widget *getnext( void )
{
    struct widget *ret = obj;
    if (!obj) return ret;
    /* have children then goto first */
    if (obj->child){
	obj = obj->child;
	return ret;
    }
    /* if not get my sibling */
    if (obj->sib){
	obj = obj->sib;
	return ret;
    }
    /* quit when we get back to origin */
    if (obj->mom == iobj ){
	obj = 0;
	return ret;
    }
    /* fixme: not needed?: if I have a mother goto her sibling */
    if (obj->mom){
	obj = obj->mom->sib;
	return ret;
    }
    /* if not then I'm the moms' MOM */
    obj = 0;
    return ret;
}

/* get next sibling */
void nextsib( void )
{
    if (obj == iobj){
	obj = 0;
	return;
    }
    if (obj->mom)
	obj = obj->mom->sib;
}

void printw( struct widget *w ){
    char c;
    struct widget *i;
    istart(root);
    while (i = getnext() ){
	fprintf(stderr,"%x: mom %x sib %x child %x %d %s\n", i, i->mom, i->sib, i->child, i->type, i->text);
	read(0,&c,1);
    }
}


/* build a list of selectable widget */
void select_first( struct widget *s )
{
    struct widget *i;
    istart(s);
    sptr = 0;
    while (i=getnext() ){
	if (i->stat & HIDDEN )
	    nextsib();
	else if (i->stat & FOCUSABLE)
	    list[sptr++] = i;
    }
    cur = 0;
    list[cur]->stat |= FOCUSED | DIRTY;
}

void select_off( void ){
    list[cur]->stat &= ~FOCUSED;
    list[cur]->stat |= DIRTY;
}

void select_warp( int i ){
    select_off();
    cur = i;
    list[cur]->stat |= DIRTY | FOCUSED;
}

void select_next( void )
{
    if(cur+1 < sptr)
	select_warp( cur+1 );
}


void select_prev( void )
{
    if(cur)
	select_warp( cur-1 );
}

void select_click( void )
{
    if (list[cur]->clicked)
	(*(list[cur]->clicked))(list[cur]);
}

void display_widgets( struct widget *s )
{
    struct widget *i;
    istart(s);
    while (i = getnext() ){
	if (i->stat & HIDDEN){
	    nextsib();
	}
	else if (i->stat & DIRTY){
	    i->stat &= ~DIRTY;
	    if (i->draw)
		(*(i->draw))(i);
	}
    }
}

/* make dirty any primitives (no children) that intersect widget*/
void dirty_rect( struct widget *w )
{
    struct widget *i;
    istart(root);
    while (i = getnext()){
	if(
	   (i->x + i->w) < (w->x) ||
	   (i->x) > (w->x + w->w) ||
	   (i->y + i->h) < (w->y) ||
	   (i->y) > (w->y + w->h) ){
	    nextsib();
	    continue;
	}
	i->stat |= DIRTY;
    }
}


int input( void )
{
    char c;
    while (1){
	read(0,&c,1);
	switch (c){
	case 'j':
	    select_next();
	    break;
	case 'k':
	    select_prev();
	    break;
	case 'q':
	    return -1;
	case ' ':
	case '\n':
	    select_click();
	    break;
	case 0x1b:
	    return -1;
	}
	display_widgets(root);
    }
}


void myexit( int ret ) 
{
    ll_deinit();
    exit(ret);
}

struct widget *new_widget( void )
{
    struct widget *w = malloc(sizeof(struct widget));
    if (w == NULL)
	return NULL;
    memset(w, 0, sizeof(struct widget));
    list[sptr++] = w;
    return w;
}

void clicked_check( struct widget *w)
{
    w->stat ^= SELECTED;
    w->stat |= DIRTY;
}

struct widget *check(char *text)
{
    struct widget *w = new_widget();
    if (w == NULL)
	return NULL;
    w->type = CHECK;
    w->text = text;
    w->draw = draw_check;
    w->clicked = clicked_check;
    hint_check( w );
    w->stat |= DIRTY | FOCUSABLE;
    return w;
}


void clicked_radio( struct widget *w )
{
    struct radiodata *r = w->data.radio_data;
    r->cur->stat &= ~SELECTED;
    r->cur->stat |= DIRTY;
    r->cur = w;
    r->cur->stat |= SELECTED | DIRTY;
}

struct widget *radio(char *text, struct radiodata *d)
{
    struct widget *w = check(text);
    if (w == NULL)
	return NULL;
    w->clicked = clicked_radio;
    w->data.radio_data = d;
    return w;
}

struct widget *button(char *text, void(*f)(struct widget *) )
{
    struct widget *w = new_widget();
    if (w == NULL )
	return NULL;
    w->type = BUTTON;
    w->text = text;
    w->draw = draw_button;
    w->clicked = f;
    hint_button( w );
    w->stat |= DIRTY | FOCUSABLE;
    return w;
}

struct widget *label(char *text)
{
    struct widget *w = new_widget();
    if (w == NULL )
	return NULL;
    w->type = LABEL;
    w->text = text;
    w->draw = draw_label;
    hint_label( w );
    w->stat |= DIRTY;
    return w;
}



/* Needs factoring */
void box_pack( struct widget *w, struct widget *c)
{
    struct boxdata *b;
    b = &w->data.box_data;
    switch (b->type){
    case 0:
	if (b->xpos + c->w + b->xpad > w->w){
	    b->xpos = b->xpad;
	    b->ypos += b->max + b->ypad;
	    b->max = 0;
	}
	c->x = w->x + b->xpos;
	c->y = w->y + b->ypos;
	b->xpos += c->w + b->xpad;
	if (b->max < c->h) 
	    b->max = c->h;
	break;
    case 1:
	if( b->ypos + c->h + b->ypad > w->h){
	    b->ypos = b->ypad;
	    b->xpos += b->max + b->xpad;
	    b->max = 0;
	}
	c->x = w->x + b->xpos;
	c->y = w->y + b->ypos;
	b->ypos += c->h + b->ypad;
	if (b->max < c->w)
	    b->max = c->w;
	break;
    }
}

void box_draw( struct widget *w)
{
    display_widgets( w->child );
}


#define HBOX 0
#define VBOX 1
struct widget *box( int type )
{
    struct widget *w = new_widget();
    struct boxdata *b;
    if (w == NULL)
	return NULL;
    b = &w->data.box_data;
    w->type = BOX;
    w->draw = box_draw;
    w->pack = box_pack;
    w->stat |= DIRTY;
    b->xpad = def_xpad;
    b->ypad = def_ypad;
    b->xpos = w->x + b->xpad;
    b->ypos = w->y + b->ypad;
    b->type = type;
    return w;
}




void setgeom_widget( struct widget *wid, int x, int y, int w, int h)
{
    wid->x = x;
    wid->y = y;
    wid->w = w;
    wid->h = h;
}

struct widget *pack_widget( struct widget *p, struct widget *c )
{
    struct widget *last;
    c->sib = NULL;
    c->mom = p;
    last = p->child;
    if (!last){
	p->child = c;
    } 
    else {
	while (last->sib) last = last->sib;
	last->sib = c;
    }
    if (p->pack)
	(*(p->pack))(p,c);
    return c;
}

void post_widget( struct widget *w )
{
    if (w->post)
	(*(w->post))(w);
}

struct widget *menu;

struct widget *menu_item( char *text )
{
    struct widget *w;
    w = label(text);
    w->stat |= FOCUSABLE;
    hint_label(w);
    return w;
}

void clicked_exit( struct widget *w ){
    menu->stat |= HIDDEN;
    select_first(root);
    clear_area(menu);
    dirty_rect(menu);
}


void clicked_menu( struct widget *w ) 
{
    menu->stat &= ~HIDDEN;
    menu->stat |= DIRTY;
    clear_area(menu);
    select_off();
    select_first(menu);
    select_warp(4);
}

void post_menu( struct widget *w )
{
    struct widget *i;
    int no = 0;
    int y = w->y;
    w->h = 0;
    w->w = 0;
    istart(w);
    getnext(); /* skip w itself */
    while (i = getnext()){
	no++;
	fprintf(stderr,"%s %d %d\n", i->text, w->w, i->w);
	if (i->w > w->w ) w->w = i->w;
	w->h += i->h;
	i->x = w->x;
	i->y = y;
	y += i->h;
    }
}

int main( int argc, char *argv[] )
{
    char c;
    struct widget *top;
    struct widget *mid;
    struct widget *bottom;
    struct widget *vbottom;
    struct radiodata rad;
    cls();
    ll_init();
    /* setting root should be done via ll_init ? */
    root = box(VBOX);
    setgeom_widget(root,0,0,80,25);
    /* */
    top = box(HBOX);
    setgeom_widget(top,0,0,80,4);
    pack_widget( root, top );
    mid = label("This is a Label");
    pack_widget( root, mid );

    bottom = box(VBOX);
    setgeom_widget(bottom,0,0,80,5);
    pack_widget( root, bottom);

    vbottom = box(HBOX);
    setgeom_widget(vbottom,0,0,80,1);
    pack_widget( root, vbottom);

    pack_widget( top, button("Menu", clicked_menu) );
    pack_widget( top, button("Ok", NULL) );
    pack_widget( top, button("Cancel", NULL) );
    pack_widget( top, button("one", NULL) );
    pack_widget( top, button("two", NULL) );
    pack_widget( top, button("three", NULL) );

    pack_widget( bottom, check("Awesome!") );
    pack_widget( bottom, check("cool!") );
    pack_widget( bottom, check("radical!") );
    pack_widget( bottom, check("narly!") );
    pack_widget( bottom, check("blah de blah") );
    pack_widget( bottom, check("42!") );
    pack_widget( bottom, label("does this works?") );


    pack_widget( vbottom, radio("mutex", &rad) );
    pack_widget( vbottom, radio("xetum", &rad) );
    pack_widget( vbottom, radio("now now", &rad) );

    menu = box(VBOX);
    menu->post = post_menu;
    pack_widget( root, menu );
    setgeom_widget(menu,3,1,15,5);
    pack_widget( menu, menu_item("Open") );
    pack_widget( menu, menu_item("Print") );
    pack_widget( menu, menu_item("File") );
    pack_widget( menu, menu_item("Run") );
    pack_widget( menu, menu_item("Exit") )->clicked = clicked_exit;
    post_widget( menu );
    menu->stat |= HIDDEN;

    select_first(root);
		 
    display_widgets(root);
    while ( input() >= 0 );
    ll_deinit();
    myexit(0);
}

