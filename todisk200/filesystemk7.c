#include "system.h"
#include "global.h"
#include "filesystemk7.h"
#include "convert.h"
#include "util.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    27-08-2018 (Seg)    Remaniement suite gestion des noms avec accents
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    21-01-2013 (Seg)    Gestion du file system d'un fichier K7 (Mo ou To)
*/


/***** Prototypes */
struct FileSystemK7 *K7_AllocFileSystem(const char *);
void K7_FreeFileSystem(struct FileSystemK7 *);

void K7_ExamineFileObject(struct FileSystemK7 *, struct FileObjectK7 *);
BOOL K7_ExamineNextFileObject(struct FileObjectK7 *);

struct FSHandleK7 *K7_OpenFile(struct FileSystemK7 *, const char *, LONG *, BOOL, LONG *);
LONG K7_CloseFile(struct FSHandleK7 *);
LONG K7_ReadFile(struct FSHandleK7 *, UBYTE *, LONG);
LONG K7_Seek(struct FSHandleK7 *, LONG);
LONG K7_FindFile(struct FileSystemK7 *, const char *, LONG *, BOOL, LONG *);
LONG K7_GetSize(struct FSHandleK7 *);
LONG K7_GetTypeFromHandle(struct FSHandleK7 *);

BOOL P_K7_ScanFileName(struct FileSystemK7 *, UBYTE *, LONG *, LONG *);
BOOL P_K7_ScanNextData(struct FileSystemK7 *, LONG *, LONG *, LONG *, BOOL *);
BOOL P_K7_CheckSum(BOOL, LONG, LONG);


/*****
    Allocation d'une structure FileSystemK7 pour la gestion des fichiers d'une image K7
*****/

struct FileSystemK7 *K7_AllocFileSystem(const char *Name)
{
    struct FileSystemK7 *FS=(struct FileSystemK7 *)Sys_AllocMem(sizeof(struct FileSystemK7));

    if(FS!=NULL)
    {
        FS->BufferSize=0;
        FS->CurrentOffset=0;
        if((FS->BufferPtr=Sys_AllocFile(Name,&FS->BufferSize))!=NULL) return FS;

        K7_FreeFileSystem(FS);
    }

    return NULL;
}


/*****
    Libere les ressources allouees par K7_AllocFileSystem()
*****/

void K7_FreeFileSystem(struct FileSystemK7 *FS)
{
    if(FS!=NULL)
    {
        Sys_FreeMem((void *)FS);
    }
}


/*****
    Initialisation d'une structure FileObjectK7 en vue de scanner les fichiers de l'image K7
*****/

void K7_ExamineFileObject(struct FileSystemK7 *FS, struct FileObjectK7 *FO)
{
    FO->FS=FS;
    FO->Offset=0;
    FO->OffsetHeader=0;
    FO->OffsetData=0;
    FO->OffsetEnd=0;
}


/*****
    Scan les fichiers de l'image K7
    Retour:
    - TRUE s'il y a une nouvelle entree
    - FALSE si on est en fin de K7
*****/

BOOL K7_ExamineNextFileObject(struct FileObjectK7 *FO)
{
    FO->Type=0;
    FO->Size=0;
    FO->Offset=FO->OffsetEnd;

    if(P_K7_ScanFileName(FO->FS,FO->FileInfo,&FO->OffsetHeader,&FO->Offset))
    {
        LONG Len=0,SumBase=0;
        BOOL IsMo5=FALSE;

        Cnv_ConvertThomsonNameToHostName(FO->FileInfo,FO->Name,TRUE);
        FO->Type=((LONG)FO->FileInfo[K7FIO_TYPE]<<8)+(LONG)FO->FileInfo[K7FIO_TYPE+1];

        FO->OffsetData=FO->Offset;
        while(P_K7_ScanNextData(FO->FS,&FO->Offset,&Len,&SumBase,&IsMo5))
        {
            FO->Size+=Len;
            FO->Offset+=Len+1; /* on zap le checksum */
        }
        FO->OffsetEnd=FO->Offset;

        FO->FS->CurrentOffset=FO->OffsetHeader;

        return TRUE;
    }

    return FALSE;
}


/*****
    Ouverture d'un fichier en lecture sur le file system
*****/

struct FSHandleK7 *K7_OpenFile(struct FileSystemK7 *FS, const char *Name, LONG *Type, BOOL IsSensitive, LONG *ErrorCode)
{
    struct FSHandleK7 *h=NULL;
    LONG OffsetData;
    LONG OffsetHeader=K7_FindFile(FS,Name,Type,IsSensitive,&OffsetData);

