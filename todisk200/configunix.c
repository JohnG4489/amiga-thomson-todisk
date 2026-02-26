#include "system.h"

#ifdef SYSTEM_UNIX
#include "configunix.h"
#include <stdlib.h>

/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    23-09-2013 (Seg)    Gestion de la ligne commande façon Unix
*/


/***** Prototypes */
LONG Cfg_ParseArg(int, const char *[], LONG *);
void Cfg_FreeArg(LONG *);

const char *P_Cmd_GetNextArgString(int, const char *[], LONG *, LONG *);
LONG *P_Cmd_GetNextArgInteger(int, const char *[], LONG *, LONG *);
LONG *P_Cmd_AddEntry(LONG *, LONG *, const char *, LONG *);


/*****
    Recherche les paramètres de la ligne de commande
*****/

LONG Cfg_ParseArg(int narg, const char *argv[], LONG *MyPars)
{
    LONG Error=0;

    if(narg>1)
    {
        LONG IdxArg,CountOfFiles=0;

        for(IdxArg=1; IdxArg<narg && Error==0;)
        {
            const char *CurArg=argv[IdxArg++];

            if(Sys_StrCmpNoCase(CurArg,"-dsk")==0 || Sys_StrCmpNoCase(CurArg,"-k7")==0)
                MyPars[ARG_DSK]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-dir")==0)
                MyPars[ARG_DIR]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-load")==0 || Sys_StrCmpNoCase(CurArg,"-copy")==0)
                MyPars[ARG_LOAD]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-save")==0)
                MyPars[ARG_SAVE]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-delete")==0)
                MyPars[ARG_DELETE]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-sortdir")==0)
                MyPars[ARG_SORTDIR]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-cmpdskraw")==0)
                MyPars[ARG_CMPDSKRAW]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-cmpdskdos")==0)
                MyPars[ARG_CMPDSKDOS]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-finddsk")==0)
                MyPars[ARG_FINDDSK]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-norfd")==0)
                MyPars[ARG_NORFD]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-formatflp")==0)
                MyPars[ARG_FORMATFLP]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-formattds")==0)
                MyPars[ARG_FORMATTDS]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-formatsap")==0)
                MyPars[ARG_FORMATSAP]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-formatfd")==0)
                MyPars[ARG_FORMATFD]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-toflp")==0)
                MyPars[ARG_TOFLP]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-totds")==0)
                MyPars[ARG_TOTDS]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-tosap")==0)
                MyPars[ARG_TOSAP]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-tofd")==0)
                MyPars[ARG_TOFD]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-nofmt")==0)
                MyPars[ARG_NOFMT]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-dumpsector")==0)
                MyPars[ARG_DUMPSECTOR]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-view")==0)
                MyPars[ARG_VIEW]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-viewmap")==0)
                MyPars[ARG_VIEWMAP]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-viewand")==0)
                MyPars[ARG_VIEWAND]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-dump")==0 || Sys_StrCmpNoCase(CurArg,"-viewbin")==0)
                MyPars[ARG_DUMP]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-unprot")==0)
                MyPars[ARG_UNPROT]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-convasc")==0)
                MyPars[ARG_CONVASC]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-convbas")==0)
                MyPars[ARG_CONVBAS]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-convass")==0)
                MyPars[ARG_CONVASS]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-convasm")==0)
                MyPars[ARG_CONVASM]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-convmap")==0)
                MyPars[ARG_CONVMAP]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-convall")==0)
                MyPars[ARG_CONVALL]=TRUE;
            else if(Sys_StrCmpNoCase(CurArg,"-importxt")==0)
                MyPars[ARG_IMPORTTXT]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-mapmode")==0)
                MyPars[ARG_MAPMODE]=(LONG)P_Cmd_GetNextArgInteger(narg,argv,&IdxArg,&Error);
            else if(Sys_StrCmpNoCase(CurArg,"-mapcfg")==0)
                MyPars[ARG_MAPCFG]=(LONG)P_Cmd_GetNextArgString(narg,argv,&IdxArg,&Error);
            else
                MyPars[ARG_FILES]=(LONG)P_Cmd_AddEntry((LONG *)MyPars[ARG_FILES],&CountOfFiles,CurArg,&Error);
        }
    }
    else
    {
        Sys_Printf(
            "Utilisation: todisk [OPTION]\n\n"
            "Gestion du système de fichiers\n"
            "  -dsk FICHIER_IMAGE, -k7 FICHIER_IMAGE  disque ou cassette source\n"
            "  -dir [MOTIF]                           affiche le contenu du répertoire de la\n"
            "                                         source (-dsk ou -k7)\n"
            "  -load MOTIF_FICHIER [DESTINATION]      copie un ou plusieurs fichiers de la source\n"
            "                                         thomson vers l'ordinateur hôte\n"
            "  -save MOTIF_FICHIER [DESTINATION]      copie un ou plusieurs fichiers du système hôte\n"
            "                                         vers le disque thomson\n"
            "  -delete MOTIF_FICHIER [DESTINATION]    efface un ou plusieurs fichiers de la source\n"
            "  -sortdir                               trie le répertoire de la source\n\n"
            "Gestion des disquettes images\n"
            "  -cmpdskraw FICHIER_IMAGE               comparaison brute du disque source avec un autre\n"
            "                                         fichier image de n'importe quel type\n"
            "  -cmpdskdos FICHIER_IMAGE               comparaison dos du disque source avec un autre\n"
            "                                         fichier image de n'importe quel type\n"
            "  -finddsk MOTIF_FICHIER_IMAGE           recherche un fichier image identique à la source\n"
            "  -norfd                                 'No Recursive Find' Permet de désactiver la\n"
            "                                         recherche récursive de l'option -finddsk\n\n"
            "Création des disquettes images ou réelles\n"
            "  -formatflp u[0|1]s[0|1]i[n] [NOM]      formate une disquette thomson réelle. Le nom de\n"
            "                                         la disquette est facultatif\n"
            "  -formattds FICHIER_IMAGE [NOM]         formate une disquette virtuelle au format tds.\n"
            "                                         Le nom de la disquette est facultatif\n"
            "  -formatsap FICHIER_IMAGE [NOM]         formate une disquette virtuelle au format sap.\n"
            "                                         Le nom de la disquette est facultatif\n"
            "  -formatfd FICHIER_IMAGE [NOM]          formate une disquette virtuelle au format fd.\n"
            "                                         Le nom de la disquette est facultatif\n"
            "  -toflp u(0|1)[s(0|1)][i(n)]            backup une disquette source sur une vraie\n"
            "                                         disquette placée dans le lecteur\n"
            "  -totds FICHIER_IMAGE                   backup une disquette source pour créer une\n"
            "                                         disquette image au format tds\n"
            "  -tosap FICHIER_IMAGE                   backup une disquette source pour créer une\n"
            "                                         disquette image au format sap\n"
            "  -tofd FICHIER_IMAGE                    backup une disquette source pour créer une\n"
            "                                         disquette image au format fd\n"
            "  -nofmt                                 désactive l'auto formatage des pistes de\n"
            "                                         l'option -toflp\n\n"
            "Options pour visualiser des données\n"
            "  -dumpsector piste[,secteur]            affiche le dump ascii du numéro de piste passé\n"
            "                                         en paramètre. Le numéro de secteur est optionnel\n"
            "  -view MOTIF_FICHIER                    affiche/visualise le contenu d'un ou plusieurs\n"
            "                                         fichiers\n"
            "  -viewmap MOTIF_FICHIER                 visualise les images au format map\n"
            "  -viewand MOTIF_FICHIER                 visualise les fichiers du jeu androides\n"
            "  -dump MOTIF_FICHIER                    dump un ou plusieurs fichiers\n\n"
            "Options pour convertir des données\n"
            "  -unprot                                permet de déprotéger les fichiers lors d'une copie\n"
            "                                         avec les options -save ou -load\n"
            "  -convasc                               permet de convertir en ansi les fichiers texte\n"
            "                                         thomson lors d'une copie avec les options -save ou -load\n"
            "  -convbas                               permet de convertir en texte ansi les fichiers basic\n"
            "                                         lors d'une copie avec les options -save ou -load\n"
            "  -convass                               permet de convertir en texte ansi les fichiers assdesass\n"
            "                                         lors d'une copie avec les options -save ou -load\n"
            "  -convasm                               permet de convertir en texte ansi les fichiers assembleur\n"
            "                                         lors d'une copie avec les options -save ou -load\n"
            "  -convmap                               permet de convertir au format iff les fichiers bitmap\n"
            "                                         thomson lors d'une copie avec les options -save ou -load\n"
            "  -convall                               active en une seule option les options de conversions\n"
            "                                         précédemment décrites\n"
            "  -importtxt (ass[,base[,incr]]|ascg2)   permet de convertir un fichier texte ansi dans le format\n"
            "                                         ascii g2 du thomson (ascg2) ou au format assdesass (ass),\n"
            "                                         avec l'option -save\n"
            "                                         L'option import ass peut prendre 2 arguments facultatifs\n"
            "                                         pour spécifier la numérotation auto du source final\n"
            "  -mapmode (0-3)                         spécifie le mode d'écran d'un fichier bitmap pour les\n"
            "                                         options -convmap et -viewmap (0: standard, 1:80 col,\n"
            "                                         2:bitmap 4 coul, 3:bitmap 16 coul)\n"
            "  -mapcfg fichier[\\fichier]              spécifie un fichier palette à associer au fichier\n"
            "                                         bitmap pour les options -convmap et -viewmap (le\n"
            "                                         caratère '\\' devant le nom de fichier permet de cibler\n"
            "                                         un fichier palette sur la machine hôte)\n"
        );
        Error=-1;
    }

    return Error;
}


