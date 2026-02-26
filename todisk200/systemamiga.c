#include "system.h"

#ifdef SYSTEM_AMIGA
#include <stdarg.h>
#include <time.h>
#include "systemamiga.h"

/*
    28-08-2018 (Seg)    Ajout fonction Sys_GetBestModeID()
    23-09-2013 (Seg)    Migration des routines spécifique à AmigaOS
*/


struct Library *UtilityBase=NULL;
struct GfxBase *GfxBase=NULL;
struct Library *CyberGfxBase=NULL;


/***** Prototypes */
BOOL Sys_OpenAllLibs(void);
void Sys_CloseAllLibs(void);
void *Sys_AllocMem(ULONG);
void Sys_FreeMem(void *);

CDIR *Sys_CurrentDir(const char *);
void Sys_RestoreDir(CDIR *);
BOOL Sys_CreateDir(const char *);

SDIR *Sys_AllocExamineDir(const char *);
void Sys_FreeExamineDir(SDIR *);
BOOL Sys_ExamineDir(SDIR *);
BOOL Sys_ExamineDirNext(SDIR *);

BOOL Sys_SetFileDate(const char *, LONG, LONG, LONG, LONG, LONG, LONG);

REGEX *Sys_AllocPatternNoCase(const char *, BOOL *);
void Sys_FreePattern(REGEX *);
BOOL Sys_MatchPattern(REGEX *, const char *);

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
LONG Sys_Printf(const char *, ...);
void Sys_SPrintf(char *, const char *,...);
#endif

ULONG Sys_GetClock(void);
ULONG Sys_GetBestModeID(LONG, LONG, LONG);


/*****
    Ouverture des librairies system.
*****/


BOOL Sys_OpenAllLibs(void)
{
    UtilityBase=OpenLibrary("utility.library",36L);
    GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",36L);
    CyberGfxBase=OpenLibrary(CYBERGFXNAME,CYBERGFX_INCLUDE_VERSION);

    if(UtilityBase!=NULL && GfxBase!=NULL) return TRUE;

    return FALSE;
}


/*****
    Fermeture des librairies system.
*****/

