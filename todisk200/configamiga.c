#include "system.h"

#ifdef SYSTEM_AMIGA
#include "configamiga.h"

/*
    16-09-2013 (Seg)    Gestion de la ligne commande facon Amiga
*/


struct RDArgs *RDArgs=NULL;

/***** Prototypes */
LONG Cfg_ParseArg(int, const char *[], LONG *);
void Cfg_FreeArg(LONG *);


/*****
    Recherche les paramètres de la ligne de commande
*****/

LONG Cfg_ParseArg(int narg, const char *argv[], LONG *MyPars)
{
    LONG Error=0;

    if((RDArgs=ReadArgs(
        "FILES/M,DSK=K7,"
        "DIR/S,LOAD=COPY,SAVE,DELETE,SORTDIR/S,"
        "CMPDSKRAW,CMPDSKDOS,FINDDSK,NORFD/S,"
        "FORMATFLP,FORMATTDS,FORMATSAP,FORMATFD,"
        "TOFLP,TOTDS,TOSAP,TOFD,NOFMT/S,"
        "DUMPSECTOR,"
        "VIEW,VIEWMAP,VIEWAND,DUMP=VIEWBIN,"
        "UNPROT/S,CONVASC/S,CONVBAS/S,CONVASS/S,CONVASM/S,CONVMAP/S,CONVALL/S,"
        "IMPORTTXT,MAPMODE/N,MAPCFG",MyPars,NULL))==NULL)
    {
        Error=Sys_IoErr();
    }

    return Error;
}


/*****
    Libération des ressources allouées
*****/

void Cfg_FreeArg(LONG *MyPars)
{
    FreeArgs(RDArgs);
    RDArgs=NULL;
}

#endif
