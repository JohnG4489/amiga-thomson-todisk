#ifndef SYSTEMUNIX_H
#define SYSTEMUNIX_H

#include <dirent.h>


typedef long            LONG;       /* signed 32-bit quantity */
typedef unsigned long   ULONG;      /* unsigned 32-bit quantity */
typedef short           WORD;       /* signed 16-bit quantity */
typedef unsigned short  UWORD;      /* unsigned 16-bit quantity */
typedef char            BYTE;       /* signed 8-bit quantity */
typedef unsigned char   UBYTE;      /* unsigned 8-bit quantity */
typedef float           FLOAT;
typedef double          DOUBLE;
typedef long            BOOL;
typedef void           *HPTR;
typedef void            CDIR;

typedef struct
{
    const char *Pattern;
    char *NewPattern;
} REGEX;

typedef struct
{
    DIR *DEnt;
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

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif
#ifndef NULL
#define NULL            0
#endif

#define MODE_OLDFILE                1005 /* code amigados */
#define MODE_NEWFILE                1006 /* code amigados */
#define MODE_READWRITE              1004 /* code amigados */

#define OFFSET_BEGINNING           -1 /* code amigados */
#define OFFSET_CURRENT              0 /* code amigados */
#define OFFSET_END                  1 /* code amigados */

#define ERROR_BREAK                 304 /* code amigados */

#define ERROR_OBJECT_IN_USE         202 /* code amigados */
#define ERROR_DIR_NOT_FOUND         204 /* code amigados */
#define ERROR_DIRECTORY_NOT_EMPTY   216 /* code amigados */
#define ERROR_SEEK_ERROR            219 /* code amigados */
#define ERROR_DISK_NOT_VALIDATED    213 /* code amigados */
#define ERROR_DISK_WRITE_PROTECTED  214 /* code amigados */
#define ERROR_COMMENT_TOO_BIG       220 /* code amigados */
#define ERROR_READ_PROTECTED        224 /* code amigados */
#define ERROR_NOT_A_DOS_DISK        225 /* code amigados */
#define ERROR_NO_DISK               226 /* code amigados */
#define ERROR_OBJECT_NOT_FOUND      205 /* code amigados */
#define ERROR_DISK_FULL             221 /* code amigados */
#define ERROR_OBJECT_EXISTS         203 /* code amigados */
#define ERROR_NO_FREE_STORE         103 /* code amigados */
#define ERROR_DELETE_PROTECTED      222 /* code amigados */
#define ERROR_WRITE_PROTECTED       223 /* code amigados */
#define ERROR_REQUIRED_ARG_MISSING  116 /* code amigados */

/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

#define Sys_OpenAllLibs() (1)
#define Sys_CloseAllLibs()

extern void *Sys_AllocMem(ULONG);
extern void Sys_FreeMem(void *);

extern CDIR *Sys_CurrentDir(const char *);
extern void Sys_RestoreDir(CDIR *);
extern BOOL Sys_CreateDir(const char *);

extern SDIR *Sys_AllocExamineDir(const char *);
extern void Sys_FreeExamineDir(SDIR *);
extern BOOL Sys_ExamineDir(SDIR *);
extern BOOL Sys_ExamineDirNext(SDIR *);

extern HPTR Sys_Open(const char *, LONG);
extern void Sys_Close(HPTR);
extern LONG Sys_Read(HPTR, void *, LONG);
extern LONG Sys_Write(HPTR, void *, LONG);
extern LONG Sys_Seek(HPTR, LONG, LONG);
extern LONG Sys_PutC(HPTR, char);
extern void Sys_Flush(HPTR);
extern LONG Sys_SetFileSize(HPTR, LONG);
extern BOOL Sys_SetFileComment(const char *, const char *);
extern BOOL Sys_SetFileDate(const char *, LONG, LONG, LONG, LONG, LONG, LONG);

extern BOOL Sys_CheckCtrlC(void);
extern BOOL Sys_CheckCtrlE(void);

extern void Sys_PrintFault(ULONG);
extern LONG Sys_IoErr(void);

extern const char *Sys_FilePart(const char *);
extern const char *Sys_PathPart(const char *);
extern void Sys_AddPart(char *, const char *, ULONG);

extern REGEX *Sys_AllocPatternNoCase(const char *, BOOL *);
extern void Sys_FreePattern(REGEX *);
extern BOOL Sys_MatchPattern(REGEX *, const char *);

extern BOOL Sys_WaitForChar(LONG);
extern char Sys_GetChar(void);

extern LONG Sys_Printf(const char *, ...);
extern void Sys_SPrintf(char *, const char *, ...);
extern void Sys_PrintLen(const char *, LONG);

extern char Sys_CharToLower(char);

extern void Sys_FlushOutput(void);
extern void Sys_FlushInput(void);
extern ULONG Sys_GetClock(void);

#endif  /* SYSTEMUNIX_H */
