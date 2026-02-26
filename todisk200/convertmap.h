#ifndef CONVERTMAP_H
#define CONVERTMAP_H


#define TM_TYPE_MAP     0
#define TM_TYPE_TOSNAP  1
#define TM_TYPE_PPM     2


struct ToMAP
{
    LONG    Type;
    LONG    Width,Height;
    LONG    BytesPerRow;
    LONG    MAPWidth,MAPHeight;
    LONG    Mode;
    UBYTE   *Buffer1;
    UBYTE   *Buffer2;
    UBYTE   Palette[16*3];
};


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern BOOL CM_ConvertMAP(LONG, char *, struct MDHandle *, struct MDHandle *);
extern BOOL CM_LoadImageMAP(LONG, char *, struct MDHandle *, struct ToMAP *);
extern void CM_UnloadImageMAP(struct ToMAP *);
extern void CM_ConvertLine(struct ToMAP *, UBYTE *, LONG);
extern LONG CM_GetDepthFromMode(LONG);

#endif  /* CONVERTMAP_H */
