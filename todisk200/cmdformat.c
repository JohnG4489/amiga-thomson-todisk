#include "system.h"
#include "disklayer.h"
#include "global.h"
#include "cmdformat.h"
#include "convert.h"


/*
    27-08-2018 (Seg)    Remaniement suite gestion des noms avec accents
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    31-08-2006 (Seg)    Formatage du disque
*/


/***** Prototypes */
BOOL Cmd_Format(struct ToDiskData *, struct DiskLayer *, const char *, ULONG);


/*****
    Formatage d'un disque
*****/

BOOL Cmd_Format(struct ToDiskData *TDData, struct DiskLayer *DLayer, const char *VolumeName, ULONG Interleave)
{
    BOOL IsExit=FALSE;
    LONG i,Track;

    Sys_Printf("Starting format:\n");

    for(Track=0;Track<DLayer->TrackPerUnit && !IsExit;Track++)
    {
        Sys_Printf("Format track %ld/%ld...\r",Track,DLayer->TrackPerUnit-1);
        Sys_FlushOutput();

        if(!Sys_CheckCtrlC())
        {
            if(Track==20)
            {
                UBYTE *Ptr=TDData->Buffer;

                /* Initialisation de la piste */
                for(i=0;i<SIZEOF_TRACK;i++) Ptr[i]=0xff;
                Cnv_ConvertHostLabelToThomsonLabel(VolumeName,Ptr);

                /* Initialisation de la FAT */
                Ptr=&TDData->Buffer[1*SIZEOF_SECTOR];
                for(i=0;i<SIZEOF_SECTOR;i++) Ptr[i]=0;
                for(i=1;i<=160 && i<SIZEOF_SECTOR;i++) Ptr[i]=0xff;
                Ptr[0x29]=0xfe;
                Ptr[0x2a]=0xfe;
            } else for(i=0;i<SIZEOF_TRACK;i++) TDData->Buffer[i]=0xe5;

            if(!DL_FormatTrack(DLayer,Track,Interleave,TDData->Buffer))
            {
                if(DL_IsDLFatalError(DLayer->Error))
                {
                    Sys_Printf("\nError: %s\n",DL_GetDLTextErr(DLayer->Error));
                    IsExit=TRUE;
                }
                else
                {
                    Sys_Printf("Error writing track %ld: %s\n",Track,DL_GetDLTextErr(DLayer->Error));
                }
            }
        }
        else
        {
            Sys_Printf("\n");
            Sys_PrintFault(ERROR_BREAK);
            IsExit=TRUE;
        }
    }

    if(!IsExit) {Sys_Printf("\n"); return TRUE;}

    return FALSE;
}
