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
 * @brief Main function of the application
 * 
 * @param argc Number of arguments
 * @param argv Array of arguments
 * 
 * @return 0 if success, error code otherwise
 */
int main(int argc, char *argv[])
{
    Dmod_Printf("Hello, World!\n");
    return 0;
}
