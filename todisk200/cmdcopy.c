#include "system.h"
#include "global.h"
#include "cmdcopy.h"
#include "filesystem.h"
#include "multidos.h"
#include "convert.h"
#include "convertmap.h"
#include "util.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    27-08-2018 (Seg)    Remaniement suite gestion des noms avec accents
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    21-01-2013 (Seg)    CopyFile() est maintenant une fonction publique
    17-01-2013 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    14-10-2012 (Seg)    On prend maintenant en compte les paramètres Line Start/Step du convertisseur Txt->Ass
    11-09-2012 (Seg)    Modification des convertisseur ASM, ASS et BAS pour prendre en compte la conversion
                        du jeu de caractères G2 du thomson.
    06-09-2012 (Seg)    Ajout de l'option de conversion Ansi -> Assdesass
    03-09-2012 (Seg)    Amélioration du code
    29-07-2010 (Seg)    Modification de SubCopyFile() pour la prise en compte du changement de type après les conversions
    27-07-2010 (Seg)    Prise en compte de l'option de conversion des fichier Assembleur x.0
    07-03-2010 (Seg)    Refonte du processus de copie
    24-02-2010 (Seg)    Modification de la gestion de l'handle multisource. Deplace dans multidos.c
    02-09-2006 (Seg)    Copie de fichier
*/


#define COPY_CONVERT_NONE               0
#define COPY_CONVERT_UNPROTECT          1
#define COPY_CONVERT_MAP_FILE           2
#define COPY_CONVERT_BAS_FILE           3
#define COPY_CONVERT_ASS_FILE           4
#define COPY_CONVERT_ASM_FILE           5
#define COPY_CONVERT_ASCII              6
#define COPY_CONVERT_TXT_FILE_TO_ASS    7
#define COPY_CONVERT_TXT_FILE_TO_ASCG2  8

#define ACT_NONE                        0
#define ACT_COPY                        1
#define ACT_ERASE                       2
#define ACT_RENAME                      3
#define ACT_SKIP                        4
#define ACT_ABORT                       5
#define ACT_REPEAT                      6


/***** Prototypes */
void Cmd_Copy(struct ToDiskData *, struct MultiDOS *, struct MultiDOS *, const char *, const char *);
BOOL Cmd_CopyFile(struct ToDiskData *, ULONG *, struct MultiDOS *, const char *, const char *, LONG *, LONG, LONG, LONG, LONG, LONG, LONG, const char *, LONG, LONG, struct MultiDOS *, const char *, const char *);

LONG P_Cmd_SubCopyFile(struct ToDiskData *, struct MDHandle *, struct MDHandle *, LONG);
LONG P_Cmd_ControlName(ULONG *CopyMode, struct MultiDOS *, const char *, const char *, LONG *, struct MultiDOS *, const char *, const char *, char *, LONG);

LONG P_Cmd_DeclineToName(struct MultiDOS *, const char *, LONG *, char *);
LONG P_Cmd_DeclineHostName(const char *, const char *, char *);
BOOL P_Cmd_PrepareDestination(const char *, const char *);

const char *P_Cmd_GetAllocatedFullName(struct MultiDOS *, const char *, const char *);
void P_Cmd_FreeAllocatedFullName(const char *);


/*****
    Copie d'un fichier d'une source vers une autre
*****/

