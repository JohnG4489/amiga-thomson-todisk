#include "system.h"
#include "convert.h"
#include "util.h"

/*
    10-09-2020 (Seg)    Ajout de Cnv_SplitHostName()
    27-08-2018 (Seg)    Ajout des fonctions pour gérér les accents du file system
    08-09-2015 (Seg)    Ajout passage en majuscule des instructions assdesass
    14-10-2012 (Seg)    Fix conversion txt->ass + Ajout option d'import assdesass
    19-09-2012 (Seg)    Fonction pour convertir de l'Ansi en Ascii + G2
    12-09-2012 (Seg)    Ajout d'une fonction pour convertir l'Ansi en Ascii G2
    06-09-2012 (Seg)    Ajout des fonction Cnv_TextToAssdesass() et Cnv_TextToAssdesassFlush() pour
                        convertir un fichier texte en fichier assdesass.
    27-07-2010 (Seg)    Ajout de la fonction Cnv_AssemblerToAscii() + renommage des fonctions publiques
    02-09-2006 (Seg)    Toolkit permettant de faire plusieurs types de conversions
*/


/***** Prototypes */
void Cnv_InitConvertContext(struct ConvertContext *, const UBYTE *, LONG, UBYTE *, LONG, LONG);

void Cnv_SplitHostName(const char *, char *);
void Cnv_ConvertHostNameToThomsonName(const char *, UBYTE *);
void Cnv_ConvertThomsonNameToHostName(const UBYTE *, char *, BOOL);
void Cnv_ConvertHostLabelToThomsonLabel(const char *, UBYTE *);
void Cnv_ConvertThomsonLabelToHostLabel(const UBYTE *, char *, BOOL);
void Cnv_ConvertHostCommentToThomsonComment(const char *, UBYTE *, LONG *, LONG *);
void Cnv_ConvertThomsonCommentToHostComment(const UBYTE *, char *, BOOL);

LONG Cnv_BasicToAscii(struct ConvertContext *);
LONG Cnv_AssdesassToAscii(struct ConvertContext *);
LONG Cnv_AssemblerToAscii(struct ConvertContext *);
LONG Cnv_UnprotectBasic(struct ConvertContext *);
LONG Cnv_AnsiToAsciiG2(struct ConvertContext *);
LONG Cnv_AsciiG2ToAnsi(struct ConvertContext *);
LONG Cnv_TextToAssdesass(struct ConvertContext *);
LONG Cnv_TextToAssdesassFlush(struct ConvertContext *, BOOL);
const char *Cnv_IsImportTxtMode(const char *, const char *);
void Cnv_GetImportTxtParamAss(const char *, LONG *, LONG *);

void P_Cnv_AddToTmpBuffer(struct AnsiToAssContext *, LONG, BOOL);
LONG P_Cnv_AnsiToAsciiG2(UBYTE *, char);
LONG P_Cnv_WriteNumber(LONG, UBYTE *, LONG);


/*****
    Initialisation de la structure de conversion
    Note:
    - SrcLen>=1 et DstLen>=8
    - State doit commencer a 0
*****/

void Cnv_InitConvertContext(struct ConvertContext *Ctx, const UBYTE *Src, LONG SrcLen, UBYTE *Dst, LONG DstLen, LONG State)
{
    Ctx->Src=Src;
    Ctx->SrcLen=SrcLen;
    Ctx->Dst=Dst;
    Ctx->DstLen=DstLen;
    Ctx->SrcPos=0;
    Ctx->State=State;
}


/*****
    Réduction d'un nom hôte en un autre nom hôte respectant le nombre de caractères
    possibles pour un nom de fichier du file system.
*****/

void Cnv_SplitHostName(const char *HostName, char *SplittedHostName)
{
    UBYTE Tmp[11];

    Cnv_ConvertHostNameToThomsonName(HostName,Tmp);
    Cnv_ConvertThomsonNameToHostName(Tmp,SplittedHostName,TRUE);
}


/*****
    Conversion d'un nom de fichier Hôte dans le format de nom Thomson,
    avec conversion du jeu de caractères accentés.
    - Name: chaîne ANSI
    - ToName: buffer de 11 octets minimum (8 pour le nom + 3 pour le suffixe)
*****/

void Cnv_ConvertHostNameToThomsonName(const char *Name, UBYTE *ToName)
{
    struct ConvertContext CtxConv;
    char Tmp[33];
    LONG Len;

    /* Conversion du jeu de caractères */
    Cnv_InitConvertContext(&CtxConv,
        (UBYTE *)Name,Sys_StrLen(Name),
        (UBYTE *)Tmp,sizeof(Tmp)-sizeof(char),
        0);
    Len=Cnv_AnsiToAsciiG2(&CtxConv);
    Tmp[Len]=0;

    /* Réduction du nom */
    Utl_ConvertHostNameToThomsonName(Tmp,ToName);
}


