#ifndef CMDBACKUP_H
#define CMDBACKUP_H


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern void Cmd_Backup(struct ToDiskData *, struct DiskLayer *, struct DiskLayer *, ULONG);
extern void Cmd_BackupFromK7(struct ToDiskData *, const char *, struct DiskLayer *, ULONG);


#endif  /* CMDBACKUP_H */