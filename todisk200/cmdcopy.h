#ifndef CMDCOPY_H
#define CMDCOPY_H


#define COPY_MODE_QUERY                 0
#define COPY_MODE_ERASE_ALL             1
#define COPY_MODE_RENAME_ALL            2


/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern void Cmd_Copy(struct ToDiskData *, struct MultiDOS *, struct MultiDOS *, const char *, const char *);
extern BOOL Cmd_CopyFile(struct ToDiskData *, ULONG *, struct MultiDOS *, const char *, const char *, LONG *, LONG, LONG, LONG, LONG, LONG, LONG, const char *, LONG, LONG, struct MultiDOS *, const char *, const char *);


#endif  /* CMDCOPY_H */