/*****
    Conversion d'un nom au format Thomson en nom de fichier Hôte,
    avec conversion du jeu de caractères accentés.
    - ToName: buffer de 11 octets contenant le nom et le suffixe
    - Name: buffer de 13 caractères minimum (12 pour le nom + 1 zéro de terminaison)
*****/

void Cnv_ConvertThomsonNameToHostName(const UBYTE *ToName, char *Name, BOOL IsNormalize)
{
    struct ConvertContext CtxConv;
    UBYTE Tmp[33];
    LONG i;

    for(i=0; i<sizeof(Tmp); i++) Tmp[i]=' ';

    /* Conversion du jeu de caractères du nom */
    Cnv_InitConvertContext(&CtxConv,
        ToName,8,
        Tmp,sizeof(Tmp),
        0);
    Cnv_AsciiG2ToAnsi(&CtxConv);

    /* Conversion du jeu de caractères du suffixe */
    Cnv_InitConvertContext(&CtxConv,
        &ToName[8],3,
        &Tmp[8],sizeof(Tmp)-8,
        0);
    Cnv_AsciiG2ToAnsi(&CtxConv);

    /* Génération du nom sur le host */
    Utl_ConvertThomsonNameToHostName(Tmp,Name);

    /* Normalisation */
    if(IsNormalize) Utl_NormalizeName(Name,Sys_StrLen(Name));

}


/*****
    Conversopn d'un nom de volume dans le format thomson, avec conversion
    du jeu de caractères accentués.
    - Label: nom au format ANSI
    - ToLabel: buffer de 8 caractères minimum
*****/

void Cnv_ConvertHostLabelToThomsonLabel(const char *Label, UBYTE *ToLabel)
{
    struct ConvertContext CtxConv;
    char Tmp[33];
    LONG Len;

    /* Conversion du jeu de caractères */
    Cnv_InitConvertContext(&CtxConv,
        (UBYTE *)Label,Sys_StrLen(Label),
        (UBYTE *)Tmp,sizeof(Tmp)-sizeof(char),
        0);
    Len=Cnv_AnsiToAsciiG2(&CtxConv);
    Tmp[Len]=0;

    /* Réduction du nom */
    Utl_ConvertHostLabelToThomsonLabel(Tmp,ToLabel);
}


/*****
    Conversion d'un nom de volume Thomson au format ANSI, avec conversion
    du jeu de caractères accentués.
    - ToLabel: buffer 8 octets contenant le nom
    - Label: buffers de 9 caractères contenant le nom converti
*****/

void Cnv_ConvertThomsonLabelToHostLabel(const UBYTE *ToLabel, char *Label, BOOL IsNormalize)
{
    struct ConvertContext CtxConv;
    char Tmp[33];
    LONG Len;

    /* Génération du nom sur le host */
    Utl_ConvertThomsonLabelToHostLabel(ToLabel,Label);

    /* Conversion du jeu de caractères du nom */
    Cnv_InitConvertContext(&CtxConv,
        (UBYTE *)Label,Sys_StrLen(Label),
        (UBYTE *)Tmp,sizeof(Tmp),
        0);
    Len=Cnv_AsciiG2ToAnsi(&CtxConv);
    Tmp[Len]=0;
    Sys_StrCopy(Label,Tmp,9);

    /* Normalisation */
    if(IsNormalize) Utl_NormalizeString(Label,Sys_StrLen(Label));
}


/*****
    Conversion d'un commentaire dans le format de commentaire Thomson,
    avec conversion du jeu de caractères accentués et extraction des metas
    informations.
    - Comment: chaîne ANSI contenant le commentaire
    - ToComment: buffer de 8 octets pour la destination
    - Type: pointeur sur un long qui va contenir le type extrait du commentaire Hôte (NULL possible)
    - ExtraData: pointeur sur un long qui va contenir les extradata du commentaire Hôte (NULL possible)
*****/

void Cnv_ConvertHostCommentToThomsonComment(const char *Comment, UBYTE *ToComment, LONG *Type, LONG *ExtraData)
{
    struct ConvertContext CtxConv;
    char Tmp[33];
    LONG Len;

    /* Conversion du jeu de caractères */
    Cnv_InitConvertContext(&CtxConv,
        Comment,Sys_StrLen(Comment),
        (UBYTE *)Tmp,sizeof(Tmp)-sizeof(char),
        0);
    Len=Cnv_AnsiToAsciiG2(&CtxConv);
    Tmp[Len]=0;

    Utl_ConvertHostCommentToThomsonComment(Tmp,ToComment,Type,ExtraData);
}


/*****
    Conversion d'un commentaire thomson au format de commentaire Hôte,
    avec conversion du jeu de caractères accentués.
    - ToComment: buffer de 8 octets contenant le commentaire
    - Comment: buffer de 9 caractères min pour la destination
*****/

