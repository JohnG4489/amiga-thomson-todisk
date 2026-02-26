#include "system.h"
#include "global.h"
#include "cmddir.h"
#include "disklayer.h"
#include "multidos.h"
#include "util.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    27-08-2018 (Seg)    Remaniement suite gestion des noms avec accents
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    30-08-2006 (Seg)    Affiche le directory d'un disque
*/


/***** Prototypes */
void Cmd_Dir(struct ToDiskData *, struct MultiDOS *, const char *);
void P_Cmd_PrintFileInfo(struct MDFileObject *);


/*****
    Affiche le directory
*****/

void Cmd_Dir(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Pattern)
{
    REGEX *PPatt=Sys_AllocPatternNoCase(Pattern,NULL);

    if(PPatt!=NULL)
    {
        struct MDFileObject *FO=&TDData->FObject;
        char Name[13];
        LONG CountOfFiles=0,TotalSize=0,TotalBlock=0;
        BOOL IsExit=FALSE;

        /* Lecture du nom de volume */
        MD_GetVolumeName(MD,Name);
        if(Sys_StrLen(Name)==0) Sys_SPrintf(Name,"<no name>");
        Sys_Printf("Volume name: '%s' - Free space: %ld KB\n",Name,MD_GetFreeSpace(MD));

        Sys_Printf("============================================================\n");
        Sys_Printf("Name          Type  Size  Blk  Date       Time      Comment\n");
        Sys_Printf("============================================================\n");

        /* Lecture des informations des fichiers */
        MD_InitFileObject(FO,MD,NULL);
        MD_ExamineFileObject(FO);
        while(!IsExit)
        {
            if(!Sys_CheckCtrlC())
            {
                if(MD_ExamineNextFileObject(FO))
                {
                    if(Sys_MatchPattern(PPatt,FO->Name))
                    {
                        P_Cmd_PrintFileInfo(FO);
                        TotalSize+=FO->Size;
                        TotalBlock+=2*FO->CountOfBlocks;
                        CountOfFiles++;
                    }
                } else IsExit=TRUE;
            }
            else
            {
                Sys_PrintFault(ERROR_BREAK);
                IsExit=TRUE;
            }
        }

        Sys_Printf("------------------------------------------------------------\n");
        Sys_Printf("%3ld file(s) %12ld %4ld\n",CountOfFiles,TotalSize,TotalBlock);

        Sys_FreePattern(PPatt);
    }
}


/*****
    Affiche toutes les informations concernant le fichier pointe par FileInfo
*****/

void P_Cmd_PrintFileInfo(struct MDFileObject *FO)
{
    static const char CharTab[]={'B','D','M','A','4','5','6','7','8','9'};
    char Name[13],Date[11],Time[9];
    char Type=(66+FO->Type)&255;

    Type=(char)(Type<32 || Type>127?'~':Type);

    /* Recuperation du nom sans conversion! */
    Utl_ConvertHostNameToThomsonName(FO->Name,(UBYTE *)Name);
    Name[12]=0;
    Name[11]=Name[10];
    Name[10]=Name[9];
    Name[9]=Name[8];
    Name[8]=' ';

    /* Recuperation des informations sur la date */
    Sys_SPrintf(Date,"..........");
    Sys_SPrintf(Time,"........");
    if(FO->IsDateOk)
    {
        Sys_SPrintf(Date,"%02ld/%02ld/%04ld",FO->Day,FO->Month,FO->Year);

        if(FO->IsTimeOk)
        {
            Sys_SPrintf(Time,"%02ld:%02ld:%02ld",FO->Hour,FO->Min,FO->Sec);
        }
    }

    /* Affichage de la ligne d'informations */
    Sys_Printf("%12s  %lc %lc %6ld  %3ld  %-10s %-9s %s\n",
            Name,
            (LONG)(FO->Type>>8<10?CharTab[FO->Type>>8]:'*'),
            (LONG)Type,
            FO->Size,
            2*FO->CountOfBlocks,
            Date,
            Time,
            FO->Comment);
}
