#include "system.h"
#include "global.h"
#include "disklayer.h"
#include "filesystem.h"
#include "cmddir.h"
#include "cmddelete.h"
#include "cmdformat.h"
#include "cmdcopy.h"
#include "cmdbackup.h"
#include "cmddumpsector.h"
#include "cmddump.h"
#include "cmdview.h"
#include "cmdviewand.h"
#include "cmdviewmap.h"
#include "cmdsortdir.h"
#include "cmdcompareraw.h"
#include "cmdcomparedos.h"
#include "cmdfinddisk.h"
#include "convert.h"
#include "util.h"

#ifdef SYSTEM_AMIGA
#include "configamiga.h"
#endif

#ifdef SYSTEM_UNIX
#include "configunix.h"
#endif


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    21-01-2013 (Seg)    Nouvelle fonctionnalité pour lire les fichiers K7 (Mo et To)
    17-01-2013 (Seg)    Réorganisation suite à l'utilisation massive de MultiDOS
    14-10-2012 (Seg)    Ajout des paramètres Line Start et Line Step dans l'option d'import ASS
    11-09-2012 (Seg)    Ajout de l'option NORFD pour desactiver la recherche recursive de FINDDSK
    06-09-2012 (Seg)    Ajout de l'option de conversion TXTTOASS pour convertir des fichiers Texte
                        dans le format Assdesass.
    03-09-2012 (Seg)    Amélioration du code
    29-08-2012 (Seg)    Ajout de l'option VIEW
    11-08-2012 (Seg)    Ajout de FINDDSK pour rechercher une image disque équivalente à la source
    08-08-2012 (Seg)    Ajout de DUMPSECTOR pour afficher le contenu d'un secteur
    10-04-2011 (Seg)    Ajout de CMPDSKDOS pour faire une comparaison DOS de deux disques
    09-04-2011 (Seg)    Gestion du backup avec preformatage de piste + Ajout de l'option NOFMT
                        pour desactiver le formatage.
                        Ajout de l'option CMPDSKRAW
    27-07-2010 (Seg)    Ajout de l'option CONVASM
    22-05-2010 (Seg)    Refonte du code
    24-03-2010 (Seg)    Gestion de la detection automatique de la source
    21-03-2010 (Seg)    Ajout de la commande de visualisation des tableaux Androids
    18-03-2010 (Seg)    Ajout de la commande Dump + Gestion des arguments de conversion
    10-03-2010 (Seg)    Optimisations diverses
    01-09-2006 (Seg)    Routine principale: début du projet
*/


char Version[]="\0$VER:ToDisk v2.71b by Seg (c) Dimension & Dpt22 "__AMIGADATE__;

/***** Prototypes */
BOOL CheckArg(BOOL *, LONG, struct DiskLayer *);
BOOL InitMultiDOSFileSystem(struct DiskLayer *, const char *, struct MultiDOS *);
void FreeMultiDOSFileSystem(struct MultiDOS *);
void GetParam(LONG [], struct ToDiskData *);


/*****
    Routine principale
*****/

