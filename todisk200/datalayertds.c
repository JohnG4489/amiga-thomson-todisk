#include "system.h"
#include "datalayertds.h"
#include "sectorcache.h"
#include "disklayer.h"

/* Type de cluster */
#define CLST_FREE               0xff
#define CLST_RESERVED           0xfe
#define CLST_TERM               0xc0

/*
    16-09-2020 (Seg)    Refonte, suite à la refonte de la lib filesystem.c
    23-09-2013 (Seg)    Adaptation du code suite à la migration sous Unix
    24-03-2010 (Seg)    Ajout de la detection automatique du format
    10-03-2010 (Seg)    Finalisation du format: lecture et ecriture
    08-03-2010 (Seg)    Gestion du format TDS
*/


/***** Prototypes */
BOOL DTds_CheckType(const char *, ULONG *);
struct DataLayerTDS *DTds_Open(const char *, ULONG, ULONG, ULONG, ULONG *);
void DTds_Close(struct DataLayerTDS *);
ULONG DTds_Finalize(struct DataLayerTDS *);
ULONG DTds_FormatTrack(struct DataLayerTDS *, ULONG, const UBYTE *);
ULONG DTds_ReadSector(struct DataLayerTDS *, ULONG, ULONG, UBYTE *);
ULONG DTds_WriteSector(struct DataLayerTDS *, ULONG, ULONG, const UBYTE *);

struct SectorCacheNode *P_DTds_ObtainSector(struct DataLayerTDS *, LONG, LONG, BOOL);
ULONG P_DTds_GetOffset(struct DataLayerTDS *, ULONG, ULONG);


/*****
    Indique s'il s'agit d'un fichier image TDS
*****/

BOOL DTds_CheckType(const char *Name, ULONG *ErrorCode)
{
    BOOL Result=FALSE;
    HPTR h;

    *ErrorCode=DL_OPEN_FILE;
    if((h=Sys_Open(Name,MODE_OLDFILE))!=NULL)
    {
        UBYTE Tmp[160];

        /* TDS est un format DOS. On commence donc par récupérer la bitmap */
        *ErrorCode=DL_READ_FILE;
        if(Sys_Read(h,Tmp,sizeof(Tmp))==sizeof(Tmp))
        {
            LONG i,j,Size,Count=0;

            Sys_Seek(h,0,OFFSET_END);
            Size=Sys_Seek(h,0,OFFSET_BEGINNING);

            /* On regarde combien on est censé avoir de secteur dans le fichier image */
            for(i=0; i<sizeof(Tmp); i++)
            {
                UBYTE Val=Tmp[i];
                for(j=0; j<8; j++) if(Val&(1<<j)) Count++;
            }

            /* On regarde si la taille correspond à ce qu'on peut déduire de la bitmap */
            if(Size==sizeof(Tmp)+Count*SIZEOF_SECTOR) Result=TRUE;

            *ErrorCode=DL_SUCCESS;
        }

        Sys_Close(h);
    }

    return Result;
}


/*****
    Allocation des ressources necessaires pour la lecture/ecriture
    d'un fichier au format TDS.
*****/

struct DataLayerTDS *DTds_Open(const char *Name, ULONG IOMode, ULONG Unit, ULONG Side, ULONG *ErrorCode)
{
    struct DataLayerTDS *DLayer=(struct DataLayerTDS *)Sys_AllocMem(sizeof(struct DataLayerTDS));

    *ErrorCode=DL_NOT_ENOUGH_MEMORY;
    if(DLayer!=NULL)
    {
        /* Initialisation du cache d'écriture interne à la lib */
        Sch_Init(&DLayer->SectorCache,SIZEOF_SECTOR);

        DLayer->Unit=Unit;
        DLayer->Side=Side;

        *ErrorCode=DL_OPEN_FILE;
        if((DLayer->Handle=Sys_Open(Name,IOMode))!=NULL)
        {
            /* Lecture de la bitmap */
            if(Sys_Read(DLayer->Handle,DLayer->Bitmap,sizeof(DLayer->Bitmap))==sizeof(DLayer->Bitmap))
            {
                LONG IdxCluster,IdxSector,Offset=sizeof(DLayer->Bitmap);

                for(IdxCluster=0; IdxCluster<sizeof(DLayer->Bitmap); IdxCluster++)
                {
                    ULONG b=(ULONG)DLayer->Bitmap[IdxCluster];

                    for(IdxSector=0; IdxSector<8; IdxSector++)
                    {
                        LONG Idx=IdxCluster*8+IdxSector;

                        DLayer->SectorOffset[Idx]=0;
                        if((b&1)!=0) {DLayer->SectorOffset[Idx]=Offset; Offset+=SIZEOF_SECTOR;}
                        b>>=1;
                    }
                }
            }

            *ErrorCode=DL_SUCCESS;
        }
    }

