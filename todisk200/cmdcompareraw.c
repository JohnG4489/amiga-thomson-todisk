#include "system.h"
#include "global.h"
#include "cmdcompareraw.h"
#include "disklayer.h"
#include "diskgeometry.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    09-04-2011 (Seg)    Comparaison brute d'un disque avec un autre
*/


/***** Prototypes */
void Cmd_CompareRaw(struct ToDiskData *, struct DiskLayer *, struct DiskLayer *);

BOOL P_Cmd_ReadTrackToCompare(struct DiskLayer *, ULONG, UBYTE *);
ULONG P_Cmd_CompareSector(UBYTE *, UBYTE *, ULONG);


/*****
    Comparaison d'un layer avec un autre
*****/

void Cmd_CompareRaw(struct ToDiskData *TDData, struct DiskLayer *DLayerSrc, struct DiskLayer *DLayerDst)
{
    BOOL IsExit=FALSE;
    ULONG Count=0;
    ULONG Track;

    if(!DL_IsSameDiskLayer(DLayerSrc,DLayerDst))
    {
        UBYTE *Ptr=(UBYTE *)Sys_AllocMem(SIZEOF_TRACK);

        if(Ptr!=NULL)
        {
            Sys_Printf("Start comparaison...\n");

            /* On travaille sur un support different. Pas besoin de stocker en memoire */
            for(Track=0;Track<COUNTOF_TRACK && !IsExit;Track++)
            {
                IsExit=P_Cmd_ReadTrackToCompare(DLayerSrc,Track,Ptr);

                if(!IsExit)
                {
                    IsExit=P_Cmd_ReadTrackToCompare(DLayerDst,Track,TDData->Buffer);

                    if(!IsExit)
                    {
                        Count+=P_Cmd_CompareSector(Ptr,TDData->Buffer,Track);
                    }
                }
            }

            if(!IsExit)
            {
                Sys_Printf("\n%ld difference(s) found\n",(long)Count);
            }

            Sys_FreeMem((void *)Ptr);
        }
    }
}


/*****
    Compare tous les secteurs des 2 pistes
*****/

ULONG P_Cmd_CompareSector(UBYTE *TrackSrcPtr, UBYTE *TrackDstPtr, ULONG Track)
{
    ULONG Result=0;
    ULONG Sector;

    for(Sector=1; Sector<=COUNTOF_SECTOR_PER_TRACK; Sector++)
    {
        UBYTE *SectorSrcPtr=&TrackSrcPtr[(Sector-1)*SIZEOF_SECTOR];
        UBYTE *SectorDstPtr=&TrackDstPtr[(Sector-1)*SIZEOF_SECTOR];
        LONG Count=SIZEOF_SECTOR;

        Sys_Printf("\rCompare track %2ld, sector %2ld",(long)Track,(long)Sector);

        while(--Count>=0)
        {
            if(*(SectorSrcPtr++)!=*(SectorDstPtr++))
            {
                LONG Offset=SIZEOF_SECTOR-Count-1;

                Sys_Printf(": difference at offset %02lx (%ld)\n",(long)Offset,(long)Offset);
                Result++;
                break;
            }
        }
    }

    return Result;
}


/*****
    Lecture complete de la piste
*****/

BOOL P_Cmd_ReadTrackToCompare(struct DiskLayer *DLayerSrc, ULONG Track, UBYTE *BufferTrack)
{
    BOOL IsExit=FALSE;
    ULONG Sector;

    for(Sector=1; Sector<=COUNTOF_SECTOR_PER_TRACK && !IsExit; Sector++)
    {
        UBYTE *SecPtr=&BufferTrack[(Sector-1)*SIZEOF_SECTOR];

        if(!Sys_CheckCtrlC())
        {
            if(!DL_ReadSector(DLayerSrc,Track,Sector,SecPtr))
            {
                if(!DL_IsDLFatalError(DLayerSrc->Error)) Sys_Printf("Error Track %ld, Sector %ld: ",(long)Track,(long)Sector);
                else IsExit=TRUE;

                Sys_Printf("%s\n",DL_GetDLTextErr(DLayerSrc->Error));
            }
        }
        else
        {
            Sys_PrintFault(ERROR_BREAK);
            IsExit=TRUE;
        }
    }

    return IsExit;
}
