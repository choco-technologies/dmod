#ifndef INC_DMOD_TYPES_H_
#define INC_DMOD_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "dmod_defs.h"

typedef enum 
{
    Dmod_ApiType_Input,
    Dmod_ApiType_Output,

    Dmod_ApiType_Count
} Dmod_ApiType_t;

typedef enum 
{
    Dmod_ModuleType_Unknown,
    Dmod_ModuleType_Module, 
    Dmod_ModuleType_Application, 

    Dmod_ModuleType_Count,
} Dmod_ModuleType_t;

typedef struct 
{
    uint32_t       Size;
    uint32_t       Version;
    uint32_t       Flags;
    char           Data[0];
} Dmod_Config_t;

typedef int (*Dmod_Init_t)(Dmod_Config_t *Config);
typedef int (*Dmod_Main_t)(int argc, char *argv[]);
typedef int (*Dmod_Deinit_t)(void);
typedef int (*Dmod_Signal_t)( int SignalNumber );

typedef struct 
{
    uint32_t            Signature;  // DMOD
    uint32_t            HeaderSize;
    uint32_t            Version;
    char                Arch[DMOD_MAX_ARCH_NAME_LENGTH];
    char                Name[DMOD_MAX_MODULE_NAME_LENGTH];
    Dmod_Init_t         Init;
    Dmod_Main_t         Main;
    Dmod_Deinit_t       Deinit;
    Dmod_Signal_t       Signal;
    uint64_t            RequiredStackSize;
    uint32_t            Priority;
    uint8_t             ModuleType;
} Dmod_ModuleHeader_t;

typedef struct 
{
    uint32_t       SectionStart;
    uint32_t       SectionSize;
} Dmod_ModuleSection_t;

typedef struct
{
    Dmod_ModuleSection_t    Header;
    Dmod_ModuleSection_t    Inputs;
    Dmod_ModuleSection_t    Outputs;
    Dmod_ModuleSection_t    Text;
    Dmod_ModuleSection_t    Data;
    Dmod_ModuleSection_t    Bss;
    Dmod_ModuleSection_t    Got;
    Dmod_ModuleSection_t    Bin;
} Dmod_ModuleFooter_t;

typedef struct 
{
    void*       Function;
    const char* Signature;
} Dmod_ApiRegistration_t;

/**
 * @brief Stores data in the module output section
 */
typedef struct
{
    void*   Entries[2];
} Dmod_OutputsSection_t;


typedef struct 
{
    Dmod_ApiRegistration_t* Entries;
} Dmod_InputsSection_t;

typedef struct 
{
    void*   Entries[2];
} Dmod_GotSection_t;

typedef struct 
{
    union 
    {
        Dmod_InputsSection_t*    InputSection;
        Dmod_OutputsSection_t*   OutputSection;
    };
    size_t              SectionSize;
    Dmod_ApiType_t      ApiType;
} Dmod_Api_t;

/**
 * @brief Context handle
 * 
 * @note This handle is used to store the context of the module
 */
typedef struct 
{
    uint32_t                 Signature;
    Dmod_ModuleHeader_t*     Header;
    Dmod_ModuleFooter_t*     Footer;
    Dmod_Api_t               Inputs;
    Dmod_Api_t               Outputs;
    void*                    Data;
    size_t                   Size;
    bool                     Enabled;
    bool                     Running;
} Dmod_Context_t;


#endif /* INC_DMOD_TYPES_H_ */
