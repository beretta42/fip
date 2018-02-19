/* 
     Low level output driver need to implement this
     interface

     The major widgets are: 
             labels: text labels
	     button: a clickable button with a text label
	     check: a togglable button
	     input: an text input box

     each widget type has two methods: draw and hint. draw
     immediately causes the widget to be redrawn, and hint
     is called by the engine to get an idea of how big the
     build widget is.


*/

/* clear the entire screen */
void cls( void );
/* clear the widget's area to background */
void clear_area( struct widget *w );
/* widget methods */
void hint_label( struct widget *w );
void draw_label( struct widget *w );
void hint_button( struct widget *w );
void draw_button( struct widget *w );
void hint_check( struct widget *w );
void draw_check( struct widget *w );
void hint_input( struct widget *w );
void draw_input( struct widget *w );
/* get suggested default x and y padding from driver */
const int def_ypad;
const int def_xpad;
/* init and shutdown of low level driver */
int  ll_init();
void ll_deinit();
