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
 * @brief Private header for DMOD IRQ table management
 * @file dmod_irq.h
 */

#ifndef DMOD_IRQ_H
#define DMOD_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DMOD_PRIVATE
#   error "This is private DMOD header. Don't include this outside DMOD library"
#endif

#include "dmod_types.h"

/**
 * @brief Initialize the IRQ handler table
 * 
 * Allocates a flat table of (NumIrqs * MaxHandlersPerIrq) entries.
 * 
 * @param NumIrqs           Number of distinct IRQ lines supported
 * @param MaxHandlersPerIrq Maximum number of module handlers per IRQ line
 * 
 * @return true on success (or when both parameters are 0), false on allocation failure
 */
extern bool Dmod_Irq_Init( size_t NumIrqs, size_t MaxHandlersPerIrq );

/**
 * @brief Free the IRQ handler table
 */
extern void Dmod_Irq_Deinit( void );

/**
 * @brief Register all IRQ handlers declared by a module
 * 
 * Scans the module's input section for entries with DMOD_IRQ_SIGNATURE_PREFIX,
 * parses the IRQ number from the signature and inserts the handler into the table.
 * 
 * @param Context Loaded module context (Inputs must already be populated)
 */
extern void Dmod_Irq_RegisterModule( Dmod_Context_t* Context );

/**
 * @brief Unregister all IRQ handlers that belong to a module
 * 
 * Removes every table entry whose owner is Context.
 * 
 * @param Context Module context about to be unloaded
 */
extern void Dmod_Irq_UnregisterModule( Dmod_Context_t* Context );

#ifdef __cplusplus
}
#endif

#endif // DMOD_IRQ_H
