#include "system.h"
#include "datalayerfd.h"
#include "sectorcache.h"
#include "disklayer.h"


/*
    16-09-2020 (Seg)    Refonte, suite à la refonte de la lib filesystem.c
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-08-2012 (Seg)    Indique le nombre d'unités et le nombre de faces par unité de l'image disque
    24-03-2010 (Seg)    Ajout de la detection automatique du format
    25-12-2008 (Seg)    Ajout du formatage de piste
    28-08-2006 (Seg)    Gestion du format FD
*/


/***** Prototypes */
BOOL DFd_CheckType(const char *, ULONG *);
struct DataLayerFD *DFd_Open(const char *, ULONG, ULONG, ULONG, ULONG *);
void DFd_Close(struct DataLayerFD *);
ULONG DFd_Finalize(struct DataLayerFD *);
ULONG DFd_FormatTrack(struct DataLayerFD *, ULONG, const UBYTE *);
ULONG DFd_ReadSector(struct DataLayerFD *, ULONG, ULONG, UBYTE *);
ULONG DFd_WriteSector(struct DataLayerFD *, ULONG, ULONG, const UBYTE *);

ULONG P_DFd_SeekExt(struct DataLayerFD *, ULONG, ULONG);
ULONG P_DFd_GetOffset(struct DataLayerFD *, ULONG, ULONG);


/*****
    Indique s'il s'agit d'un fichier image FD
*****/

BOOL DFd_CheckType(const char *Name, ULONG *ErrorCode)
{
    BOOL Result=FALSE;
    HPTR h;

    *ErrorCode=DL_OPEN_FILE;
    if((h=Sys_Open(Name,MODE_OLDFILE))!=NULL)
    {
        LONG Size;

        Sys_Seek(h,0,OFFSET_END);
        Size=Sys_Seek(h,0,OFFSET_BEGINNING);

        if(Size==COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) Result=TRUE;
        if(Size==2*COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) Result=TRUE;
        if(Size==3*COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) Result=TRUE;
        if(Size==4*COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) Result=TRUE;

        *ErrorCode=DL_SUCCESS;

        Sys_Close(h);
    }

    return Result;
}


/*****
    Allocation des ressources necessaires pour la lecture/ecriture
    d'un fichier au format FD.
*****/

struct DataLayerFD *DFd_Open(const char *Name, ULONG IOMode, ULONG Unit, ULONG Side, ULONG *ErrorCode)
{
    struct DataLayerFD *DLayer=(struct DataLayerFD *)Sys_AllocMem(sizeof(struct DataLayerFD));

    *ErrorCode=DL_NOT_ENOUGH_MEMORY;
    if(DLayer!=NULL)
    {
        DLayer->Unit=Unit;
        DLayer->Side=Side;
        DLayer->CountOfUnit=1;
        DLayer->CountOfSideOfUnit0=1;
        DLayer->CountOfSideOfUnit1=0;

        *ErrorCode=DL_OPEN_FILE;
        if((DLayer->Handle=Sys_Open(Name,IOMode))!=NULL)
        {
            LONG Size;
            Sys_Seek(DLayer->Handle,0,OFFSET_END);
            Size=Sys_Seek(DLayer->Handle,0,OFFSET_BEGINNING);

            if(Size>COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) DLayer->CountOfSideOfUnit0=2;
            if(Size>2*COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) {DLayer->CountOfUnit=2; DLayer->CountOfSideOfUnit1=1;}
            if(Size>3*COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR) DLayer->CountOfSideOfUnit1=2;

            *ErrorCode=DL_SUCCESS;
        }

        if(*ErrorCode)
        {
            DFd_Close(DLayer);
            DLayer=NULL;
        }
    }

    return DLayer;
}


/*****
    Liberation des ressources allouees par DFd_Close()
*****/

void DFd_Close(struct DataLayerFD *DLayer)
{
    if(DLayer!=NULL)
    {
        if(DLayer->Handle!=NULL)
        {
            /*Flush(DLayer->Handle);*/
            Sys_Close(DLayer->Handle);
        }

        Sys_FreeMem((void *)DLayer);
    }
}


