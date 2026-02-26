#include "system.h"

/*
    28-08-2018 (Seg)    Fix sur les fonctions de comparaison de chaînes
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    19-05-2004 (Seg)    Ajout des fonctions Sys_MemCopy() et Sys_StrCopy()
    03-09-2000 (Seg)    Utilisation des fonctions systeme du 3dengine
*/


UBYTE *Sys_AllocFile(const char *, LONG *);
void Sys_FreeFile(UBYTE *);

void Sys_MemCopy(void *, void *, LONG);
void Sys_StrCopy(char *, const char *, LONG);
LONG Sys_StrLen(const char *);
LONG Sys_StrCmp(const char *, const char *);
LONG Sys_StrCmpNoCase(const char *, const char *);

LONG Sys_GetTime(LONG *, LONG *, LONG *, LONG *, LONG *, LONG *);

/*****
    Chargement d'un fichier dans un buffer.
    - Name: nom du fichier
    - FileSize: adresse d'un LONG ou stocker la taille du fichier (peut être NULL)
      Si FileSize est NULL ou si le LONG situé à cette adresse est égal à 0, alors
      le fichier sera chargé tout entier et FileSize sera initialisé à la taille
      du fichier si FileSize est non NULL.
      Si à l'initialisation, la taille de FileSize est égale à n (n>0), alors
      AllocFile() tentera de charger les n premiers octets du fichier.
*****/

UBYTE *Sys_AllocFile(const char *Name, LONG *FileSize)
{
    HPTR h;
    UBYTE *Ptr=NULL;

    if((h=Sys_Open(Name,MODE_OLDFILE))!=0)
    {
        LONG Size;
        Sys_Seek(h,0,OFFSET_END);
        Size=Sys_Seek(h,0,OFFSET_BEGINNING);
        if(FileSize!=NULL)
        {
            if(*FileSize==0L) *FileSize=Size; else Size=*FileSize;
        }
        if((Ptr=(UBYTE *)Sys_AllocMem((ULONG)Size))!=NULL) Sys_Read(h,Ptr,Size);
        Sys_Close(h);
    }
    return Ptr;
}


/*****
    Libération du buffer alloué par AllocFile().
*****/

void Sys_FreeFile(UBYTE *Ptr)
{
    Sys_FreeMem((void *)Ptr); /* NULL Autorisé */
}


/*****
    Equivalent de memcpy()
*****/

void Sys_MemCopy(void *Dst, void *Src, LONG Size)
{
    UBYTE *pdst=(UBYTE *)Dst, *psrc=(UBYTE *)Src;
    while(--Size>=0) *(pdst++)=*(psrc++);
}


/*****
    Equivalent de strcpy()
*****/

void Sys_StrCopy(char *Dst, const char *Src, LONG SizeOfDst)
{
    while(--SizeOfDst>0 && *Src!=0) *(Dst++)=*(Src++);
    *Dst=0;
}


/*****
    Permet d'obtenir le nombre de caractères du chaîne.
*****/

LONG Sys_StrLen(const char *String)
{
    LONG Count=0;

    while(String[Count]!=0) Count++;

    return Count;
}


/*****
    Comparaison de deux chaînes
*****/

LONG Sys_StrCmp(const char *Str1, const char *Str2)
{
    LONG c1,c2;

    do
    {
        c1=(LONG)(UBYTE)*(Str1++);
        c2=(LONG)(UBYTE)*(Str2++);
    } while(c1==c2 && c1!=0 && c2!=0);

    return c1-c2;
}


/*****
    Comparaison de deux chaînes
*****/

LONG Sys_StrCmpNoCase(const char *Str1, const char *Str2)
{
    LONG c1,c2;

    do
    {
        c1=(LONG)(UBYTE)*(Str1++);
        c2=(LONG)(UBYTE)*(Str2++);
        if(c1>='A' && c1<='Z') c1|=32;
        if(c2>='A' && c2<='Z') c2|=32;
    } while(c1==c2 && c1!=0 && c2!=0);

    return c1-c2;
}


/*****
    Permet d'obtenir l'heure courante
*****/

LONG Sys_GetTime(LONG *Year, LONG *Month, LONG *Day, LONG *Hour, LONG *Min, LONG *Sec)
{
    struct ClockData cd;
    struct DateStamp ds;
    LONG Second;

    DateStamp(&ds);
    Second=ds.ds_Days*24*60*60+ds.ds_Minute*60+ds.ds_Tick/TICKS_PER_SECOND;

    Amiga2Date(Second,&cd);

    *Year=(LONG)cd.year;
    *Month=(LONG)cd.month;
    *Day=(LONG)cd.mday;
    *Hour=(LONG)cd.hour;
    *Min=(LONG)cd.min;
    *Sec=(LONG)cd.sec;

    return Second;
}