void Cmd_Copy(struct ToDiskData *TDData, struct MultiDOS *MDSrc, struct MultiDOS *MDDst, const char *SrcPathName, const char *DstPathName)
{
    /* Pour gerer le chemin et le nom de la source */
    const char *SrcName=Sys_FilePart(SrcPathName);
    LONG SrcPathLen=Sys_PathPart(SrcPathName)-SrcPathName;
    char *SrcPath=(char *)Sys_AllocMem(SrcPathLen+sizeof(char));

    /* Pour gerer la destination */
    LONG DstPathSize=Sys_StrLen(DstPathName)+16; /* Terminaison 0 + separateur */
    char *DstPath=(char *)Sys_AllocMem(DstPathSize);

    /* Preparation du buffer de pattern. Si c'est un pattern, alors la copie est multi fichier */
    if(SrcPath!=NULL && DstPath!=NULL)
    {
        BOOL IsPattern=FALSE;
        REGEX *PPatt=Sys_AllocPatternNoCase(SrcName,&IsPattern);

        if(PPatt!=NULL)
        {
            BOOL IsSuccess=TRUE;
            LONG i,DstPathLen=Sys_PathPart(DstPathName)-DstPathName;
            const char *DstName=Sys_FilePart(DstPathName);

            if(Sys_StrLen(DstName)==0) DstName=NULL;

            for(i=0;i<SrcPathLen;i++) SrcPath[i]=SrcPathName[i];
            SrcPath[i]=0;

            for(i=0;i<DstPathLen;i++) DstPath[i]=DstPathName[i];
            DstPath[i]=0;

            if(IsPattern && DstName!=NULL)
            {
                /* Si la source est un pattern, alors on fait potentiellement des
                   operations sur la destination
                */
                IsSuccess=P_Cmd_PrepareDestination(DstPath,DstName);
                if(IsSuccess)
                {
                    Sys_AddPart(DstPath,DstName,DstPathSize);
                    DstName=NULL;
                }
            }

            if(IsSuccess)
            {
                struct MDFileObject MDFO;

                IsSuccess=MD_InitFileObject(&MDFO,MDSrc,SrcPath);
                if(IsSuccess)
                {
                    /* Debut de scan du repertoire */
                    IsSuccess=MD_ExamineFileObject(&MDFO);
                    if(IsSuccess)
                    {
                        BOOL IsEntry=TRUE;
                        ULONG CopyMode=0;

                        while(IsEntry)
                        {
                            BOOL IsBreak=FALSE;
                            if(Sys_CheckCtrlC()) IsBreak=TRUE;

                            if(!IsBreak)
                            {
                                IsEntry=MD_ExamineNextFileObject(&MDFO);
                                if(IsEntry && !MDFO.IsDir)
                                {
                                    /* On lance la copie si le nom correspond au pattern */
                                    if(Sys_MatchPattern(PPatt,MDFO.Name))
                                    {
                                        if(!Cmd_CopyFile(TDData,&CopyMode,
                                            MDSrc,NULL,MDFO.Name,&MDFO.Type,MDFO.Year,MDFO.Month,MDFO.Day,MDFO.Hour,MDFO.Min,MDFO.Sec,MDFO.Comment,MDFO.Type,MDFO.ExtraData,
                                            MDDst,DstPath,DstName)) IsBreak=TRUE;
                                    }
                                }
                            }

                            /* Affichage du message 'break' en cas d'interruption par CTRL+C */
                            if(IsBreak)
                            {
                                Sys_PrintFault(ERROR_BREAK);
                                IsEntry=FALSE;
                            }
                        }

                        MD_Flush(MDDst);
                    }

                    MD_FreeFileObject(&MDFO);
                }
            }

            Sys_FreePattern(PPatt);
        }
    }

    Sys_FreeMem((void *)SrcPath);
    Sys_FreeMem((void *)DstPath);
}


/*****
    Copie d'un fichier
    - TDData: contient des options de copie
    - CopyMode: pointeur sur un parametre indiquant le mode operatoire de copie: Interactif, Renommer tout, Ecraser tout
    - MDSrc: pointeur vers le file system source, ou NULL si la source est l'hote
    - SrcPath: chemin du fichier source, ou NULL si la source MDSrc est non NULL
    - SrcName: nom du fichier source
    - Type: pointeur sur le type Thomson du fichier source, ou NULL si le type est inconnu
    - Year, Month, Day, Hour, Min, Sec: date du fichier
    - Comment: Commentaire associe au fichier
    - MDDst: pointeur sur le file system destination, ou NULL si la destination est l'hote
    - DstPath: Chemin destination. Peut etre NULL si MDDst est non NULL
    - DstName: Nom du fichier destination
*****/

