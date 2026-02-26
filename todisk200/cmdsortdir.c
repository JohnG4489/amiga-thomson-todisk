#include "system.h"
#include "disklayer.h"
#include "global.h"
#include "cmdsortdir.h"
#include "util.h"


/*
    16-09-2020 (Seg)    Modifications suite refonte filesystem
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    22-03-2010 (Seg)    Tri du repertoire
*/


/***** Prototypes */
void Cmd_SortDir(struct ToDiskData *, struct DiskLayer *);

BOOL P_Cmd_CompareFileInfo(UBYTE *, UBYTE *, LONG);


/*****
    Tri du repertoire
*****/

void Cmd_SortDir(struct ToDiskData *TDData, struct DiskLayer *DLayer)
{
    LONG Method=0,Sector;

    Sys_Printf("Sort directory:\n");
    while(Method<=0)
    {
        Sys_Printf("Method (A)scending or (D)escending? ");
        Sys_FlushOutput();

        Method=0;
        while(Method==0)
        {
            if(Sys_WaitForChar(100000))
            {
                switch(Sys_GetChar())
                {
                    case 'a':
                    case 'A':
                        Method=1;
                        break;

                    case 'd':
                    case 'D':
                        Method=2;
                        break;

                    default:
                        Method=-1;
                        break;
                }
            }

            if(Sys_CheckCtrlC())
            {
                Sys_PrintFault(ERROR_BREAK);
                return;
            }
        }
    }

    Sys_FlushInput();

    if(Method!=0)
    {
        UBYTE Tmp[SIZEOF_FILEINFO];
        LONG Idx1,Idx2,i;

        /* Lecture du repertoire */
        Sys_Printf("Reading directory...\n");
        for(Sector=3;Sector<=COUNTOF_SECTOR_PER_TRACK;Sector++)
        {
            if(!DL_ReadSector(DLayer,20,Sector,&TDData->Buffer[(Sector-3)*SIZEOF_SECTOR]))
            {
                ULONG DLError=DLayer->Error;

                if(DL_IsDLFatalError(DLError)) return;
                if(DLError) Sys_Printf("Track 20, Sector %ld: %s\n",(long)Sector,DL_GetDLTextErr(DLError));
            }
        }

        /* Tri */
        for(Idx1=0;Idx1<112;Idx1++)
        {
            UBYTE *Ptr1=&TDData->Buffer[Idx1*SIZEOF_FILEINFO];

            Sys_Printf("\rSort... %ld%%",Idx1*100/111);

            for(Idx2=Idx1+1;Idx2<112;Idx2++)
            {
                UBYTE *Ptr2=&TDData->Buffer[Idx2*SIZEOF_FILEINFO];

                if(P_Cmd_CompareFileInfo(Ptr1,Ptr2,Method))
                {
                    for(i=0;i<sizeof(Tmp);i++) Tmp[i]=Ptr1[i];
                    for(i=0;i<sizeof(Tmp);i++) Ptr1[i]=Ptr2[i];
                    for(i=0;i<sizeof(Tmp);i++) Ptr2[i]=Tmp[i];
                }
            }
        }

        /* Ecriture du repertoire */
        Sys_Printf("\nWriting directory...\n");
        for(Sector=3;Sector<=COUNTOF_SECTOR_PER_TRACK;Sector++)
        {
            if(!DL_WriteSector(DLayer,20,Sector,&TDData->Buffer[(Sector-3)*SIZEOF_SECTOR]))
            {
                ULONG DLError=DLayer->Error;

                if(DL_IsDLFatalError(DLError)) return;
                if(DLError) Sys_Printf("Track 20, Sector %ld: %s\n",(long)Sector,DL_GetDLTextErr(DLError));
            }
        }

        /* Fin */
        Sys_Printf("Finished!\n");
    }
}


/*****
    Comparaison de 2 FileInfo
*****/

BOOL P_Cmd_CompareFileInfo(UBYTE *Ptr1, UBYTE *Ptr2, LONG Method)
{
    if(Ptr1[0]==0 && Ptr2[0]==0xff) return TRUE;
    else if((Ptr1[0]==0 || Ptr1[0]==0xff) && Ptr2[0]!=0 && Ptr2[0]!=0xff) return TRUE;
    else if(Ptr1[0]!=0 && Ptr1[0]!=0xff && Ptr2[0]!=0 && Ptr2[0]!=0xff)
    {
        LONG i;
        for(i=0;i<13;i++)
        {
            if(Ptr1[i]>Ptr2[i]) return (BOOL)(Method==1?TRUE:FALSE);
            else if(Ptr1[i]<Ptr2[i]) return (BOOL)(Method==1?FALSE:TRUE);
        }
    }

    return FALSE;
}
