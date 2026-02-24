/**
 * MIT License
 * 
 * Copyright (c) 2024 Patryk Kubiak
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @brief DMOD IRQ handler table
 * 
 * Provides a pre-built, interrupt-safe lookup table so that Dmod_IrqAll()
 * can dispatch to all registered module handlers in O(MaxHandlersPerIrq)
 * time without any string comparisons or memory allocation.
 * 
 * @file dmod_irq.c
 */

#define DMOD_PRIVATE
#include "dmod.h"
#include "private/dmod_ctx.h"
#include "private/dmod_irq.h"
#include <string.h>
#include <stdlib.h>

//==============================================================================
//                              TYPES
//==============================================================================

/**
 * @brief One slot in the IRQ handler table
 */
typedef struct
{
    void             (*Handler)(void); /**< Handler function pointer (NULL = empty slot) */
    Dmod_Context_t*    Owner;          /**< Module that registered this handler           */
} Dmod_IrqEntry_t;

//==============================================================================
//                              STATIC VARIABLES
//==============================================================================

/** Flat table: entry at [irq * MaxHandlersPerIrq + slot] */
static Dmod_IrqEntry_t* Dmod_IrqTable        = NULL;
static size_t            Dmod_IrqNumIrqs      = 0;
static size_t            Dmod_IrqMaxHandlers  = 0;

//==============================================================================
//                              LOCAL HELPERS
//==============================================================================

/**
 * @brief Parse the IRQ number encoded in a DIRQ signature string
 * 
 * Signature format: DMOD_IRQ_SIGNATURE_PREFIX followed by decimal digits.
 * 
 * @param Signature  Full registration signature string
 * @param outIrqNum  Parsed IRQ number (written on success)
 * 
 * @return true if the signature is a valid DIRQ signature with a parseable number
 */
static bool ParseIrqSignature( const char* Signature, int* outIrqNum )
{
    if( Signature == NULL || outIrqNum == NULL )
    {
        return false;
    }

    const size_t prefixLen = sizeof( DMOD_IRQ_SIGNATURE_PREFIX ) - 1;
    if( strncmp( Signature, DMOD_IRQ_SIGNATURE_PREFIX, prefixLen ) != 0 )
    {
        return false;
    }

    const char* numStr = Signature + prefixLen;
    if( *numStr == '\0' )
    {
        return false;
    }

    char* endPtr = NULL;
    long value = strtol( numStr, &endPtr, 10 );
    if( endPtr == numStr || value < 0 )
    {
        return false;
    }

    *outIrqNum = (int)value;
    return true;
}

//==============================================================================
//                              PUBLIC FUNCTIONS
//==============================================================================

bool Dmod_Irq_Init( size_t NumIrqs, size_t MaxHandlersPerIrq )
{
    /* Allow initialisation with zero dimensions (IRQ table disabled) */
    if( NumIrqs == 0 || MaxHandlersPerIrq == 0 )
    {
        Dmod_IrqTable       = NULL;
        Dmod_IrqNumIrqs     = 0;
        Dmod_IrqMaxHandlers = 0;
        return true;
    }

    size_t totalEntries = NumIrqs * MaxHandlersPerIrq;
    Dmod_IrqTable = (Dmod_IrqEntry_t*)Dmod_Malloc( totalEntries * sizeof( Dmod_IrqEntry_t ) );
    if( Dmod_IrqTable == NULL )
    {
        DMOD_LOG_ERROR("Cannot initialize IRQ table - allocation failed (NumIrqs=%zu, MaxHandlers=%zu)\n",
                       NumIrqs, MaxHandlersPerIrq);
        return false;
    }

    memset( Dmod_IrqTable, 0, totalEntries * sizeof( Dmod_IrqEntry_t ) );
    Dmod_IrqNumIrqs     = NumIrqs;
    Dmod_IrqMaxHandlers = MaxHandlersPerIrq;

    DMOD_LOG_INFO("IRQ table initialized: %zu IRQs x %zu handlers\n", NumIrqs, MaxHandlersPerIrq);
    return true;
}

void Dmod_Irq_Deinit( void )
{
    if( Dmod_IrqTable != NULL )
    {
        Dmod_Free( Dmod_IrqTable );
        Dmod_IrqTable       = NULL;
    }
    Dmod_IrqNumIrqs     = 0;
    Dmod_IrqMaxHandlers = 0;
}

bool Dmod_Irq_RegisterModule( Dmod_Context_t* Context )
{
    if( Dmod_IrqTable == NULL || !Dmod_Context_IsValid( Context ) )
    {
        return true;
    }

    if( Context->Inputs.InputSection == NULL || Context->Inputs.SectionSize == 0 )
    {
        return true;
    }

    size_t numberOfEntries = Dmod_Api_GetNumberOfEntries( &Context->Inputs );
    for( size_t i = 0; i < numberOfEntries; i++ )
    {
        const char* sig = Context->Inputs.InputSection->Entries[i].Signature;
        void* fn        = Context->Inputs.InputSection->Entries[i].Function;

        int irqNum = 0;
        if( !ParseIrqSignature( sig, &irqNum ) )
        {
            continue;
        }

        if( (size_t)irqNum >= Dmod_IrqNumIrqs )
        {
            DMOD_LOG_WARN("IRQ %d exceeds table size (%zu) for module %s - skipping\n",
                          irqNum, Dmod_IrqNumIrqs, Dmod_Context_GetModuleName( Context ));
            continue;
        }

        Dmod_EnterCritical();
        Dmod_IrqEntry_t* row = &Dmod_IrqTable[irqNum * Dmod_IrqMaxHandlers];
        bool registered = false;
        size_t registeredSlot = 0;
        for( size_t slot = 0; slot < Dmod_IrqMaxHandlers; slot++ )
        {
            if( row[slot].Handler == NULL )
            {
                row[slot].Handler = (void (*)(void))fn;
                row[slot].Owner   = Context;
                registered = true;
                registeredSlot = slot;
                break;
            }
        }
        Dmod_ExitCritical();

        if( registered )
        {
            DMOD_LOG_VERBOSE("Registered IRQ %d handler for module %s (slot %zu)\n",
                             irqNum, Dmod_Context_GetModuleName( Context ), registeredSlot);
        }
        else
        {
            DMOD_LOG_ERROR("No free slot for IRQ %d in module %s - increase MaxHandlersPerIrq\n",
                           irqNum, Dmod_Context_GetModuleName( Context ));
            return false;
        }
    }
    return true;
}

void Dmod_Irq_UnregisterModule( Dmod_Context_t* Context )
{
    if( Dmod_IrqTable == NULL || Context == NULL )
    {
        return;
    }

    size_t totalEntries = Dmod_IrqNumIrqs * Dmod_IrqMaxHandlers;
    for( size_t i = 0; i < totalEntries; i++ )
    {
        if( Dmod_IrqTable[i].Owner == Context )
        {
            Dmod_IrqTable[i].Handler = NULL;
            Dmod_IrqTable[i].Owner   = NULL;
        }
    }
}

void Dmod_IrqAll( int IrqNumber )
{
    if( Dmod_IrqTable == NULL || IrqNumber < 0 || (size_t)IrqNumber >= Dmod_IrqNumIrqs )
    {
        return;
    }

    Dmod_IrqEntry_t* row = &Dmod_IrqTable[IrqNumber * Dmod_IrqMaxHandlers];
    for( size_t slot = 0; slot < Dmod_IrqMaxHandlers; slot++ )
    {
        if( row[slot].Handler != NULL )
        {
            row[slot].Handler();
        }
    }
}
