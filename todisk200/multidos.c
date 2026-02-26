#include "system.h"
#include "global.h"
#include "multidos.h"
#include "filesystem.h"
#include "util.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    27-08-2018 (Seg)    Remaniement suite gestion des noms avec accents
    06-06-2017 (Seg)    Modifications pour gérer les "extradata" des fichiers .CHG
    30-09-2016 (Seg)    Ajout de MD_Flush()
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Ajout de MD_InitDOSType() + réaménagement du code pour gérer
                        plus de 2 filesystems + Ajout de MD_DeleteFile()
    19-12-2012 (Seg)    Fix GetFileSize() en mode Host
    22-03-2010 (Seg)    Gestion de Examine et ExamineNext
    21-03-2010 (Seg)    Renommage de mhandle.c en multidos.c
    24-02-2010 (Seg)    Gestion d'un handle multisource
                        Ce code provient de cmdcopy.c et a ete supprime de ce dernier
*/


/***** Prototypes */
void MD_InitDOSType(struct MultiDOS *, LONG, void *);
void MD_Flush(struct MultiDOS *);

LONG MD_OpenFile(struct MDHandle *, struct MultiDOS *, ULONG, const char *, LONG *);
void MD_CloseFile(struct MDHandle *);
LONG MD_ReadFile(struct MDHandle *, UBYTE *, LONG);
LONG MD_WriteFile(struct MDHandle *, UBYTE *, LONG);
LONG MD_DeleteFile(struct MultiDOS *, const char *, LONG *, BOOL);
LONG MD_GetFileSize(struct MDHandle *);
LONG MD_SetComment(struct MultiDOS *, const char *, const LONG *, const char *);
LONG MD_SetDate(struct MultiDOS *, const char *, LONG *, LONG, LONG, LONG, LONG, LONG, LONG);
LONG MD_GetTypeFromHandle(struct MDHandle *);

BOOL MD_InitFileObject(struct MDFileObject *, struct MultiDOS *, const char *);
void MD_FreeFileObject(struct MDFileObject *);
BOOL MD_ExamineFileObject(struct MDFileObject *);
BOOL MD_ExamineNextFileObject(struct MDFileObject *);
LONG MD_FindFile(struct MultiDOS *, const char *, LONG *, BOOL);

const char *MD_GetTextErr(ULONG);

LONG P_MD_GetMDCodeFromFileSystemCode(LONG);
LONG P_MD_GetMDCodeFromFileSystemK7Code(LONG);
LONG P_MD_GetMDCodeFromAmigaDOSCode(void);


/*****
    Initialisation de la structure MultiDOS pour savoir sur quel filesystem on travaille.
    Note: seul le premier pointeur de filesystem non nul est pris en compte
*****/

void MD_InitDOSType(struct MultiDOS *MD, LONG Type, void *FS)
{
    MD->Type=Type;
    MD->FS=FS;
}


/*****
    Ecriture des données répertoire/FAT qui sont potentiellement en cache
*****/

void MD_Flush(struct MultiDOS *MD)
{
    switch(MD->Type)
    {
        case MDOS_HOST:
            break;

        case MDOS_DISK:
            FS_FlushFileInfo((struct FileSystem *)MD->FS);
            break;

        case MDOS_K7:
            break;
    }
}


/*****
    Ouverture du fichier en fonction du filesystem utilise
    Parametres:
    - h: pointeur sur une structure MDHandle fraichement instanciee
    - MD: pointeur sur un file systeme intialisé par MD_InitDOSType()
    - Mode: mode d'ouverture du fichier. 3 modes possibles:
        * MD_MODE_OLDFILE
        * MD_MODE_NEWFILE
        * MD_MODE_READWRITE
    - FileName: Nom du fichier a ouvrir en lecture ou en ecriture
    - Type: pointeur sur le type de fichier requis. Peut etre NULL si le type n'est pas important.
*****/

