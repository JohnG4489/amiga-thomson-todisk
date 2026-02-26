#include "system.h"
#include "global.h"
#include "disklayer.h"
#include "cmdbackup.h"
#include "cmdformat.h"
#include "cmdcopy.h"
#include "filesystemk7.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    28-08-2018 (Seg)    Adaptation du code suite gestion de la géométrie du disque
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    21-01-2013 (Seg)    Ajout de Cmd_BackupFromK7() pour convertir des K7 thomson en disquette
    09-04-2011 (Seg)    Gestion du backup avec preformatage de piste
    01-09-2006 (Seg)    Copie d'un disque
*/


/***** Prototypes */
void Cmd_Backup(struct ToDiskData *, struct DiskLayer *, struct DiskLayer *, ULONG);
void Cmd_BackupFromK7(struct ToDiskData *, const char *, struct DiskLayer *, ULONG);

BOOL P_Cmd_ReadTrack(struct DiskLayer *, ULONG, UBYTE *);
BOOL P_Cmd_WriteTrack(struct DiskLayer *, ULONG, UBYTE *);
BOOL P_Cmd_FormatWriteTrack(struct DiskLayer *, ULONG, UBYTE *, ULONG);


/*****
    Copie d'un disque thomson
    - Interleave: 0=ne pas formater les pistes, >0=formater les pistes
*****/

void Cmd_Backup(struct ToDiskData *TDData, struct DiskLayer *DLayerSrc, struct DiskLayer *DLayerDst, ULONG Interleave)
{
    BOOL IsExit=FALSE;
    LONG Track;

    /* Message de bienvenue */
    Sys_Printf("Starting backup...\n");
    if(Interleave>0) Sys_Printf("Force track interleave to %ld\n",(long)Interleave);
    else Sys_Printf("No track interleave defined (the disk must be already formated)\n");

    if(!DL_IsSameDiskLayer(DLayerSrc,DLayerDst))
    {
        /* On travaille sur un support different. Pas besoin de stocker en memoire */
        for(Track=0;Track<COUNTOF_TRACK && !IsExit;Track++)
        {
            IsExit=P_Cmd_ReadTrack(DLayerSrc,Track,TDData->Buffer);

            if(!IsExit)
            {
                if(Interleave>0) IsExit=P_Cmd_FormatWriteTrack(DLayerDst,Track,TDData->Buffer,Interleave);
                else IsExit=P_Cmd_WriteTrack(DLayerDst,Track,TDData->Buffer);
            }
        }
    }
    else
    {
        /* On travaille sur le meme peripherique. On doit tout mettre en memoire */
        UBYTE *Ptr=(UBYTE *)Sys_AllocMem(COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR);

        if(Ptr!=NULL)
        {
            LONG Track;

            Sys_Printf("Read source into memory\n");

            for(Track=0; Track<COUNTOF_TRACK && !IsExit; Track++)
            {
                IsExit=P_Cmd_ReadTrack(DLayerSrc,Track,&Ptr[Track*SIZEOF_TRACK]);
            }

            if(!IsExit)
            {
                Sys_Printf("\nInsert Disk Destination and press Enter.\n");
                Sys_GetChar();
                Sys_Printf("Copy Memory buffer into Destination Disk...\n");

                for(Track=0;Track<COUNTOF_TRACK && !IsExit;Track++)
                {
                    if(Interleave>0) IsExit=P_Cmd_FormatWriteTrack(DLayerDst,Track,&Ptr[Track*SIZEOF_TRACK],Interleave);
                    else IsExit=P_Cmd_WriteTrack(DLayerDst,Track,&Ptr[Track*SIZEOF_TRACK]);
                }
            }

            Sys_FreeMem((void *)Ptr);
        }
    }

    if(!IsExit) Sys_Printf("\nCompleted!\n");
}


/*****
    Conversion du contenu d'un fichier K7 en disque thomson
    - Interleave: 0=ne pas formater les pistes, >0=formater les pistes
*****/

