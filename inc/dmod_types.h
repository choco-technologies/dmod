#ifndef INC_DMOD_TYPES_H_
#define INC_DMOD_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "dmod_defs.h"

#if UINTPTR_MAX == UINT32_MAX
typedef uint64_t Dmod_CrossPtr_t;
#elif UINTPTR_MAX == UINT64_MAX
typedef uint32_t Dmod_CrossPtr_t;
#else
#   error "Unsupported pointer size"
#endif

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

/**
 * @brief Log levels
 */
typedef enum 
{
    Dmod_LogLevel_None = 0,     //!< No logging
    Dmod_LogLevel_Error,        //!< Error log level
    Dmod_LogLevel_Warn,         //!< Warning log level
    Dmod_LogLevel_Info,         //!< Info log level
    Dmod_LogLevel_Verbose,      //!< Verbose log level

    Dmod_LogLevel_Count         //!< Number of log levels
} Dmod_LogLevel_t;

/**
 * @brief Timestamp type
 * 
 * This type represents a timestamp in milliseconds. It is used to store
 * the time elapsed since the system started (uptime).
 */
typedef uint64_t Dmod_Timestamp_t;

/**
 * @brief Process ID type
 * 
 * This type represents a process ID. On POSIX systems, this is typically pid_t.
 * A positive value indicates the process ID of the spawned process.
 * A negative value indicates an error code.
 * 
 * Note: int32_t is used for cross-platform consistency. On most systems, PIDs
 * fit comfortably within 32-bit signed integers (typical max PID is ~4 million).
 * The weak implementation uses DMOD_CURRENT_PROCESS_PID as a placeholder
 * when running in the current process (no real process spawning available).
 */
typedef int32_t Dmod_Pid_t;

/**
 * @brief Placeholder PID for the current process
 * 
 * This value is returned by weak implementations of spawn functions when
 * they run the module in the current process instead of spawning a new one.
 * This is not meant to represent a real system PID.
 */
#define DMOD_CURRENT_PROCESS_PID ((Dmod_Pid_t)1)

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

typedef union
{
    void*          Ptr;
    uint64_t       Address; 
} Dmod_UniPtr_t;

typedef struct 
{
    uint32_t            Signature;  // DMOD
    uint32_t            HeaderSize;
    uint32_t            DmodVersion;
    uint32_t            PointerSize;   
    char                Arch[DMOD_MAX_ARCH_NAME_LENGTH];
    char                CpuName[DMOD_MAX_CPU_NAME_LENGTH];
    char                Name[DMOD_MAX_MODULE_NAME_LENGTH];
    char                Author[DMOD_MAX_AUTHOR_NAME_LENGTH];
    char                Version[DMOD_MAX_VERSION_LENGTH];   //!< Module Version
    Dmod_UniPtr_t       Preinit;
    Dmod_UniPtr_t       Init;
    Dmod_UniPtr_t       Main;
    Dmod_UniPtr_t       Deinit;
    Dmod_UniPtr_t       Signal;
    uint64_t            RequiredStackSize;
    uint32_t            Priority;
    uint8_t             ModuleType;
    Dmod_UniPtr_t       License;
    Dmod_UniPtr_t       Footer;
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

typedef struct 
{
    Dmod_CrossPtr_t  Function;
    Dmod_CrossPtr_t  Signature;
} Dmod_ApiRegistrationCross_t;

/**
 * @brief Stores data in the module output section
 */
typedef struct
{
    void*   Entries[2];
} Dmod_OutputsSection_t;

/**
 * @brief Stores data in the module output section in crossplatform mode
 */
typedef struct 
{
    Dmod_CrossPtr_t   Entries[2];
} Dmod_OutputsSectionCross_t;


typedef struct 
{
    Dmod_ApiRegistration_t Entries[2];
} Dmod_InputsSection_t;

typedef struct 
{
    Dmod_ApiRegistrationCross_t Entries[2];
} Dmod_InputsSectionCross_t;

typedef struct 
{
    void*   Entries[2];
} Dmod_GotSection_t;

typedef struct 
{
    Dmod_CrossPtr_t   Entries[2];
} Dmod_GotSectionCross_t;

typedef struct 
{
    union 
    {
        Dmod_InputsSection_t*       InputSection;
        Dmod_InputsSectionCross_t*  InputSectionCross;
        Dmod_OutputsSection_t*      OutputSection;
        Dmod_OutputsSectionCross_t* OutputSectionCross;
    };
    size_t              SectionSize;
    Dmod_ApiType_t      ApiType;
    bool                Crossplatform;
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
 * @brief Per-module log level node
 * 
 * This structure is used to store the per-module log level in a singly
 * linked list. The list is traversed when checking the log level for a
 * specific module.
 */
typedef struct Dmod_ModuleLogLevel
{
    char                            Name[DMOD_MAX_MODULE_NAME_LENGTH]; //!< Module name
    Dmod_LogLevel_t                 Level;                             //!< Log level for this module
    struct Dmod_ModuleLogLevel*     Next;                              //!< Pointer to next node
} Dmod_ModuleLogLevel_t;

/**
 * @brief Search path node
 */
typedef struct Dmod_SearchNode
{
    char*                   Path;   //!< Path to the repository directory
    struct Dmod_SearchNode* Prev;   //!< Pointer to the prev node
} Dmod_SearchNode_t;

/**
 * @brief Module iteration node
 * 
 * Structure used for iterating through all available modules in the system.
 * The user should allocate this structure and pass it to Dmod_ReadNextModule.
 */
typedef struct 
{
    Dmod_ModuleHeader_t header;                 //!< Header of current module
    char path[DMOD_MAX_PATH_LENGTH];            //!< Path to the current module
    void* _Data;                                //!< Internal data used for iteration
} Dmod_ModuleNode_t;

/**
 * @brief Directory entry type
 */
typedef enum
{
    Dmod_DirEntryType_Unknown = 0,  //!< Unknown type
    Dmod_DirEntryType_File,         //!< Regular file
    Dmod_DirEntryType_Dir,          //!< Directory
    Dmod_DirEntryType_Link,         //!< Symbolic link
    Dmod_DirEntryType_Other         //!< Other type (socket, FIFO, etc.)
} Dmod_DirEntryType_t;

/**
 * @brief Directory entry structure
 * 
 * Structure returned by Dmod_ReadDirEx containing information about a directory entry.
 */
typedef struct
{
    const char* name;               //!< Name of the entry
    Dmod_DirEntryType_t type;       //!< Type of the entry
} Dmod_DirEntry_t;

#ifdef __cplusplus
}
#endif
#endif /* INC_DMOD_TYPES_H_ */
