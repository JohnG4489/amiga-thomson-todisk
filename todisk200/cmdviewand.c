#include "system.h"
#include "multidos.h"
#include "global.h"
#include "cmdviewand.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    27-08-2018 (Seg)    Remaniement de code
    21-08-2018 (Seg)    Adaptation pour rom 2.x
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    17-12-2012 (Seg)    Ajout des raccourcis clavier CTRL+'+' et
                        CTRL+'-', ainsi que CTRL+SHIFT+'+' et
                        CTRL+SHIFT+'-' pour débuguer des tableaux.
    03-09-2012 (Seg)    Amélioration du code
    21-03-2010 (Seg)    Visualise les tableaux du jeu Androids
*/


/*** Defines locaux */
#define SCR_WIDTH 320
#define SCR_HEIGHT 200
#define SCR_DEPTH 3


/*** Variables globales */
#ifdef SYSTEM_AMIGA
UWORD __chip Data_Mur_Bleu[]=
{
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000
};
UWORD __chip Data_Mur_Jaune[]=
{
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000,
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};
UWORD __chip Data_Mur_Rouge[]=
{
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};
UWORD __chip Data_Mur_Vert[]=
{
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};
UWORD __chip Data_Mur_BleuClair[]=
{
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000,
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000
};
UWORD __chip Data_Mur_Rose[]=
{
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0xfe00,0xfe00,0xfe00,0x0000,0xf700,0xf700,0xf700,0x0000
};

UWORD __chip Data_Ech[]=
{
    0x8100,0x8100,0x8100,0xff00,0x8100,0x8100,0x8100,0x8100,
    0x8100,0x8100,0x8100,0xff00,0x8100,0x8100,0x8100,0x8100,
    0x8100,0x8100,0x8100,0xff00,0x8100,0x8100,0x8100,0x8100
};
UWORD __chip Data_Lia[]=
{
    0x0000,0x0000,0x0000,0xff00,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0xff00,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0xff00,0x0000,0x0000,0x0000,0x0000
};
UWORD __chip Data_Col[]=
{
    0x0000,0x0000,0x2c00,0x1000,0xff00,0x9900,0x9900,0xff00,
    0x0000,0x0000,0x2c00,0x1000,0xff00,0x9900,0x9900,0xff00,
    0x0000,0x0000,0x2c00,0x1000,0xff00,0x9900,0x9900,0xff00
};
UWORD __chip Data_And[]=
{
    0x0000,0x1000,0x3800,0x5400,0x3800,0x1000,0x1000,0x1000,
    0x0000,0x1000,0x3800,0x5400,0x3800,0x1000,0x1000,0x1000,
    0x0000,0x1000,0x3800,0x5400,0x3800,0x1000,0x1000,0x1000
};
UWORD __chip Data_Jou[]=
{
    0xff00,0xef00,0xc700,0xab00,0xc700,0xef00,0xef00,0xef00,
    0xff00,0xef00,0xc700,0xab00,0xc700,0xef00,0xef00,0xef00,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};

struct Image Img_Mur_Bleu={0,0,8,8,3,Data_Mur_Bleu,7,0,NULL};
struct Image Img_Mur_Jaune={0,0,8,8,3,Data_Mur_Jaune,7,0,NULL};
struct Image Img_Mur_Rouge={0,0,8,8,3,Data_Mur_Rouge,7,0,NULL};
struct Image Img_Mur_Vert={0,0,8,8,3,Data_Mur_Vert,7,0,NULL};
struct Image Img_Mur_BleuClair={0,0,8,8,3,Data_Mur_BleuClair,7,0,NULL};
struct Image Img_Mur_Rose={0,0,8,8,3,Data_Mur_Rose,7,0,NULL};
struct Image Img_Ech={0,0,8,8,3,Data_Ech,7,0,NULL};
struct Image Img_Lia={0,0,8,8,3,Data_Lia,7,0,NULL};
struct Image Img_Col={0,0,8,8,3,Data_Col,7,0,NULL};
struct Image Img_And={0,0,8,8,3,Data_And,7,0,NULL};
struct Image Img_Jou={0,0,8,8,3,Data_Jou,7,0,NULL};


struct TextAttr TAttribute[]=
{
    "topaz.font",
    8,
    0,1
};
#endif

/***** Prototypes */
void Cmd_ViewAndPattern(struct ToDiskData *, struct MultiDOS *, const char *);
LONG Cmd_ViewAnd(struct ToDiskData *, struct MultiDOS *, const char *);

UBYTE *P_Cmd_LoadTables(struct MultiDOS *, const char *, LONG *);