/*****
    Libération des ressources allouées
*****/

void Cfg_FreeArg(LONG *MyPars)
{
    Sys_FreeMem((void *)MyPars[ARG_MAPMODE]);
    Sys_FreeMem((void *)MyPars[ARG_FILES]);
}


/*****
    Récupère l'argument chaine de caractère courant et contrôle si erreur
*****/

const char *P_Cmd_GetNextArgString(int narg, const char *argv[], LONG *Idx, LONG *Error)
{
    if(*Idx<narg) return argv[(*Idx)++];
    *Error=ERROR_REQUIRED_ARG_MISSING;
    return NULL;
}


/*****
    Récupère l'argument numérique courant et contrôle si erreur
*****/

LONG *P_Cmd_GetNextArgInteger(int narg, const char *argv[], LONG *Idx, LONG *Error)
{
    const char *Arg=P_Cmd_GetNextArgString(narg,argv,Idx,Error);

    if(Arg!=NULL)
    {
        LONG val=(LONG)atoi(Arg),*Ptr;
        if((Ptr=(LONG *)Sys_AllocMem(sizeof(val)))!=NULL) {*Ptr=val; return Ptr;}
        *Error=ERROR_NO_FREE_STORE;
    }

    return NULL;
}


/*****
    Ajoute une entrée LONG dans la table "Table"
*****/

LONG *P_Cmd_AddEntry(LONG *Table, LONG *Count, const char *Entry, LONG *Error)
{
    LONG *NewTable=(LONG *)Sys_AllocMem(sizeof(LONG)*(*Count+2));

    if(NewTable!=NULL)
    {
        LONG i;
        for(i=0; i<*Count; i++) NewTable[i]=Table[i];
        NewTable[(*Count)++]=(LONG)Entry;
        NewTable[*Count]=0;
    }
    else
    {
        *Error=ERROR_NO_FREE_STORE;
    }

    /* Suppression de la table précédente */
    Sys_FreeMem((void *)Table);

    return NewTable;
}

#endif