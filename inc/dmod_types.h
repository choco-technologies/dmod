#ifndef INC_DMOD_TYPES_H_
#define INC_DMOD_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

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
    Dmod_ModuleType_Library, 
    Dmod_ModuleType_Application, 

    Dmod_ModuleType_Count,
} Dmod_ModuleType_t;

typedef struct 
{
    uint32_t       Size;
    uint32_t       Version;
    uint32_t       Flags;
    char           Name[DMOD_MAX_MODULE_NAME_LENGTH];
    char           Data[0];
} Dmod_Config_t;

typedef void (*Dmod_Preinit_t)(void);
typedef int (*Dmod_Init_t)(const Dmod_Config_t *Config);
typedef int (*Dmod_Main_t)(int argc, char *argv[]);
typedef int (*Dmod_Deinit_t)(void);
typedef int (*Dmod_Signal_t)( int SignalNumber );

typedef struct 
{
    char           Name[DMOD_MAX_LICENSE_NAME_LENGTH];
    char           Url[DMOD_MAX_URL_LENGTH];
    char*          Text;
} Dmod_License_t;

typedef struct 
{
    uint32_t            Signature;  // DMOD
    uint32_t            HeaderSize;
    uint32_t            DmodVersion;
    char                Arch[DMOD_MAX_ARCH_NAME_LENGTH];
    char                CpuName[DMOD_MAX_CPU_NAME_LENGTH];
    char                Name[DMOD_MAX_MODULE_NAME_LENGTH];
    char                Author[DMOD_MAX_AUTHOR_NAME_LENGTH];
    char                Version[DMOD_MAX_VERSION_LENGTH];   //!< Module Version
    Dmod_Preinit_t      Preinit;
    Dmod_Init_t         Init;
    Dmod_Main_t         Main;
    Dmod_Deinit_t       Deinit;
    Dmod_Signal_t       Signal;
    uint64_t            RequiredStackSize;
    uint32_t            Priority;
    uint8_t             ModuleType;
    Dmod_License_t*     License;
    void*               Footer;
    bool                ManualLoad;  
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
    Dmod_ApiRegistration_t Entries[2];
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
 * @brief Required module
 */
typedef struct 
{
    char           Name[DMOD_MAX_MODULE_NAME_LENGTH];
    char           Version[DMOD_MAX_VERSION_LENGTH];
} Dmod_RequiredModule_t;

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
    void*                    Mutex;
    int                      UsageCounter;
    Dmod_RequiredModule_t    RequiredModules[DMOD_MAX_REQUIRED_MODULES];
    bool                     Enabled;
    bool                     Running;
} Dmod_Context_t;

/**
 * @brief List handle
 * 
 * @note This handle is used to store the list of elements
 */
typedef void* Dmod_List_t;

/**
 * @brief List element handle
 */
typedef void* Dmod_ListElement_t;

#ifdef __cplusplus
}
#endif
#endif /* INC_DMOD_TYPES_H_ */