BOOL Cmd_CopyFile(
    struct ToDiskData *TDData, ULONG *CopyMode,
    struct MultiDOS *MDSrc, const char *SrcPath, const char *SrcName, LONG *Type, LONG Year, LONG Month, LONG Day, LONG Hour, LONG Min, LONG Sec, const char *Comment, LONG SrcType, LONG SrcExtraData,
    struct MultiDOS *MDDst, const char *DstPath, const char *DstName)
{
    char DstNewName[33];
    LONG Action;
    LONG Mode=COPY_CONVERT_NONE;
    LONG TypeSrc=*Type;

    if(TDData->ImportTxtMode==IMPORTTXTMODE_ASS) {Mode=COPY_CONVERT_TXT_FILE_TO_ASS; *Type=0x0300;}
    else if(TDData->ImportTxtMode==IMPORTTXTMODE_ASCG2) {Mode=COPY_CONVERT_TXT_FILE_TO_ASCG2; *Type=0x01ff;}
    else if(TDData->ConvertBasToAscii && TypeSrc==0x0000) {Mode=COPY_CONVERT_BAS_FILE; *Type=0x01ff;}
    else if(TDData->UnprotectBas && TypeSrc==0x0000) Mode=COPY_CONVERT_UNPROTECT;
    else if(TDData->ConvertAscii && TypeSrc==0x01ff) Mode=COPY_CONVERT_ASCII;
    else if(TDData->ConvertAssToAscii && TypeSrc==0x0300) {Mode=COPY_CONVERT_ASS_FILE; *Type=0x01ff;}
    else if(TDData->ConvertAsmToAscii && TypeSrc==0x03ff) {Mode=COPY_CONVERT_ASM_FILE; *Type=0x01ff;}
    else if(TDData->ConvertMap && TypeSrc==0x0200 && Utl_CheckName("#?.map",SrcName)) Mode=COPY_CONVERT_MAP_FILE;

    /* On controle le nom du fichier destination */
    Action=P_Cmd_ControlName(CopyMode,MDSrc,SrcPath,SrcName,Type,MDDst,DstPath,DstName,DstNewName,sizeof(DstNewName));

    if(Action!=ACT_ABORT)
    {
        const char *SrcFullName=P_Cmd_GetAllocatedFullName(MDSrc,SrcPath,SrcName);
        const char *DstFullName=P_Cmd_GetAllocatedFullName(MDDst,DstPath,DstNewName);

        if(SrcFullName!=NULL && DstFullName!=NULL)
        {
            LONG ErrorCode=0;
            struct MDHandle GSrcH;

            Sys_Printf("%-16s -> ",SrcName);

            if(Action!=ACT_SKIP)
            {
                /* Ouverture du fichier source */
                ErrorCode=MD_OpenFile(&GSrcH,MDSrc,MD_MODE_OLDFILE,SrcFullName,&TypeSrc);
                if(ErrorCode>=0)
                {
                    struct MDHandle GDstH;

                    /* Ouverture du fichier destination */
                    ErrorCode=MD_OpenFile(&GDstH,MDDst,MD_MODE_NEWFILE,DstFullName,Type);
                    if(ErrorCode>=0)
                    {
                        /* Copie du fichier */
                        ErrorCode=P_Cmd_SubCopyFile(TDData,&GSrcH,&GDstH,Mode);
                        MD_CloseFile(&GDstH);

                        /* Ecriture du commentaire */
                        if(ErrorCode>=0)
                        {
                            /* On doit passer les metas données dans le commentaire */
                            char FullComment[33];
                            Utl_SetCommentMetaData(Comment,FullComment,SrcType,SrcExtraData);

                            ErrorCode=MD_SetComment(MDDst,DstFullName,Type,FullComment);
                            if(ErrorCode<0) Sys_Printf("Error writing destination file comment: %s\n",MD_GetTextErr(ErrorCode));
                        }

                        /* Ecriture de la date du fichier */
                        if(ErrorCode>=0)
                        {
                            ErrorCode=MD_SetDate(MDDst,DstFullName,Type,Year,Month,Day,Hour,Min,Sec);
                            if(ErrorCode<0) Sys_Printf("Error writing destination file date: %s\n",MD_GetTextErr(ErrorCode));
                        }
                    }
                    else
                    {
                        Sys_Printf("Error opening destination file \"%s\": %s\n",DstFullName,MD_GetTextErr(ErrorCode));
                    }

                    MD_CloseFile(&GSrcH);
                }
                else
                {
                    Sys_Printf("Error opening source file \"%s\": %s\n",SrcFullName,MD_GetTextErr(ErrorCode));
                    ErrorCode=0;
                }
            }

            if(ErrorCode>=0)
            {
                switch(Action)
                {
                    case ACT_COPY:
                        Sys_Printf("copied : %s\n",DstFullName);
                        break;

                    case ACT_ERASE:
                        Sys_Printf("erased : %s\n",DstFullName);
                        break;

                    case ACT_RENAME:
                        Sys_Printf("renamed: %s\n",DstFullName);
                        break;

                    case ACT_SKIP:
                        Sys_Printf("skiped!\n");
                        break;
                }
            }
        }
        else
        {
            Sys_Printf("Error: %s\n",MD_GetTextErr(MD_NOT_ENOUGH_MEMORY));
        }

        P_Cmd_FreeAllocatedFullName((void *)SrcFullName);
        P_Cmd_FreeAllocatedFullName((void *)DstFullName);
    }

