#ifndef INC_DMOD_TYPES_H_
#define INC_DMOD_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include "dmod_defs.h"

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

typedef struct 
{
    uint32_t        Signature;  // DMOD
    uint32_t        HeaderSize;
    uint32_t        Version;
    char            Arch[DMOD_MAX_ARCH_NAME_LENGTH];
    char            Name[DMOD_MAX_MODULE_NAME_LENGTH];
    Dmod_Init_t     Init;
    Dmod_Main_t     Main;
    Dmod_Deinit_t   Deinit;
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
    Dmod_ModuleSection_t    Got;
    Dmod_ModuleSection_t    Bin;
} Dmod_ModuleFooter_t;

/**
 * @brief Context handle
 * 
 * @note This handle is used to store the context of the module
 */
typedef struct Dmod_Context_t* Dmod_Context_t;

/**
 * @brief Memory interface
 * 
 * @note This interface is used to allocate and free memory
 */
typedef struct 
{
    /**
     * @brief Allocate memory
     * 
     * @param Size Size of memory to allocate
     * 
     * @return Pointer to allocated memory
     */
    void* (*Malloc)(size_t Size);         

    /**
     * @brief Allocate aligned memory
     * 
     * @param Size Size of memory to allocate
     * @param Alignment Alignment of memory
     * 
     * @return Pointer to allocated memory
     * 
     * @note Optional - set to NULL if not supported
     */                  
    void* (*AlignedMalloc)(size_t Size, size_t Alignment);  

    /**
     * @brief Free memory
     * 
     * @param ptr Pointer to memory to free
     * 
     * @return NULL
     */
    void* (*Free)(void *ptr);                               
} Dmod_MemIf_t;

/**
 * @brief File interface
 * 
 * @note This interface is used to open, read and close files
 */
typedef struct 
{
    /**
     * @brief Open file
     * 
     * @param Path Path to file
     * @param Mode Mode to open file
     * 
     * @return Pointer to file
     */
    void* (*Open)(const char *Path, const char *Mode);

    /**
     * @brief Read file
     * 
     * @param File Pointer to file
     * @param Buffer Buffer to read data into
     * @param Size Size of data to read
     * 
     * @return Number of bytes read
     */
    size_t (*Read)(void *File, void *Buffer, size_t Size);

    /**
     * @brief Write file
     * 
     * @param File Pointer to file
     * @param Buffer Buffer to write data from
     * @param Size Size of data to write
     * 
     * @return Number of bytes written
     */
    int (*Close)(void *File);
} Dmod_FileIf_t;

/**
 * @brief Debug interface
 * 
 * @note This interface is used to print and assert messages
 */
typedef struct 
{
    /**
     * @brief Printf function
     * 
     * @param Format Format string
     * 
     * @return Number of characters printed
     */
    int (*Printf)(const char *Format, ...);

    /**
     * @brief Assert function
     * 
     * @param Condition Condition to assert
     * @param Message Message to print
     * @param File File name
     * @param Line Line number
     */
    void (*Assert)(int Condition, const char *Message, const char *File, int Line);
} Dmod_DbgIf_t;

/**
 * @brief System interface
 * 
 * @note This interface is used to provide memory, file and debug interfaces
 */
typedef struct 
{
    Dmod_MemIf_t    Memory;     //!< Memory interface
    Dmod_FileIf_t   File;       //!< File interface
    Dmod_DbgIf_t    Debug;      //!< Debug interface
} Dmod_SystemIf_t;

#endif /* INC_DMOD_TYPES_H_ */
