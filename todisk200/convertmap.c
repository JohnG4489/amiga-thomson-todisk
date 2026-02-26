#include "system.h"
#include "multidos.h"
#include "convertmap.h"
#include "diskgeometry.h"
#include "global.h"

/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    17-01-2013 (Seg)    Utilisation de MultiDOS à la place de FileSystem
    31-03-2010 (Seg)    Gestion du format PPM
    30-03-2010 (Seg)    Gestion du format To-Snap
    23-03-2010 (Seg)    Ajout de la fonction CM_LoadImageMAP()
    15-03-2010 (Seg)    Gestion des palettes et des modes d'ecran
    23-02-2010 (Seg)    Conversion d'un fichier map
*/


/*** Defines locaux */

#define MAKEID(c1,c2,c3,c4) ((c1<<24)+(c2<<16)+(c3<<8)+c4)
#define SCRMODE_STANDARD    0
#define SCRMODE_HIRES       1
#define SCRMODE_BM4         2
#define SCRMODE_BM16        3


/*** Structures locales */

struct Tag_BMHD
{
    LONG    LabelBMHD,BMHDsize;
    UWORD   Width,Height;
    UWORD   xLoc,yLoc;
    UBYTE   NPlanes,Masking,Compression;
    UBYTE   Pad1;
    UWORD   TransparentColor;
    UBYTE   xAspect,yAspect;
    WORD    PageWidth,PageHeight;
};

struct Tag_Header
{
    LONG    LabelFORM,FORMsize;
    LONG    LabelILBM;
};

struct Tag_XXXX
{
    LONG    LabelTAG,TAGsize;
};


/*** Variable globale */

const UWORD StdPal[16]=
{
    0x000, 0x00F, 0x0F0, 0x0FF,
    0xF00, 0xF0F, 0xFF0, 0xFFF,
    0xAAA, 0xAAF, 0xAFA, 0xAFF,
    0xFAA, 0xFAF, 0xFFA, 0x0AF
};


/***** Prototypes */
BOOL CM_ConvertMAP(LONG, char *, struct MDHandle *, struct MDHandle *);
BOOL CM_LoadImageMAP(LONG, char *, struct MDHandle *, struct ToMAP *);
void CM_UnloadImageMAP(struct ToMAP *);
void CM_ConvertLine(struct ToMAP *, UBYTE *, LONG);
LONG CM_GetDepthFromMode(LONG);

BOOL P_CM_WriteIFF(struct ToMAP *, struct MDHandle *);
BOOL P_CM_InitToMAP(LONG, UBYTE *, LONG, struct ToMAP *);
void P_CM_FreeToMAP(struct ToMAP *);
LONG P_CM_UnPackMAP(LONG *, LONG, UBYTE **, UBYTE *);
void P_CM_GetDataBitmap(struct ToMAP *, LONG, LONG, UBYTE *, UBYTE *);
LONG P_CM_GetModeID(UBYTE);
void P_CM_ConvertBVR12ToRVB24(UBYTE *, UBYTE *);
ULONG P_CM_GetGammaColor(ULONG);

BOOL P_CM_LoadPalette(UBYTE *, struct MultiDOS *, char *);


/*****
    Lecture et decompression d'un fichier MAP
    Si Mode=-1 alors autodetect
*****/

BOOL CM_ConvertMAP(LONG Mode, char *MapPalName, struct MDHandle *hsrc, struct MDHandle *hdst)
{
    BOOL IsSuccess=TRUE;
    LONG Size=MD_GetFileSize(hsrc);

    if(Size>0)
    {
        UBYTE *Buffer=(UBYTE *)Sys_AllocMem(Size);

        IsSuccess=FALSE;
        if(Buffer!=NULL)
        {
            if(MD_ReadFile(hsrc,Buffer,Size)==Size)
            {
                struct ToMAP ToMAP;

                if(P_CM_InitToMAP(Mode,Buffer,Size,&ToMAP))
                {
                    if(MapPalName!=NULL)
                    {
                        struct MultiDOS MD=*hsrc->MD;
                        P_CM_LoadPalette(ToMAP.Palette,&MD,MapPalName);
                    }

                    P_CM_WriteIFF(&ToMAP,hdst);

                    P_CM_FreeToMAP(&ToMAP);

                    IsSuccess=TRUE;
                }
            }

            Sys_FreeMem((void *)Buffer);
        }
    }

    return IsSuccess;
}