int main(int narg, char *argv[])
{
    LONG Error=0;

    if(Sys_OpenAllLibs())
    {
        LONG MyPars[COUNTOF_ARGS],i;

        /* Lecture des arguments */
        for(i=0;i<COUNTOF_ARGS;i++) MyPars[i]=0;
        if((Error=Cfg_ParseArg(narg,(const char **)argv,MyPars))==0)
        {
            static struct ToDiskData TDData;
            struct MultiDOS MDSrc;
            ULONG SrcIOMode=MODE_OLDFILE;
            const char *SrcName=(const char *)MyPars[ARG_DSK],*SrcFinalName;
            ULONG SrcId=DISKLAYER_TYPE_FLOPPY,Unit=0,Side=0,Interleave=7;
            BOOL IsFormat=TRUE;

            /* Choix de la source et de son mode de lecture en fonction des options choisies */
            if(MyPars[ARG_FORMATFLP]) {SrcName=(const char *)MyPars[ARG_FORMATFLP]; SrcId=DISKLAYER_TYPE_FLOPPY; SrcIOMode=MODE_NEWFILE;}
            else if(MyPars[ARG_FORMATTDS]) {SrcName=(const char *)MyPars[ARG_FORMATTDS]; SrcId=DISKLAYER_TYPE_TDS; SrcIOMode=MODE_NEWFILE;}
            else if(MyPars[ARG_FORMATSAP]) {SrcName=(const char *)MyPars[ARG_FORMATSAP]; SrcId=DISKLAYER_TYPE_SAP; SrcIOMode=MODE_NEWFILE;}
            else if(MyPars[ARG_FORMATFD]) {SrcName=(const char *)MyPars[ARG_FORMATFD]; SrcId=DISKLAYER_TYPE_FD; SrcIOMode=MODE_READWRITE;}
            else {IsFormat=FALSE; if(MyPars[ARG_SAVE] || MyPars[ARG_DELETE] || MyPars[ARG_SORTDIR]) SrcIOMode=MODE_READWRITE;}

            /* Récupération des paramètres de certaines options */
            GetParam(MyPars,&TDData);

            SrcFinalName=DL_ParseOption(SrcName,&Unit,&Side,&Interleave);
            if(SrcFinalName!=NULL)
            {
                BOOL IsArg=FALSE;
                BOOL IsK7=Utl_CheckName("#?.k7",SrcFinalName),IsK7Possible=FALSE;
                struct DiskLayer *DLSrc=NULL;
                ULONG DLError=DL_SUCCESS;

                /* Ouverture du fichier image */
                if(IsFormat || !IsK7)
                {
                    if(!IsFormat) SrcId=DL_GetDiskLayerType((char *)SrcFinalName,&DLError);
                    if(!DLError) DLSrc=DL_Open(SrcId,(char *)SrcFinalName,SrcIOMode,Unit,Side,&DLError);
                }

                /*************************************************
                 * Cas des commandes FORMATxxx pour la création  *
                 * d'un fichier image ou d'une disquette thomson *
                 *************************************************/
                if(CheckArg(&IsArg,IsFormat,DLSrc))
                {
                    /* Formatage d'un disque thomson */
                    char *Name="";
                    LONG *Files=(LONG *)MyPars[ARG_FILES];

                    if(Files!=NULL) if(Files[0]!=NULL) Name=(char *)Files[0];
                    Cmd_Format(&TDData,DLSrc,Name,Interleave);
                    DL_Finalize(DLSrc,TRUE);
                }
                /*******************************
                 * Comparaison DOS d'un disque *
                 *******************************/
                else if(CheckArg(&IsArg,MyPars[ARG_CMPDSKDOS],DLSrc))
                {
                    struct DiskLayer *DLDst=DL_OpenDiskLayerAuto((char *)MyPars[ARG_CMPDSKDOS],&DLError);
                    if(DLDst!=NULL) Cmd_CompareDos(&TDData,DLSrc,DLDst);
                    DL_Close(DLDst);
                }
                /*********************************
                 * Comparaison brute d'un disque *
                 *********************************/
                else if(CheckArg(&IsArg,MyPars[ARG_CMPDSKRAW],DLSrc))
                {
                    struct DiskLayer *DLDst=DL_OpenDiskLayerAuto((char *)MyPars[ARG_CMPDSKRAW],&DLError);
                    if(DLDst!=NULL) Cmd_CompareRaw(&TDData,DLSrc,DLDst);
                    DL_Close(DLDst);
                }
                /**********************
                 * Backup d'un disque *
                 **********************/
                else if(MyPars[ARG_TOFLP] || MyPars[ARG_TOTDS] || MyPars[ARG_TOSAP] || MyPars[ARG_TOFD])
                {
                    IsArg=TRUE;
                    IsK7Possible=TRUE;
                    if(DLSrc!=NULL || IsK7)
                    {
                        ULONG DstIOMode=MODE_NEWFILE;
                        ULONG DstId=DISKLAYER_TYPE_FLOPPY;
                        const char *DstName,*DstFinalName;

                        if(MyPars[ARG_TOTDS]) {DstName=(const char *)MyPars[ARG_TOTDS]; DstId=DISKLAYER_TYPE_TDS;}
                        else if(MyPars[ARG_TOSAP]) {DstName=(const char *)MyPars[ARG_TOSAP]; DstId=DISKLAYER_TYPE_SAP;}
                        else if(MyPars[ARG_TOFD]) {DstName=(const char *)MyPars[ARG_TOFD]; DstId=DISKLAYER_TYPE_FD; DstIOMode=MODE_READWRITE;}
                        else DstName=(const char *)MyPars[ARG_TOFLP];

                        if((DstFinalName=DL_ParseOption(DstName,&Unit,&Side,&Interleave))!=NULL)
                        {
                            struct DiskLayer *DLDst=DL_Open(DstId,(char *)DstFinalName,DstIOMode,Unit,Side,&DLError);
                            if(DLDst!=NULL)
                            {
                                /* Gestion du backup TOFLP, TOTDS, TOSAP et TOFD */
                                if(MyPars[ARG_NOFMT]) Interleave=0;
                                if(DLSrc!=NULL) Cmd_Backup(&TDData,DLSrc,DLDst,Interleave);
                                else Cmd_BackupFromK7(&TDData,SrcFinalName,DLDst,Interleave);

                                DL_Finalize(DLDst,TRUE);
                                DL_Close(DLDst);
                            }

                            DL_FreeOption(DstFinalName);
                        }
                        else
                        {
                            Sys_Printf("%s\n",MD_GetTextErr(MD_NOT_ENOUGH_MEMORY));
                        }
                    }
                }
                /********************************/
                /* Gestion du tri de repertoire */
                /********************************/
                else if(CheckArg(&IsArg,MyPars[ARG_SORTDIR],DLSrc))
                {
                    Cmd_SortDir(&TDData,DLSrc);
                    DL_Finalize(DLSrc,TRUE);
                }
                /******************************/
                /* Gestion du dump de secteur */
                /******************************/
                else if(CheckArg(&IsArg,MyPars[ARG_DUMPSECTOR],DLSrc))
                {
                    LONG Track,Sector,Count;
                    Cmd_ParseDumpSectorParam((char *)MyPars[ARG_DUMPSECTOR],&Track,&Sector,&Count);
                    Cmd_DumpSector(&TDData,DLSrc,Track,Sector,Count);
                }
                /***********************/
                /* Recherche de disque */
                /***********************/
                else if(CheckArg(&IsArg,MyPars[ARG_FINDDSK],DLSrc))
                {
                    Cmd_FindDisk(&TDData,DLSrc,(char *)MyPars[ARG_FINDDSK],MyPars[ARG_NORFD]?FALSE:TRUE);
                }
                /*************************
                 * Gestion du filesystem *
                 *************************/
                else if(!IsArg && (MyPars[ARG_DIR] || MyPars[ARG_LOAD] || MyPars[ARG_SAVE] || MyPars[ARG_DELETE] ||
                        MyPars[ARG_DUMP] || MyPars[ARG_VIEW] || MyPars[ARG_VIEWMAP] || MyPars[ARG_VIEWAND]))
                {
                    IsArg=TRUE;
                    IsK7Possible=TRUE;
                    if(InitMultiDOSFileSystem(DLSrc,SrcFinalName,&MDSrc))
                    {
                        struct MultiDOS MDDst;

                        MD_InitDOSType(&MDDst,MDOS_HOST,NULL);

                        /* Affichage du repertoire du disque */
                        if(MyPars[ARG_DIR])
                        {
                            char *Pattern="#?";
                            LONG *Files=(LONG *)MyPars[ARG_FILES];

                            if(Files!=NULL) if(Files[0]!=NULL) Pattern=(char *)Files[0];
                            Cmd_Dir(&TDData,&MDSrc,Pattern);
                        }
                        /* Copie de fichiers vers hote */
                        else if(MyPars[ARG_LOAD])
                        {
                            char *DestName="";
                            LONG *Files=(LONG *)MyPars[ARG_FILES];

                            if(Files!=NULL) if(Files[0]!=NULL) DestName=(char *)Files[0];
                            Cmd_Copy(&TDData,&MDSrc,&MDDst,(char *)MyPars[ARG_LOAD],DestName);
                        }
                        /* Copie de fichiers vers disque thomson */
                        else if(MyPars[ARG_SAVE])
                        {
                            char *DestName="";
                            LONG *Files=(LONG *)MyPars[ARG_FILES];

                            if(Files!=NULL) if(Files[0]!=NULL) DestName=(char *)Files[0];
                            Cmd_Copy(&TDData,&MDDst,&MDSrc,(char *)MyPars[ARG_SAVE],DestName);
                            DL_Finalize(DLSrc,TRUE);
                        }
                        /* Effacement d'un fichier */
                        else if(MyPars[ARG_DELETE])
                        {
                            Cmd_Delete(&TDData,&MDSrc,(char *)MyPars[ARG_DELETE]);
                            DL_Finalize(DLSrc,TRUE);
                        }
                        /* Dump d'un fichier */
                        else if(MyPars[ARG_DUMP])
                        {
                            Cmd_DumpPattern(&TDData,&MDSrc,(char *)MyPars[ARG_DUMP]);
                        }
                        /* Visualisation de n'importe quel type de fichier */
                        else if(MyPars[ARG_VIEW])
                        {
                            Cmd_View(&TDData,&MDSrc,(char *)MyPars[ARG_VIEW]);
                        }
                        /* Visualisation d'un fichier MAP */
                        else if(MyPars[ARG_VIEWMAP])
                        {
                            Cmd_ViewMapPattern(&TDData,&MDSrc,(char *)MyPars[ARG_VIEWMAP]);
                        }
                        /* Visualisation des tableaux du jeu Androids */
                        else if(MyPars[ARG_VIEWAND])
                        {
                            Cmd_ViewAndPattern(&TDData,&MDSrc,(char *)MyPars[ARG_VIEWAND]);
                        }

                        FreeMultiDOSFileSystem(&MDSrc);
                    }
                }

                if(!IsArg)
                {
                    Sys_PrintFault(ERROR_REQUIRED_ARG_MISSING);
                }
                else if(DLSrc!=NULL && DLError!=DL_SUCCESS)
                {
                    Sys_Printf("%s\n",DL_GetDLTextErr(DLError));
                }
                else if(!IsK7Possible && DLSrc==NULL && IsK7)
                {
                    Sys_Printf("Bad image source (K7 not supported with this option)\n");
                }

                DL_Close(DLSrc);
                DL_FreeOption(SrcFinalName);
            }
            else
            {
                Sys_Printf("%s\n",MD_GetTextErr(MD_NOT_ENOUGH_MEMORY));
            }

            Cfg_FreeArg(MyPars);
        }
        else
        {
            if(Error>0) Sys_PrintFault(Error);
        }

        Sys_CloseAllLibs();
    }

    return Error;
}


