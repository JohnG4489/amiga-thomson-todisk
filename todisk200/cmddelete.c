#include "system.h"
#include "global.h"
#include "cmddelete.h"
#include "disklayer.h"
#include "multidos.h"


/*
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    03-09-2012 (Seg)    Amélioration du code
    31-08-2006 (Seg)    Effacement de fichier
*/


/***** Prototypes */
void Cmd_Delete(struct ToDiskData *, struct MultiDOS *, const char *);


/*****
    Permet d'effacer le ou les fichiers correspondants au pattern
*****/

void Cmd_Delete(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Pattern)
{
    REGEX *PPatt=Sys_AllocPatternNoCase(Pattern,NULL);

    if(PPatt!=NULL)
    {
        struct MDFileObject *FO=&TDData->FObject;
        LONG Count=0;
        BOOL IsExit=FALSE;

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
                        if(MD_DeleteFile(MD,FO->Name,&FO->Type,FALSE)>=0)
                        {
                            LONG Len=16-Sys_StrLen(FO->Name);

                            Sys_Printf("\"%s\"",FO->Name);
                            while(--Len>=0) Sys_Printf(".");
                            Sys_Printf("deleted\n");

                            Count++;
                        }
                        else
                        {
                            Sys_Printf("Error when deleting file %s!\n",FO->Name);
                        }
                    }
                } else IsExit=TRUE;
            }
            else
            {
                Sys_PrintFault(ERROR_BREAK);
                IsExit=TRUE;
            }
        }

        MD_Flush(MD);

        Sys_Printf("%ld file(s) deleted\n",Count);

        Sys_FreePattern(PPatt);
    }
}