#ifdef SYSTEM_AMIGA
void P_Cmd_DrawTable(struct RastPort *, LONG, LONG, UBYTE *, UBYTE *, LONG);
void P_Cmd_NormalizeCoordinate(LONG *, LONG *);
void P_Cmd_InitPalette(struct ViewPort *);
void P_Cmd_PrintText(struct RastPort *, const char *, LONG, LONG, LONG, LONG);
#endif


/*****
    Visualisation des tableaux du jeu Androides
*****/

void Cmd_ViewAndPattern(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Pattern)
{
    LONG SrcPathLen;
    char *SrcPath;

    if(*Pattern=='\\') {Pattern++; MD->Type=MDOS_HOST; MD->FS=NULL;}

    SrcPathLen=Sys_PathPart(Pattern)-Pattern;
    SrcPath=(char *)Sys_AllocMem(SrcPathLen+sizeof(char));

    if(SrcPath!=NULL)
    {
        REGEX *PPatt;
        const char *SrcName=Sys_FilePart(Pattern);
        LONG i;

        for(i=0;i<SrcPathLen;i++) SrcPath[i]=Pattern[i];
        SrcPath[i]=0;

        if((PPatt=Sys_AllocPatternNoCase(SrcName,NULL))!=NULL)
        {
            struct MDFileObject MDFO;

            if(MD_InitFileObject(&MDFO,MD,SrcPath))
            {
                LONG ExitCode=1;

                MD_ExamineFileObject(&MDFO);
                while(ExitCode>=0 && MD_ExamineNextFileObject(&MDFO))
                {
                    if(Sys_MatchPattern(PPatt,MDFO.Name))
                    {
                        Sys_Printf("Display Androides tables \"%s\"\n",MDFO.Name);
                        ExitCode=Cmd_ViewAnd(TDData,MD,MDFO.Name);
                        if(ExitCode<=0) Sys_PrintFault(ERROR_BREAK);
                    }
                }

                MD_FreeFileObject(&MDFO);
            }

            Sys_FreePattern(PPatt);
        }

        Sys_FreeMem((void *)SrcPath);
    }
}


/*****
    Visualisation d'un tableau du jeu Androides
*****/

LONG Cmd_ViewAnd(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Name)
{
    LONG ExitCode=1,Size=0;
    UBYTE *TableBase=P_Cmd_LoadTables(MD,Name,&Size);

    if(TableBase!=NULL)
    {
#ifdef SYSTEM_AMIGA
        struct Screen *ScreenBase=OpenScreenTags(NULL,
            SA_Left,0,
            SA_Top,0,
            SA_Width,SCR_WIDTH,
            SA_Height,SCR_HEIGHT,
            SA_Depth,SCR_DEPTH,
            SA_FullPalette,TRUE,
            SA_Overscan,OSCAN_TEXT,
            SA_Type,CUSTOMSCREEN,
            SA_DisplayID,Sys_GetBestModeID(SCR_WIDTH,SCR_HEIGHT,SCR_DEPTH),
            SA_ShowTitle,0,
            TAG_DONE);

        if(ScreenBase!=NULL)
        {
            struct Window *WindowBase=WindowBase=OpenWindowTags(NULL,
                WA_PubScreen,ScreenBase,
                WA_RMBTrap,TRUE,
                WA_SimpleRefresh,TRUE,
                //WA_BackFill,LAYERS_NOBACKFILL,
                WA_Flags,WFLG_BACKDROP|WFLG_BORDERLESS|WFLG_ACTIVATE,
                WA_IDCMP,IDCMP_VANILLAKEY,
                TAG_DONE);

            if(WindowBase!=NULL)
            {
                struct RastPort *rp=WindowBase->RPort;
                struct IntuiMessage *msg;
                LONG OrgX=((LONG)ScreenBase->Width-SCR_WIDTH)/2;
                LONG OrgY=((LONG)ScreenBase->Height-SCR_HEIGHT)/2;
                LONG Offset=0,TableNum=0;
                UBYTE *TableEnd=&TableBase[Size];

                P_Cmd_InitPalette(&ScreenBase->ViewPort);

                /* Si un promotor d'ecran a ouvert un ecran plus grand, on nettoie
                   et on centre tout.
                */
                if(ScreenBase->Width>SCR_WIDTH || ScreenBase->Height>SCR_HEIGHT)
                {
                    SetAPen(rp,0);
                    RectFill(rp,0,0,ScreenBase->Width-1,ScreenBase->Height-1);
                }
                P_Cmd_DrawTable(rp,OrgX,OrgY,TableBase,TableEnd,0);

                while(ExitCode>0)
                {
                    BOOL IsRefresh=FALSE;

                    WaitPort(WindowBase->UserPort);
                    if((msg=(struct IntuiMessage *)GetMsg(WindowBase->UserPort))!=NULL)
                    {
                        if(msg->Code==0x1b) ExitCode=0;
                        if(msg->Code==3) ExitCode=-1; /* Ctrl+C */
                        if(msg->Code==5) ExitCode=0; /* Ctrl+E */
                        if(msg->Code>='0' && msg->Code<='9') {TableNum=msg->Code-'0'; IsRefresh=TRUE;}
                        if(msg->Qualifier&IEQUALIFIER_CONTROL)
                        {
                            LONG Step=msg->Qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)?1:40;
                            if(msg->Qualifier&IEQUALIFIER_LALT) Step=40*10;

                            if(msg->Code=='+')
                            {
                                IsRefresh=TRUE;
                                if(Offset+991*9+Step<=Size) Offset+=Step; else Offset=Size-991*9;
                            }
                            if(msg->Code=='-')
                            {
                                IsRefresh=TRUE;
                                if(Offset-Step>=0) Offset-=Step; else Offset=0;
                            }
                            Sys_Printf("Offset: %lx (%ld)\n",(long)Offset,(long)Offset);
                        }

                        ReplyMsg((struct Message *)msg);
                    }

                    if(IsRefresh) P_Cmd_DrawTable(rp,OrgX,OrgY,TableBase+Offset,TableEnd,TableNum);
                }

                CloseWindow(WindowBase);
            }

            CloseScreen(ScreenBase);
        }
#endif
        Sys_FreeMem((void *)TableBase);
    }
    else
    {
        Sys_Printf("Error when openning '%s' file\n",Name);
    }

    return ExitCode;
}


