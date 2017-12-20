/* header file for extening PICOL */

enum {PICOL_OK, PICOL_ERR, PICOL_RETURN, PICOL_BREAK, PICOL_CONTINUE};
enum {PT_ESC,PT_STR,PT_CMD,PT_VAR,PT_SEP,PT_EOL,PT_EOF};

struct picolVar {
    char *name, *val;
    struct picolVar *next;
};

struct picolInterp; /* forward declaration */
typedef int (*picolCmdFunc)(struct picolInterp *i, int argc, char **argv, void *privdata);

struct picolCmd {
    char *name;
    picolCmdFunc func;
    void *privdata;
    struct picolCmd *next;
};

struct picolCallFrame {
    struct picolVar *vars;
    struct picolCallFrame *parent; /* parent is NULL at top level */
};

struct picolInterp {
    int level; /* Level of nesting */
    struct picolCallFrame *callframe;
    struct picolCmd *commands;
    char *result;
};

void picolSetResult(struct picolInterp *i, char *s);
int picolRegisterCommand(struct picolInterp *i, char *name, picolCmdFunc f, void *privdata);
int picolArityErr(struct picolInterp *i, char *name);
