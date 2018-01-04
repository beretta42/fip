
struct boxdata {
    int xpos;
    int ypos;
    int xpad;
    int ypad;
    int max;
    int type;
};

struct inputdata {
    int size;
    int cur;
    char *buf;
};


struct radiodata {
    struct widget *cur;
};


struct widget {
    int type;                 /* widget type */
#define  LABEL    0           /* a text label */
#define  CHECK    1           /* check list menu item */
#define  BUTTON   2           /* button */
#define  INPUT    3           /* an input box */
#define  BOX      4           /* an area widget (box) */
#define  ICON     5           /* some drawn graphics */
#define  SEPAR    6           /* horizontal separator */
    int x;                    /* X coord of widget */
    int y;                    /* Y coord of widget */
    int w;                    /* width of widget */
    int h;                    /* height of widget */
    char *text;               /* text field of widget */
    int stat;                 /* state of widget */
#define  SELECTED      1      /* this menu item is selected */
#define  FOCUSED       2      /* this widget is taking input */
#define  FOCUSABLE     4      /* this widget can take input */
#define  DIRTY         8      /* this widget needs redrawing */
#define  HIDDEN        16     /* This widget is hidden */
    struct widget *mom;       /* parent widget */
    struct widget *sib;       /* next sibling */
    struct widget *child;     /* linked list of children */
    union {                   /* widget specific data */
	struct inputdata input_data;
	struct boxdata   box_data;
	struct radiodata *radio_data;
    } data;
    void (*draw)  (struct widget *);  /* do draw */
    void (*clicked) (struct widget *);  /* was clicked */
    void (*focus) (struct widget *);  /* was focused */
    void (*nudge) (struct widget *);  /* was resized, controlled, etc */
    void (*pack)  (struct widget *, struct widget *child);  /* was packed */
    void (*post)  (struct widget *); /* post packing */ 
};


extern int def_ypad;
extern int def_xpad;

void draw_button(struct widget *w);
void draw_check(struct widget *w);
void draw_label(struct widget *w);