/*****
    Chargement des tables
*****/

UBYTE *P_Cmd_LoadTables(struct MultiDOS *MD, const char *FileName, LONG *Size)
{
    UBYTE *Ptr=NULL;
    struct MDHandle MH;

    if(MD_OpenFile(&MH,MD,MD_MODE_OLDFILE,FileName,NULL)>=0)
    {
        LONG CurSize=MD_GetFileSize(&MH);

        if(CurSize!=NULL) *Size=CurSize;

        if(CurSize>0)
        {
            Ptr=(UBYTE *)Sys_AllocMem(CurSize);

            if(Ptr!=NULL)
            {
                LONG Type=MD_GetTypeFromHandle(&MH);

                if(Type==0x0200)
                {
                    /* Lecture des tableaux Androides dans l'ancien format */
                    UBYTE Header[5];
                    UBYTE *Tmp=Ptr;

                    while(MD_ReadFile(&MH,Header,sizeof(Header))>0)
                    {
                        LONG Len=((LONG)Header[1]<<8)+(LONG)Header[2];
                        MD_ReadFile(&MH,Tmp,Len);
                        Tmp=&Tmp[Len];
                    }
                }
                else
                {
                    /* Lecture des tableaux Androides dans le format Super Androides */
                    MD_ReadFile(&MH,Ptr,CurSize);
                }
            }
        }

        MD_CloseFile(&MH);
    }

    return Ptr;
}

#ifdef SYSTEM_AMIGA

/*****
    Affichage du tableau sur le rastport
*****/

