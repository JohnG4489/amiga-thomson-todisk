#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef __SASC
#define SYSTEM_AMIGA 1
#endif
#ifndef __SASC
#define SYSTEM_UNIX 1
#endif

#ifdef SYSTEM_AMIGA
#include "systemamiga.h"
#endif

#ifdef SYSTEM_UNIX
#include "systemunix.h"
#endif

/***** VARIABLES ET FONCTIONS    *****/
/***** PUBLIQUES UTILISABLES PAR *****/
/***** D'AUTRES BLOCS DU PROJET  *****/

extern UBYTE *Sys_AllocFile(const char *, LONG *);
extern void Sys_FreeFile(UBYTE *);

extern void Sys_MemCopy(void *, void *, LONG);
extern void Sys_StrCopy(char *, const char *, LONG);
extern LONG Sys_StrLen(const char *);
extern LONG Sys_StrCmp(const char *, const char *);
extern LONG Sys_StrCmpNoCase(const char *, const char *);

extern LONG Sys_GetTime(LONG *, LONG *, LONG *, LONG *, LONG *, LONG *);

#define Sys_CharToLower(Char) ToLower(Char)

#endif  /* SYSTEM_H */
