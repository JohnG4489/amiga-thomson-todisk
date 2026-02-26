#ifndef MULTIDOS_H
#define MULTIDOS_H

#include "filesystem.h"
#include "filesystemk7.h"


#define MDOS_HOST       0
#define MDOS_DISK       1
#define MDOS_K7         2

#define MD_SUCCESS                  0
#define MD_FILE_NOT_FOUND           -1
#define MD_FILE_ALREADY_EXISTS      -2
#define MD_DIRECTORY_FULL           -3
#define MD_DISK_FULL                -4
#define MD_NOT_ENOUGH_MEMORY        -5
#define MD_DISKLAYER_ERROR          -6
#define MD_FILE_ERROR               -7
#define MD_SUPPORT_PROTECTED        -8
#define MD_OTHER                    -9

#define MD_MODE_OLDFILE             0
#define MD_MODE_NEWFILE             1
#define MD_MODE_READWRITE           2


struct MultiDOS
{
    LONG Type;
    void *FS;
};

struct MDHandle
{
    struct MultiDOS *MD;
    void *Handle;
};


struct MDFileObject
{
    /* public */
    BOOL IsDir;
    const char *Name;
    const char *Comment;
    LONG Day;
    LONG Month;
    LONG Year;
    LONG Hour;
    LONG Min;
    LONG Sec;
    LONG Type;
    LONG Size;
    LONG ExtraData;
    BOOL IsDateOk;
    BOOL IsTimeOk;
    LONG CountOfBlocks;

    /* prive */
    struct FileObject FO;
    struct FileObjectK7 FOK7;
    struct MultiDOS *MD;
    SDIR *PSDir;
};


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern void MD_InitDOSType(struct MultiDOS *, LONG, void *);
extern void MD_Flush(struct MultiDOS *);

extern LONG MD_OpenFile(struct MDHandle *, struct MultiDOS *, ULONG, const char *, LONG *);
extern void MD_CloseFile(struct MDHandle *);
extern LONG MD_ReadFile(struct MDHandle *, UBYTE *, LONG);
extern LONG MD_WriteFile(struct MDHandle *, UBYTE *, LONG);
extern LONG MD_DeleteFile(struct MultiDOS *, const char *, LONG *, BOOL);
extern LONG MD_GetFileSize(struct MDHandle *);
extern LONG MD_SetComment(struct MultiDOS *, const char *, const LONG *, const char *);
extern LONG MD_SetDate(struct MultiDOS *, const char *, LONG *, LONG, LONG, LONG, LONG, LONG, LONG);
extern LONG MD_GetTypeFromHandle(struct MDHandle *);

extern BOOL MD_InitFileObject(struct MDFileObject *, struct MultiDOS *, const char *);
extern void MD_FreeFileObject(struct MDFileObject *);
extern BOOL MD_ExamineFileObject(struct MDFileObject *);
extern BOOL MD_ExamineNextFileObject(struct MDFileObject *);
extern LONG MD_FindFile(struct MultiDOS *, const char *, LONG *, BOOL);

extern const char *MD_GetTextErr(ULONG);

extern void MD_GetVolumeName(struct MultiDOS *, char *);
extern LONG MD_GetFreeSpace(struct MultiDOS *);

#endif  /* MULTIDOS_H */