void Cnv_ConvertThomsonCommentToHostComment(const UBYTE *ToComment, char *Comment, BOOL IsNormalize)
{
    struct ConvertContext CtxConv;
    char Tmp[33];
    LONG Len;

    Utl_ConvertThomsonCommentToHostComment(ToComment,Comment);

    /* Conversion du jeu de caractères du nom */
    Cnv_InitConvertContext(&CtxConv,
        (UBYTE *)Comment,Sys_StrLen(Comment),
        (UBYTE *)Tmp,sizeof(Tmp)-sizeof(char),
        0);
    Len=Cnv_AsciiG2ToAnsi(&CtxConv);
    Tmp[Len]=0;
    Sys_StrCopy(Comment,Tmp,9);

    /* Normalisation */
    if(IsNormalize) Utl_NormalizeString(Comment,Sys_StrLen(Comment));
}


/*****
    Conversion de donnees Basic
*****/

LONG Cnv_BasicToAscii(struct ConvertContext *Ctx)
{
    static const char *Instr[]={
        "END","FOR","NEXT","DATA","DIM","READ","LET","GO","RUN","IF","RESTORE",
        "RETURN","REM","'","STOP","ELSE","TRON","TROFF","DEFSTR","DEFINT",
        "DEFSNG","DEFDBL","ON","WAIT","ERROR","RESUME","AUTO","DELETE","LOCATE",
        "CLS","CONSOLE","PSET","MOTOR","SKIPF","EXEC","BEEP","COLOR","LINE","BOX",
        "UNMASK","ATTRB","DEF","POKE","PRINT","CONT","LIST","CLEAR","INTERVAL",
        "KEY","NEW","SAVE","LOAD","MERGE","OPEN","CLOSE","INPEN","PEN","PLAY",
        "TAB(","TO","SUB","FN","SPC(","USING","USR","ERL","ERR","OFF","THEN",
        "NOT","STEP","+","-","*","/","^","AND","OR","XOR","EQV","IMP","MOD",
        "@",">","=","<","DSKINI","DSKO$","KILL","NAME","FIELD","LSET","RSET",
        "PUT","GET","VERIFY","DEVICE","DIR","FILES","WRITE","UNLOAD","BACKUP",
        "COPY","CIRCLE","PAINT","RESET","RENUM","SWAP","*","WINDOW","PATTERN",
        "DO","LOOP","EXIT","INMOUSE","MOUSE","CHAIN","COMMON","SEARCH","FWD",
        "TURTLE"
    };
    static const char *Func[]={
        "SGN","INT","ABS","FRE","SQR","LOG","EXP","COS","SIN","TAN",
        "PEEK","LEN","STR$","VAL","ASC","CHR$","EOF","CINT","CSNG","CDBL","FIX",
        "HEX$","OCT$","STICK","STRIG","GR$","LEFT$","RIGHT$","MID$","INSTR",
        "VARPTR","RND","INKEY$","INPUT","CSRLIN","POINT","SCREEN","POS","PTRIG",
        "DSKF","CVI","CVS","CVD","MKI$","MKS$","MKD$","LOC","LOF","SPACE$",
        "STRING$","DSKI$","FKEY$","MIN(","MAX(","ATN","CRUNCH$","MTRIG","EVAL",
        "PALETTE","BANK","HEAD","ROT","SHOW","ZOOM","TRACE"
    };
    LONG DstPos=0;

    /* Automate de decodage d'un programme Basic */
    while(Ctx->SrcPos<Ctx->SrcLen && DstPos+7<Ctx->DstLen)
    {
        LONG CurChar=(LONG)Ctx->Src[Ctx->SrcPos++];

        switch(Ctx->State)
        {
            case 0: /* Demarrage: Codage du fichier */
            case 1: /* Poids fort de la longueur du programme */
            case 2: /* Poids faible de la longueur du programme */
            case 3: /* Poids fort de la longueur de la ligne */
            case 4: /* Poids faible de la longueur de la ligne */
                Ctx->State++;
                break;

            case 5: /* Poids fort du numero de la ligne */
                Ctx->Data=CurChar;
                Ctx->State++;
                break;

            case 6: /* Poids faible du numero de la ligne */
                DstPos+=P_Cnv_WriteNumber(((LONG)Ctx->Data<<8)+CurChar,&Ctx->Dst[DstPos],1);
                Ctx->Dst[DstPos++]=' ';
                Ctx->State++;
                break;

            case 7: /* Traitement de la ligne */
                switch(CurChar)
                {
                    case 0x00: /* Fin de la ligne */
                        Ctx->Dst[DstPos++]='\n';
                        Ctx->State=3;
                        break;

                    case 0x3A: /* Separateur d'instruction */
                        Ctx->State=9;
                        break;

                    case 0xFF: /* Fonction */
                        Ctx->State++;
                        break;

                    default:   /* Instruction */
                        if(CurChar>=0x80)
                        {
                            if(CurChar<=0xF8)
                            {
                                const char *InstrPtr=Instr[CurChar-0x80];
                                while(*InstrPtr!=0) Ctx->Dst[DstPos++]=*(InstrPtr++);
                            } else Ctx->Dst[DstPos++]='!';
                        } else Ctx->Dst[DstPos++]=(UBYTE)CurChar;
                        break;
                }
                break;

            case 8: /* Traitement des fonctions */
                if(CurChar>=0x80 && CurChar<=0xC0)
                {
                    const char *FuncPtr=Func[CurChar-0x80];
                    while(*FuncPtr!=0) Ctx->Dst[DstPos++]=*(FuncPtr++);
                } else Ctx->Dst[DstPos++]='!';
                Ctx->State=7;
                break;

            case 9: /* Traitement du separateur. Cas special pour "'" et "ELSE" */
                if(CurChar!=0x8D && CurChar!=0x8F) Ctx->Dst[DstPos++]=':';
                Ctx->SrcPos--;
                Ctx->State=7;
                break;
        }
    }

    return DstPos;
}


