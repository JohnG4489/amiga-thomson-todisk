#ifndef DATALAYERTDS_H
#define DATALAYERTDS_H

#include "diskgeometry.h"
#include "sectorcache.h"

struct DataLayerTDS
{
    HPTR Handle;
    ULONG IOMode;
    ULONG Unit;
    ULONG Side;
    UBYTE Bitmap[160];
    ULONG SectorOffset[COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK];
    struct SectorCache SectorCache;
};


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern BOOL DTds_CheckType(const char *, ULONG *);
extern struct DataLayerTDS *DTds_Open(const char *, ULONG, ULONG, ULONG, ULONG *);
extern void DTds_Close(struct DataLayerTDS *);
extern ULONG DTds_Finalize(struct DataLayerTDS *);
extern ULONG DTds_FormatTrack(struct DataLayerTDS *, ULONG, const UBYTE *);
extern ULONG DTds_ReadSector(struct DataLayerTDS *, ULONG, ULONG, UBYTE *);
extern ULONG DTds_WriteSector(struct DataLayerTDS *, ULONG, ULONG, const UBYTE *);

#endif  /* DATALAYERTDS_H */