LONG MD_OpenFile(struct MDHandle *h, struct MultiDOS *MD, ULONG Mode, const char *FileName, LONG *Type)
{
    LONG Result=MD_SUCCESS;

    h->MD=MD;
    h->Handle=NULL;
    switch(MD->Type)
    {
        case MDOS_DISK:
            h->Handle=(void *)FS_OpenFile((struct FileSystem *)MD->FS,Mode,FileName,Type,FALSE,TRUE,&Result);
            Result=P_MD_GetMDCodeFromFileSystemCode(Result);
            break;

        case MDOS_HOST:
            switch(Mode)
            {
                case MD_MODE_OLDFILE:
                    h->Handle=(void *)Sys_Open(FileName,MODE_OLDFILE);
                    break;

                case MD_MODE_NEWFILE:
                    h->Handle=(void *)Sys_Open(FileName,MODE_NEWFILE);
                    break;

                case MD_MODE_READWRITE:
                    h->Handle=(void *)Sys_Open(FileName,MODE_READWRITE);
                    break;
            }
            if(h->Handle==NULL) Result=P_MD_GetMDCodeFromAmigaDOSCode();
            break;

        case MDOS_K7:
            if(Mode==MD_MODE_OLDFILE)
            {
                h->Handle=(void *)K7_OpenFile((struct FileSystemK7 *)MD->FS,FileName,Type,FALSE,&Result);
                Result=P_MD_GetMDCodeFromFileSystemK7Code(Result);
            } else Result=MD_SUPPORT_PROTECTED;
            break;
    }

    return Result;
}


/*****
    Fermeture du fichier
*****/

void MD_CloseFile(struct MDHandle *h)
{
    switch(h->MD->Type)
    {
        case MDOS_HOST:
            if(h->Handle!=NULL) Sys_Close((HPTR)h->Handle);
            break;

        case MDOS_DISK:
            FS_CloseFile((struct FSHandle *)h->Handle);
            break;

        case MDOS_K7:
            K7_CloseFile((struct FSHandleK7 *)h->Handle);
            break;
    }

    h->Handle=NULL;
}


/*****
    Lecture d'un fichier sur le bon handle
*****/

LONG MD_ReadFile(struct MDHandle *h, UBYTE *Buffer, LONG Len)
{
    LONG Result=0;

    if(h->Handle!=NULL)
    {
        switch(h->MD->Type)
        {
            case MDOS_HOST:
                Result=Sys_Read((HPTR)h->Handle,Buffer,Len);
                if(Result<0) Result=P_MD_GetMDCodeFromAmigaDOSCode();
                break;

            case MDOS_DISK:
                Result=FS_ReadFile((struct FSHandle *)h->Handle,Buffer,Len);
                if(Result<0) Result=P_MD_GetMDCodeFromFileSystemCode(Result);
                break;

            case MDOS_K7:
                Result=K7_ReadFile((struct FSHandleK7 *)h->Handle,Buffer,Len);
                if(Result<0) Result=P_MD_GetMDCodeFromFileSystemK7Code(Result);
                break;
        }
    }

    return Result;
}


/*****
    Ecriture d'un fichier sur le bon handle
*****/

LONG MD_WriteFile(struct MDHandle *h, UBYTE *Buffer, LONG Len)
{
    LONG Result=0;

    if(h->Handle!=NULL)
    {
        switch(h->MD->Type)
        {
            case MDOS_HOST:
                Result=Sys_Write((HPTR)h->Handle,Buffer,Len);
                if(Result<=0) Result=P_MD_GetMDCodeFromAmigaDOSCode();
                break;

            case MDOS_DISK:
                Result=FS_WriteFile((struct FSHandle *)h->Handle,Buffer,Len);
                if(Result<=0) Result=P_MD_GetMDCodeFromFileSystemCode(Result);
                break;

            case MDOS_K7:
                Result=MD_SUPPORT_PROTECTED;
                break;
        }
    }

    return Result;
}


/*****
    Efface un fichier
*****/

LONG MD_DeleteFile(struct MultiDOS *MD, const char *Name, LONG *Type, BOOL IsSensitive)
{
    LONG Result=MD_SUCCESS;

    switch(MD->Type)
    {
        case MDOS_DISK:
            Result=FS_DeleteFile((struct FileSystem *)MD->FS,Name,Type,IsSensitive);
            if(Result<=0) Result=P_MD_GetMDCodeFromFileSystemCode(Result);
            break;

        case MDOS_HOST:
            break;

        case MDOS_K7:
            Result=MD_SUPPORT_PROTECTED;
            break;
    }

    return Result;
}


/*****
    Permet d'obtenir la taille d'un fichier
*****/