    if(*ErrorCode)
    {
        DTds_Close(DLayer);
        DLayer=NULL;
    }

    return DLayer;
}


/*****
    Liberation des ressources allouees par DTds_Close()
*****/

void DTds_Close(struct DataLayerTDS *DLayer)
{
    if(DLayer!=NULL)
    {
        if(DLayer->Handle!=NULL)
        {
            /*Flush(DLayer->Handle);*/
            Sys_Close(DLayer->Handle);
        }

        Sch_Flush(&DLayer->SectorCache);
        Sys_FreeMem((void *)DLayer);
    }
}


/*****
    Permet de terminer les operations en cache, avant de fermer
    le DataLayer.
*****/

ULONG DTds_Finalize(struct DataLayerTDS *DLayer)
{
    ULONG ErrorCode=DL_SUCCESS;

    if(DLayer->IOMode!=MODE_OLDFILE)
    {
        /* Récupération de la FAT */
        struct SectorCacheNode *NodeFATPtr=P_DTds_ObtainSector(DLayer,20,2,TRUE);

        if(NodeFATPtr!=NULL)
        {
            LONG IdxCluster;

            /* On va scanner chaque cluster pour connaître les secteurs que l'on retient dans le fichier.
               Pour cela, on utilise le système de cache de cette lib DTds_XXX.
               Les secteurs en cache qui ne sont plus utilisés, sont retirés du cache.
               Les secteurs utilisés qui ne sont pas encore en cache, y sont ajoutés.
               Pour les secteurs utilisés qui sont déjà en cache, on ne fait rien.
               A la fin, tous les secteurs utilisés, et uniquement ceux-là, seront en cache.
            */
            for(IdxCluster=0; !ErrorCode && IdxCluster<sizeof(DLayer->Bitmap); IdxCluster++)
            {
                ULONG Cluster=(ULONG)NodeFATPtr->BufferPtr[IdxCluster+1];
                LONG Count=Cluster!=CLST_FREE?(Cluster<CLST_TERM || Cluster==CLST_RESERVED?8:Cluster-CLST_TERM):0;
                LONG Track=IdxCluster/2;
                LONG Sector=8*(IdxCluster&1)+1;
                LONG Offset=sizeof(DLayer->Bitmap);
                LONG IdxSector;

                /* On scanne chaque secteur du cluster pour savoir si on doit l'intégrer au fichier image */
                for(IdxSector=0; !ErrorCode && IdxSector<8; IdxSector++)
                {
                    LONG CurSector=Sector+IdxSector;
                    LONG IdxBit=Track*COUNTOF_SECTOR_PER_TRACK+CurSector-1;
                    LONG BmIdx=IdxBit/8;
                    LONG BmBit=1<<(IdxBit%8);
                    struct SectorCacheNode *NodePtr;

                    /* Teste si le secteur doit être intégré dans le fichier image */
                    if(IdxSector<Count)
                    {
                        NodePtr=P_DTds_ObtainSector(DLayer,Track,CurSector,TRUE);
                        if(NodePtr==NULL) ErrorCode=DL_NOT_ENOUGH_MEMORY;
                        DLayer->Bitmap[BmIdx]|=BmBit;
                        DLayer->SectorOffset[IdxBit]=Offset;
                        Offset+=SIZEOF_SECTOR;
                    }
                    /* Si le secteur n'est pas utilisé, on le retire du cache si jamais il existe */
                    else
                    {
                        NodePtr=Sch_Find(&DLayer->SectorCache,Track,CurSector);
                        Sch_FreeNode(&DLayer->SectorCache,NodePtr); /* Safe si NodePtr=NULL */
                        DLayer->Bitmap[BmIdx]&=~BmBit;
                        DLayer->SectorOffset[IdxBit]=0;
                    }
                }
            }

            /* Maintenant qu'on a la bitmap correctement initialisée, et le cache correctement
               plein avec tous les secteurs utilisés, on va commencer à construire le fichier image.
            */
            if(!ErrorCode)
            {
                if(Sys_Seek(DLayer->Handle,0,OFFSET_BEGINNING)>=0)
                {
                    LONG Offset=sizeof(DLayer->Bitmap);
                    struct SectorCacheNode *NodePtr;

                    /* Ecriture de la bitmap */
                    Sys_Write(DLayer->Handle,DLayer->Bitmap,sizeof(DLayer->Bitmap));

                    while((NodePtr=Sch_GetMinSectorCacheNode(&DLayer->SectorCache,FALSE))!=NULL)
                    {
                        Sys_Write(DLayer->Handle,NodePtr->BufferPtr,SIZEOF_SECTOR);
                        Offset+=SIZEOF_SECTOR;
                        Sch_FreeNode(&DLayer->SectorCache,NodePtr);
                    }

                    Sys_SetFileSize(DLayer->Handle,Offset);
                }
            }
        }
    }

    return ErrorCode;
}


