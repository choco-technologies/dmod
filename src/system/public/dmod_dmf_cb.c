#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include <errno.h>
#include <string.h>

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

    Dmod_Preinit_t preinit = (Dmod_Preinit_t)Context->Header->Preinit.Ptr;
    if( preinit == NULL )
    {
        DMOD_LOG_INFO("Preinit function not set\n");
        return;
    }
    preinit();
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
        Dmod_Init_t init = (Dmod_Init_t)Context->Header->Init.Ptr;
        if( init == NULL )
        {
            DMOD_LOG_INFO("Init function not set\n");
            result = 0;
        }
        else 
        {
            result = init( Config );
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
        Dmod_Main_t mainFunc = (Dmod_Main_t)Context->Header->Main.Ptr;
        if( mainFunc == NULL )
        {
            DMOD_LOG_INFO("Main function not set\n");
            result = 0;
        }
        else 
        {
            result = mainFunc( argc, argv );
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
        Dmod_Deinit_t deinit = (Dmod_Deinit_t)Context->Header->Deinit.Ptr;
        if( deinit == NULL )
        {
            DMOD_LOG_INFO("Deinit function not set\n");
            result = 0;
        }
        else 
        {
            result = deinit();
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
        Dmod_Signal_t signal = (Dmod_Signal_t)Context->Header->Signal.Ptr;
        if( signal == NULL )
        {
            DMOD_LOG_INFO("Signal function not set\n");
            result = 0;
        }
        else 
        {
            result = signal( SignalNumber );
        }
    }
    return result;
}

/**
 * @brief Call IRQ handler for a specific module
 * 
 * @param Context   Context to call IRQ handler for
 * @param IrqNumber IRQ number
 * 
 * @return 0 on success, errno on error
 * 
 * @note This is the slow per-module path. For dispatching to all modules use
 *       Dmod_IrqAll() which uses the pre-built handler table.
 */
int Dmod_Irq( Dmod_Context_t* Context, int IrqNumber )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        return -EINVAL;
    }

    if( Context->Inputs.InputSection == NULL )
    {
        return -EINVAL;
    }

    char signature[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf( signature, sizeof(signature), DMOD_IRQ_SIGNATURE_PREFIX "%d", IrqNumber );

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for( size_t i = 0; i < numberOfEntries; i++ )
    {
        const char* entrySig = Context->Inputs.InputSection->Entries[i].Signature;
        if( entrySig != NULL && strcmp( entrySig, signature ) == 0 )
        {
            void (*function)(void) = Context->Inputs.InputSection->Entries[i].Function;
            if( function != NULL )
            {
                DMOD_LOG_VERBOSE("Calling IRQ %d for %s\n", IrqNumber, Dmod_Context_GetModuleName( Context ));
                function();
            }
        }
    }
    return 0;
}