/*****
    Conversion de donnees Assdesass
*****/

LONG Cnv_AssdesassToAscii(struct ConvertContext *Ctx)
{
    LONG DstPos=0;

    /* Automate de decodage d'un programme Assdesass */
    while(Ctx->SrcPos<Ctx->SrcLen && DstPos+8<Ctx->DstLen)
    {
        LONG CurChar=(LONG)Ctx->Src[Ctx->SrcPos++];

        switch(Ctx->State)
        {
            case 0: /* Debut */
                Ctx->State++;
                break;

            case 1: /* Poids fort du numero de la ligne */
                Ctx->Data=CurChar;
                Ctx->State++;
                break;

            case 2: /* Poids faible du numero de la ligne */
                DstPos+=P_Cnv_WriteNumber(((LONG)Ctx->Data<<8)+CurChar,&Ctx->Dst[DstPos],5);
                Ctx->Dst[DstPos++]=' ';
                Ctx->State++;
                break;

            case 3: /* Traitement de la ligne */
                if(CurChar<=8) while(--CurChar>=0) Ctx->Dst[DstPos++]=' ';
                else if(CurChar==0x0D) {Ctx->Dst[DstPos++]='\n'; Ctx->State=1;}
                else Ctx->Dst[DstPos++]=(UBYTE)CurChar;
                break;
        }
    }

    return DstPos;
}


/*****
    Conversion de donnees Asssembleur x.0
*****/

LONG Cnv_AssemblerToAscii(struct ConvertContext *Ctx)
{
    LONG DstPos=0;
    static const char Accent[]=
    {
        'ç','a','â','ä','à','é','ê','ë',
        'è','î','ï','ô','ö','û','ü','ù'
    };

    /* Automate de decodage d'un programme Assembleur */
    while(Ctx->SrcPos<Ctx->SrcLen && DstPos+16<Ctx->DstLen)
    {
        LONG CurChar=(LONG)Ctx->Src[Ctx->SrcPos++];

        if(CurChar<=0x7f)
        {
            Ctx->Dst[DstPos++]=(CurChar=='\r'?'\n':CurChar);
        }
        else if(CurChar<=0x8f)
        {
            CurChar=CurChar-0x80;
            Ctx->Dst[DstPos++]=Accent[CurChar];
        }
        else
        {
            CurChar=CurChar&0x0f;
            while(--CurChar>=0) Ctx->Dst[DstPos++]=' ';
        }
    }

    return DstPos;
}


/*****
    Deprotection d'un fichier Basic
*****/

LONG Cnv_UnprotectBasic(struct ConvertContext *Ctx)
{
    static const UBYTE Tab1[13]={0x86,0x1E,0xD7,0xBA,0x87,0x99,0x26,0x64,0x87,0x23,0x34,0x58,0x86};
    static const UBYTE Tab2[11]={0x80,0x19,0x56,0xAA,0x80,0x76,0x22,0xF1,0x82,0x38,0xAA};
    LONG DstPos=0;

    while(Ctx->SrcPos<Ctx->SrcLen && DstPos+1<Ctx->DstLen)
    {
        LONG CurChar=(LONG)Ctx->Src[Ctx->SrcPos++];

        switch(Ctx->State)
        {
            case 0: /* Demarrage: Codage du fichier */
                Ctx->Data=0;
                if(CurChar==0xFF) Ctx->State=4;
                else {CurChar=0xFF; Ctx->State++;}
                Ctx->Dst[DstPos++]=CurChar;
                break;

            case 1: /* Poids fort de la longueur du programme */
            case 2: /* Poids faible de la longueur du programme */
                Ctx->Dst[DstPos++]=(UBYTE)CurChar;
                Ctx->State++;
                break;

            case 3:
                {
                    LONG d1=Ctx->Data>>8,d2=Ctx->Data&0xff;

                    if(--d1<=0) d1=0x0b;
                    if(--d2<=0) d2=0x0d;
                    Ctx->Data=(d1<<8)+d2;

                    Ctx->Dst[DstPos++]=(UBYTE)(d1+((CurChar-d2)^Tab2[d1-1]^Tab1[d2-1]));
                }
                break;

            case 4:
                Ctx->Dst[DstPos++]=CurChar;
                break;
        }
    }

    return DstPos;
}


