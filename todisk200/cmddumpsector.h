#ifndef CMDDUMPSECTOR_H
#define CMDDUMPSECTOR_H


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern void Cmd_DumpSector(struct ToDiskData *, struct DiskLayer *, LONG, LONG, LONG);
extern void Cmd_ParseDumpSectorParam(char *, LONG *, LONG *, LONG *);


#endif  /* CMDDUMPSECTOR_H */