/* 
Brett's extension to PICOL tcl scipt
*/

#include <stdio.h>
#include <stdlib.h>
#include "picol.h"


/* run the 'system' Clib call */
int cmd_system( struct picolInterp *i, int argc, char **argv, void *pd){
    char buf[64];
    int ret;
    if (argc != 2) return picolArityErr(i,argv[0]);
    ret = system( argv[1] );
    snprintf(buf,64,"%ld",ret);
    picolSetResult(i,buf);
    return PICOL_OK;
}

void init_brett( struct picolInterp *i )
{
    picolRegisterCommand(i,"system", cmd_system, NULL);
    return;
}