/*****
    Conversion Ansi -> Ascii Thomson
*****/

LONG Cnv_AnsiToAsciiG2(struct ConvertContext *Ctx)
{
    LONG DstPos=0;

    while(Ctx->SrcPos<Ctx->SrcLen && DstPos+4<Ctx->DstLen)
    {
        LONG CurChar=(char)Ctx->Src[Ctx->SrcPos++];

        switch(Ctx->State)
        {
            case 0:
                if(CurChar=='\r') Ctx->State=1;
                else if(CurChar=='\n') Ctx->State=2;
                else DstPos+=P_Cnv_AnsiToAsciiG2(&Ctx->Dst[DstPos],CurChar);
                break;

            case 1: /* on est précédé d'un retour chariot */
                if(CurChar=='\n') {Ctx->Dst[DstPos++]='\r'; Ctx->Dst[DstPos++]='\n'; Ctx->State=0;}
                else if(CurChar!='\r') {DstPos+=P_Cnv_AnsiToAsciiG2(&Ctx->Dst[DstPos],CurChar); Ctx->State=0;}
                break;

            case 2: /* on est précédé d'un \n */
                Ctx->State=0;
                Ctx->Dst[DstPos++]='\r';
                Ctx->Dst[DstPos++]='\n';
                if(CurChar!='\r') DstPos+=P_Cnv_AnsiToAsciiG2(&Ctx->Dst[DstPos],CurChar);
                else if(CurChar=='\n') Ctx->State=2;
                break;
        }
    }

    return DstPos;
}


/*****
    Conversion Ascii Thomson -> Ansi
*****/

LONG Cnv_AsciiG2ToAnsi(struct ConvertContext *Ctx)
{
    /* Tableaux: respectivement a,e,i,o,u,y,A,E,I,O,U,Y */
    static const UBYTE TableGrave[]      = {0x60,0xe0,0xe8,0xec,0xf2,0xf9, 'y',0xc0,0xc8,0xcc,0xd2,0xd9, 'Y'};
    static const UBYTE TableAigu[]       = {0xb4,0xe1,0xe9,0xed,0xf3,0xfa,0xfd,0xc1,0xc9,0xcd,0xd3,0xda,0xdd};
    static const UBYTE TableCirconflexe[]= {0x5e,0xe2,0xea,0xee,0xf4,0xfb, 'y',0xc2,0xca,0xce,0xd4,0xdb, 'Y'};
    static const UBYTE TableTrema[]      = {0xa8,0xe4,0xeb,0xef,0xf6,0xfc,0xff,0xc4,0xcb,0xcf,0xd6,0xdc, 'Y'};
    const UBYTE **CurTable=(const UBYTE **)&Ctx->UserData;
    LONG DstPos=0;

    while(Ctx->SrcPos<Ctx->SrcLen && DstPos+3<Ctx->DstLen)
    {
        LONG CurChar=(LONG)Ctx->Src[Ctx->SrcPos++];
        LONG NewChar=0;

        switch(Ctx->State)
        {
            case 0:
                if(CurChar==0x16) Ctx->State=1;
                else Ctx->Dst[DstPos++]=CurChar;
                break;

            case 1:
                switch(CurChar)
                {
                    case 0x23: NewChar='£'; break;
                    case 0x24: NewChar='$'; break;
                    case 0x26: NewChar='#'; break;
                    case 0x27: NewChar=0xa7; break;
                    case 0x2c:
                    case 0x2e: NewChar='-'; break;
                    case 0x2d:
                    case 0x2f: NewChar='|'; break;
                    case 0x30: NewChar=' '; break;
                    case 0x31: NewChar=0xb1; break;
                    case 0x38: NewChar=0xf7; break;
                    case 0x3c: NewChar=0xbc; break;
                    case 0x3d: NewChar=0xbd; break;
                    case 0x3e: NewChar=0xbe; break;
                    case 0x6a: Ctx->Dst[DstPos++]='O'; Ctx->Dst[DstPos++]='E'; break;
                    case 0x7a: Ctx->Dst[DstPos++]='o'; Ctx->Dst[DstPos++]='e'; break;
                    case 0x7b: NewChar=0xdf; break;
                    case 0x41: Ctx->State=2; *CurTable=TableGrave; break;
                    case 0x42: Ctx->State=2; *CurTable=TableAigu; break;
                    case 0x43: Ctx->State=2; *CurTable=TableCirconflexe; break;
                    case 0x48: Ctx->State=2; *CurTable=TableTrema; break;
                    case 0x4b: Ctx->State=3; break;
                }
                if(NewChar!=0) Ctx->Dst[DstPos++]=(UBYTE)NewChar;
                if(Ctx->State==1) Ctx->State=0;
                break;

            case 2: /* Accent */
                NewChar=CurChar;
                switch(CurChar)
                {
                    case ' ': NewChar=(LONG)(*CurTable)[0]; break;
                    case 'a': NewChar=(LONG)(*CurTable)[1]; break;
                    case 'e': NewChar=(LONG)(*CurTable)[2]; break;
                    case 'i': NewChar=(LONG)(*CurTable)[3]; break;
                    case 'o': NewChar=(LONG)(*CurTable)[4]; break;
                    case 'u': NewChar=(LONG)(*CurTable)[5]; break;
                    case 'y': NewChar=(LONG)(*CurTable)[6]; break;
                    case 'A': NewChar=(LONG)(*CurTable)[7]; break;
                    case 'E': NewChar=(LONG)(*CurTable)[8]; break;
                    case 'I': NewChar=(LONG)(*CurTable)[9]; break;
                    case 'O': NewChar=(LONG)(*CurTable)[10]; break;
                    case 'U': NewChar=(LONG)(*CurTable)[11]; break;
                    case 'Y': NewChar=(LONG)(*CurTable)[12]; break;
                }
                Ctx->Dst[DstPos++]=(UBYTE)NewChar;
                Ctx->State=0;
                break;

            case 3: /* cedille */
                switch(CurChar)
                {
                    case ' ': CurChar=0xb8; break;
                    case 'c': CurChar=0xe7; break;
                    case 'C': CurChar=0xc7; break;
                }
                Ctx->Dst[DstPos++]=CurChar;
                Ctx->State=0;
                break;
        }
    }

    return DstPos;
}


