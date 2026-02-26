#include "system.h"
#include "global.h"
#include "cmdfinddisk.h"
#include "disklayer.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    11-09-2012 (Seg)    Prise en compte de l'option de recherche recursive
    16-08-2012 (Seg)    Amélioration du log d'information
    11-08-2012 (Seg)    Recherche un disque image equivalent à celui passé en source
*/


/***** Prototypes */
void Cmd_FindDisk(struct ToDiskData *, struct DiskLayer *, const char *, BOOL);

LONG P_Cmd_DeepFindFile(struct ToDiskData *, const UBYTE *, const char *, const char *, REGEX *, BOOL, LONG *, LONG *, LONG *);
LONG P_Cmd_CompareDiskRaw(struct ToDiskData *, const UBYTE *, struct DiskLayer *, const char *, ULONG *);
LONG P_Cmd_LoadDiskSrc(struct DiskLayer *, UBYTE **);
void P_Cmd_CleanLine(LONG);


/*****
    Recherche recursive du disque
*****/

void Cmd_FindDisk(struct ToDiskData *TDData, struct DiskLayer *DLayer, const char *PathName, BOOL IsRecursive)
{
    const char *Path=Sys_PathPart(PathName);
    LONG NewPathPartSize=(LONG)((const char *)Path-PathName)+sizeof(char);
    char *NewPathPart=(char *)Sys_AllocMem(NewPathPartSize);

    Sys_Printf("Find disk like source disk\n");

    if(NewPathPart!=NULL)
    {
        REGEX *PPatt;

        /* Extraction de la partie pattern (si jamais il y en a une) */
        const char *PatternPart=Sys_FilePart(PathName);
        if(Sys_StrLen((const char *)PatternPart)==0) PatternPart="#?.(sap|fd|tds)";

        /* Extraction de la partie chemin */
        Sys_StrCopy(NewPathPart,(char *)PathName,NewPathPartSize);

        PPatt=Sys_AllocPatternNoCase(PatternPart,NULL);
        if(PPatt!=NULL)
        {
            UBYTE *DiskRawBase;
            LONG Result=P_Cmd_LoadDiskSrc(DLayer,&DiskRawBase);

            if(Result>0)
            {
                LONG PreviousNameLen=0;
                LONG CountOfImageFile=0;
                LONG CountOfImageFilePerfect=0;

                Sys_Printf("Scan image file using pattern \"%s\"\n",PatternPart);
                Sys_Printf("=============================\n");

                Result=P_Cmd_DeepFindFile(TDData,DiskRawBase,NewPathPart,NULL,PPatt,IsRecursive,&PreviousNameLen,&CountOfImageFile,&CountOfImageFilePerfect);
                P_Cmd_CleanLine(PreviousNameLen+4+12);
                if(Result>0)
                {
                    Sys_Printf("=============================\n");
                    Sys_Printf("Total files analyzed: %ld - Total perfect match: %ld\n",(long)CountOfImageFile,(long)CountOfImageFilePerfect);
                }

                Sys_FreeMem((void *)DiskRawBase);
            }

            if(!Result) Sys_PrintFault(ERROR_BREAK);

            Sys_FreePattern(PPatt);
        }

        Sys_FreeMem(NewPathPart);
    }
}


/*****
    Recherche récursive ou non du fichier image correspondant à la source
    Retourne:
    - Result: >1=succes, 0=break, <0=erreur
*****/

LONG P_Cmd_DeepFindFile(struct ToDiskData *TDData, const UBYTE *DiskRawBase, const char *PathBase, const char *CurrentDir, REGEX *PPatt, BOOL IsRecursive, LONG *PreviousNameLen, LONG *CountOfImageFile, LONG *CountOfImageFilePerfect)
{
    LONG Result=-1;
    SDIR *PSDir=Sys_AllocExamineDir(CurrentDir!=NULL?CurrentDir:PathBase);

    if(PSDir!=NULL)
    {
        if(Sys_ExamineDir(PSDir))
        {
            Result=1;
            while(Result>0 && Sys_ExamineDirNext(PSDir))
            {
                if(Sys_CheckCtrlC())
                {
                    Result=0;
                    break;
                }

                if(PSDir->IsDir)
                {
                    if(IsRecursive)
                    {
                        LONG NewPathBaseSize=Sys_StrLen(PathBase)+Sys_StrLen(PSDir->Name)+4*sizeof(char);
                        char *NewPathBase=(char *)Sys_AllocMem(NewPathBaseSize);

                        if(NewPathBase!=NULL)
                        {
                            Sys_StrCopy(NewPathBase,PathBase,NewPathBaseSize);
                            Sys_AddPart(NewPathBase,PSDir->Name,(ULONG)NewPathBaseSize);

                            Result=P_Cmd_DeepFindFile(TDData,DiskRawBase,NewPathBase,PSDir->Name,PPatt,IsRecursive,PreviousNameLen,CountOfImageFile,CountOfImageFilePerfect);

                            Sys_FreeMem((void *)NewPathBase);
                        }
                    }
                }
                else
                {
                    const char *Name=(char *)PSDir->Name;
                    if(Sys_MatchPattern(PPatt,Name))
                    {
                        ULONG DLError=DL_SUCCESS;
                        struct DiskLayer *DLDst=DL_OpenDiskLayerAuto(Name,&DLError);

                        if(DLDst!=NULL)
                        {
                            ULONG Rate;

                            /* On efface la ligne précédente pour réécrire dessus */
                            P_Cmd_CleanLine(*PreviousNameLen+4+12);

                            (*CountOfImageFile)++;
                            Sys_Printf("Scan file %ld: %s",(long)*CountOfImageFile,Name);
                            Sys_FlushOutput();

                            *PreviousNameLen=Sys_StrLen(Name);

                            Result=P_Cmd_CompareDiskRaw(TDData,DiskRawBase,DLDst,Name,&Rate);
                            if(Result>0 && Rate>=80)
                            {
                                Sys_Printf("\r* Match %3ld%% * : %s/%s\n",(long)Rate,PathBase,Name);
                                if(Rate>=100) (*CountOfImageFilePerfect)++;
                            }

                            if(!Result) Sys_Printf("\n");

                            DL_Close(DLDst);
                        }
                    }
                }
            }
        }

        Sys_FreeExamineDir(PSDir);
    }

    return Result;
}