    if(OffsetHeader>=0)
    {
        h=(struct FSHandleK7 *)Sys_AllocMem(sizeof(struct FSHandleK7));

        *ErrorCode=FS_NOT_ENOUGH_MEMORY;

        if(h!=NULL)
        {
            *ErrorCode=FS_SUCCESS;
            h->FS=FS;
            h->Type=*Type;
            h->OffsetHeader=OffsetHeader;
            h->OffsetData=OffsetData;
            h->CurrentOffset=OffsetData;
            h->Len=0;
            h->IsEOF=FALSE;
        }
    }

    return h;
}


/*****
    Fermeture des ressources allouees par K7_OpenFile()
*****/

LONG K7_CloseFile(struct FSHandleK7 *h)
{
    if(h!=NULL) Sys_FreeMem((void *)h);
    return K7_SUCCESS;
}


/*****
    Routine de lecture du fichier.
    Retourne:
    - Le nombre d'octets lus
    - 0 si End Of File
*****/

LONG K7_ReadFile(struct FSHandleK7 *h, UBYTE *Buffer, LONG Len)
{
    LONG Result=0;
    LONG State=(h->Len<=0?2:0);

    while(!h->IsEOF && Result<Len && h->CurrentOffset<h->FS->BufferSize)
    {
        UBYTE Data=h->FS->BufferPtr[h->CurrentOffset++];

        switch(State)
        {
            case 0:
                *(Buffer++)=Data;
                if(--h->Len<=0) State=1;
                h->Sum+=(LONG)Data;
                Result++;
                break;

            case 1:
                State=2;
                if(!P_K7_CheckSum(h->IsMo5,(LONG)Data,(LONG)(UBYTE)h->Sum))
                {
                    h->CurrentOffset--;
                    Result=K7_FILE_ERROR;
                    h->IsEOF=TRUE;
                }
                break;

            case 2:
                State=0;
                if(!P_K7_ScanNextData(h->FS,&h->CurrentOffset,&h->Len,&h->Sum,&h->IsMo5))
                {
                    h->IsEOF=TRUE;
                }
                break;
        }
    }

    return Result;
}


/*****
    Fonction de positionnement dans le fichier
*****/

LONG K7_Seek(struct FSHandleK7 *h, LONG Pos)
{
    return 0;
}


/*****
    Recherche un nom de fichier sur le support K7
    Retour:
    - l'offset sur le marqueur du nom de fichier recherché
    - FS_FILE_NOT_FOUND si le fichier n'existe pas
*****/

LONG K7_FindFile(struct FileSystemK7 *FS, const char *Name, LONG *Type, BOOL IsSensitive, LONG *OffsetData)
{
    LONG FileOffset=K7_FILE_NOT_FOUND;
    UBYTE FileInfo[13];
    char CurrentName[13],FinalName[13];
    LONG FileOffsetS=K7_FILE_NOT_FOUND,DataOffsetS=0;
    LONG FileOffsetI=K7_FILE_NOT_FOUND,DataOffsetI=0;
    BOOL FoundS=FALSE,FoundI=FALSE;
    LONG Offset=FS->CurrentOffset,OffsetHeader=Offset;

    /* On réduit le nom passé en paramètre, au format Thomson, c'est-à-dire
       à 12 caractères (avec le point de séparation nom/suffixe).
    */
    Utl_SplitHostName(Name,FinalName);
    while(Offset<FS->BufferSize)
    {
        if(P_K7_ScanFileName(FS,FileInfo,&OffsetHeader,&Offset))
        {
            Cnv_ConvertThomsonNameToHostName(FileInfo,CurrentName,TRUE);

            if(Sys_StrCmp(FinalName,CurrentName)==0 && !FoundS)
            {
                if(Type!=NULL)
                {
                    ULONG CurrentType=((ULONG)FileInfo[K7FIO_TYPE]<<8)+(ULONG)FileInfo[K7FIO_TYPE+1];
                    if(CurrentType==*Type) {FileOffsetS=OffsetHeader; DataOffsetS=Offset; FoundS=TRUE;}
                } else if(FileOffsetS<0) {FileOffsetS=OffsetHeader; DataOffsetS=Offset;}
            }

            if(Sys_StrCmpNoCase(FinalName,CurrentName)==0 && !FoundI)
            {
                if(Type!=NULL)
                {
                    ULONG CurrentType=((ULONG)FileInfo[K7FIO_TYPE]<<8)+(ULONG)FileInfo[K7FIO_TYPE+1];
                    if(CurrentType==*Type) {FileOffsetI=OffsetHeader; DataOffsetI=Offset; FoundI=TRUE;}
                } else if(FileOffsetI<0) {FileOffsetI=OffsetHeader; DataOffsetI=Offset;}
            }
        }
    }

    if(FileOffsetS>=0)
    {
        if(Type==NULL || (Type!=NULL && FoundS)) {FileOffset=FileOffsetS; *OffsetData=DataOffsetS;}
    }

    if(FileOffsetI>=0 && FileOffset<0 && !IsSensitive)
    {
        if(Type==NULL || (Type!=NULL && FoundI)) {FileOffset=FileOffsetI, *OffsetData=DataOffsetI;}
    }

    return FileOffset;
}


