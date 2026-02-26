#include "system.h"

#ifdef SYSTEM_UNIX
#define _GNU_SOURCE
#define __USE_GNU
#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <utime.h>


/*
    23-09-2013 (Seg)    Routines spécifiques à Unix
*/


/***** Prototypes */
void *Sys_AllocMem(ULONG);
void Sys_FreeMem(void *);

CDIR *Sys_CurrentDir(const char *);
void Sys_RestoreDir(CDIR *);
BOOL Sys_CreateDir(const char *);

SDIR *Sys_AllocExamineDir(const char *);
void Sys_FreeExamineDir(SDIR *);
BOOL Sys_ExamineDir(SDIR *);
BOOL Sys_ExamineDirNext(SDIR *);

HPTR Sys_Open(const char *, LONG);
void Sys_Close(HPTR);
LONG Sys_Read(HPTR, void *, LONG);
LONG Sys_Write(HPTR, void *, LONG);
LONG Sys_PutC(HPTR, char);
void Sys_Flush(HPTR);
LONG Sys_SetFileSize(HPTR, LONG);
BOOL Sys_SetFileComment(const char *, const char *);
BOOL Sys_SetFileDate(const char *, LONG, LONG, LONG, LONG, LONG, LONG);

BOOL Sys_CheckCtrlC(void);
BOOL Sys_CheckCtrlE(void);

void Sys_PrintFault(ULONG);
LONG Sys_IoErr(void);

const char *Sys_FilePart(const char *);
const char *Sys_PathPart(const char *);
void Sys_AddPart(char *, const char *, ULONG);

REGEX *Sys_AllocPatternNoCase(const char *, BOOL *);
void Sys_FreePattern(REGEX *);
BOOL Sys_MatchPattern(REGEX *, const char *);

BOOL Sys_WaitForChar(LONG Timeout);
char Sys_GetChar(void);

LONG Sys_Printf(const char *, ...);
void Sys_SPrintf(char *, const char *, ...);
void Sys_PrintLen(const char *, LONG);

char Sys_CharToLower(char);

void Sys_FlushOutput(void);
void Sys_FlushInput(void);
ULONG Sys_GetClock(void);


/*****
    Allocation mémoire
    - la mémoire allouée est initialisée à zéro.
    Retourne NULL en cas d'echec
*****/

void *Sys_AllocMem(ULONG Size)
{
    return calloc(Size,sizeof(UBYTE));
}


/*****
    Désallocation de la mémoire allouée par Sys_AllocMem()
    Précondition: Ptr peut être NULL
*****/

void Sys_FreeMem(void *Ptr)
{
    if(Ptr!=NULL) free(Ptr);
}


/*****
    Retourne un pointeur sur le répertoire précédent et se positionne sur
    le répertoire désiré.
    La valeur retour doit être désallouée par Sys_RestoreDir()
*****/

CDIR *Sys_CurrentDir(const char *Path)
{
    char *Ptr=get_current_dir_name();
    chdir(Path);
    return (CDIR *)Ptr;
}


/*****
    Désalloue la ressource allouée par Sys_CurrentDir() et se positionne
    sur le répertoire précédent l'appel de Sys_CurrentDit()
*****/

void Sys_RestoreDir(CDIR *PDir)
{
    if(PDir!=NULL)
    {
        chdir((const char *)PDir);
        free((void *)PDir);
    }
}


/*****
    Crée un répertoire
*****/

BOOL Sys_CreateDir(const char *FullPath)
{
    if(mkdir(FullPath,777)==0) return TRUE;
    return FALSE;
}


/*****
    Alloue les ressources nécessaires pour scanner un répertoire
*****/

SDIR *Sys_AllocExamineDir(const char *Path)
{
    SDIR *PSDir=(SDIR *)calloc(sizeof(SDIR),1);

    if(PSDir!=NULL)
    {
        if((PSDir->DEnt=opendir(*Path!=0?Path:"."))!=NULL) return PSDir;
        Sys_FreeExamineDir(PSDir);
    }

    return NULL;
}


/*****
    Désalloue les ressources allouées par Sys_AllocExamineDir()
*****/

