#include "system.h"
#include "global.h"
#include "cmddumpsector.h"
#include "disklayer.h"


/*
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    16-08-2012 (Seg)    Possibilité de lister plusieurs secteurs
    08-08-2012 (Seg)    Dump d'un secteur
*/


/***** Prototypes */
void Cmd_DumpSector(struct ToDiskData *, struct DiskLayer *, LONG, LONG, LONG);
void Cmd_ParseDumpSectorParam(char *, LONG *, LONG *, LONG *);


/*****
    Affiche le contenu binaire d'un secteur
*****/

void Cmd_DumpSector(struct ToDiskData *TDData, struct DiskLayer *DLayer, LONG Track, LONG Sector, LONG Count)
{
    UBYTE *Buffer=TDData->Buffer;
    LONG Offset=0;

    Sys_Printf("Dump sectors\nFirst Track: %02ld, First Sector: %02ld, Count of sectors: %ld\n",(long)Track,(long)Sector,(long)Count);

    while(Track<COUNTOF_TRACK && --Count>=0)
    {
        if(DL_ReadSector(DLayer,Track,Sector,Buffer))
        {
            LONG Size=SIZEOF_SECTOR,Len=16,i;

            while(Size>0)
            {
                Sys_Printf("%06lx  ",(long)Offset);

                for(i=0; i<Len; i++)
                {
                    if(i<Size) Sys_Printf("%02lx ",(long)Buffer[i]);
                    else Sys_Printf("   ");
                }

                Sys_Printf(" ");

                for(i=0; i<(Size<Len?Size:Len); i++)
                {
                    char Char=(char)Buffer[i];
                    Sys_Printf("%lc",(long)(Char>32 && Char<127?Char:'.'));
                }

                Sys_Printf("\n");
                Offset+=Len;
                Size-=Len;
                Buffer+=Len;
            }
        }
        else
        {
            Sys_Printf("Error when reading Track %ld, Sector %ld\n");
        }

        Sector++;
        if(Sector>COUNTOF_SECTOR_PER_TRACK) {Track++; Sector=1;}
    }
}


/*****
    Fonction pour decoder le parametre Track,Sector de la ligne de commande
*****/

void Cmd_ParseDumpSectorParam(char *Param, LONG *Track, LONG *Sector, LONG *Count)
{
    *Track=0;
    *Sector=0;
    *Count=0;

    if(Param!=NULL)
    {
        while(*Param!=0 && *Param!=',' && *Param>='0' && *Param<='9')
        {
            *Track=*Track*10 + (LONG)(*Param-'0');
            Param++;
        }
        if(*Param!=0)
        {
            Param++;
            while(*Param!=0 && *Param!=',' && *Param>='0' && *Param<='9')
            {
                *Sector=*Sector*10 + (LONG)(*Param-'0');
                Param++;
            }
        }
        if(*Param!=0)
        {
            Param++;
            while(*Param!=0 && *Param!=',' && *Param>='0' && *Param<='9')
            {
                *Count=*Count*10 + (LONG)(*Param-'0');
                Param++;
            }
        }
    }

    if(*Track>=COUNTOF_TRACK) *Track=0;
    if(*Sector<=0 && *Count<=0) {*Sector=1; *Count=COUNTOF_SECTOR_PER_TRACK;}
    if(*Sector<=0 || *Sector>COUNTOF_SECTOR_PER_TRACK) *Sector=1;
    else if(*Count<=0) *Count=1;
}