/*****
    Conversion d'un fichier Ansi Amiga en fichier Assdesass
*****/

LONG Cnv_TextToAssdesass(struct ConvertContext *Ctx)
{
    struct AnsiToAssContext *SubCtx=(struct AnsiToAssContext *)Ctx->UserData;

    SubCtx->DstPos=0;

    /* Automate: On veut au minimum 3 caracteres d'avance en destination */
    while(Ctx->SrcPos<Ctx->SrcLen && SubCtx->DstPos+2<Ctx->DstLen)
    {
        LONG Char=0;

        if(Ctx->State<10) Char=(LONG)Ctx->Src[Ctx->SrcPos++];
        if(Ctx->State==0) {SubCtx->TmpBufferLen=0; Ctx->State++;}
        if(Char==10 || Char==13) Ctx->State=8;

        switch(Ctx->State)
        {
            case 1: /* Teste si on commence par un blanc, un numero de ligne, un commentaire, ou un label */
                if(Char==32 || Char==9) {P_Cnv_AddToTmpBuffer(SubCtx,9,FALSE); Ctx->State=4;}
                else
                {
                    if(Char>='0' && Char<='9') Ctx->State=2;
                    else {P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE); Ctx->State=(Char=='*'?7:3);}
                }
                break;

            case 2: /* Squizz le numero */
                if(Char==9) {P_Cnv_AddToTmpBuffer(SubCtx,9,FALSE); Ctx->State=4;}
                else
                {
                    if(Char==32) Ctx->State=3;
                    else if(Char<'0' || Char>'9') {P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE); Ctx->State=(Char=='*'?7:3);}
                }
                break;

            case 3: /* Scan le label */
                if(Char==32 || Char==9) {P_Cnv_AddToTmpBuffer(SubCtx,9,FALSE); Ctx->State=4;}
                else
                {
                    if(Char=='*') Ctx->State=7;
                    P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE);
                }
                break;

            case 4: /* Squizz les espaces avant l'instruction */
                if(Char!=9 && Char!=32)
                {
                    Ctx->State=(Char=='*'?7:5);
                    P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE);
                }
                break;

            case 5: /* Scan l'instruction */
                if(Char==32 || Char==9) {P_Cnv_AddToTmpBuffer(SubCtx,9,FALSE); Ctx->State=6;}
                else
                {
                    if(Char=='*') Ctx->State=7;
                    P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE);
                }
                break;

            case 6: /* Squizz les espaces avant l'operande */
                if(Char!=9 && Char!=32)
                {
                    P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE);
                    Ctx->State=9;
                }
                break;

            case 7: /* C'est du commentaire, on prend tout */
                P_Cnv_AddToTmpBuffer(SubCtx,Char,FALSE);
                break;

            case 9: /* On prend tout ce qui est a partir de l'operande */
                if(Char=='*') Ctx->State=7;
                P_Cnv_AddToTmpBuffer(SubCtx,Char,TRUE);
                break;

            case 8: /* Fin de remplissage du tampon, et debut du traitement */
                /* On vire les blancs à la fin */
                while(SubCtx->TmpBufferLen>0)
                {
                    Char=SubCtx->TmpBufferPtr[SubCtx->TmpBufferLen-1];
                    if(Char!=9 && Char!=32 && Char!=1) break;
                    SubCtx->TmpBufferLen--;
                }
                if(SubCtx->TmpBufferLen==0) Ctx->State=0;
                else
                {
                    SubCtx->TmpBufferPos=0;
                    SubCtx->DstPos=Cnv_TextToAssdesassFlush(Ctx,FALSE);
                }
                break;

            default:
                SubCtx->DstPos=Cnv_TextToAssdesassFlush(Ctx,FALSE);
                break;
        }
    }

    return SubCtx->DstPos;
}