/*****
    Chargement d'une image MAP en vu d'une conversion ligne par ligne
*****/

BOOL CM_LoadImageMAP(LONG Mode, char *MapPalName, struct MDHandle *h, struct ToMAP *ToMAP)
{
    BOOL IsSuccess=FALSE;
    LONG Size=MD_GetFileSize(h);

    if(Size>0)
    {
        UBYTE *Buffer=(UBYTE *)Sys_AllocMem(Size);

        if(Buffer!=NULL)
        {
            if(MD_ReadFile(h,Buffer,Size)==Size)
            {
                IsSuccess=P_CM_InitToMAP(Mode,Buffer,Size,ToMAP);
                if(IsSuccess)
                {
                    if(MapPalName!=NULL)
                    {
                        struct MultiDOS MD=*h->MD;
                        P_CM_LoadPalette(ToMAP->Palette,&MD,MapPalName);
                    }
                }
            }

            Sys_FreeMem((void *)Buffer);
        }
    }

    return IsSuccess;
}


/*****
    Liberation de l'image allouee par CM_LoadImageMAP()
*****/

void CM_UnloadImageMAP(struct ToMAP *ToMAP)
{
    P_CM_FreeToMAP(ToMAP);
}


/*****
    Conversion d'une ligne
*****/

void CM_ConvertLine(struct ToMAP *ToMAP, UBYTE *Buffer, LONG Line)
{
    LONG i;
    UBYTE Plane0,Plane1;
    LONG BytesPerRow=ToMAP->BytesPerRow;
    LONG WordsPerRow=(BytesPerRow+1)>>1;

    /* BUG: En mode hires et bitmap16, la routine lit les données dans
            le vide si le nb de colonnes est impaire.
            Solution utilisée: on test lorsque i>MAPWidth dans P_CM_GetDataBitmap()
    */

    switch(ToMAP->Mode)
    {
        case SCRMODE_STANDARD:  /* Standard */
            for(i=0;i<BytesPerRow;i++)
            {
                UBYTE cform,cfond,Data=0;
                LONG k;

                P_CM_GetDataBitmap(ToMAP,i,Line,&Plane0,&Plane1);
                cform=(((Plane1&0x40)>>3)+((Plane1&0x38)>>3))^0x08;
                cfond=(((Plane1&0x80)>>4)+(Plane1&0x07))^0x08;
                for(k=0;k<4;k++)
                {
                    switch(((cform&1)<<1)+(cfond&1))
                    {
                        case 3: Data=0xff; break;
                        case 2: Data=Plane0; break;
                        case 1: Data=~Plane0; break;
                        case 0: Data=0x00; break;
                    }
                    Buffer[i+2*k*WordsPerRow]=Data;
                    cform>>=1;
                    cfond>>=1;
                }
            }
            break;

        case SCRMODE_HIRES:     /* High resolution */
            for(i=0;i<WordsPerRow;i++)
            {
                P_CM_GetDataBitmap(ToMAP,i,Line,&Plane0,&Plane1);
                Buffer[i*2+0]=Plane0;
                Buffer[i*2+1]=Plane1;
            }
            break;

        case SCRMODE_BM4:       /* Bitmap 4 */
            for(i=0;i<BytesPerRow;i++)
            {
                P_CM_GetDataBitmap(ToMAP,i,Line,&Plane0,&Plane1);
                Buffer[i+0*2*WordsPerRow]=Plane1;
                Buffer[i+1*2*WordsPerRow]=Plane0;
            }
            break;

        case SCRMODE_BM16:      /* Bitmap 16 */
            for(i=0;i<BytesPerRow;i++)
            {
                P_CM_GetDataBitmap(ToMAP,i,Line,&Plane0,&Plane1);
                Buffer[i+0*2*WordsPerRow]=(((Plane0&0x10)>>4)*0xc0) | ((Plane0&0x01)*0x30)      | (((Plane1&0x10)>>4)*0x0c) | ((Plane1&0x01)*0x03);
                Buffer[i+1*2*WordsPerRow]=(((Plane0&0x20)>>5)*0xc0) | (((Plane0&0x02)>>1)*0x30) | (((Plane1&0x20)>>5)*0x0c) | (((Plane1&0x02)>>1)*0x03);
                Buffer[i+2*2*WordsPerRow]=(((Plane0&0x40)>>6)*0xc0) | (((Plane0&0x04)>>2)*0x30) | (((Plane1&0x40)>>6)*0x0c) | (((Plane1&0x04)>>2)*0x03);
                Buffer[i+3*2*WordsPerRow]=(((Plane0&0x80)>>7)*0xc0) | (((Plane0&0x08)>>3)*0x30) | (((Plane1&0x80)>>7)*0x0c) | (((Plane1&0x08)>>3)*0x03);
            }
            break;
    }
}


