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

/**
 * @brief Run all test steps registered in a module
 *
 * Iterates the module's .dmod.inputs section, discovers all entries whose
 * signature begins with DMOD_TEST_SIGNATURE_PREFIX, and executes them one
 * by one.  Three special reserved step names drive the fixture mechanism:
 *
 *   __setup__      — called before every step (optional)
 *   __teardown__   — called after  every step (optional)
 *   __step_failed__ — pointer to the module-side volatile int flag that
 *                     assertion macros set on failure
 *
 * These reserved entries are registered automatically by the
 * dmod_test_main.c translation unit compiled in by dmod_add_test().
 *
 * @param Context  Context of the loaded test module
 * @param argc     Argument count forwarded from main()
 * @param argv     Argument vector; argv[1..] are treated as a step-name
 *                 allow-list — only the named steps run.  Pass argc == 1
 *                 (or argc == 0) to run all steps.
 *
 * @return Number of failed test steps (0 = all passed)
 */
int Dmod_RunTests( Dmod_Context_t* Context, int argc, char* argv[] )
{
    if( !Dmod_Context_IsValid( Context ) )
    {
        DMOD_LOG_ERROR("Cannot run tests - invalid context\n");
        return -1;
    }

    if( Context->Inputs.InputSection == NULL )
    {
        DMOD_LOG_ERROR("Cannot run tests - no input section\n");
        return -1;
    }

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );

    /* Locate the optional fixture pointers registered by dmod_test_main.c */
    void         (*setup_fn)(void)   = NULL;
    void         (*teardown_fn)(void) = NULL;
    volatile int  *pStepFailed        = NULL;

    for( size_t i = 0; i < numberOfEntries; i++ )
    {
        const char* sig = Context->Inputs.InputSection->Entries[i].Signature;
        void*       fn  = Context->Inputs.InputSection->Entries[i].Function;

        if( sig == NULL || fn == NULL )               { continue; }
        if( !Dmod_ApiSignature_IsTest( sig ) )        { continue; }

        const char* name = Dmod_ApiSignature_GetName( sig );
        if( name == NULL )                             { continue; }

        if(      strcmp( name, "__setup__" )      == 0 ) { setup_fn    = (void (*)(void))fn; }
        else if( strcmp( name, "__teardown__" )   == 0 ) { teardown_fn = (void (*)(void))fn; }
        else if( strcmp( name, "__step_failed__" ) == 0 ) { pStepFailed = (volatile int*)fn; }
    }

    int total_steps  = 0;
    int failed_steps = 0;

    Dmod_Printf( "=== DMOD Test Runner ===\n" );

    for( size_t i = 0; i < numberOfEntries; i++ )
    {
        const char* sig = Context->Inputs.InputSection->Entries[i].Signature;
        void*       fn  = Context->Inputs.InputSection->Entries[i].Function;

        if( sig == NULL || fn == NULL )               { continue; }
        if( !Dmod_ApiSignature_IsTest( sig ) )        { continue; }

        const char* name = Dmod_ApiSignature_GetName( sig );
        if( name == NULL )                             { continue; }

        /* Skip reserved fixture entries */
        if( strcmp( name, "__setup__" )       == 0 )  { continue; }
        if( strcmp( name, "__teardown__" )    == 0 )  { continue; }
        if( strcmp( name, "__step_failed__" ) == 0 )  { continue; }

        /* Apply optional step-name filter from argv */
        if( argc > 1 )
        {
            int found = 0;
            for( int j = 1; j < argc; j++ )
            {
                if( argv[j] != NULL && strcmp( name, argv[j] ) == 0 )
                {
                    found = 1;
                    break;
                }
            }
            if( !found ) { continue; }
        }

        Dmod_Printf( "[ RUN  ] %s\n", name );

        if( pStepFailed != NULL )  { *pStepFailed = 0; }

        void (*stepFn)(void) = (void (*)(void))fn;
        if( setup_fn    != NULL )  { setup_fn(); }
        stepFn();
        if( teardown_fn != NULL )  { teardown_fn(); }

        total_steps++;

        int step_failed = ( pStepFailed != NULL ) ? *pStepFailed : 0;
        if( step_failed )
        {
            Dmod_Printf( "[FAILED] %s\n", name );
            failed_steps++;
        }
        else
        {
            Dmod_Printf( "[  OK  ] %s\n", name );
        }
    }

    if( total_steps == 0 )
    {
        Dmod_Printf( "\nNo test steps found.\n" );
    }
    else
    {
        Dmod_Printf( "\n=== Results: %d/%d passed ===\n",
                     total_steps - failed_steps, total_steps );
    }

    return failed_steps;
}