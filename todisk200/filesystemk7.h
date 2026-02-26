#ifndef FILESYSTEMK7_H
#define FILESYSTEMK7_H


/* Offsets dans le descripteur de fichier */
#define K7FIO_NAME                  0
#define K7FIO_SUFFIX                8
#define K7FIO_TYPE                  11

/* Retours d'erreurs du FS */
#define K7_SUCCESS                  0
#define K7_FILE_NOT_FOUND           -1
#define K7_FILE_ERROR               -7


struct FileSystemK7
{
    UBYTE *BufferPtr;
    LONG BufferSize;
    LONG CurrentOffset;
};


struct FSHandleK7
{
    struct FileSystemK7 *FS;
    LONG OffsetHeader;
    LONG OffsetData;
    LONG CurrentOffset;
    BOOL IsEOF;
    BOOL IsMo5;
    LONG Sum;
    LONG Len;
    LONG Type;
};


struct FileObjectK7
{
    struct FileSystemK7 *FS;
    char Name[13];
    LONG Type;
    LONG Size;
    LONG OffsetHeader;
    LONG OffsetData;
    LONG OffsetEnd;
    LONG Offset;
    UBYTE FileInfo[13];
};


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern struct FileSystemK7 *K7_AllocFileSystem(const char *);
extern void K7_FreeFileSystem(struct FileSystemK7 *);
extern void K7_ExamineFileObject(struct FileSystemK7 *, struct FileObjectK7 *);
extern BOOL K7_ExamineNextFileObject(struct FileObjectK7 *);
extern struct FSHandleK7 *K7_OpenFile(struct FileSystemK7 *, const char *, LONG *, BOOL, LONG *);
extern LONG K7_CloseFile(struct FSHandleK7 *);
extern LONG K7_ReadFile(struct FSHandleK7 *, UBYTE *, LONG);
extern LONG K7_Seek(struct FSHandleK7 *, LONG);
extern LONG K7_FindFile(struct FileSystemK7 *, const char *, LONG *, BOOL, LONG *);
extern LONG K7_GetSize(struct FSHandleK7 *);
extern LONG K7_GetTypeFromHandle(struct FSHandleK7 *);


#endif  /* FILESYSTEMK7_H */