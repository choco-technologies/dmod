#ifndef INC_DMOD_TYPES_H_
#define INC_DMOD_TYPES_H_

#include <stdint.h>
#include "dmod_defs.h"

typedef struct 
{
    uint32_t       Size;
    uint32_t       Version;
    uint32_t       Flags;
    char           Data[0];
} Dmod_Config_t;

typedef struct 
{
    uint32_t        HeaderSize;
    uint32_t        Version;
    char            Name[DMOD_MAX_MODULE_NAME_LENGTH];
} Dmod_ModuleHeader_t;

typedef struct 
{
    uint32_t       SectionStart;
    uint32_t       SectionSize;
} Dmod_ModuleSection_t;

typedef struct
{
    Dmod_ModuleSection_t    Header;
    Dmod_ModuleSection_t    Input;
    Dmod_ModuleSection_t    Output;
    Dmod_ModuleSection_t    Text;
    Dmod_ModuleSection_t    Data;
    Dmod_ModuleSection_t    Bss;
    Dmod_ModuleSection_t    RoData;
    Dmod_ModuleSection_t    Bin;
} Dmod_ModuleFooter_t;

#endif /* INC_DMOD_TYPES_H_ */