void Cmd_BackupFromK7(struct ToDiskData *TDData, const char *FileNameK7, struct DiskLayer *DLayerDst, ULONG Interleave)
{
    Sys_Printf("Starting K7 conversion...\nStep 1: Format destination\n");
    if(Cmd_Format(TDData,DLayerDst,"",Interleave))
    {
        BOOL IsSuccess=FALSE;
        struct MultiDOS MDSrc,MDDst;

        MD_InitDOSType(&MDSrc,MDOS_K7,(void *)K7_AllocFileSystem(FileNameK7));
        MD_InitDOSType(&MDDst,MDOS_DISK,(void *)FS_AllocFileSystem(COUNTOF_TRACK,SIZEOF_SECTOR,COUNTOF_SECTOR_PER_TRACK,TRUE));

        if(MDDst.FS!=NULL) IsSuccess=FS_InitFileSystem(MDDst.FS,DLayerDst)>=0?TRUE:FALSE;

        if(MDSrc.FS!=NULL && IsSuccess)
        {
            struct MDFileObject MDFO;

            Sys_Printf("Step 2: Copy files\n");

            /* Debut de scan du repertoire */
            MD_InitFileObject(&MDFO,&MDSrc,NULL);
            IsSuccess=MD_ExamineFileObject(&MDFO);
            if(IsSuccess)
            {
                while(MD_ExamineNextFileObject(&MDFO))
                {
                    ULONG CopyMode=COPY_MODE_RENAME_ALL;

                    if(Sys_CheckCtrlC())
                    {
                        Sys_PrintFault(ERROR_BREAK);
                        break;
                    }

                    Cmd_CopyFile(TDData,&CopyMode,
                        &MDSrc,NULL,MDFO.Name,&MDFO.Type,MDFO.Year,MDFO.Month,MDFO.Day,MDFO.Hour,MDFO.Min,MDFO.Sec,MDFO.Comment,MDFO.Type,MDFO.ExtraData,
                        &MDDst,NULL,MDFO.Name);
                }
            }
        }

        if(MDDst.FS!=NULL) FS_FreeFileSystem((struct FileSystem *)MDDst.FS);
        if(MDSrc.FS!=NULL) K7_FreeFileSystem((struct FileSystemK7 *)MDSrc.FS);

        Sys_Printf("Conversion completed\n");
    }
}


/*****
    Lecture complete de la piste
*****/

BOOL P_Cmd_ReadTrack(struct DiskLayer *DLayerSrc, ULONG Track, UBYTE *BufferTrack)
{
    BOOL IsExit=FALSE;
    ULONG Sector;

    Sys_Printf("Read source track %-16ld\r",(long)Track);
    for(Sector=1; Sector<=COUNTOF_SECTOR_PER_TRACK && !IsExit; Sector++)
    {
        UBYTE *SecPtr=&BufferTrack[(Sector-1)*SIZEOF_SECTOR];

        if(!Sys_CheckCtrlC())
        {
            if(!DL_ReadSector(DLayerSrc,Track,Sector,SecPtr))
            {
                Sys_Printf("\n");

                if(!DL_IsDLFatalError(DLayerSrc->Error)) Sys_Printf("Error Track %ld, Sector %ld: ",(long)Track,(long)Sector);
                else IsExit=TRUE;

                Sys_Printf("%s\n",DL_GetDLTextErr(DLayerSrc->Error));
            }
        }
        else
        {
            Sys_Printf("\n");
            Sys_PrintFault(ERROR_BREAK);
            IsExit=TRUE;
        }
    }

    return IsExit;
}


/*****
    Ecriture complete de la piste
*****/

BOOL P_Cmd_WriteTrack(struct DiskLayer *DLayerDst, ULONG Track, UBYTE *BufferTrack)
{
    BOOL IsExit=FALSE;
    ULONG Sector;

    Sys_Printf("Write destination track %-3ld\r",(long)Track);
    for(Sector=1; Sector<=COUNTOF_SECTOR_PER_TRACK && !IsExit; Sector++)
    {
        UBYTE *SecPtr=&BufferTrack[(Sector-1)*SIZEOF_SECTOR];

        if(!Sys_CheckCtrlC())
        {
            if(!DL_WriteSector(DLayerDst,Track,Sector,SecPtr))
            {
                Sys_Printf("\n");

                if(!DL_IsDLFatalError(DLayerDst->Error)) Sys_Printf("Error Track %ld, Sector %ld: ",(long)Track,(long)Sector);
                else IsExit=TRUE;

                Sys_Printf("%s\n",DL_GetDLTextErr(DLayerDst->Error));
            }
        }
        else
        {
            Sys_Printf("\n");
            Sys_PrintFault(ERROR_BREAK);
            IsExit=TRUE;
        }
    }

    return IsExit;
}


/*****
    Ecriture complete de la piste avec formatage
*****/

BOOL P_Cmd_FormatWriteTrack(struct DiskLayer *DLayerDst, ULONG Track, UBYTE *BufferTrack, ULONG Interleave)
{
    BOOL IsExit=FALSE;

    Sys_Printf("Format/Write destination track %-3ld\r",(long)Track);
    if(!Sys_CheckCtrlC())
    {
        if(!DL_FormatTrack(DLayerDst,Track,Interleave,BufferTrack))
        {
            Sys_Printf("\n");

            if(!DL_IsDLFatalError(DLayerDst->Error)) Sys_Printf("Error Track %ld: ",(long)Track);
            else IsExit=TRUE;

            Sys_Printf("%s\n",DL_GetDLTextErr(DLayerDst->Error));
        }
    }
    else
    {
        Sys_Printf("\n");
        Sys_PrintFault(ERROR_BREAK);
        IsExit=TRUE;
    }

    return IsExit;
}