/*****
    Routine pour terminer de vider le cache de conversion de la fonction
    ci-dessus.
    A n'utliser qu'apres celle-ci avec IsClose=TRUE!
*****/

LONG Cnv_TextToAssdesassFlush(struct ConvertContext *Ctx, BOOL IsClose)
{
    struct AnsiToAssContext *SubCtx=(struct AnsiToAssContext *)Ctx->UserData;
    LONG DstPos=SubCtx->DstPos;

    if(Ctx->State>0 && Ctx->State<10)
    {
        /* On force le flush */
        SubCtx->TmpBufferPos=0;
        Ctx->State=10;
    }

    /* Automate: On veut au minimum 3 caracteres d'avance en destination */
    while(DstPos+2<Ctx->DstLen && Ctx->State>0)
    {
        char Char=0;

        if(SubCtx->TmpBufferPos>=SubCtx->TmpBufferLen)
        {
            if(IsClose) Ctx->Dst[DstPos++]=0;
            Ctx->State=0;
        }

        switch(Ctx->State)
        {
            case 10: /* Debut de ligne: insersion du numero de ligne */
                SubCtx->TabIdx=0;
                SubCtx->CurPos=0;
                Ctx->Dst[DstPos++]=13;
                Ctx->Dst[DstPos++]=(UBYTE)(SubCtx->LineNumber>>8);
                Ctx->Dst[DstPos++]=(UBYTE)SubCtx->LineNumber;
                SubCtx->LineNumber+=SubCtx->LineNumberStep;
                Ctx->State=11;
                break;

            case 11:
                Char=SubCtx->TmpBufferPtr[SubCtx->TmpBufferPos++];
                if(Char==32) SubCtx->TabIdx++;
                else if(Char==9) SubCtx->TabIdx=(SubCtx->TabIdx&~7)+8;
                else if(SubCtx->TabIdx>SubCtx->CurPos) {SubCtx->TmpBufferPos--; Ctx->State=12;}
                else {DstPos+=P_Cnv_AnsiToAsciiG2(&Ctx->Dst[DstPos],Char); SubCtx->CurPos++; SubCtx->TabIdx++;}
                break;

            case 12: /* Traitement des blancs */
                {
                    LONG Count=SubCtx->TabIdx-SubCtx->CurPos>8?8:SubCtx->TabIdx-SubCtx->CurPos;
                    Ctx->Dst[DstPos++]=(UBYTE)Count;
                    SubCtx->CurPos+=Count;
                    if(SubCtx->CurPos>=SubCtx->TabIdx) Ctx->State=11;
                }
                break;
        }
    }

    return DstPos;
}


/*****
    Cette fonction permet de tester le mode d'import pour l'option IMPORTTXT
    - Str: Chaîne à analyser
    - ModeName: Chaîne à tester (ASS ou ASCG2)
    Retourne NULL si le mode ne correspond pas, sinon retourne un pointeur
    sur la zone de paramètres de la chaîne Str pour pouvoir les analyser
    avec les fonctions spécifiques
*****/

const char *Cnv_IsImportTxtMode(const char *Str, const char *ModeName)
{
    while((*Str!=0 && *Str!=',') || *ModeName!=0)
    {
        char Char1=*Str>='A' && *Str<='Z'?*Str|32:*Str;
        char Char2=*ModeName>='A' && *ModeName<='Z'?*ModeName|32:*ModeName;
        if(Char1!=Char2) return NULL;
        Str++;
        ModeName++;
    }
    if(*Str==',') Str++;

    return Str;
}