    if(Action==ACT_ABORT) return FALSE;
    return TRUE;
}


/*****
    Mecanisme de controle du nom avec recuperation du nom final eventuellement renomme
*****/

LONG P_Cmd_ControlName(
    ULONG *CopyMode,
    struct MultiDOS *MDSrc, const char *SrcPath, const char *SrcName, LONG *Type,
    struct MultiDOS *MDDst, const char *DstPath, const char *DstName, char *DstNewName, LONG DstNewNameSize)
{
    LONG Action=ACT_NONE;
    char FinalName[33];

    Sys_StrCopy(DstNewName,DstName!=NULL?DstName:SrcName,DstNewNameSize);
    while(Action==ACT_NONE)
    {
        LONG res=0;

        /* Etape 1: Determination du nom du fichier final */
        Sys_StrCopy(FinalName,DstNewName,sizeof(FinalName));
        if(!(*CopyMode & COPY_MODE_ERASE_ALL))
        {
            if(MDDst->Type!=MDOS_HOST) res=P_Cmd_DeclineToName(MDDst,DstNewName,Type,FinalName);
            else res=P_Cmd_DeclineHostName(DstPath,DstNewName,FinalName);
        }

        if(res==0) Action=ACT_COPY;
        else
        {
            if(*CopyMode==COPY_MODE_ERASE_ALL) Action=ACT_ERASE;
            if(*CopyMode==COPY_MODE_RENAME_ALL) Action=ACT_RENAME;
        }

        /* Etape 2: On interroge l'utilisateur en cas de probleme */
        if(Action==ACT_NONE)
        {
            /* Affichage de la raison de l'interrogation posee a l'utilisateur */
            if(res<0)
            {
                Sys_Printf("The file name \"%s\" is too long and must be renamed!\n",DstNewName);
                Sys_Printf("(S)kip, (R)ename [%s], (A)uto rename all? ",FinalName);
            }
            else
            {
                Sys_Printf("The file \"%s\" already exists!\n",DstNewName);
                Sys_Printf("(S)kip, (E)rase, (F) Erase all, (R)ename [%s], (A)uto rename all? ",FinalName);
            }

            Sys_FlushOutput();

            /* Attente de la saisie au clavier, ou d'un Ctrl+C */
            while(Action==ACT_NONE)
            {
                if(Sys_WaitForChar(100000))
                {
                    switch(Sys_GetChar())
                    {
                        case 's':
                        case 'S':
                            Action=ACT_SKIP;
                            break;

                        case 'e':
                        case 'E':
                            Action=ACT_REPEAT;
                            if(res>=0)
                            {
                                Sys_StrCopy(FinalName,DstNewName,sizeof(FinalName));
                                Action=ACT_ERASE;
                            }
                            break;

                        case 'f':
                        case 'F':
                            Action=ACT_REPEAT;
                            if(res>=0)
                            {
                                Sys_StrCopy(FinalName,DstNewName,sizeof(FinalName));
                                Action=ACT_ERASE;
                                *CopyMode=COPY_MODE_ERASE_ALL;
                            }
                            break;

                        case 10:
                            Action=ACT_RENAME;
                            break;

                        case 'a':
                        case 'A':
                            Action=ACT_RENAME;
                            *CopyMode=COPY_MODE_RENAME_ALL;
                            break;

                        case 'r':
                        case 'R':
                            {
                                LONG Count=0,c;

                                while(Count<DstNewNameSize-1 && (c=Sys_GetChar())>=32) DstNewName[Count++]=c;
                                DstNewName[Count]=0;

                                if(Count<=0) Sys_StrCopy(FinalName,DstNewName,sizeof(FinalName));
                                Action=ACT_REPEAT;
                            }
                            break;

                        default:
                            Action=ACT_REPEAT;
                            break;
                    }
                }

                if(Sys_CheckCtrlC()) Action=ACT_ABORT;
            }

            Sys_FlushInput();

            if(Action==ACT_REPEAT)
            {
                Sys_StrCopy(FinalName,DstNewName,sizeof(FinalName));
                Action=ACT_NONE;
            }
        }

        Sys_StrCopy(DstNewName,FinalName,DstNewNameSize);
    }

