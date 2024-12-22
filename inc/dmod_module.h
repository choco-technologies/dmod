#ifndef DMOD_MODULE_H
#define DMOD_MODULE_H

#include "dmod_defs.h"
#include "dmod_types.h"

/**
 * @brief Defines builtin API
 * 
 * @param MODULE Module name
 * @param VERSION Module version
 * @param RET Return type
 * @param NAME Function name
 * @param PARAMS Function parameters, example: (int a, int b)
 */
#define DMOD_BUILTIN_API( MODULE, VERSION, RET, NAME, PARAMS )      DMOD_OUTPUT_API( MODULE, VERSION, RET, NAME, PARAMS )

/**
 * @brief Defines module API
 * 
 * @param MODULE Module name
 * @param VERSION Module version
 * @param RET Return type
 * @param NAME Function name
 * @param PARAMS Function parameters, example: (int a, int b)
 */
#define DMOD_API( MODULE, VERSION, RET, NAME, PARAMS )              DMOD_INPUT_API( MODULE, VERSION, RET, NAME, PARAMS )


#endif // DMOD_MODULE_H