/*****
    Permet d'obtenir le nombre de plans en fonction du mode
*****/

LONG CM_GetDepthFromMode(LONG Mode)
{
    static const LONG NPlanes[4]={4,1,2,4};
    if(Mode<0 || Mode>3) Mode=0;
    return NPlanes[Mode];
}


/*****
    Ecriture du fichier IFF
*****/

BOOL P_CM_WriteIFF(struct ToMAP *ToMAP, struct MDHandle *h)
{
    LONG WordsPerRow=(ToMAP->BytesPerRow+1)>>1;
    LONG Depth=CM_GetDepthFromMode(ToMAP->Mode);
    LONG LineSize=WordsPerRow*2*Depth;
    UBYTE *BufferIFF;

    if((BufferIFF=(UBYTE *)Sys_AllocMem(LineSize))!=NULL)
    {
        struct Tag_BMHD BMHD;
        struct Tag_Header Header;
        struct Tag_XXXX GenericTAG;
        LONG Line;

        /* Write Header */
        Header.LabelFORM=MAKEID('F','O','R','M');
        Header.FORMsize=(4+8+0x14) + (8+3*16) + (8+LineSize*ToMAP->Height);
        Header.LabelILBM=MAKEID('I','L','B','M');
        MD_WriteFile(h,(UBYTE *)&Header,sizeof(struct Tag_Header));

        /* Write BMHD */
        BMHD.LabelBMHD=MAKEID('B','M','H','D');
        BMHD.BMHDsize=0x14;
        BMHD.Width=ToMAP->Width;
        BMHD.Height=ToMAP->Height;
        BMHD.xLoc=0;
        BMHD.yLoc=0;
        BMHD.NPlanes=Depth;
        BMHD.Masking=0;
        BMHD.Compression=0;
        BMHD.Pad1=0;
        BMHD.TransparentColor=0;
        BMHD.xAspect=1;
        BMHD.yAspect=1;
        BMHD.PageWidth=ToMAP->Width;
        BMHD.PageHeight=ToMAP->Height;
        MD_WriteFile(h,(UBYTE *)&BMHD,sizeof(struct Tag_BMHD));

        /* Write CMAP */
        GenericTAG.LabelTAG=MAKEID('C','M','A','P');
        GenericTAG.TAGsize=3*16;
        MD_WriteFile(h,(UBYTE *)&GenericTAG,sizeof(struct Tag_XXXX));
        MD_WriteFile(h,(UBYTE *)ToMAP->Palette,sizeof(ToMAP->Palette));

        /* Write BODY */
        GenericTAG.LabelTAG=MAKEID('B','O','D','Y');
        GenericTAG.TAGsize=LineSize*ToMAP->Height;
        MD_WriteFile(h,(UBYTE *)&GenericTAG,sizeof(struct Tag_XXXX));

        for(Line=0;Line<ToMAP->Height;Line++)
        {
            CM_ConvertLine(ToMAP,BufferIFF,Line);
            MD_WriteFile(h,BufferIFF,LineSize);
        }

        Sys_FreeMem((void *)BufferIFF);

        return TRUE;
    }

    return FALSE;
}


/*****
    Initialisation de la structure ToMAP (decompression du buffer)
*****/

