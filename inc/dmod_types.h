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

typedef enum 
{
    Dmod_ModuleState_Available,     //!< Module is available but not loaded
    Dmod_ModuleState_Loaded,        //!< Module is loaded but not enabled
    Dmod_ModuleState_Enabled,       //!< Module is enabled (library modules)
    Dmod_ModuleState_Running,       //!< Module is running (application modules)

    Dmod_ModuleState_Count,
} Dmod_ModuleState_t;

typedef struct 
{
    char                   ModuleName[DMOD_MAX_MODULE_NAME_LENGTH];
    char                   Version[DMOD_MAX_VERSION_LENGTH];
    Dmod_ModuleState_t     State;
} Dmod_ModuleInfo_t;

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
    uint64_t            Preinit;
    uint64_t            Init;
    uint64_t            Main;
    uint64_t            Deinit;
    uint64_t            Signal;
    uint64_t            RequiredStackSize;
    uint32_t            Priority;
    uint8_t             ModuleType;
    uint64_t            License;
    uint64_t            Footer;
    bool                ManualLoad;  
} Dmod_ModuleHeader_t;

/**
 * @brief DMFC Header
 * 
 * DMF Compressed Header - This header is used to store the compressed module
 */
typedef struct 
{
    uint32_t       Signature;       //!< DMFC
    uint16_t       HeaderSize;      //!< Size of this header
    uint16_t       HeaderVersion;   //!< Header version
    char           Compression[DMOD_MAX_COMPRESSION_NAME_LENGTH];   //!< Compression algorithm
    char           Name[DMOD_MAX_MODULE_NAME_LENGTH];               //!< Module name
    uint32_t       OriginalSize;                                    //!< Original size of the module
} Dmod_DmfcHeader_t;

typedef struct 
{
    uint32_t       Signature;       //!< DMPH
    uint16_t       HeaderSize;      //!< Size of this header
    uint16_t       HeaderVersion;   //!< Header version
    char           Name[DMOD_MAX_PACKAGE_NAME_LENGTH];   //!< Package name
    uint32_t       MainIndex;       //!< Index of the main module
    uint32_t       ModuleCount;     //!< Number of modules
} Dmod_DmpHeader_t;

typedef struct 
{
    uint32_t       ModuleOffset;    //!< Offset of the module
    uint32_t       FileSize;        //!< Size of the module's file
    char           ModuleName[DMOD_MAX_MODULE_NAME_LENGTH]; //!< Name of the module
} Dmod_DmpModuleEntry_t;

typedef struct 
{
    const void*             PackageBuffer;      //!< Pointer to the package buffer
    size_t                  PackageSize;        //!< Size of the package
    const char*             FilePath;           //!< File path if loaded from file
    Dmod_DmpHeader_t*       DmpHeader;          //!< DMP Header
    Dmod_DmpModuleEntry_t*  ModuleEntries;      //!< Module entries
} Dmod_PackageSlot_t;

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
    bool           SystemModule;
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
    const char*              PackageName;  
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

/**
 * @brief Search path node
 */
typedef struct Dmod_SearchNode
{
    char*                   Path;   //!< Path to the repository directory
    struct Dmod_SearchNode* Prev;   //!< Pointer to the prev node
} Dmod_SearchNode_t;

#ifdef __cplusplus
}
#endif
#endif /* INC_DMOD_TYPES_H_ */