void P_Cmd_DrawTable(struct RastPort *rp, LONG OrgX, LONG OrgY, UBYTE *TableBase, UBYTE *TableEnd, LONG Num)
{
    UBYTE *CurPos,*Table=(UBYTE *)TableBase+991*Num;
    LONG c,l,i,NCol=0,Temps=0,Vitesse=0,Intelligence=0;
    struct Image *Img_Mur_xxx;
    UBYTE Text[64];

    SetAPen(rp,0);
    RectFill(rp,OrgX,OrgY,OrgX+SCR_WIDTH-1,OrgY+SCR_HEIGHT-1);

    switch(Num)
    {
        case 0: Img_Mur_xxx=&Img_Mur_Bleu; break;
        case 1: Img_Mur_xxx=&Img_Mur_Jaune; break;
        case 2: Img_Mur_xxx=&Img_Mur_Rouge; break;
        case 3: Img_Mur_xxx=&Img_Mur_Vert; break;
        case 4: Img_Mur_xxx=&Img_Mur_BleuClair; break;
        case 5: Img_Mur_xxx=&Img_Mur_Rose; break;
        case 6: Img_Mur_xxx=&Img_Mur_Rouge; break;
        case 7: Img_Mur_xxx=&Img_Mur_Vert; break;
        case 8: Img_Mur_xxx=&Img_Mur_Bleu; break;
        default: Img_Mur_xxx=&Img_Mur_Jaune; break;
    }

    for(l=0;l<22;l++)
    {
        for(c=0;c<40;c++)
        {
            CurPos=&Table[c+l*40];
            if(CurPos<TableEnd)
            {
                switch(*CurPos)
                {
                    case 0x80: DrawImage(rp,Img_Mur_xxx,OrgX+c*8,OrgY+l*8); break;
                    case 0x40: DrawImage(rp,&Img_Ech,OrgX+c*8,OrgY+l*8); break;
                    case 0x20: DrawImage(rp,&Img_Lia,OrgX+c*8,OrgY+l*8); break;
                }
            }
        }
    }

    for(i=0;i<20;i++)
    {
        CurPos=&Table[40*22+i*5];
        if(CurPos<TableEnd)
        {
            l=(int)CurPos[1]*4;
            c=(int)CurPos[2]*8;
            P_Cmd_NormalizeCoordinate(&c,&l);
            switch(CurPos[0])
            {
                case 0x0e: DrawImage(rp,&Img_Col,OrgX+c,OrgY+l); break;
                case 0x02: DrawImage(rp,&Img_And,OrgX+c,OrgY+l); break;
            }
        }
    }

    CurPos=&Table[40*22+100];
    if(CurPos+8<TableEnd)
    {
        NCol=(LONG)CurPos[0];
        Temps=(LONG)CurPos[1]*256+(int)CurPos[2];
        Vitesse=(LONG)CurPos[3]*256+(int)CurPos[4];
        Intelligence=(LONG)CurPos[5];

        l=(LONG)CurPos[7]*4;
        c=(LONG)CurPos[8]*8;
        P_Cmd_NormalizeCoordinate(&c,&l);
        DrawImage(rp,&Img_Jou,OrgX+(WORD)c,OrgY+(WORD)l);
    }

    Sys_SPrintf(Text,"Vitesse    :%ld",(long)Vitesse);
    P_Cmd_PrintText(rp,Text,OrgX+0,OrgY+22*8,7,0);

    Sys_SPrintf(Text,"Difficulté :%ld",(long)Intelligence);
    P_Cmd_PrintText(rp,Text,OrgX+0,OrgY+23*8,7,0);

    Sys_SPrintf(Text,"Temps   :%ld",(long)Temps);
    P_Cmd_PrintText(rp,Text,OrgX+22*8,OrgY+22*8,7,0);

    Sys_SPrintf(Text,"Colis   :%ld",(long)NCol);
    P_Cmd_PrintText(rp,Text,OrgX+22*8,OrgY+23*8,7,0);
}


/*****
    Normalise les coordonnees des objets
*****/

void P_Cmd_NormalizeCoordinate(LONG *c, LONG *l)
{
    //Sys_Printf("%ld x %ld\n",*c,*l);
    if(*c<0) *c=0; else if(*c>=40*8) *c=39*8;
    if(*l<0) *l=0; else if(*l>=22*8) *l=21*8;
}


/*****
    Initialisation des palette du ViewPort
*****/

void P_Cmd_InitPalette(struct ViewPort *vp)
{
    SetRGB4(vp,0,0x00,0x00,0x00);
    SetRGB4(vp,1,0x0f,0x00,0x00);
    SetRGB4(vp,2,0x00,0x0f,0x00);
    SetRGB4(vp,3,0x0f,0x0f,0x00);
    SetRGB4(vp,4,0x00,0x00,0x0f);
    SetRGB4(vp,5,0x0f,0x00,0x0f);
    SetRGB4(vp,6,0x00,0x0f,0x0f);
    SetRGB4(vp,7,0x0f,0x0f,0x0f);
}


/*****
    Affichage d'un texte sur le rastport
*****/

void P_Cmd_PrintText(struct RastPort *rp, const char *Text, LONG Left, LONG Top, LONG Color1, LONG Color2)
{
    struct IntuiText FinalText;

    FinalText.FrontPen=(UBYTE)Color1;
    FinalText.BackPen=(UBYTE)Color2;
    FinalText.DrawMode=AUTOFRONTPEN;
    FinalText.LeftEdge=0;
    FinalText.TopEdge=0;
    FinalText.ITextFont=TAttribute;
    FinalText.IText=(UBYTE *)Text;
    FinalText.NextText=NULL;

    PrintIText(rp,&FinalText,Left,Top);
}

#endif