BOOL P_CM_InitToMAP(LONG Mode, UBYTE *Buffer, LONG BufferSize, struct ToMAP *ToMAP)
{
    BOOL IsSuccess=FALSE;
    LONG BlockSize,UnPackSize;
    UWORD BytesPerRow,Width;
    UBYTE *BufferMAP;

    P_CM_ConvertBVR12ToRVB24((UBYTE *)StdPal,ToMAP->Palette);

    ToMAP->Type=TM_TYPE_MAP;
    ToMAP->Buffer1=NULL;
    ToMAP->Buffer2=NULL;

    ToMAP->MAPWidth=(UWORD)Buffer[6];
    ToMAP->MAPHeight=(UWORD)Buffer[7];
    BytesPerRow=ToMAP->MAPWidth+1;
    Width=BytesPerRow*8;
    ToMAP->Height=(ToMAP->MAPHeight+1)*8;
    BlockSize=(LONG)Buffer[1]*256+(LONG)Buffer[2]-5;
    UnPackSize=(LONG)BytesPerRow*(LONG)ToMAP->Height;
    BufferMAP=&Buffer[8];

    if((ToMAP->Buffer1=(UBYTE *)Sys_AllocMem(UnPackSize))!=NULL)
    {
        P_CM_UnPackMAP(&BlockSize,UnPackSize,&BufferMAP,ToMAP->Buffer1);

        IsSuccess=TRUE;
        if(BlockSize>=4)
        {
            BufferMAP+=2;
            BlockSize-=2;

            if(BufferMAP[0]!=0 || BufferMAP[1]!=0)
            {
                if((ToMAP->Buffer2=(UBYTE *)Sys_AllocMem(UnPackSize))!=NULL)
                {
                    P_CM_UnPackMAP(&BlockSize,UnPackSize,&BufferMAP,ToMAP->Buffer2);
                } else IsSuccess=FALSE;
            }
        }

        if(ToMAP->Buffer2==NULL && IsSuccess)
        {
            Width/=2;
            BytesPerRow=(BytesPerRow+1)>>1;
        }

        /* Detection du format To-Snap */
        if(BlockSize>=40)
        {
            UBYTE *ExtPtr=&Buffer[BufferSize-5-40];

            if(ExtPtr[38]==0xa5 && ExtPtr[39]==0x5a)
            {
                Mode=(ExtPtr[4]<<8)+ExtPtr[5];
                P_CM_ConvertBVR12ToRVB24(&ExtPtr[6],ToMAP->Palette);
                ToMAP->Type=TM_TYPE_TOSNAP;
            }
        }

        /* Detection du format PPM */
        if(BlockSize>=36 && ToMAP->Type==TM_TYPE_MAP)
        {
            UBYTE *ExtPtr=&Buffer[BufferSize-5-36];

            if(ExtPtr[34]==0x48 && ExtPtr[35]==0x4c)
            {
                LONG i;
                UWORD BVR12[16];

                Mode=(ExtPtr[32]<<8)+ExtPtr[33];
                for(i=0;i<16;i++)
                {
                    WORD Val=((WORD *)ExtPtr)[i];
                    BVR12[15-i]=(UWORD)(Val>=0?Val:~Val);
                }

                P_CM_ConvertBVR12ToRVB24((UBYTE *)BVR12,ToMAP->Palette);
                ToMAP->Type=TM_TYPE_PPM;
            }
        }

        /* Detection automatique des modes */
        if(Mode<0 || Mode>3) Mode=P_CM_GetModeID(Buffer[5]);
        ToMAP->Mode=Mode;

        if(Mode==SCRMODE_HIRES) /* High Resolution */
        {
            BytesPerRow=ToMAP->MAPWidth+1;
            Width=BytesPerRow*8;
        }

        ToMAP->Width=Width;
        ToMAP->BytesPerRow=BytesPerRow;
    }

    if(!IsSuccess) P_CM_FreeToMAP(ToMAP);

    return IsSuccess;
}


/*****
    Libere les donnees allouees par P_CM_InitToMAP()
*****/

void P_CM_FreeToMAP(struct ToMAP *ToMAP)
{
    if(ToMAP->Buffer1!=NULL) Sys_FreeMem((void *)ToMAP->Buffer1);
    if(ToMAP->Buffer2!=NULL) Sys_FreeMem((void *)ToMAP->Buffer2);
}