/*****
    Retourne la taille du fichier en fonction de son Handle
*****/

LONG K7_GetSize(struct FSHandleK7 *h)
{
    LONG Size=0;
    LONG Offset=h->OffsetData,Len,SumBase;
    BOOL IsMo5;

    while(P_K7_ScanNextData(h->FS,&Offset,&Len,&SumBase,&IsMo5))
    {
        Size+=Len;
        Offset+=Len+1; /* on zap le checksum */
    }

    return Size;
}


/*****
    Retourne le type du fichier en cours
*****/

LONG K7_GetTypeFromHandle(struct FSHandleK7 *h)
{
    return h->Type;
}


/*****
    Permet d'extraire le premier nom de fichier à partir de l'index courant
*****/

BOOL P_K7_ScanFileName(struct FileSystemK7 *FS, UBYTE *FileInfo, LONG *OffsetHeader, LONG *Offset)
{
    LONG FISize=13,State=0,IdxName=0,Len=0,Sum=0;
    BOOL IsMo5=FALSE;

    *OffsetHeader=*Offset;

    while(State>=0 && *Offset<FS->BufferSize)
    {
        UBYTE Data=FS->BufferPtr[(*Offset)++];

        switch(State)
        {
            case 0: /* Recherche un marqueur */
                if(Data==0x3c) {*OffsetHeader=*Offset-1; State=1;}
                break;

            case 1: /* Recherche une entete de fichier */
                if(Data==0x5a) IsMo5=TRUE;
                else if(Data==0x00) State=2; /* On est sur un bloc Header */
                else {*Offset=*OffsetHeader+1; State=0;} /* On est sur n'importe quoi. On recommence */
                break;

            case 2: /* Récupération de la longueur du bloc */
                for(IdxName=0; IdxName<FISize; IdxName++) FileInfo[IdxName]=0;
                Len=IsMo5?(LONG)Data-2:(LONG)Data;
                Sum=IsMo5?0:Data;
                IdxName=0;
                State=Len>0?3:0;
                break;

            case 3: /* Récupération du nom */
                if(IdxName<FISize) FileInfo[IdxName++]=Data;
                Sum+=Data;
                if(--Len<=0) State=4;
                break;

            case 4: /* Controle checksum */
                if(P_K7_CheckSum(IsMo5,(LONG)Data,(LONG)(UBYTE)Sum)) return TRUE;
                /* Le checksum est mauvais, on recommence */
                *Offset=*OffsetHeader+1;
                State=0;
                break;
            }
    }

    return FALSE;
}


/*****
    Permet de se positionner sur le bloc Data suivant
*****/

BOOL P_K7_ScanNextData(struct FileSystemK7 *FS, LONG *Offset, LONG *Len, LONG *SumBase, BOOL *IsMo5)
{
    LONG State=0;

    *IsMo5=FALSE;
    *SumBase=0;
    *Len=0;
    while(State>=0 && *Offset<FS->BufferSize)
    {
        UBYTE Data=FS->BufferPtr[(*Offset)++];

        switch(State)
        {
            case 0: /* Recherche un marqueur */
                if(Data==0x3c) State=1;
                break;

            case 1: /* Recherche le début du bloc Data */
                if(Data==0x5a) *IsMo5=TRUE;
                else if(Data==0x01) {*SumBase=*IsMo5?0:(LONG)Data; State=2;} /* On est sur un bloc Data */
                else State=-1; /* Fin du fichier */
                break;

            case 2: /* On prend la longueur du bloc */
                *Len=(LONG)Data;
                if(*IsMo5) *Len=(LONG)(Data?Data:256)-2; else *SumBase+=(LONG)Data;
                if(*Len>=0) return TRUE;
                State=-1;
                break;
        }
    }

    return FALSE;
}


/*****
    Teste la validité du checksum en fonction du mode dans lequel on est
*****/

BOOL P_K7_CheckSum(BOOL IsMo5, LONG SumBase, LONG Sum)
{
    if((!IsMo5 && Sum!=SumBase) || (IsMo5 && 0x100-Sum!=SumBase)) return FALSE;
    return TRUE;
}
