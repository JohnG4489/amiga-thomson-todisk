#ifndef DATALAYERFD_H
#define DATALAYERFD_H

#include "diskgeometry.h"

struct DataLayerFD
{
    HPTR Handle;
    ULONG Unit;
    ULONG Side;
    ULONG CountOfUnit;
    ULONG CountOfSideOfUnit0;
    ULONG CountOfSideOfUnit1;
};


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern BOOL DFd_CheckType(const char *, ULONG *);
extern struct DataLayerFD *DFd_Open(const char *, ULONG, ULONG, ULONG, ULONG *);
extern void DFd_Close(struct DataLayerFD *);
extern ULONG DFd_Finalize(struct DataLayerFD *);
extern ULONG DFd_FormatTrack(struct DataLayerFD *, ULONG, const UBYTE *);
extern ULONG DFd_ReadSector(struct DataLayerFD *, ULONG, ULONG, UBYTE *);
extern ULONG DFd_WriteSector(struct DataLayerFD *, ULONG, ULONG, const UBYTE *);


#endif  /* DATALAYERFD_H */