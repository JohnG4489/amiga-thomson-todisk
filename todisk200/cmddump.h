#ifndef CMDDUMP_H
#define CMDDUMP_H


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern void Cmd_DumpPattern(struct ToDiskData *, struct MultiDOS *, const char *);
extern LONG Cmd_Dump(struct ToDiskData *, struct MultiDOS *, const char *);


#endif  /* CMDDUMP_H */