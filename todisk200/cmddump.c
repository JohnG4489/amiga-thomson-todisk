#include "system.h"
#include "global.h"
#include "cmddump.h"
#include "disklayer.h"
#include "multidos.h"


/*
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2012 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    03-09-2012 (Seg)    Amélioration du code et ajout du dump par pattern de nom
    18-03-2010 (Seg)    Affiche le dump d'un fichier
*/


/***** Prototypes */
void Cmd_DumpPattern(struct ToDiskData *, struct MultiDOS *, const char *);
LONG Cmd_Dump(struct ToDiskData *, struct MultiDOS *, const char *);


/*****
    Affiche le contenu binaire d'un fichier
*****/

void Cmd_DumpPattern(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Pattern)
{
    REGEX *PPatt=Sys_AllocPatternNoCase(Pattern,NULL);

    if(PPatt!=NULL)
    {
        struct MDFileObject *FO=&TDData->FObject;
        LONG ExitCode=1;

        MD_InitFileObject(FO,MD,NULL);
        MD_ExamineFileObject(FO);
        while(ExitCode>=0 && MD_ExamineNextFileObject(FO))
        {
            if(Sys_MatchPattern(PPatt,FO->Name))
            {
                Sys_Printf("Dump file \"%s\"\n",FO->Name);
                ExitCode=Cmd_Dump(TDData,MD,FO->Name);
                if(ExitCode<=0) Sys_PrintFault(ERROR_BREAK);
            }
        }

        Sys_FreePattern(PPatt);
    }
}


/*****
    Affiche le contenu binaire d'un fichier
*****/

LONG Cmd_Dump(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Name)
{
    LONG ExitCode=1;
    struct MDHandle MH;
    LONG ErrorCode=MD_OpenFile(&MH,MD,MD_MODE_OLDFILE,Name,NULL);

    if(ErrorCode>=0)
    {
        UBYTE Buffer[16];
        LONG Offset=0,Size,i;

        while((Size=MD_ReadFile(&MH,Buffer,sizeof(Buffer)))>0)
        {
            if(Sys_CheckCtrlC()) {ExitCode=-1; break;}
            if(Sys_CheckCtrlE()) {ExitCode=0; break;}

            Sys_Printf("%06lx  ",(long)Offset);

            for(i=0;i<sizeof(Buffer);i++)
            {
                if(i<Size) Sys_Printf("%02lx ",(long)Buffer[i]);
                else Sys_Printf("   ");
            }

            Sys_Printf(" ");

            for(i=0;i<Size;i++)
            {
                char Char=Buffer[i];
                Sys_Printf("%lc",(long)(Char>32 && Char<127?Char:'.'));
            }

            Sys_Printf("\n");
            Offset+=Size;
        }

        MD_CloseFile(&MH);
    }
    else
    {
        Sys_Printf("Error when opening file \"%s\": %s\n",Name,MD_GetTextErr(ErrorCode));
    }

    return ExitCode;
}