void Sys_FreeExamineDir(SDIR *PSDir)
{
    if(PSDir!=NULL)
    {
        if(PSDir->DEnt!=NULL) closedir(PSDir->DEnt);
        free((void *)PSDir);
    }
}


/*****
    Initialise le début du scan de répertoire
*****/

BOOL Sys_ExamineDir(SDIR *PSDir)
{
    return TRUE;
}


/*****
    Retourne un descripteur sur une entrée du répertoire scanné
*****/

BOOL Sys_ExamineDirNext(SDIR *PSDir)
{
    struct dirent *de=readdir(PSDir->DEnt);

    if(de!=NULL)
    {
        struct stat st;
        time_t t;
        struct tm *tm;

        stat(de->d_name,&st);
        t=st.st_mtime;
        tm=localtime(&t);

        PSDir->IsDir=de->d_type==DT_DIR?TRUE:FALSE;
        PSDir->Name=de->d_name;
        PSDir->Year=(LONG)tm->tm_year+1900;
        PSDir->Month=(LONG)tm->tm_mon+1;
        PSDir->Day=(LONG)tm->tm_mday;
        PSDir->Hour=(LONG)tm->tm_hour;
        PSDir->Min=(LONG)tm->tm_min;
        PSDir->Sec=(LONG)tm->tm_sec;
        PSDir->Size=(LONG)st.st_size;
        PSDir->Comment="";
        return TRUE;

    }

    return FALSE;
}


/*****
    Ouverture d'un fichier
*****/

HPTR Sys_Open(const char *Name, LONG Mode)
{
    HPTR Handle=NULL;

    switch(Mode)
    {
        case MODE_OLDFILE:
            Handle=(HPTR)fopen(Name,"rb");
            break;

        case MODE_NEWFILE:
            Handle=(HPTR)fopen(Name,"wb");
            break;

        case MODE_READWRITE:
            Handle=(HPTR)fopen(Name,"rb+");
            break;
    }

    return Handle;
}


/*****
    Fermeture d'un fichier
*****/

void Sys_Close(HPTR Handle)
{
    if(Handle!=NULL) fclose((FILE *)Handle);
}


/*****
    Lecture bufferée d'un fichier
*****/

LONG Sys_Read(HPTR Handle, void *Buffer, LONG Size)
{
    return (LONG)fread(Buffer,sizeof(UBYTE),(int)Size,(FILE *)Handle);
}


/*****
    Ecriture bufferée d'un fichier
*****/

LONG Sys_Write(HPTR Handle, void *Buffer, LONG Size)
{
    return (LONG)fwrite(Buffer,sizeof(UBYTE),(int)Size,(FILE *)Handle);
}


/*****
    Permet de positionner le curseur d'écriture/lecture dans un fichier
*****/

LONG Sys_Seek(HPTR Handle, LONG Pos, LONG Mode)
{
    LONG OffsetEnd,Offset=(LONG)ftell((FILE *)Handle);

    fseek((FILE *)Handle,0,SEEK_END);
    OffsetEnd=(LONG)ftell((FILE *)Handle);

    switch(Mode)
    {
        default:
        case OFFSET_BEGINNING:
            if(Pos<=OffsetEnd) fseek((FILE *)Handle,Pos,SEEK_SET);
            else {fseek((FILE *)Handle,OffsetEnd,SEEK_SET); Offset=-1;}
            break;

        case OFFSET_CURRENT:
            if(Offset+Pos<=OffsetEnd) fseek((FILE *)Handle,Offset+Pos,SEEK_SET);
            else {fseek((FILE *)Handle,OffsetEnd,SEEK_SET); Offset=-1;}
            break;

        case OFFSET_END:
            if(OffsetEnd-Pos>=0) fseek((FILE *)Handle,OffsetEnd-Pos,SEEK_SET);
            else {fseek((FILE *)Handle,0,SEEK_SET); Offset=-1;}
            break;
    }

    return Offset;
}


/*****
    Ecrit un caractère dans un fichier
*****/

LONG Sys_PutC(HPTR Handle, char Char)
{
    return (LONG)fputc((int)Char,(FILE *)Handle);
}


/*****
    Nettoie le buffer système
*****/

void Sys_Flush(HPTR Handle)
{
    fflush((FILE *)Handle);
}