/*****
    Vérification des arguments
*****/

BOOL CheckArg(BOOL *IsArg, LONG Option, struct DiskLayer *DLSrc)
{
    if(!*IsArg)
    {
        if(Option!=0) {*IsArg=TRUE; if(DLSrc!=NULL) return TRUE;}
    }

    return FALSE;
}


/*****
    Allocation du bon filesystem et génération des messages d'erreur si besoin
*****/

BOOL InitMultiDOSFileSystem(struct DiskLayer *DLSrc, const char *Name, struct MultiDOS *MD)
{
    MD->FS=NULL;
    if(DLSrc!=NULL)
    {
        MD_InitDOSType(MD,MDOS_DISK,(void *)FS_AllocFileSystem(COUNTOF_TRACK,SIZEOF_SECTOR,COUNTOF_SECTOR_PER_TRACK,TRUE));
        if(MD->FS!=NULL)
        {
            LONG ErrorCode=FS_InitFileSystem((struct FileSystem *)MD->FS,DLSrc);
            if(DLSrc->Error) {Sys_Printf("%s\n",DL_GetDLTextErr(DLSrc->Error)); return FALSE;}
        }
    }
    else if(Utl_CheckName("#?.k7",Name))
    {
        MD_InitDOSType(MD,MDOS_K7,(void *)K7_AllocFileSystem(Name));
    }
    else
    {
        Sys_Printf("%s\n",MD_GetTextErr(MD_DISKLAYER_ERROR));
        return FALSE;
    }

    if(MD->FS==NULL) {Sys_Printf("%s\n",MD_GetTextErr(MD_NOT_ENOUGH_MEMORY)); return FALSE;}

    return TRUE;
}