LONG MD_GetFileSize(struct MDHandle *h)
{
    LONG Size=0;

    if(h->Handle!=NULL)
    {
        switch(h->MD->Type)
        {
            case MDOS_HOST:
                {
                    LONG Pos=Sys_Seek((HPTR)h->Handle,0,OFFSET_END);
                    Size=Sys_Seek((HPTR)h->Handle,Pos,OFFSET_BEGINNING);
                }
                break;

            case MDOS_DISK:
                Size=FS_GetSize((struct FSHandle *)h->Handle);
                break;

            case MDOS_K7:
                Size=K7_GetSize((struct FSHandleK7 *)h->Handle);
                break;
        }
    }

    return Size;
}


/*****
    Ecriture du commentaire
    - MD: Handler
    - FileName: nom au format Hôte
    - Type: pointeur sur le type associé au nom (ou NULL)
    - Comment: commentaire du fichier. Peut contenir les metas data qui seront correctement intégrées sur le FS Thomson.
*****/

LONG MD_SetComment(struct MultiDOS *MD, const char *FileName, const LONG *Type, const char *Comment)
{
    LONG Result=MD_SUCCESS;

    switch(MD->Type)
    {
        case MDOS_DISK:
            Result=FS_SetComment((struct FileSystem *)MD->FS,FileName,Type,FALSE,Comment);
            if(Result<=0) Result=P_MD_GetMDCodeFromFileSystemCode(Result);
            break;

        case MDOS_HOST:
            if(!Sys_SetFileComment(FileName,Comment)) Result=P_MD_GetMDCodeFromAmigaDOSCode();
            break;

        case MDOS_K7:
            Result=MD_SUPPORT_PROTECTED;
            break;
    }

    return Result;
}


/*****
    Ecriture de la date
*****/

LONG MD_SetDate(struct MultiDOS *MD, const char *FileName, LONG *Type, LONG Year, LONG Month, LONG Day, LONG Hour, LONG Min, LONG Sec)
{
    LONG Result=MD_SUCCESS;

    if(Hour<0 || Hour>23) Hour=0;
    if(Min<0 || Min>59) Min=0;
    if(Sec<0 || Sec>59) Sec=0;

    switch(MD->Type)
    {
        case MDOS_DISK:
            Result=FS_SetDate(MD->FS,FileName,Type,FALSE,Year,Month,Day,Hour,Min,Sec);
            if(Result<=0) Result=P_MD_GetMDCodeFromFileSystemCode(Result);
            break;

        case MDOS_HOST:
            if(Day>=1 && Day<=31 && Month>=1 && Month<=12)
            {
                if(!Sys_SetFileDate(FileName,Year,Month,Day,Hour,Min,Sec)) Result=P_MD_GetMDCodeFromAmigaDOSCode();
            }
            break;

        case MDOS_K7:
            Result=MD_SUPPORT_PROTECTED;
            break;
    }

    return Result;
}


/*****
    Permet d'obtenir le type du fichier en cours
*****/

LONG MD_GetTypeFromHandle(struct MDHandle *MH)
{
    switch(MH->MD->Type)
    {
        case MDOS_DISK:
            return FS_GetTypeFromHandle((struct FSHandle *)MH->Handle);

        case MDOS_K7:
            return K7_GetTypeFromHandle((struct FSHandleK7 *)MH->Handle);
    }

    return 0x0200;
}


/*****
    Preparation au scan du repertoire
*****/

BOOL MD_InitFileObject(struct MDFileObject *MDFO, struct MultiDOS *MD, const char *Path)
{
    BOOL IsSuccess=FALSE;

    MDFO->MD=MD;
    MDFO->PSDir=NULL;

    switch(MD->Type)
    {
        case MDOS_DISK:
            FS_ExamineFileObject((struct FileSystem *)MD->FS,&MDFO->FO);
            IsSuccess=TRUE;
            break;

        case MDOS_HOST:
            if((MDFO->PSDir=Sys_AllocExamineDir(Path))!=NULL) IsSuccess=TRUE;
            break;

        case MDOS_K7:
            K7_ExamineFileObject((struct FileSystemK7 *)MD->FS,&MDFO->FOK7);
            IsSuccess=TRUE;
            break;
    }

    return IsSuccess;
}


/*****
    Libere les ressources potentiellement allouees par MD_InitFileObject()
*****/

