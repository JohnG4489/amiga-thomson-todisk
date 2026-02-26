#include "system.h"
#include "datalayersap.h"
#include "sectorcache.h"
#include "disklayer.h"


/*
    16-09-2020 (Seg)    Refonte, suite à la refonte de la lib filesystem.c
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    24-03-2010 (Seg)    Ajout de la detection automatique du format
    25-12-2008 (Seg)    Ajout du formatage de piste
    22-09-2006 (Seg)    Gestion du format SAP
*/


UWORD SapCRCTable[]=
{
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
};

char SapHeader[]="\1SYSTEME D'ARCHIVAGE PUKALL S.A.P. (c) Alexandre PUKALL Avril 1998";


/***** Prototypes */
BOOL DSap_CheckType(const char *, ULONG *);
struct DataLayerSAP *DSap_Open(const char *, ULONG, ULONG *);
void DSap_Close(struct DataLayerSAP *);
ULONG DSap_Finalize(struct DataLayerSAP *);
ULONG DSap_FormatTrack(struct DataLayerSAP *, ULONG, const UBYTE *);
ULONG DSap_ReadSector(struct DataLayerSAP *, ULONG, ULONG, UBYTE *);
ULONG DSap_WriteSector(struct DataLayerSAP *, ULONG, ULONG, const UBYTE *);

ULONG P_DSap_SeekExt(struct DataLayerSAP *, ULONG, ULONG);
void P_DSap_EncodeSector(const UBYTE *, ULONG, ULONG, UBYTE [262]);
void P_DSap_DecodeSector(UBYTE *, LONG);
ULONG P_DSap_GetOffset(ULONG, ULONG);
ULONG P_DSap_CRC_Pukall(ULONG, ULONG);


/*****
    Indique s'il s'agit d'un fichier image SAP
*****/

BOOL DSap_CheckType(const char *Name, ULONG *ErrorCode)
{
    BOOL Result=FALSE;
    HPTR h;

    *ErrorCode=DL_OPEN_FILE;
    if((h=Sys_Open(Name,MODE_OLDFILE))!=NULL)
    {
        UBYTE Tmp[sizeof(SapHeader)];

        *ErrorCode=DL_READ_FILE;
        if(Sys_Read(h,Tmp,sizeof(Tmp))==sizeof(Tmp))
        {
            LONG i;

            Result=TRUE;
            for(i=0;i<sizeof(Tmp);i++) if(Tmp[i]!=SapHeader[i]) Result=FALSE;

            *ErrorCode=DL_SUCCESS;
        }

        Sys_Close(h);
    }

    return Result;
}


/*****
    Allocation des ressources necessaires pour la lecture/ecriture
    d'un fichier au format SAP.
*****/

struct DataLayerSAP *DSap_Open(const char *Name, ULONG IOMode, ULONG *ErrorCode)
{
    struct DataLayerSAP *DLayer=(struct DataLayerSAP *)Sys_AllocMem(sizeof(struct DataLayerSAP));

    *ErrorCode=DL_NOT_ENOUGH_MEMORY;
    if(DLayer!=NULL)
    {
        *ErrorCode=DL_OPEN_FILE;
        if((DLayer->Handle=Sys_Open(Name,IOMode))!=NULL)
        {
            *ErrorCode=DL_SUCCESS;
        }

        if(*ErrorCode)
        {
            DSap_Close(DLayer);
            DLayer=NULL;
        }
    }

    return DLayer;
}


/*****
    Liberation des ressources allouees par DSap_Close()
*****/

void DSap_Close(struct DataLayerSAP *DLayer)
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

ULONG DSap_Finalize(struct DataLayerSAP *DLayer)
{
    ULONG ErrorCode=P_DSap_SeekExt(DLayer,COUNTOF_TRACK,1);
    Sys_Flush(DLayer->Handle);

    return ErrorCode;
}


/*****
    Formatage d'une piste
*****/

ULONG DSap_FormatTrack(struct DataLayerSAP *DLayer, ULONG Track, const UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_SUCCESS;
    LONG Sector;

    for(Sector=1; !ErrorCode && Sector<=COUNTOF_SECTOR_PER_TRACK; Sector++)
    {
        ErrorCode=DSap_WriteSector(DLayer,Track,Sector,&BufferPtr[(Sector-1)*SIZEOF_SECTOR]);
    }

    return ErrorCode;
}


/*****
    Lecture d'un secteur
*****/

