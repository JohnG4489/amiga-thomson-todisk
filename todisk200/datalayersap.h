#ifndef DATALAYERSAP_H
#define DATALAYERSAP_H

#include "diskgeometry.h"

struct DataLayerSAP
{
    HPTR Handle;
    UBYTE Buffer[262];
};


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern BOOL DSap_CheckType(const char *, ULONG *);
extern struct DataLayerSAP *DSap_Open(const char *, ULONG, ULONG *);
extern void DSap_Close(struct DataLayerSAP *);
extern ULONG DSap_Finalize(struct DataLayerSAP *);
extern ULONG DSap_FormatTrack(struct DataLayerSAP *, ULONG, const UBYTE *);
extern ULONG DSap_ReadSector(struct DataLayerSAP *, ULONG, ULONG, UBYTE *);
extern ULONG DSap_WriteSector(struct DataLayerSAP *, ULONG, ULONG, const UBYTE *);


#endif  /* DATALAYERSAP_H */