void MD_FreeFileObject(struct MDFileObject *MDFO)
{
    if(MDFO->MD->Type==MDOS_HOST)
    {
        Sys_FreeExamineDir(MDFO->PSDir);
        MDFO->PSDir=NULL;
    }
}


/*****
    Debut de scan de fichier
*****/

BOOL MD_ExamineFileObject(struct MDFileObject *MDFO)
{
    BOOL IsSuccess=FALSE;

    switch(MDFO->MD->Type)
    {
        case MDOS_DISK:
            FS_ExamineFileObject((struct FileSystem *)MDFO->MD->FS,&MDFO->FO);
            IsSuccess=TRUE;
            break;

        case MDOS_HOST:
            IsSuccess=Sys_ExamineDir(MDFO->PSDir);
            break;

        case MDOS_K7:
            K7_ExamineFileObject((struct FileSystemK7 *)MDFO->MD->FS,&MDFO->FOK7);
            IsSuccess=TRUE;
            break;
    }

    return IsSuccess;
}


/*****
    Scan des fichiers
*****/

BOOL MD_ExamineNextFileObject(struct MDFileObject *MDFO)
{
    BOOL IsSuccess=FALSE;

    switch(MDFO->MD->Type)
    {
        case MDOS_DISK:
            IsSuccess=FS_ExamineNextFileObject(&MDFO->FO);
            if(IsSuccess)
            {
                MDFO->IsDir=FALSE;
                MDFO->Name=MDFO->FO.Name;
                MDFO->Comment=MDFO->FO.Comment;
                MDFO->Year=MDFO->FO.Year;
                MDFO->Month=MDFO->FO.Month;
                MDFO->Day=MDFO->FO.Day;
                MDFO->Hour=MDFO->FO.Hour;
                MDFO->Min=MDFO->FO.Min;
                MDFO->Sec=MDFO->FO.Sec;
                MDFO->Type=MDFO->FO.Type;
                MDFO->ExtraData=MDFO->FO.ExtraData;
                MDFO->Size=MDFO->FO.Size;
                MDFO->IsTimeOk=MDFO->FO.IsTimeOk;
                MDFO->IsDateOk=MDFO->FO.IsDateOk;
                MDFO->CountOfBlocks=MDFO->FO.CountOfBlocks;
            }
            break;

        case MDOS_HOST:
            IsSuccess=Sys_ExamineDirNext(MDFO->PSDir);
            if(IsSuccess)
            {
                MDFO->IsDir=MDFO->PSDir->IsDir;
                MDFO->Name=MDFO->PSDir->Name;
                MDFO->Year=MDFO->PSDir->Year;
                MDFO->Month=MDFO->PSDir->Month;
                MDFO->Day=MDFO->PSDir->Day;
                MDFO->Hour=MDFO->PSDir->Hour;
                MDFO->Min=MDFO->PSDir->Min;
                MDFO->Sec=MDFO->PSDir->Sec;
                MDFO->Size=MDFO->PSDir->Size;
                MDFO->Type=Utl_GetTypeFromHostName(MDFO->PSDir->Name); /* Récupération du type par défaut */
                MDFO->ExtraData=-1;
                MDFO->IsTimeOk=TRUE;
                MDFO->IsDateOk=TRUE;
                MDFO->Comment=Utl_ExtractInfoFromHostComment(MDFO->PSDir->Comment,&MDFO->Type,&MDFO->ExtraData); /* Ecrasement potentiel du type et de l'extradata */
                MDFO->CountOfBlocks=(MDFO->Size+255*8)/(255*8);
            }
            break;

        case MDOS_K7:
            IsSuccess=K7_ExamineNextFileObject(&MDFO->FOK7);
            if(IsSuccess)
            {
                MDFO->IsDir=FALSE;
                MDFO->Name=MDFO->FOK7.Name;
                MDFO->Comment="";
                MDFO->Year=0;
                MDFO->Month=0;
                MDFO->Day=0;
                MDFO->Hour=0;
                MDFO->Min=0;
                MDFO->Sec=0;
                MDFO->Type=MDFO->FOK7.Type;
                MDFO->ExtraData=-1;
                MDFO->Size=MDFO->FOK7.Size;
                MDFO->IsTimeOk=FALSE;
                MDFO->IsDateOk=FALSE;
                MDFO->CountOfBlocks=(MDFO->Size+255*8)/(255*8);
            }
            break;
    }

    return IsSuccess;
}