ULONG DSap_ReadSector(struct DataLayerSAP *DLayer, ULONG Track, ULONG Sector, UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_SUCCESS;
    LONG Count=0;
    ULONG Offset=P_DSap_GetOffset(Track,Sector);

    if(Sys_Seek(DLayer->Handle,Offset+4,OFFSET_BEGINNING)>=0)
    {
        ErrorCode=DL_READ_FILE;
        Count=Sys_Read(DLayer->Handle,BufferPtr,SIZEOF_SECTOR);
        if(Count>0) P_DSap_DecodeSector(BufferPtr,Count);
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

ULONG DSap_WriteSector(struct DataLayerSAP *DLayer, ULONG Track, ULONG Sector, const UBYTE *BufferPtr)
{
    ULONG ErrorCode=P_DSap_SeekExt(DLayer,Track,Sector);

    if(!ErrorCode)
    {
        P_DSap_EncodeSector(BufferPtr,Track,Sector,DLayer->Buffer);
        if(Sys_Write(DLayer->Handle,(void *)DLayer->Buffer,sizeof(DLayer->Buffer))<0) ErrorCode=DL_WRITE_FILE;
    }

    return ErrorCode;
}


/*****
    Positionne le curseur a l'offset donne.
    Note: Cette fonction crée le header, voire agrandit le fichier si besoin.
*****/

ULONG P_DSap_SeekExt(struct DataLayerSAP *DLayer, ULONG Track, ULONG Sector)
{
    const LONG HeaderLen=sizeof(SapHeader)-sizeof(char);
    const LONG SectorSAPLen=sizeof(DLayer->Buffer);
    LONG Offset=P_DSap_GetOffset(Track,Sector);
    LONG Size;

    /* On récupère la taille du fichier et on se repositionne au début */
    Sys_Seek(DLayer->Handle,0,OFFSET_END);
    Size=Sys_Seek(DLayer->Handle,0,OFFSET_BEGINNING);

    /* Si le fichier est plus petit que le header, alors on reconstruit ce header */
    if(Size>=0 && Size<HeaderLen)
    {
        Size=Sys_Write(DLayer->Handle,(void *)SapHeader,HeaderLen);
        /* Du coup, sauf si erreur, Size contient la nouvelle taille du fichier */
    }

    if(Size>=HeaderLen)
    {
        LONG IdxFinalSector=Track*COUNTOF_SECTOR_PER_TRACK+Sector-1;
        LONG IdxSector=(Size-HeaderLen)/SectorSAPLen;

        if(IdxSector>IdxFinalSector) IdxSector=IdxFinalSector;

        /* Avant de commencer potentiellement à écrire, on se positionne au bon endroit */
        Offset=Sys_Seek(DLayer->Handle,HeaderLen+IdxSector*SectorSAPLen,OFFSET_BEGINNING);

        /* On reconstruit les secteurs manquants, si besoin */
        while(Offset>=0 && IdxSector<IdxFinalSector)
        {
            Track=IdxSector/COUNTOF_SECTOR_PER_TRACK;
            Sector=IdxSector-Track*COUNTOF_SECTOR_PER_TRACK+1;
            P_DSap_EncodeSector(NULL,Track,Sector,DLayer->Buffer);
            Offset=Sys_Write(DLayer->Handle,(void *)DLayer->Buffer,SectorSAPLen);
            IdxSector++;
        }
    }

    if(Sys_IoErr()<=0) return DL_SUCCESS;
    return DL_WRITE_FILE;
}


/*****
    Encodage d'un secteur
*****/

void P_DSap_EncodeSector(const UBYTE *BufferPtr, ULONG Track, ULONG Sector, UBYTE Buffer[262])
{
    LONG crc,i;
    UBYTE *Ptr=Buffer;

    Ptr[0]=0; /* format */
    Ptr[1]=0; /* protection */
    Ptr[2]=(UBYTE)Track;
    Ptr[3]=(UBYTE)Sector;
    crc=P_DSap_CRC_Pukall((ULONG)*(Ptr++),0xffff);
    crc=P_DSap_CRC_Pukall((ULONG)*(Ptr++),crc);
    crc=P_DSap_CRC_Pukall((ULONG)*(Ptr++),crc);
    crc=P_DSap_CRC_Pukall((ULONG)*(Ptr++),crc);

    for(i=0; i<SIZEOF_SECTOR; i++)
    {
        ULONG Value=BufferPtr!=NULL?(ULONG)BufferPtr[i]:0;
        crc=P_DSap_CRC_Pukall(Value,crc);
        *(Ptr++)=(UBYTE)(Value^0xB3);
    }

    *(Ptr++)=(UBYTE)(crc>>8);
    *Ptr=(UBYTE)crc;
}


/*****
    Décodage d'un secteur
*****/

void P_DSap_DecodeSector(UBYTE *BufferPtr, LONG Count)
{
    while(--Count>=0) *(BufferPtr++)^=0xB3;
}

/*****
    Retourne l'offset dans le fichier correspondant a la piste et au secteur donne
*****/

ULONG P_DSap_GetOffset(ULONG Track, ULONG Sector)
{
    return (sizeof(SapHeader)-sizeof(char))+Track*(262*COUNTOF_SECTOR_PER_TRACK)+(Sector-1)*262;
}


/*****
    Calcule le Crc avec la nouvelle valeur "c"
*****/

ULONG P_DSap_CRC_Pukall(ULONG c, ULONG crc)
{
    register ULONG index;

    index = (crc ^ c) & 0xf;
    crc = ((crc>>4) & 0xfff) ^ SapCRCTable[index];

    c >>= 4;

    index = (crc ^ c) & 0xf;
    crc = ((crc>>4) & 0xfff) ^ SapCRCTable[index];

    return crc;
}