/*****
    Formatage d'une piste
*****/

ULONG DTds_FormatTrack(struct DataLayerTDS *DLayer, ULONG Track, const UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_SUCCESS;
    LONG Sector;

    for(Sector=1; !ErrorCode && Sector<=COUNTOF_SECTOR_PER_TRACK; Sector++)
    {
        ErrorCode=DTds_WriteSector(DLayer,Track,Sector,&BufferPtr[(Sector-1)*SIZEOF_SECTOR]);
    }

    return ErrorCode;
}


/*****
    Lecture d'un secteur
*****/

ULONG DTds_ReadSector(struct DataLayerTDS *DLayer, ULONG Track, ULONG Sector, UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_READ_FILE;
    struct SectorCacheNode *NodePtr=Sch_Find(&DLayer->SectorCache,Track,Sector);
    LONG Count=0;

    if(NodePtr!=NULL)
    {
        /* Si le secteur existe dans le cache, on recopie ses données */
        while(Count<SIZEOF_SECTOR) *(BufferPtr++)=NodePtr->BufferPtr[Count++];
        ErrorCode=DL_SUCCESS;
    }
    else
    {
        /* Si le secteur n'existe pas dans le cahe interne, alors on tente de le lire directement */
        ULONG Offset=P_DTds_GetOffset(DLayer,Track,Sector);

        if(Offset>0)
        {
            if(Sys_Seek(DLayer->Handle,Offset,OFFSET_BEGINNING)>=0)
            {
                Count=Sys_Read(DLayer->Handle,BufferPtr,SIZEOF_SECTOR);
            }
        }

        if(Count>=0 && Sys_IoErr()<=0)
        {
            /* On complete le secteur avec des zeros si le fichier est incomplet */
            while(Count<SIZEOF_SECTOR) BufferPtr[Count++]=0;
            ErrorCode=DL_SUCCESS;
        }
    }


    return ErrorCode;
}


/*****
    Ecriture d'un secteur
*****/

ULONG DTds_WriteSector(struct DataLayerTDS *DLayer, ULONG Track, ULONG Sector, const UBYTE *BufferPtr)
{
    ULONG ErrorCode=DL_NOT_ENOUGH_MEMORY;
    struct SectorCacheNode *NodePtr=P_DTds_ObtainSector(DLayer,Track,Sector,FALSE);

    if(NodePtr!=NULL)
    {
        LONG Idx=0;
        while(Idx<SIZEOF_SECTOR) NodePtr->BufferPtr[Idx++]=*(BufferPtr++);
        NodePtr->Status=SCN_UPDATED;
        ErrorCode=DL_SUCCESS;
    }

    return ErrorCode;
}


/*****
    Récupération d'un secteur dans le cache
*****/

struct SectorCacheNode *P_DTds_ObtainSector(struct DataLayerTDS *DLayer, LONG Track, LONG Sector, BOOL IsLoad)
{
    struct SectorCacheNode *NodePtr=Sch_Obtain(&DLayer->SectorCache,Track,Sector,TRUE);

    if(NodePtr!=NULL && NodePtr->Status==SCN_NEW && IsLoad)
    {
        ULONG Offset=P_DTds_GetOffset(DLayer,Track,Sector);
        Sys_Seek(DLayer->Handle,Offset,OFFSET_BEGINNING);
        Sys_Read(DLayer->Handle,NodePtr->BufferPtr,SIZEOF_SECTOR);
        NodePtr->Status=SCN_INITIALIZED;
    }

    return NodePtr;
}


/*****
    Retourne l'offset dans le fichier correspondant a la piste et au secteur donne
*****/

ULONG P_DTds_GetOffset(struct DataLayerTDS *DLayer, ULONG Track, ULONG Sector)
{
    return DLayer->SectorOffset[Track*COUNTOF_SECTOR_PER_TRACK+Sector-1];
}