/*****
    Decompression des donnees MAP
*****/

LONG P_CM_UnPackMAP(LONG *BlockSize, LONG UnPackSize, UBYTE **PtrBufferMAP, UBYTE *BufferDest)
{
    UBYTE *Tmp=BufferDest;
    UBYTE Count,Data,*BufferMAP=*PtrBufferMAP;

    while(UnPackSize>0 && *BlockSize>0)
    {
        Count=*BufferMAP++;
        (*BlockSize)--;
        if(Count)
        {
            Data=*BufferMAP++;
            (*BlockSize)--;
            do
            {
                *BufferDest++=Data;
                Count--;
                UnPackSize--;
            } while(Count>0 && UnPackSize>0);
        }
        else
        {
            Count=*BufferMAP++;
            (*BlockSize)--;
            do
            {
                Data=*BufferMAP++;
                (*BlockSize)--;
                *BufferDest++=Data;
                Count--;
                UnPackSize--;
            } while(Count>0 && UnPackSize>0);
        }
    }

    *PtrBufferMAP=BufferMAP;

    return (LONG)(BufferDest-Tmp);
}


/*****
    Permet d'obtenir une donnee en colonne X, ligne Y
*****/

void P_CM_GetDataBitmap(struct ToMAP *ToMAP, LONG x, LONG y, UBYTE *P0, UBYTE *P1)
{
    if(ToMAP->Buffer2!=NULL)
    {
        *P0=ToMAP->Buffer1[x*ToMAP->Height+y];
        *P1=ToMAP->Buffer2[x*ToMAP->Height+y];
    }
    else
    {
        *P0=ToMAP->Buffer1[x*ToMAP->Height*2+y];
        if(x>(ToMAP->BytesPerRow-1)) *P1=0;
            else *P1=ToMAP->Buffer1[x*(ToMAP->Height*2)+ToMAP->Height+y];
    }
}


/*****
    Retourne le mode d'ecran en fonction de l'Id fourni dans la structure map
*****/

LONG P_CM_GetModeID(UBYTE ID)
{
    switch(ID)
    {
        case 0x40: return SCRMODE_BM16;
        case 0x80: return SCRMODE_HIRES;
        case 0x00: return SCRMODE_STANDARD;
    }

    return SCRMODE_BM4;
}


/*****
    Construction d'une palette RVB 24 bits a partir d'une palette BVR 12 bits
*****/

void P_CM_ConvertBVR12ToRVB24(UBYTE *BVR12, UBYTE *RVB24)
{
    LONG i;

    for(i=0;i<16;i++)
    {
        UBYTE Val1=BVR12[i*2+0];
        UBYTE Val2=BVR12[i*2+1];
        *(RVB24++)=P_CM_GetGammaColor((ULONG)(Val2&0x0f));
        *(RVB24++)=P_CM_GetGammaColor((ULONG)(Val2&0xf0)>>4);
        *(RVB24++)=P_CM_GetGammaColor((ULONG)(Val1&0x0f));
    }
}


/*****
    Applique une correction gamme sur une composante de couleur
*****/

ULONG P_CM_GetGammaColor(ULONG Comp)
{
    static const UBYTE gamma[16]={0,25,32,37,41,44,46,49,51,53,55,57,59,61,62,63};

    return (ULONG)(gamma[Comp]*255/63);
}


/*****
    Chargement d'un fichier de pallette CFG
*****/

BOOL P_CM_LoadPalette(UBYTE *pal, struct MultiDOS *MD, char *Name)
{
    struct MDHandle MH;

    /* Si le nom est precede du caractere '\', alors on recherche sur l'amiga */
    if(Name[0]=='\\') {MD->Type=MDOS_HOST; MD->FS=NULL;  Name=&Name[1];}

    if(MD_OpenFile(&MH,MD,MD_MODE_OLDFILE,Name,NULL)>=0)
    {
        UBYTE BVR12[16*2];

        MD_ReadFile(&MH,BVR12,sizeof(BVR12));
        P_CM_ConvertBVR12ToRVB24(BVR12,pal);

        MD_CloseFile(&MH);

        return TRUE;
    }

    return FALSE;
}