/*****
    Recherche un nom de fichier dans la table des noms de fichier
    Retour:
    - l'offset sur les informations du fichier dans le buffer "Dir",
      ainsi que le numero de la piste et le numero du secteur qui contient
      les informations sur le fichier.
    - MD_FILE_NOT_FOUND si le fichier n'existe pas
*****/

LONG MD_FindFile(struct MultiDOS *MD, const char *Name, LONG *Type, BOOL IsSensitive)
{
    LONG Result=MD_FILE_NOT_FOUND;

    switch(MD->Type)
    {
        case MDOS_DISK:
            Result=FS_FindFile((struct FileSystem *)MD->FS,Name,Type,IsSensitive);
            if(Result<0) Result=P_MD_GetMDCodeFromFileSystemCode(Result);
            break;

        case MDOS_HOST:
            break;

        case MDOS_K7:
            {
                LONG OffsetData=0;
                Result=K7_FindFile((struct FileSystemK7 *)MD->FS,Name,Type,IsSensitive,&OffsetData);
                if(Result<0) Result=P_MD_GetMDCodeFromFileSystemK7Code(Result);
            }
            break;
    }

    return Result;
}


/*****
    Retourne le nom du volume
*****/

void MD_GetVolumeName(struct MultiDOS *MD, char *VolumeName)
{
    switch(MD->Type)
    {
        case MDOS_DISK:
            FS_GetVolumeName((struct FileSystem *)MD->FS,VolumeName);
            break;

        case MDOS_HOST:
            *VolumeName=0;
            break;

        case MDOS_K7:
            *VolumeName=0;
            break;
    }
}


/*****
    Calcule le nombre de kilo octets encore disponible sur le support
*****/

LONG MD_GetFreeSpace(struct MultiDOS *MD)
{
    LONG Result=0;

    switch(MD->Type)
    {
        case MDOS_DISK:
            Result=FS_GetFreeSpace((struct FileSystem *)MD->FS);
            break;

        case MDOS_HOST:
            break;

        case MDOS_K7:
            break;
    }

    return Result;
}


/*****
    Retourne le texte de l'erreur MultiDOS
*****/

const char *MD_GetTextErr(ULONG ErrorCode)
{
    return FS_GetTextErr(ErrorCode);
}


/*****
    Conversion d'un code FileSystem
*****/

LONG P_MD_GetMDCodeFromFileSystemCode(LONG Code)
{
    return Code;
}


/*****
    Conversion d'un code FileSystemK7
*****/

LONG P_MD_GetMDCodeFromFileSystemK7Code(LONG Code)
{
    return Code;
}


/*****
    Conversion d'un code Amiga Dos
*****/

LONG P_MD_GetMDCodeFromAmigaDOSCode(void)
{
    LONG Code=MD_OTHER;

    switch(Sys_IoErr())
    {
        case ERROR_OBJECT_IN_USE:
        case ERROR_DIR_NOT_FOUND:
        case ERROR_DIRECTORY_NOT_EMPTY:
        case ERROR_SEEK_ERROR:
        case ERROR_DISK_NOT_VALIDATED:
        case ERROR_DISK_WRITE_PROTECTED:
        case ERROR_COMMENT_TOO_BIG:
        case ERROR_READ_PROTECTED:
        case ERROR_NOT_A_DOS_DISK:
        case ERROR_NO_DISK:
        default:
            Sys_Printf("Amiga Dos Code: %ld\n",Sys_IoErr());
            break;

        case ERROR_OBJECT_NOT_FOUND:
            Code=MD_FILE_NOT_FOUND;
            break;

        case ERROR_DISK_FULL:
            Code=MD_DISK_FULL;
            break;

        case ERROR_OBJECT_EXISTS:
            Code=MD_FILE_ALREADY_EXISTS;
            break;

        case ERROR_NO_FREE_STORE:
            Code=MD_NOT_ENOUGH_MEMORY;
            break;

        case ERROR_DELETE_PROTECTED:
        case ERROR_WRITE_PROTECTED:
            Code=MD_SUPPORT_PROTECTED;
            break;
    }

    return Code;
}