/*****
    Permet de terminer les operations en cache, avant de fermer
    le DataLayer.
*****/

ULONG DFd_Finalize(struct DataLayerFD *DLayer)
{
    ULONG ErrorCode=P_DFd_SeekExt(DLayer,COUNTOF_TRACK,1);
    Sys_Flush(DLayer->Handle);

    return ErrorCode;
}


/*****
    Formatage d'une piste
*****/

ULONG DFd_FormatTrack(struct DataLayerFD *DLayer, ULONG Track, const UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_SUCCESS;
    LONG Sector;

    for(Sector=1; !ErrorCode && Sector<=COUNTOF_SECTOR_PER_TRACK; Sector++)
    {
        ErrorCode=DFd_WriteSector(DLayer,Track,Sector,&BufferPtr[(Sector-1)*SIZEOF_SECTOR]);
    }

    return ErrorCode;
}


/*****
    Lecture d'un secteur
*****/

ULONG DFd_ReadSector(struct DataLayerFD *DLayer, ULONG Track, ULONG Sector, UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_SUCCESS;
    LONG Count=0;
    ULONG Offset=P_DFd_GetOffset(DLayer,Track,Sector);

    if(Sys_Seek(DLayer->Handle,Offset,OFFSET_BEGINNING)>=0)
    {
        ErrorCode=DL_READ_FILE;
        Count=Sys_Read(DLayer->Handle,BufferPtr,SIZEOF_SECTOR);
    }

    if(Sys_IoErr()<=0)
    {
        /* On complete le secteur avec des zeros si le fichier est incomplet */
        while(Count<SIZEOF_SECTOR) BufferPtr[Count++]=0;
        ErrorCode=DL_SUCCESS;
    }

    return ErrorCode;
}


/*****
    Ecriture d'un secteur
*****/

ULONG DFd_WriteSector(struct DataLayerFD *DLayer, ULONG Track, ULONG Sector, const UBYTE *BufferPtr)
{
    ULONG ErrorCode=P_DFd_SeekExt(DLayer,Track,Sector);

    if(!ErrorCode)
    {
        Sys_Write(DLayer->Handle,(void *)BufferPtr,SIZEOF_SECTOR);
        if(Sys_IoErr()>0) ErrorCode=DL_WRITE_FILE;
    }

    return ErrorCode;
}


/*****
    Positionne le curseur a l'offset donne.
    Note: Cette fonction crée le header, voire agrandit le fichier si besoin.
*****/

ULONG P_DFd_SeekExt(struct DataLayerFD *DLayer, ULONG Track, ULONG Sector)
{
    LONG Offset=P_DFd_GetOffset(DLayer,Track,Sector);
    LONG Size;

    /* On récupère la taille du fichier et on se repositionne au début */
    Sys_Seek(DLayer->Handle,0,OFFSET_END);
    Size=Sys_Seek(DLayer->Handle,0,OFFSET_BEGINNING);

    if(Size>=0)
    {
        ULONG Tmp=0;

        if(Size>Offset) Size=Offset;
        Sys_Seek(DLayer->Handle,Size,OFFSET_BEGINNING);
        for(; Size<Offset && Sys_IoErr()<=0; Size++) Sys_PutC(DLayer->Handle,Tmp);
    }

    if(Sys_IoErr()<=0) return DL_SUCCESS;
    return DL_WRITE_FILE;
}


/*****
    Retourne l'offset dans le fichier correspondant a la piste et au secteur donne
*****/

ULONG P_DFd_GetOffset(struct DataLayerFD *DLayer, ULONG Track, ULONG Sector)
{
    ULONG Offset;

    Offset=2*DLayer->Unit*SIZEOF_SIDE+DLayer->Side*SIZEOF_SIDE;
    Offset+=Track*SIZEOF_TRACK+(Sector-1)*SIZEOF_SECTOR;

    return Offset;
}