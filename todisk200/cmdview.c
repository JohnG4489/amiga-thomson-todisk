#include "system.h"
#include "global.h"
#include "multidos.h"
#include "cmdview.h"
#include "cmdviewand.h"
#include "cmdviewmap.h"
#include "cmddump.h"
#include "convert.h"
#include "util.h"


/*
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Utilisation de MultiDOS au lieu de FileSystem
    13-01-2013 (Seg)    Légère modification: déplacement d'une fonction dans util.c
    11-09-2012 (Seg)    Modification des visus ASS, ASM et BAS pour prendre en compte
                        le jeu de caracteres G2 du thomson.
    03-09-2012 (Seg)    Visualisation de fichiers par pattern de nom
*/


/***** Prototypes */
void Cmd_View(struct ToDiskData *, struct MultiDOS *, const char *);


/*****
    Visualisation d'un fichier
*****/

void Cmd_View(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Pattern)
{
    REGEX *PPatt;

    if((PPatt=Sys_AllocPatternNoCase(Pattern,NULL))!=NULL)
    {
        UBYTE BufferFileContent[32]; /* La taille est volontairement petite pour pouvoir breaker rapidement */
        LONG BufferFileContentSize=sizeof(BufferFileContent);
        UBYTE BufferConvAcc[32];
        LONG BufferConvAccSize=sizeof(BufferConvAcc);
        struct MDFileObject *FO=&TDData->FObject;
        struct MDHandle MDH;
        struct ConvertContext CtxConv,CtxUnprot,CtxAcc;
        LONG ExitCode=1,Count=0;

        MD_InitFileObject(FO,MD,NULL);
        MD_ExamineFileObject(FO);
        while(ExitCode>=0 && MD_ExamineNextFileObject(FO))
        {
            ExitCode=1;
            if(Sys_MatchPattern(PPatt,FO->Name))
            {
                if(Count>0)
                {
                    Sys_Printf("Press enter to view next file\n");
                    while(ExitCode>0)
                    {
                        if(Sys_WaitForChar(100000)) {if(Sys_GetChar()) break;}
                        if(Sys_CheckCtrlC()) ExitCode=-1;
                    }
                }

                if(ExitCode>0)
                {
                    Count++;
                    Sys_Printf("*** View %s (%04lx) ***\n",FO->Name,(long)FO->Type);

                    /* Fichier image MAP */
                    if(FO->Type==0x0200 && Utl_CheckName("#?.map",FO->Name))
                    {
                        if(Cmd_ViewMap(TDData,MD,FO->Name)<0) ExitCode=-1;
                    }
                    /* Fichier Androides */
                    else if(FO->Type==0x0500 || (FO->Type==0x0200 && Utl_CheckName("#?.and",FO->Name)))
                    {
                        if(Cmd_ViewAnd(TDData,MD,FO->Name)<0) ExitCode=-1;
                    }
                    /* Fichier Basic (binaire ou ascii) */
                    else if(FO->Type==0x0000 || FO->Type==0x00ff)
                    {
                        LONG ErrorCode=MD_OpenFile(&MDH,MD,MD_MODE_OLDFILE,FO->Name,&FO->Type);

                        if(ErrorCode>=0)
                        {
                            LONG ReadLen;

                            CtxConv.State=0;
                            CtxUnprot.State=0;
                            CtxAcc.State=0;
                            while((ReadLen=MD_ReadFile(&MDH,BufferFileContent,BufferFileContentSize))>0)
                            {
                                LONG Len;

                                if(Sys_CheckCtrlC()) {ExitCode=-1; break;}

                                /* Deprotection  si necessaire */
                                Cnv_InitConvertContext(&CtxUnprot,BufferFileContent,ReadLen,BufferFileContent,BufferFileContentSize,CtxUnprot.State);
                                while((Len=Cnv_UnprotectBasic(&CtxUnprot))>0)
                                {
                                    /* Conversion en Ascii (G2) */
                                    Cnv_InitConvertContext(&CtxConv,BufferFileContent,Len,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                                    while((Len=Cnv_BasicToAscii(&CtxConv))>0)
                                    {
                                        /* Conversion du jeu de caracteres G2 en ANSI */
                                        Cnv_InitConvertContext(&CtxAcc,TDData->ConvertBuffer,Len,BufferConvAcc,BufferConvAccSize,CtxAcc.State);
                                        while((Len=Cnv_AsciiG2ToAnsi(&CtxAcc))>0)
                                        {
                                            Sys_PrintLen((const char *)CtxAcc.Dst,Len);
                                        }
                                    }
                                }
                            }

                            MD_CloseFile(&MDH);
                        }
                    }
                    /* Fichier Assdesass, Assembleur ou Ascii */
                    else if(FO->Type==0x0300 || FO->Type==0x03ff || FO->Type==0x01ff)
                    {
                        struct MDHandle MDH;
                        LONG ErrorCode=MD_OpenFile(&MDH,MD,MD_MODE_OLDFILE,FO->Name,&FO->Type);

                        if(ErrorCode>=0)
                        {
                            LONG ReadLen;

                            CtxConv.State=0;
                            CtxAcc.State=0;
                            while((ReadLen=MD_ReadFile(&MDH,BufferFileContent,BufferFileContentSize))>0)
                            {
                                LONG Len;

                                if(Sys_CheckCtrlC()) {ExitCode=-1; break;}

                                if(FO->Type==0x01ff)
                                {
                                    /* Conversion directe du jeu de caracteres G2 en ANSI */
                                    Cnv_InitConvertContext(&CtxAcc,BufferFileContent,ReadLen,BufferConvAcc,BufferConvAccSize,CtxAcc.State);
                                    while((Len=Cnv_AsciiG2ToAnsi(&CtxAcc))>0)
                                    {
                                        Sys_PrintLen((const char *)CtxAcc.Dst,Len);
                                    }
                                }
                                else
                                {
                                    /* Conversion en Ascii (G2) */
                                    Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                                    while(CtxConv.SrcPos<CtxConv.SrcLen)
                                    {
                                        if(FO->Type==0x300) Len=Cnv_AssdesassToAscii(&CtxConv);
                                        else if(FO->Type==0x03ff) Len=Cnv_AssemblerToAscii(&CtxConv);
                                        else break;

                                        /* Conversion du jeu de caracteres G2 en ANSI */
                                        Cnv_InitConvertContext(&CtxAcc,TDData->ConvertBuffer,Len,BufferConvAcc,BufferConvAccSize,CtxAcc.State);
                                        while((Len=Cnv_AsciiG2ToAnsi(&CtxAcc))>0)
                                        {
                                            Sys_PrintLen((const char *)CtxAcc.Dst,Len);
                                        }
                                    }
                                }
                            }

                            MD_CloseFile(&MDH);
                        }
                    }
                    /* Tout autre fichier, binaire ou autre */
                    else
                    {
                        ExitCode=Cmd_Dump(TDData,MD,FO->Name);
                    }
                }

                if(ExitCode<=0) Sys_PrintFault(ERROR_BREAK);
                else Sys_Printf("\n");
            }
        }

        Sys_FreePattern(PPatt);
    }
}