/*****
    Cette fonction permet de retourner les paramètres pour l'option
    d'import IMPORTTXT=ASS. Exemple: IMPORTTXT=ASS,100,1
*****/

void Cnv_GetImportTxtParamAss(const char *Str, LONG *LineStart, LONG *LineStep)
{
    *LineStart=0;
    *LineStep=0;

    /* On parse le numéro de ligne de départ */
    while(*Str!=0 && *Str!=',')
    {
        *LineStart=*LineStart*10;
        if(*Str>='0' && *Str<='9') *LineStart+=(LONG)(*Str-'0');
        Str++;
    }

    /* On parse le pas d'incrémentation */
    while(*Str!=0)
    {
        *LineStep=*LineStep*10;
        if(*Str>='0' && *Str<='9') *LineStep+=(LONG)(*Str-'0');
        Str++;
    }

    if(*LineStart==0) *LineStart=100;
    if(*LineStep==0) *LineStep=10;
}


/*****
    Sous routine de Cnv_TextToAssdesass() pour ajouter proprement un caractere
    dans le buffer temporaire.
*****/

void P_Cnv_AddToTmpBuffer(struct AnsiToAssContext *Ctx, LONG Char, BOOL IsToUpper)
{
    if(IsToUpper && Char>='a' && Char<='z') Char=~32&Char;
    if(Ctx->TmpBufferLen<Ctx->TmpBufferSize) Ctx->TmpBufferPtr[Ctx->TmpBufferLen++]=(UBYTE)Char;
}


/*****
    Conversion d'un caractere ANSI
*****/

LONG P_Cnv_AnsiToAsciiG2(UBYTE *Buffer, char Char)
{
    struct AnsiToAsciiG2
    {
        ULONG Ansi;
        char *AsciiG2;
    };

    static const struct AnsiToAsciiG2 Table[]=
    {
        {'à',  "\101a"}, {'è',  "\101e"}, {0xec, "\101i"}, {0xf2, "\101o"}, {'ù',  "\101u"},
        {0xc0, "\101A"}, {0xc8, "\101E"}, {0xcc, "\101I"}, {0xd2, "\101O"}, {0xd9, "\101U"},
        {0xe1, "\102a"}, {'é',  "\102e"}, {0xed, "\102i"}, {0xf3, "\102o"}, {0xfa, "\102u"}, {0xfd, "\102y"},
        {0xc1, "\102A"}, {0xc9, "\102E"}, {0xcd, "\102I"}, {0xd3, "\102O"}, {0xda, "\102U"}, {0xdd, "\102Y"},
        {'â',  "\103a"}, {'ê',  "\103e"}, {'î',  "\103i"}, {'ô',  "\103o"}, {'û',  "\103u"},
        {'Â',  "\103A"}, {'Ê',  "\103E"}, {'Î',  "\103I"}, {'Ô',  "\103O"}, {'Û',  "\103U"},
        {'ä',  "\110a"}, {'ë',  "\110e"}, {'ï',  "\110i"}, {'ö',  "\110o"}, {'ü',  "\110u"}, {'ÿ',  "\110y"},
        {'Ä',  "\110A"}, {'Ë',  "\110E"}, {'Ï',  "\110I"}, {'Ö',  "\110O"}, {'Ü',  "\110U"}, {'ÿ',  "\110Y"},
        {'ç',  "\113c"}, {0xc7, "\113C"},
        {'£',  "\x23"},  {0xa7, "\x27"},  {0xb1, "\x31"},  {0xf7, "\x38"},  {0xbc, "\x3c"},
        {0xbd, "\x3d"},  {0xbe, "\x3e"},  {0xdf, "\x7b"},
        {0,    ""},
    };
    LONG Count=1;
    char *Result=NULL;

    if(Char<0 || Char>0x7f)
    {
        LONG i;
        for(i=0;Table[i].Ansi!=0;i++)
        {
            if((UBYTE)Table[i].Ansi==(UBYTE)Char) {Result=Table[i].AsciiG2; break;}
        }
    }

    if(Result==NULL) *Buffer=Char;
    else
    {
        *(Buffer++)=0x16;
        while(*Result!=0) {*(Buffer++)=*(Result++); Count++;}
    }

    return Count;
}


/*****
    Ecriture du numero de ligne dans le buffer
*****/

LONG P_Cnv_WriteNumber(LONG Value, UBYTE *Dst, LONG Completion)
{
    LONG Len=0,Div=10000;
    BOOL PrtInt=FALSE;

    while(Div>0)
    {
        LONG Num=Value/Div;
        if(Num>0 || ++Completion>5) PrtInt=TRUE;
        if(PrtInt) Dst[Len++]=(UBYTE)'0'+Num;
        Value=Value-Num*Div;
        Div=Div/10;
    }

    return Len;
}