/*****
    Permet de forcer la taille d'un fichier
*****/

LONG Sys_SetFileSize(HPTR Handle, LONG Size)
{
    fflush((FILE *)Handle);
    ftruncate(fileno((FILE *)Handle),(off_t)Size);
    return 0;
}


/*****
    Permet d'attacher un commentaire à un fichier
*****/

BOOL Sys_SetFileComment(const char *Name, const char *Comment)
{
    return TRUE;
}


/*****
    Permet de donner une date à un fichier
*****/

BOOL Sys_SetFileDate(const char *Name, LONG Year, LONG Month, LONG Day, LONG Hour, LONG Min, LONG Sec)
{
    struct tm tm;
    struct utimbuf ut;

    tm.tm_sec=(int)Sec;
    tm.tm_min=(int)Min;
    tm.tm_hour=(int)Hour;
    tm.tm_mday=(int)Day;
    tm.tm_mon=(int)Month-1;
    tm.tm_year=(int)Year-1900;
    tm.tm_sec=(int)Sec;
    tm.tm_isdst=-1;

    ut.actime=mktime(&tm);
    ut.modtime=ut.actime;

    return utime(Name,&ut)==0?TRUE:FALSE;
}


/*****
    Vérifie si un controle+C a été demandé
*****/

BOOL Sys_CheckCtrlC(void)
{
    return FALSE;
}


/*****
    Vérifie si un controle+E a été demandé
*****/

BOOL Sys_CheckCtrlE(void)
{
    return FALSE;
}


/*****
    Affiche l'erreur correspondant à l'Id passé en paramètre
*****/

void Sys_PrintFault(ULONG Id)
{
    const char *Msg="Error";

    switch(Id)
    {
        case ERROR_BREAK:
            Msg="*** Break";
            break;

        case ERROR_OBJECT_IN_USE:
            Msg="File already use";
            break;

        case ERROR_DIR_NOT_FOUND:
            Msg="Directory not found";
            break;

        case ERROR_DIRECTORY_NOT_EMPTY:
            Msg="Directory not empty";
            break;

        case ERROR_SEEK_ERROR:
            Msg="Seek error";
            break;

        case ERROR_DISK_NOT_VALIDATED:
            Msg="Bad disk format";
            break;

        case ERROR_DISK_WRITE_PROTECTED:
            Msg="Disk write protected";
            break;

        case ERROR_COMMENT_TOO_BIG:
            Msg="Comment too long";
            break;

        case ERROR_READ_PROTECTED:
            Msg="Read protected";
            break;

        case ERROR_NOT_A_DOS_DISK:
            Msg="Not a dos disk";
            break;

        case ERROR_NO_DISK:
            Msg="No disk";
            break;

        case ERROR_OBJECT_NOT_FOUND:
            Msg="File not found";
            break;

        case ERROR_DISK_FULL:
            Msg="Disk full";
            break;

        case ERROR_OBJECT_EXISTS:
            Msg="File already exists";
            break;

        case ERROR_NO_FREE_STORE:
            Msg="Not enougth memory";
            break;

        case ERROR_DELETE_PROTECTED:
            Msg="Delete protected";
            break;

        case ERROR_WRITE_PROTECTED:
            Msg="Write protected";
            break;

        case ERROR_REQUIRED_ARG_MISSING:
            Msg="Required argument missing";
            break;
    }

    printf("%s\n",Msg);
}


/*****
    Permet d'obtenir le code de l'erreur courant
*****/

LONG Sys_IoErr(void)
{
    return 0;
}


/*****
    Retourne un pointeur sur la partie nom de fichier d'un chemin
*****/

const char *Sys_FilePart(const char *Name)
{
    const char *Ptr=NULL,*Base=Name;
    while(*Name!=0) if(*(Name++)=='/') Ptr=Name;
    return Ptr!=NULL?Ptr:Base;
}


/*****
    Retourne un pointeur sur la fin du chemin
*****/

const char *Sys_PathPart(const char *Name)
{
    const char *Ptr=NULL,*Base=Name;
    while(*Name!=0) if(*(Name++)=='/') Ptr=Name;
    return Ptr!=NULL?Ptr-1:Base;
}


/*****
    Permet d'ajouter un nom de fichier à un chemin donné
*****/