/*****
    Comparaison du fichier image avec la source
    Retourne:
    - Result: >1=succes, 0=break, <0=erreur
*****/

LONG P_Cmd_CompareDiskRaw(struct ToDiskData *TDData, const UBYTE *DiskRawBase, struct DiskLayer *DLDst, const char *Name, ULONG *Rate)
{
    LONG Track;

    *Rate=0;
    for(Track=0; Track<COUNTOF_TRACK; Track++)
    {
        ULONG Sector;

        for(Sector=1; Sector<=COUNTOF_SECTOR_PER_TRACK; Sector++)
        {
            UBYTE *SecPtr=(UBYTE *)&DiskRawBase[SIZEOF_SECTOR*(Track*COUNTOF_SECTOR_PER_TRACK+(Sector-1))];

            if(DL_ReadSector(DLDst,Track,Sector,TDData->Buffer))
            {
                ULONG Idx,Error=0;

                for(Idx=0; Idx<SIZEOF_SECTOR; Idx++)
                {
                    if(TDData->Buffer[Idx]!=SecPtr[Idx]) Error++;
                }

                if(Sys_CheckCtrlC()) return 0;

                *Rate+=SIZEOF_SECTOR-Error;
            }
        }
    }

    *Rate=*Rate*100/(COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR);

    return 1;
}


/*****
    Préchargement du fichier image source
    Retourne:
    - Result: >1=succes, 0=break, <0=erreur
*****/

LONG P_Cmd_LoadDiskSrc(struct DiskLayer *DLayer, UBYTE **BufferPtr)
{
    LONG Result=-1;
    *BufferPtr=(UBYTE *)Sys_AllocMem(COUNTOF_TRACK*COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR);

    Sys_Printf("Load source disk to memory...\n");
    if(*BufferPtr!=NULL)
    {
        LONG Track;

        Result=1;
        for(Track=0; Track<COUNTOF_TRACK && Result>0; Track++)
        {
            ULONG Sector;

            for(Sector=1; Sector<=COUNTOF_SECTOR_PER_TRACK && Result>0; Sector++)
            {
                UBYTE *SecPtr=&(*BufferPtr)[SIZEOF_SECTOR*(Track*COUNTOF_SECTOR_PER_TRACK+(Sector-1))];

                Sys_Printf("\rReading track %2ld, sector %2ld",(long)Track,(long)Sector);
                Sys_FlushOutput();

                if(Sys_CheckCtrlC())
                {
                    Sys_Printf("\n");
                    Result=0; /* = break */
                    break;
                }

                if(!DL_ReadSector(DLayer,Track,Sector,SecPtr))
                {
                    Sys_Printf("\n");

                    if(!DL_IsDLFatalError(DLayer->Error)) Sys_Printf("Error Track %ld, Sector %ld: ",(long)Track,(long)Sector);
                    else Result=-1; /* = erreur fatale */

                    Sys_Printf("%s\n",DL_GetDLTextErr(DLayer->Error));
                }
            }
        }

        if(Result>0) Sys_Printf("\n");
        else
        {
            Sys_FreeMem((void *)*BufferPtr);
            *BufferPtr=NULL;
        }
    }

    return Result;
}


/*****
    Petite routine juste pour écrire des espaces pour effacer une ligne
*****/

void P_Cmd_CleanLine(LONG CountOfSpace)
{
    Sys_Printf("\r");
    while(--CountOfSpace>0) Sys_Printf(" ");
    Sys_Printf("\r");
}
