#ifndef SYSTEMAMIGA_H
#define SYSTEMAMIGA_H

#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

typedef BPTR HPTR;
typedef void REGEX;
typedef struct {BPTR lock; BPTR oldlock;} CDIR;

typedef struct
{
    struct FileInfoBlock *Fib;
    BPTR DirLock;
    BPTR PreviousLock;
    BOOL IsDir;
    const char *Name;
    LONG Year;
    LONG Month;
    LONG Day;
    LONG Hour;
    LONG Min;
    LONG Sec;
    LONG Size;
    const char *Comment;
} SDIR;


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

/* declarations necessaires pour OS3.1 et MorphOS */
#if !defined(__amigaos4__) || defined(__MORPHOS__)
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;          /* ouvert par SAS/C */
extern struct Library *UtilityBase;         /* ouvert par le projet */
extern struct GfxBase *GfxBase;             /* ouvert par le projet */
extern struct IntuitionBase *IntuitionBase; /* ouvert par SAS/C */
extern struct Library *CyberGfxBase;        /* ouvert par le projet */
#endif

extern BOOL Sys_OpenAllLibs(void);
extern void Sys_CloseAllLibs(void);
extern void *Sys_AllocMem(ULONG);
extern void Sys_FreeMem(void *);

extern CDIR *Sys_CurrentDir(const char *);
extern void Sys_RestoreDir(CDIR *);
extern BOOL Sys_CreateDir(const char *);

extern SDIR *Sys_AllocExamineDir(const char *);
extern void Sys_FreeExamineDir(SDIR *);
extern BOOL Sys_ExamineDir(SDIR *);
extern BOOL Sys_ExamineDirNext(SDIR *);

#define Sys_Open(Name,Mode) ((HPTR)Open((STRPTR)(Name),Mode))
#define Sys_Close(Handle) Close((BPTR)(Handle))
#define Sys_Read(Handle,Buffer,Size) Read((BPTR)(Handle),Buffer,Size)
#define Sys_Write(Handle,Buffer,Size) Write((BPTR)(Handle),Buffer,Size)
#define Sys_Seek(Handle,Pos,Mode) Seek((BPTR)(Handle),Pos,Mode)
#define Sys_PutC(Handle,Char) FPutC((BPTR)(Handle),Char)
#define Sys_Flush(Handle) Flush((BPTR)(Handle))
#define Sys_SetFileSize(Handle,Size) SetFileSize((BPTR)(Handle),(LONG)(Size),OFFSET_BEGINNING)
#define Sys_SetFileComment(Name,Comment) SetComment((STRPTR)(Name),(STRPTR)(Comment))

extern BOOL Sys_SetFileDate(const char *, LONG, LONG, LONG, LONG, LONG, LONG);

#define Sys_CheckCtrlC() CheckSignal(SIGBREAKF_CTRL_C)
#define Sys_CheckCtrlE() CheckSignal(SIGBREAKF_CTRL_E)

#define Sys_PrintFault(Id) PrintFault(Id,NULL)
#define Sys_IoErr() IoErr()

#define Sys_FilePart(Name) ((const char *)FilePart((STRPTR)(Name)))
#define Sys_PathPart(Name) ((const char *)PathPart((STRPTR)(Name)))
#define Sys_AddPart(Base,New,Size) AddPart((STRPTR)(Base),(STRPTR)(New),Size)

extern REGEX *Sys_AllocPatternNoCase(const char *, BOOL *);
extern void Sys_FreePattern(REGEX *);
extern BOOL Sys_MatchPattern(REGEX *, const char *);

#define Sys_WaitForChar(Timeout) WaitForChar(Input(),(Timeout))
#define Sys_GetChar() ((char)FGetC(Input()))

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
extern LONG Sys_Printf(const char *, ...);
extern void Sys_SPrintf(char *, const char *, ...);
#endif

#define Sys_PrintLen(Str,Len) Write(Output(),(UBYTE *)Str,(LONG)Len)

#define Sys_CharToLower(Char) ToLower(Char)

#define Sys_FlushOutput() Flush(Output())
#define Sys_FlushInput() Flush(Input())

extern ULONG Sys_GetClock(void);
extern ULONG Sys_GetBestModeID(LONG, LONG, LONG);

#endif  /* SYSTEMAMIGA_H */