void Sys_CloseAllLibs(void)
{
    CloseLibrary(UtilityBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary(CyberGfxBase);
}


/*****
    Allocation mémoire
    - la mémoire allouée est initialisée à zéro.
    Retourne NULL en cas d'echec
*****/

void *Sys_AllocMem(ULONG Size)
{
    return AllocVec(Size,MEMF_ANY|MEMF_CLEAR);
}


/*****
    Désallocation de la mémoire allouée par Sys_AllocMem()
    Précondition: Ptr peut être NULL
*****/

void Sys_FreeMem(void *Ptr)
{
    FreeVec(Ptr);
}


/*****
    Retourne un pointeur sur le répertoire précédent et se positionne sur
    le répertoire désiré.
    La valeur retour doit être désallouée par Sys_RestoreDir()
*****/

CDIR *Sys_CurrentDir(const char *Path)
{
    CDIR *PDir=(CDIR *)Sys_AllocMem(sizeof(CDIR));
    if(PDir!=NULL)
    {
        PDir->lock=Lock((STRPTR)Path,ACCESS_READ);
        if(PDir->lock!=NULL)
        {
            PDir->oldlock=CurrentDir(PDir->lock);
            if(PDir->oldlock!=NULL) return PDir;
        }

        Sys_RestoreDir(PDir);
    }

    return NULL;
}


/*****
    Désalloue la ressource allouée par Sys_CurrentDir() et se positionne
    sur le répertoire précédent l'appel de Sys_CurrentDit()
*****/

void Sys_RestoreDir(CDIR *PDir)
{
    if(PDir!=NULL)
    {
        if(PDir->oldlock!=NULL) CurrentDir(PDir->oldlock);
        if(PDir->lock!=NULL) UnLock(PDir->lock);
        Sys_FreeMem((void *)PDir);
    }
}


/*****
    Crée un répertoire
*****/

BOOL Sys_CreateDir(const char *FullPath)
{
    BPTR lock=CreateDir((STRPTR)FullPath);
    UnLock(lock); /* lock NULL autorise */
    return (BOOL)(lock!=NULL?TRUE:FALSE);
}


/*****
    Alloue les ressources nécessaires pour scanner un répertoire
*****/

SDIR *Sys_AllocExamineDir(const char *Path)
{
    SDIR *PSDir=(SDIR *)Sys_AllocMem(sizeof(SDIR));

    if(PSDir!=NULL)
    {
        if((PSDir->Fib=(struct FileInfoBlock *)AllocDosObjectTags(DOS_FIB,TAG_DONE))!=NULL)
        {
            if((PSDir->DirLock=Lock((STRPTR)Path,ACCESS_READ))!=NULL)
            {
                PSDir->PreviousLock=CurrentDir(PSDir->DirLock);
                return PSDir;
            }
        }

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
        if(PSDir->Fib!=NULL)
        {
            if(PSDir->PreviousLock!=NULL) CurrentDir(PSDir->PreviousLock);
            UnLock(PSDir->DirLock);

            FreeDosObject(DOS_FIB,(void *)PSDir->Fib);
        }

        Sys_FreeMem((void *)PSDir);
    }
}


/*****
    Initialise le début du scan de répertoire
*****/

BOOL Sys_ExamineDir(SDIR *PSDir)
{
    return (BOOL)Examine(PSDir->DirLock,PSDir->Fib);
}


/*****
    Retourne un descripteur sur une entrée du répertoire scanné
*****/

BOOL Sys_ExamineDirNext(SDIR *PSDir)
{
    BOOL IsSuccess=ExNext(PSDir->DirLock,PSDir->Fib);

    if(IsSuccess)
    {
        struct ClockData cd;
        struct DateStamp *ds=&PSDir->Fib->fib_Date;
        LONG Second=ds->ds_Days*24*60*60+ds->ds_Minute*60+ds->ds_Tick/TICKS_PER_SECOND;

        Amiga2Date(Second,&cd);

        PSDir->IsDir=PSDir->Fib->fib_DirEntryType>0?TRUE:FALSE;
        PSDir->Name=PSDir->Fib->fib_FileName;
        PSDir->Year=(LONG)cd.year;
        PSDir->Month=(LONG)cd.month;
        PSDir->Day=(LONG)cd.mday;
        PSDir->Hour=(LONG)cd.hour;
        PSDir->Min=(LONG)cd.min;
        PSDir->Sec=(LONG)cd.sec;
        PSDir->Size=(LONG)PSDir->Fib->fib_Size;
        PSDir->Comment=PSDir->Fib->fib_Comment;
    }

    return IsSuccess;
}


/*****
    Permet de donner une date à un fichier
*****/

BOOL Sys_SetFileDate(const char *Name, LONG Year, LONG Month, LONG Day, LONG Hour, LONG Min, LONG Sec)
{
    struct DateStamp ds;
    struct ClockData cd;
    LONG Date;

    cd.year=Year;
    cd.month=Month;
    cd.mday=Day;
    cd.hour=Hour;
    cd.min=Min;
    cd.sec=Sec;
    Date=Date2Amiga(&cd);

    ds.ds_Days=Date/(24*60*60);
    ds.ds_Minute=(Date-ds.ds_Days*24*60*60)/60;
    ds.ds_Tick=((Date-ds.ds_Days*24*60*60)-ds.ds_Minute*60)*TICKS_PER_SECOND;

    return (BOOL)SetFileDate((STRPTR)Name,&ds);
}

/*****
    Alloue les ressources nécessaires pour les expressions régulières
*****/

REGEX *Sys_AllocPatternNoCase(const char *Pattern, BOOL *IsPattern)
{
    LONG BufferLen=Sys_StrLen(Pattern)*2+3;
    REGEX *BufferPtr=(REGEX *)Sys_AllocMem(BufferLen);

    if(BufferPtr!=NULL)
    {
        LONG Result=ParsePatternNoCase((STRPTR)Pattern,(STRPTR)BufferPtr,BufferLen);
        if(Result>=0)
        {
            if(IsPattern!=NULL) *IsPattern=Result>0?TRUE:FALSE;
            return BufferPtr;
        }

        Sys_FreePattern(BufferPtr);
    }

    return NULL;
}


/*****
    Désalloue les ressources allouées par Sys_AllocTegExPatternNoCase()
*****/

void Sys_FreePattern(REGEX *Ptr)
{
    if(Ptr!=NULL) Sys_FreeMem((void *)Ptr);
}


/*****
    Test une expression
*****/

BOOL Sys_MatchPattern(REGEX *Pattern, const char *Str)
{
    return MatchPatternNoCase((STRPTR)Pattern,(STRPTR)Str);
}


/*****
    Ecriture d'une chaîne de caractères dans la sortie standard.
*****/
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
LONG Sys_Printf(const char *String, ...)
{
    LONG Error;
    va_list argptr;

    va_start(argptr,String);
    Error=VPrintf((STRPTR)String,argptr);
    va_end(argptr);

    return Error;      /*  count/error */
}
#endif


/*****
    Formatage d'une chaîne de caractères.
*****/
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
void Sys_SPrintf(char *Buffer, const char *String,...)
{
    va_list argptr;

    va_start(argptr,String);
    RawDoFmt((STRPTR)String,argptr,(void (*))"\x16\xc0\x4e\x75",Buffer);
    /*sprintf(Buffer,String,argptr);*/
    va_end(argptr);
}
#endif


/*****
    Pour obtenir le nombre de secondes courant
*****/

ULONG Sys_GetClock(void)
{
    return (ULONG)clock();
}


/*****
    Recherche du meilleur ModeID
*****/

ULONG Sys_GetBestModeID(LONG Width, LONG Height, LONG Depth)
{
    ULONG ModeID=INVALID_ID;

    if(CyberGfxBase)
    {
        /* Recuperation du ScreenModeID via cybergraphics.library */
        ModeID=BestCModeIDTags(
            CYBRBIDTG_NominalWidth,Width,
            CYBRBIDTG_NominalHeight,Height,
            CYBRBIDTG_Depth,Depth,
            TAG_DONE);
    }

    if(ModeID==INVALID_ID)
    {
        ModeID=LORES_KEY;

        if(GfxBase->LibNode.lib_Version>=39)
        {
            /* Recuperation du ScreenModeID via graphics.library */
            ModeID=BestModeID(
                BIDTAG_NominalWidth,Width,
                BIDTAG_NominalHeight,Height,
                BIDTAG_Depth,Depth,
                TAG_DONE);
        }
    }

    return ModeID;
}

#endif