/*****
    Libère ce qui a été alloué par InitMultiDOSFileSystem()
*****/

void FreeMultiDOSFileSystem(struct MultiDOS *MD)
{
    if(MD->FS!=NULL)
    {
        switch(MD->Type)
        {
            case MDOS_DISK:
                FS_FreeFileSystem((struct FileSystem *)MD->FS);
                break;

            case MDOS_K7:
                K7_FreeFileSystem((struct FileSystemK7 *)MD->FS);
                break;
        }
    }
}


/*****
    Fonction pour interpreter les options passees en parametre
*****/

void GetParam(LONG MyPars[], struct ToDiskData *TDData)
{
    /* Gestion des options de conversion de fichier */
    TDData->UnprotectBas=MyPars[ARG_UNPROT]?TRUE:FALSE;
    TDData->ConvertAscii=MyPars[ARG_CONVASC]?TRUE:FALSE;
    TDData->ConvertBasToAscii=MyPars[ARG_CONVBAS]?TRUE:FALSE;
    TDData->ConvertAssToAscii=MyPars[ARG_CONVASS]?TRUE:FALSE;
    TDData->ConvertAsmToAscii=MyPars[ARG_CONVASM]?TRUE:FALSE;
    TDData->ConvertMap=MyPars[ARG_CONVMAP]?TRUE:FALSE;
    TDData->ImportTxtMode=IMPORTTXTMODE_NONE;
    if(MyPars[ARG_IMPORTTXT])
    {
        const char *Param;

        if((Param=Cnv_IsImportTxtMode((const char *)MyPars[ARG_IMPORTTXT],"ass"))!=NULL)
        {
            TDData->ImportTxtMode=IMPORTTXTMODE_ASS;
            Cnv_GetImportTxtParamAss(Param,&TDData->ImportTxtAssLineStart,&TDData->ImportTxtAssLineStep);
        }
        if(Cnv_IsImportTxtMode((const char *)MyPars[ARG_IMPORTTXT],"ascg2")!=NULL) TDData->ImportTxtMode=IMPORTTXTMODE_ASCG2;
    }

    if(MyPars[ARG_CONVALL])
    {
        TDData->ConvertBasToAscii=TRUE;
        TDData->ConvertAssToAscii=TRUE;
        TDData->ConvertAsmToAscii=TRUE;
        TDData->ConvertAscii=TRUE;
        TDData->ConvertMap=TRUE;
        TDData->UnprotectBas=TRUE;
    }

    TDData->MapMode=-1; /* autodetect */
    if(MyPars[ARG_MAPMODE])
    {
        TDData->MapMode=(LONG)*((ULONG *)MyPars[ARG_MAPMODE]);
        if(TDData->MapMode>3) TDData->MapMode=0;
    }

    TDData->MapPalName=NULL;
    if(MyPars[ARG_MAPCFG])
    {
        TDData->MapPalName=(char *)MyPars[ARG_MAPCFG];
    }
}