void Sys_AddPart(char *Base, const char *New, ULONG Size)
{
    LONG Len=(LONG)strlen(Base);

    if(Len<Size)
    {
        if(Len>0 && Base[Len-1]!='/') Base[Len++]='/';
        while(Len<Size && *New!=0) Base[Len++]=*(New++);
    }
}


/*****
    Alloue les ressources nécessaires pour les expressions régulières
*****/

REGEX *Sys_AllocPatternNoCase(const char *Pattern, BOOL *IsPattern)
{
    REGEX *Ptr=calloc(sizeof(REGEX),1);

    if(Ptr!=NULL)
    {
        Ptr->Pattern=Pattern;

        /* Test pour compatibilité avec l'amiga. Le pattern #? est utilisé en dur dans le code de todisk. */
        if(strstr(Pattern,"#?"))
        {
            Ptr->NewPattern=(char *)calloc(sizeof(char),strlen(Pattern)+1);
            if(Ptr->NewPattern!=NULL)
            {
                char *Cur=Ptr->NewPattern,*Tmp;
                while((Tmp=strstr(Pattern,"#?"))!=NULL)
                {
                    while(Cur<Tmp) *(Cur++)=*(Pattern++);
                    *(Cur++)='*';
                    Pattern+=2; /* on zappe le #? */
                }
                while(*Pattern!=0) *(Cur++)=*(Pattern++);
                *Cur=0;
            }
            else
            {
                Sys_FreePattern(Ptr);
                Ptr=NULL;
            }
        }
    }

    if(Ptr!=NULL && IsPattern!=NULL)
    {
        Pattern=Ptr->NewPattern!=NULL?Ptr->NewPattern:Ptr->Pattern;
        if(strstr(Pattern,"*")!=NULL || strstr(Pattern,"?")!=NULL) *IsPattern=TRUE;
    }

    return Ptr;
}


/*****
    Désalloue les ressources allouées par Sys_AllocTegExPatternNoCase()
*****/

void Sys_FreePattern(REGEX *Ptr)
{
    if(Ptr!=NULL)
    {
        if(Ptr->NewPattern!=NULL) free((void *)Ptr->NewPattern);
        free((void *)Ptr);
    }
}


/*****
    Test une expression
*****/

BOOL Sys_MatchPattern(REGEX *Ptr, const char *Str)
{
    const char *Pattern=Ptr->NewPattern!=NULL?Ptr->NewPattern:Ptr->Pattern;
    return fnmatch(Pattern,Str,FNM_CASEFOLD)==0?TRUE:FALSE;
}


/*****
    Attend l'appuie sur une touche jusqu'au timeout
*****/

BOOL Sys_WaitForChar(LONG Timeout)
{
    return TRUE;
}


/*****
    Retourne le code ascii de la touche appuyée
*****/

char Sys_GetChar(void)
{
    return getchar();
}


/*****
    Ecriture d'une chaîne de caractères dans la sortie standard.
*****/

LONG Sys_Printf(const char *String, ...)
{
    va_list arglist;
    va_start(arglist,String);
    vprintf(String,arglist);
    va_end(arglist);

    return 0;
}


/*****
    Formatage d'une chaîne de caractères.
*****/

void Sys_SPrintf(char *Buffer, const char *String,...)
{
    va_list arglist;
    va_start(arglist,String);
    vsprintf(Buffer,String,arglist);
    va_end(arglist);
}


/*****
    Permet d'écrire un chaîne limitée à la taille Len
*****/

void Sys_PrintLen(const char *Str, LONG Len)
{
    fwrite((void *)Str,sizeof(char),(int)Len,stdout);
    fflush(stdout);
}


/*****
    Retourne le code minuscule du caractère passé en paramètre
*****/

char Sys_CharToLower(char Char)
{
    return (Char|32);
}


/*****
    Flush de la sortie courante
*****/

void Sys_FlushOutput(void)
{
    fflush(stdout);
}


/*****
    Flush de l'entrée courante
*****/

void Sys_FlushInput(void)
{
    fflush(stdin);
}


/*****
    Pour obtenir le nombre de secondes courant
*****/

ULONG Sys_GetClock(void)
{
    return (ULONG)clock();
}

#endif