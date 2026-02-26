#ifndef GLOBAL_H
#define GLOBAL_H


#include "multidos.h"
#include "diskgeometry.h"


#define IMPORTTXTMODE_NONE  0
#define IMPORTTXTMODE_ASS   1
#define IMPORTTXTMODE_ASCG2 2

struct ToDiskData
{
    UBYTE Buffer[SIZEOF_TRACK];
    UBYTE ConvertBuffer[512];
    struct MDFileObject FObject;
    BOOL ConvertBasToAscii;
    BOOL ConvertAssToAscii;
    BOOL ConvertAsmToAscii;
    BOOL ConvertAscii;
    BOOL ConvertMap;
    BOOL UnprotectBas;
    LONG ImportTxtMode;
    LONG ImportTxtAssLineStart;
    LONG ImportTxtAssLineStep;
    LONG MapMode;
    char *MapPalName;
};


#endif  /* GLOBAL_H */