    return Action;
}


/*****
    Permet d'obtenir un nom de fichier unique a partir d'un file system thomson
    Retourne:
    . -2: le nom original a ete raccourci et existait
    . -1: le nom original a ete raccourci mais n'existe pas
    .  0: le nom original n'existe pas
    .  1: le nom original existait
*****/

LONG P_Cmd_DeclineToName(struct MultiDOS *MD, const char *FileName, LONG *Type, char *NewFileName)
{
    LONG Result=0,Number=2,p;
    BOOL IsDeclined=FALSE;
    char ThomsonName[11],Tmp[8];

    Utl_ConvertHostNameToThomsonName(FileName,(UBYTE *)ThomsonName);
    for(p=8;p>0 && ThomsonName[p-1]==' ';p--);

    while(!IsDeclined)
    {
        Utl_ConvertThomsonNameToHostName((UBYTE *)ThomsonName,NewFileName);
        if(Sys_StrCmp(FileName,NewFileName)!=0 && !Result) Result=-1;

        if(MD_FindFile(MD,NewFileName,Type,TRUE)>=0)
        {
            LONG Len,i,p2=p;

            if(Result<0) Result=-2; else Result=1;

            Sys_SPrintf(Tmp,"%ld",Number++);
            Len=Sys_StrLen(Tmp);

            if(p+Len>8) p2=8-Len;
            for(i=0;i<Len;i++) ThomsonName[p2+i]=Tmp[i];
        } else IsDeclined=TRUE;
    }

    return Result;
}


/*****
    Permet d'obtenir un nom de fichier unique a partir du file system hote
    Retourne:
    . 0: le nom original n'existe pas
    . 1: le nom original existait
*****/

LONG P_Cmd_DeclineHostName(const char *PathName, const char *FileName, char *NewFileName)
{
    LONG Result=0,Number=2;
    BOOL IsDeclined=FALSE;
    CDIR *PDir;

    Sys_StrCopy(NewFileName,FileName,13);

    if((PDir=Sys_CurrentDir(PathName))!=NULL)
    {
        while(!IsDeclined)
        {
            HPTR h=Sys_Open(NewFileName,MODE_OLDFILE);

            if(h!=NULL)
            {
                Sys_Close(h);
                Result=1;
                Sys_SPrintf(NewFileName,"%s.%ld",FileName,Number++);
            } else IsDeclined=TRUE;
        }

        Sys_RestoreDir(PDir);
    }

    return Result;
}


/*****
    Routine permettant de preparer le chemin destination
*****/

BOOL P_Cmd_PrepareDestination(const char *DstPath, const char *DstName)
{
    BOOL IsSuccess=FALSE;

    /* Chemin sans nouveau repertoire */
    if(DstName==NULL || Sys_StrLen(DstName)==0)
    {
        CDIR *PDir=Sys_CurrentDir(DstPath);

        if(PDir==NULL) Sys_Printf("Destination path invalid\n");
        else
        {
            Sys_RestoreDir(PDir);
            IsSuccess=TRUE;
        }
    }
    else
    {
        /* chemin avec nom */
        const char *FullPath=P_Cmd_GetAllocatedFullName(NULL,DstPath,DstName);

        if(FullPath!=NULL)
        {
            IsSuccess=Sys_CreateDir(FullPath);
            if(IsSuccess) Sys_Printf("%s [created]\n",DstName);
            else Sys_PrintFault(Sys_IoErr());

            P_Cmd_FreeAllocatedFullName(FullPath);
        }
    }

    return IsSuccess;
}


