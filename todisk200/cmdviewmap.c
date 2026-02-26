#include "system.h"
#include "multidos.h"
#include "global.h"
#include "cmdviewmap.h"
#include "convertmap.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    28-08-2018 (Seg)    Remaniement de code
    21-08-2018 (Seg)    Adaptation pour rom 2.x
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    03-09-2012 (Seg)    Amélioration du code
    24-03-2010 (Seg)    Gestion de la visualisation par pattern de nom
    21-03-2010 (Seg)    Visualise d'une image MAP
*/


/***** Prototypes */
void Cmd_ViewMapPattern(struct ToDiskData *, struct MultiDOS *, const char *);
LONG Cmd_ViewMap(struct ToDiskData *, struct MultiDOS *, const char *);

#ifdef SYSTEM_AMIGA
void P_Cmd_InitPaletteMAP(struct ViewPort *, const UBYTE *);
#endif

/*****
    Visualisation d'une image MAP
*****/

void Cmd_ViewMapPattern(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Pattern)
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
            if(FO->Type==0x0200 && Sys_MatchPattern(PPatt,FO->Name))
            {
                Sys_Printf("Display image \"%s\"\n",FO->Name);
                ExitCode=Cmd_ViewMap(TDData,MD,FO->Name);
                if(ExitCode<=0) Sys_PrintFault(ERROR_BREAK);
            }
        }

        Sys_FreePattern(PPatt);
    }
}


/*****
    Visualisation
*****/

LONG Cmd_ViewMap(struct ToDiskData *TDData, struct MultiDOS *MD, const char *Name)
{
    LONG ExitCode=1;
    struct MDHandle MDH;

    if(MD_OpenFile(&MDH,MD,MD_MODE_OLDFILE,Name,NULL)>=0)
    {
        struct ToMAP ToMAP;

        if(CM_LoadImageMAP(TDData->MapMode,TDData->MapPalName,&MDH,&ToMAP))
        {
#ifdef SYSTEM_AMIGA
            LONG WordsPerLine=(ToMAP.BytesPerRow+1)/2;
            LONG DepthMax=4;
            LONG LineSize=WordsPerLine*2*DepthMax;
            UBYTE *BufferLine=(UBYTE *)AllocVec(LineSize,MEMF_CHIP);
            char *Format="Map";

            if(ToMAP.Type==TM_TYPE_TOSNAP) Format="To-Snap";
            else if(ToMAP.Type==TM_TYPE_PPM) Format="Ppm";
            Sys_Printf("%-16s -> %ldx%ld (%s)\n",Name,ToMAP.Width,ToMAP.Height,Format);

            if(BufferLine!=NULL)
            {
                struct Screen *ScreenBase=ScreenBase=OpenScreenTags(NULL,
                    SA_Left,0,
                    SA_Top,0,
                    SA_Width,ToMAP.Width,
                    SA_Height,ToMAP.Height,
                    SA_Depth,DepthMax,
                    SA_FullPalette,TRUE,
                    SA_Overscan,OSCAN_TEXT,
                    SA_Type,CUSTOMSCREEN,
                    SA_DisplayID,Sys_GetBestModeID(ToMAP.Width,ToMAP.Height,DepthMax),
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
                        WA_IDCMP,IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS,
                        TAG_DONE);

                    if(WindowBase!=NULL)
                    {
                        struct RastPort *rp=WindowBase->RPort;
                        struct Image ImgMAP={0,0,0,0,0,NULL,0,0,NULL};
                        LONG OrgX=((LONG)ScreenBase->Width-ToMAP.Width)/2;
                        LONG OrgY=((LONG)ScreenBase->Height-ToMAP.Height)/2;

                        P_Cmd_InitPaletteMAP(&ScreenBase->ViewPort,ToMAP.Palette);

                        /* Si un promotor d'ecran a ouvert un ecran plus grand, on nettoie
                           et on centre tout.
                        */
                        if(ScreenBase->Width>ToMAP.Width || ScreenBase->Height>ToMAP.Height)
                        {
                            SetAPen(rp,0);
                            RectFill(rp,0,0,ScreenBase->Width-1,ScreenBase->Height-1);
                        }

                        ImgMAP.Width=(WORD)ToMAP.Width;
                        ImgMAP.Height=(WORD)1;
                        ImgMAP.ImageData=(UWORD *)BufferLine;

                        while(ExitCode>0)
                        {
                            LONG j,Mode=ToMAP.Mode;
                            LONG Depth=CM_GetDepthFromMode(Mode);

                            ImgMAP.Depth=(WORD)Depth;
                            ImgMAP.PlanePick=(WORD)(1<<Depth)-1;

                            for(j=0;j<LineSize;j++) BufferLine[j]=0;

                            for(j=0;j<ToMAP.Height;j++)
                            {
                                CM_ConvertLine(&ToMAP,BufferLine,j);
                                DrawImage(rp,&ImgMAP,OrgX,OrgY+j);
                            }

                            while(ExitCode>0 && Mode==ToMAP.Mode)
                            {
                                struct IntuiMessage *msg;

                                WaitPort(WindowBase->UserPort);
                                if((msg=(struct IntuiMessage *)GetMsg(WindowBase->UserPort))!=NULL)
                                {
                                    switch(msg->Class)
                                    {
                                        case IDCMP_MOUSEBUTTONS:
                                            ExitCode=0;
                                            break;

                                        default:
                                            if(msg->Code==0x1b || msg->Code==' ') ExitCode=0;
                                            if(msg->Code==3) ExitCode=-1; /* Ctrl+C */
                                            if(msg->Code==5) ExitCode=0; /* Ctrl+E */
                                            if(msg->Code=='0') ToMAP.Mode=0;
                                            if(msg->Code=='1') ToMAP.Mode=1;
                                            if(msg->Code=='2') ToMAP.Mode=2;
                                            if(msg->Code=='3') ToMAP.Mode=3;
                                            break;
                                    }

                                    ReplyMsg((struct Message *)msg);
                                }
                            }
                        }

                        CloseWindow(WindowBase);
                    }

                    CloseScreen(ScreenBase);
                }

                FreeVec((void *)BufferLine);
            }
#endif
            CM_UnloadImageMAP(&ToMAP);
        }
    }
    else
    {
        Sys_Printf("Error when openning '%s' file\n",Name);
    }

    return ExitCode;
}

#ifdef SYSTEM_AMIGA

/*****
    Initialisation des palette du ViewPort
*****/

void P_Cmd_InitPaletteMAP(struct ViewPort *vp, const UBYTE *Palette)
{
    LONG i;

    for(i=0;i<16;i++)
    {
        if(GfxBase->LibNode.lib_Version>=39)
        {
            SetRGB32(vp,i,Palette[0]<<24,Palette[1]<<24,Palette[2]<<24);
        }
        else
        {
            SetRGB4(vp,i,Palette[0]>>4,Palette[1]>>4,Palette[2]>>4);
        }

        Palette+=3;
    }
}

#endif
