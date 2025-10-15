#include "dmod.h"

/**
 * @brief Pre-initialization function for the module.
 * 
 * @note This function is optional. You can remove it if you don't need it.
 * 
 * This function is called when the module enabling is in progress.
 * 
 * You can use this function to load the required dependencies, such as 
 * other modules. Please be aware that the module is not fully initialized, 
 * so not all the API functions are available - you can check if the API
 * is connected by calling the Dmod_IsFunctionConnected() function.
 */
void dmod_preinit(void)
{
    if(Dmod_IsFunctionConnected( Dmod_Printf ))
    {
        Dmod_Printf("API is connected!\n");
    }
}

/**
 * @brief Initialization function for the module.
 * 
 * This function is called when the module is enabled.
 * Please use this function to initialize the module, for instance:
 * - initialize the module variables
 * - initialize the module hardware
 * - allocate memory
 */
int dmod_init(const Dmod_Config_t *Config)
{
    Dmod_Printf("Hello, World!\n");
    return 0;
}

/**
 * @brief De-initialization function for the module.
 * 
 * This function is called when the module is disabled.
 * Please use this function to de-initialize the module, for instance:
 * - free memory
 * - de-initialize the module hardware
 * - de-initialize the module variables
 */
int dmod_deinit(void)
{
    Dmod_Printf("Goodbye, World!\n");
    return 0;
}