/*****
    Copie d'un fichier avec conversion des fichiers si besoin
*****/

LONG P_Cmd_SubCopyFile(struct ToDiskData *TDData, struct MDHandle *SrcH, struct MDHandle *DstH, LONG Mode)
{
    LONG ErrorCode=0;
    LONG ReadLen=0,WriteLen=1,Len;
    struct ConvertContext CtxConv,CtxUnprot,CtxAcc;
    UBYTE *BufferFileContent=TDData->Buffer;
    LONG BufferFileContentSize=sizeof(TDData->Buffer)-512;
    UBYTE *BufferConvAcc=&TDData->Buffer[BufferFileContentSize];
    LONG BufferConvAccSize=256; /* Note: on garde encore 256 octets pour le buffer temporaire du convertisseur txt->ass */

    switch(Mode)
    {
        case COPY_CONVERT_MAP_FILE:
            CM_ConvertMAP(TDData->MapMode,TDData->MapPalName,SrcH,DstH);
            break;

        case COPY_CONVERT_NONE:
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                WriteLen=MD_WriteFile(DstH,BufferFileContent,ReadLen);
            }
            break;

        case COPY_CONVERT_UNPROTECT:
            CtxConv.State=0;
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                while((Len=Cnv_UnprotectBasic(&CtxConv))>0)
                {
                    WriteLen=MD_WriteFile(DstH,CtxConv.Dst,Len);
                }
            }
            break;

        case COPY_CONVERT_BAS_FILE:
            CtxConv.State=0;
            CtxUnprot.State=0;
            CtxAcc.State=0;
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                /* Deprotection du fichier basic si besoin */
                Cnv_InitConvertContext(&CtxUnprot,BufferFileContent,ReadLen,BufferFileContent,ReadLen,CtxUnprot.State);
                while((Len=Cnv_UnprotectBasic(&CtxUnprot))>0)
                {
                    /* Conversion en ascii (G2) */
                    Cnv_InitConvertContext(&CtxConv,BufferFileContent,Len,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                    while((Len=Cnv_BasicToAscii(&CtxConv))>0)
                    {
                        /* Conversion de l'ascii thomson (G2) en Ansi */
                        Cnv_InitConvertContext(&CtxAcc,TDData->ConvertBuffer,Len,BufferConvAcc,BufferConvAccSize,CtxAcc.State);
                        while(WriteLen>0 && (Len=Cnv_AsciiG2ToAnsi(&CtxAcc))>0)
                        {
                            WriteLen=MD_WriteFile(DstH,CtxAcc.Dst,Len);
                        }
                    }
                }
            }
            break;

        case COPY_CONVERT_ASS_FILE:
            CtxConv.State=0;
            CtxAcc.State=0;
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                /* Conversion en ascii (G2) */
                Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                while((Len=Cnv_AssdesassToAscii(&CtxConv))>0)
                {
                    /* Conversion de l'ascii thomson (G2) en Ansi */
                    Cnv_InitConvertContext(&CtxAcc,TDData->ConvertBuffer,Len,BufferConvAcc,BufferConvAccSize,CtxAcc.State);
                    while(WriteLen>0 && (Len=Cnv_AsciiG2ToAnsi(&CtxAcc))>0)
                    {
                        WriteLen=MD_WriteFile(DstH,CtxAcc.Dst,Len);
                    }
                }
            }
            break;

        case COPY_CONVERT_ASM_FILE:
            CtxConv.State=0;
            CtxAcc.State=0;
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                /* Conversion en ascii (G2) */
                Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                while((Len=Cnv_AssemblerToAscii(&CtxConv))>0)
                {
                    /* Conversion de l'ascii thomson (G2) en Ansi */
                    Cnv_InitConvertContext(&CtxAcc,TDData->ConvertBuffer,Len,BufferConvAcc,BufferConvAccSize,CtxAcc.State);
                    while(WriteLen>0 && (Len=Cnv_AsciiG2ToAnsi(&CtxAcc))>0)
                    {
                        WriteLen=MD_WriteFile(DstH,CtxAcc.Dst,Len);
                    }
                }
            }
            break;

        case COPY_CONVERT_ASCII:
            CtxConv.State=0;
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                /* Conversion de l'ascii thomson (G2) en Ansi */
                Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                while((Len=Cnv_AsciiG2ToAnsi(&CtxConv))>0)
                {
                    WriteLen=MD_WriteFile(DstH,CtxConv.Dst,Len);
                }
            }
            break;

        case COPY_CONVERT_TXT_FILE_TO_ASS:
            {
                struct AnsiToAssContext SubCtx;

                SubCtx.TmpBufferPtr=&BufferConvAcc[BufferConvAccSize];
                SubCtx.TmpBufferSize=256; /* On prend ce qu'il reste dans TDData->Buffer */
                SubCtx.LineNumber=TDData->ImportTxtAssLineStart;
                SubCtx.LineNumberStep=TDData->ImportTxtAssLineStep;
                CtxConv.State=0;
                CtxConv.UserData=(void *)&SubCtx;

                while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
                {
                    /* Import du fichier texte et conversion dans le format Assdesass */
                    Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                    while(WriteLen>0 && (Len=Cnv_TextToAssdesass(&CtxConv))>0)
                    {
                        WriteLen=MD_WriteFile(DstH,CtxConv.Dst,Len);
                    }
                }

                /* Vidage du cache */
                while(WriteLen>0 && (Len=Cnv_TextToAssdesassFlush(&CtxConv,TRUE))>0)
                {
                    WriteLen=MD_WriteFile(DstH,CtxConv.Dst,Len);
                }
            }
            break;

        case COPY_CONVERT_TXT_FILE_TO_ASCG2:
            CtxConv.State=0;
            while(WriteLen>0 && (ReadLen=MD_ReadFile(SrcH,BufferFileContent,BufferFileContentSize))>0)
            {
                /* Conversion de Ansi en thomson (G2) */
                Cnv_InitConvertContext(&CtxConv,BufferFileContent,ReadLen,TDData->ConvertBuffer,sizeof(TDData->ConvertBuffer),CtxConv.State);
                while((Len=Cnv_AnsiToAsciiG2(&CtxConv))>0)
                {
                    WriteLen=MD_WriteFile(DstH,CtxConv.Dst,Len);
                }
            }
            break;
    }

    if(ReadLen<0)
    {
        ErrorCode=ReadLen;
        Sys_Printf("Error reading file: %s\n",MD_GetTextErr(ErrorCode));
    }
    else if(WriteLen<=0)
    {
        ErrorCode=WriteLen;
        if(ErrorCode==0) ErrorCode=MD_DISK_FULL;
        Sys_Printf("Error writing file: %s\n",MD_GetTextErr(ErrorCode));
    }

    return ErrorCode;
}


/*****
    Allocation d'un nom complet de fichier
*****/

const char *P_Cmd_GetAllocatedFullName(struct MultiDOS *MD, const char *Path, const char *Name)
{
    char *FullName;
    LONG FullNameSize=16; /* Terminaison 0 + separateur */

    if(Path!=NULL) FullNameSize+=Sys_StrLen(Path);
    if(Name!=NULL) FullNameSize+=Sys_StrLen(Name);

    if((FullName=(char *)Sys_AllocMem(FullNameSize))!=NULL)
    {
        if(Path!=NULL && MD->Type==MDOS_HOST) Sys_StrCopy(FullName,Path,FullNameSize);
        if(Name!=NULL) Sys_AddPart(FullName,Name,FullNameSize);
    }

    return FullName;
}


/*****
    Liberation du buffer alloue par P_Cmd_GetAllocatedFullName()
*****/

void P_Cmd_FreeAllocatedFullName(const char *FullName)
{
    Sys_FreeMem((void *)FullName);
}
