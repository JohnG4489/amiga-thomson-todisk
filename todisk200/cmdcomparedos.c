#include "system.h"
#include "global.h"
#include "cmdcomparedos.h"
#include "disklayer.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    28-08-2018 (Seg)    Adaptation du code suite gestion de la géométrie du disque
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-12-2012 (Seg)    Fix d'un bug dans comparaison des fichiers
    10-04-2011 (Seg)    Comparaison dos d'un disque avec un autre
*/


/***** Prototypes */
void Cmd_CompareDos(struct ToDiskData *, struct DiskLayer *, struct DiskLayer *);

BOOL P_Cmd_CompareFile(struct FileSystem *, struct FileSystem *, char *, LONG, LONG *, LONG *, LONG *, LONG *);


/*****
    Comparaison d'un layer avec un autre
*****/

void Cmd_CompareDos(struct ToDiskData *TDData, struct DiskLayer *DLayerSrc, struct DiskLayer *DLayerDst)
{
    if(!DL_IsSameDiskLayer(DLayerSrc,DLayerDst))
    {
        LONG CountOfNotFound=0,CountOfSame=0,CountOfFile=0,CountOfErr=0;
        struct FileSystem *FSSrc=FS_AllocFileSystem(COUNTOF_TRACK,SIZEOF_SECTOR,COUNTOF_SECTOR_PER_TRACK,TRUE);
        struct FileSystem *FSDst=FS_AllocFileSystem(COUNTOF_TRACK,SIZEOF_SECTOR,COUNTOF_SECTOR_PER_TRACK,TRUE);

        if(FSSrc!=NULL && FSDst!=NULL)
        {
            if(FS_InitFileSystem(FSSrc,DLayerSrc)>=0 && FS_InitFileSystem(FSDst,DLayerDst)>=0)
            {
                BOOL IsExit=FALSE;
                struct FileObject FO;

                Sys_Printf("DSK base            - DSK to compare\n");
                Sys_Printf("===========================================\n");

                FS_ExamineFileObject(FSSrc,&FO);
                while(!IsExit && FS_ExamineNextFileObject(&FO))
                {
                    IsExit=P_Cmd_CompareFile(FSSrc,FSDst,FO.Name,FO.Type,&CountOfNotFound,&CountOfSame,&CountOfFile,&CountOfErr);
                }

                if(!IsExit)
                {
                    Sys_Printf("===========================================\n");
                    Sys_Printf("Same: %ld/%ld - ",(long)CountOfSame,(long)CountOfFile);
                    Sys_Printf("Not found: %ld - ",(long)CountOfNotFound);
                    Sys_Printf("IO Error : %ld\n",(long)CountOfErr);
                }
            }
            else
            {
                Sys_Printf("Can't open disk 1 and/or disk 2\n");
            }
        }

        FS_FreeFileSystem(FSDst);
        FS_FreeFileSystem(FSSrc);
    }
}


/*****
    Teste un fichier
*****/

BOOL P_Cmd_CompareFile(
    struct FileSystem *FSSrc, struct FileSystem *FSDst,
    char *Name, LONG Type,
    LONG *CountOfNotFound, LONG *CountOfSame, LONG *CountOfFile, LONG *CountOfErr)
{
    BOOL IsExit=FALSE;

    (*CountOfFile)++;
    if(!Sys_CheckCtrlC())
    {
        LONG ErrorCodeDst=0;
        struct FSHandle *hsrc,*hdst;

        Sys_Printf("%-12s (%04lx) - ",Name,(long)Type);

        /* Ouverture de la destination */
        hdst=FS_OpenFile(FSDst,FS_MODE_OLDFILE,Name,&Type,TRUE,FALSE,&ErrorCodeDst);
        if(hdst!=NULL)
        {
            Sys_Printf("%-12s (%04lx) : ",Name,(long)Type);

            /* Ouverture de la source */
            hsrc=FS_OpenFile(FSSrc,FS_MODE_OLDFILE,Name,&Type,TRUE,FALSE,&ErrorCodeDst);
            if(hsrc!=NULL)
            {
                BOOL IsOk=TRUE;
                LONG Offset=0,res1,res2;

                do
                {
                    UBYTE b1,b2;

                    res1=FS_ReadFile(hsrc,&b1,sizeof(b1));
                    res2=FS_ReadFile(hdst,&b2,sizeof(b2));
                    if(res1<0 || res2<0)
                    {
                        Sys_Printf("IO Err");
                        (*CountOfErr)++;
                        IsOk=FALSE;
                    }
                    else if(res1>0 && res2>0)
                    {
                        if(b1!=b2)
                        {
                            Sys_Printf("Difference at offset %ld",(long)Offset);
                            IsOk=FALSE;
                        }
                    }
                    else if((res1==0 && res2>0) || (res1>0 && res2==0))
                    {
                        Sys_Printf("Different file size after offset %ld",(long)Offset);
                        IsOk=FALSE;
                    }

                    Offset++;
                }
                while((res1>0 && res2>0) && IsOk);

                if(IsOk) {Sys_Printf("Same"); (*CountOfSame)++;}

                FS_CloseFile(hsrc);
            }
            else
            {
                Sys_Printf("IO Err");
                (*CountOfErr)++;
            }

            FS_CloseFile(hdst);
        }
        else
        {
            Sys_Printf("............ (....) : Not found");
            (*CountOfNotFound)++;
        }

        Sys_Printf("\n");
    }
    else
    {
        Sys_PrintFault(ERROR_BREAK);
        IsExit=TRUE;
    }

    return IsExit;
}
