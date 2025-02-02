#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include <errno.h>

/**
 * @brief Preinitialize module
 * 
 * @param Context Context to preinitialize
 */
void Dmod_Preinit( Dmod_Context_t* Context )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot preinit module - invalid context\n");
        return;
    }

    if( Context->Header->Preinit == NULL )
    {
        DMOD_LOG_INFO("Preinit function not set\n");
        return;
    }

    Context->Header->Preinit();
}

/**
 * @brief Initialize module
 * 
 * @param Context Context to initialize
 * @param Config Configuration
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Init( Dmod_Context_t* Context, const Dmod_Config_t* Config )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Init == NULL )
        {
            DMOD_LOG_INFO("Init function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Init( Config );
        }
    }
    return result;
}

/**
 * @brief Call main function
 * 
 * @param Context Context to call main function
 * @param argc Number of arguments
 * @param argv Arguments
 * 
 * @return Return value of the main function
 */
int Dmod_Main( Dmod_Context_t* Context, int argc, char *argv[] )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Main == NULL )
        {
            DMOD_LOG_INFO("Main function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Main( argc, argv );
        }
    }
    return result;
}

/**
 * @brief Deinitialize module
 * 
 * @param Context Context to deinitialize
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Deinit( Dmod_Context_t* Context )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Deinit == NULL )
        {
            DMOD_LOG_INFO("Deinit function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Deinit();
        }
    }
    return result;
}

/**
 * @brief Call signal's handler
 * 
 * @param Context Context to signal
 * @param SignalNumber Signal number
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Signal( Dmod_Context_t* Context, int SignalNumber )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        if( Context->Header->Signal == NULL )
        {
            DMOD_LOG_INFO("Signal function not set\n");
            result = 0;
        }
        else 
        {
            result = Context->Header->Signal( SignalNumber );
        }
    }
    return result;
}

/**
 * @brief Call IRQ handler
 * 
 * @param Context Context to IRQ
 * @param IrqNumber IRQ number
 * 
 * @return 0 on success, errno on error
 */
int Dmod_Irq( Dmod_Context_t* Context, const char* Signature )
{
    int result = -EINVAL;
    if( Dmod_Context_IsValid( Context ) )
    {
        void (*function)() = Dmod_GetFunction( Context, Signature );
        if( function != NULL )
        {
            DMOD_LOG_VERBOSE("Calling IRQ %s for %s\n", Signature, Dmod_Context_GetModuleName( Context ));
            function();
        }
        result = 0;
    }
    return result;
}