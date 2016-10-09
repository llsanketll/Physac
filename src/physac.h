/**********************************************************************************************
*
*   Physac - 2D Physics library for videogames
*
*   // TODO: add description
* 
*   CONFIGURATION:
*   
*   #define PHYSAC_IMPLEMENTATION
*       Generates the implementation of the library into the included file.
*       If not defined, the library is in header only mode and can be included in other headers 
*       or source files without problems. But only ONE file should hold the implementation.
*
*   #define PHYSAC_STATIC (defined by default)
*       The generated implementation will stay private inside implementation file and all 
*       internal symbols and functions will only be visible inside that file.
*
*   #define PHYSAC_NO_THREADS
*       The generated implementation won't include pthread library and user must create a secondary thread to call PhysicsThread().
*       It is so important that the thread where PhysicsThread() is called must not have v-sync or any other CPU limitation.
*
*   #define PHYSAC_STANDALONE
*       Avoid raylib.h header inclusion in this file. Data types defined on raylib are defined
*       internally in the library and input management and drawing functions must be provided by
*       the user (check library implementation for further details).
*
*   #define PHYSAC_MALLOC()
*   #define PHYSAC_FREE()
*       You can define your own malloc/free implementation replacing stdlib.h malloc()/free() functions.
*       Otherwise it will include stdlib.h and use the C standard library malloc()/free() function.
*
*   VERY THANKS TO:
*       - Ramón Santamaria (@raysan5)
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2016 Victor Fisac
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#ifndef PHYSAC_H
#define PHYSAC_H

#include "raylib.h"

#define PHYSAC_STATIC
#ifdef PHYSAC_STATIC
    #define PHYSACDEF static            // Functions just visible to module including this file
#else
    #ifdef __cplusplus
        #define PHYSACDEF extern "C"    // Functions visible from other files (no name mangling of functions in C++)
    #else
        #define PHYSACDEF extern        // Functions visible from other files
    #endif
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define     PHYSAC_COLLISION_INTERATIONS    10
#define     MAX_TIMESTEP                    0.02

//----------------------------------------------------------------------------------
// Types and Structures Definition
// NOTE: Below types are required for PHYSAC_STANDALONE usage
//----------------------------------------------------------------------------------
#if defined(PHYSAC_STANDALONE)
    // TODO: add standalone type structs
#endif

// TODO: add global structs

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
PHYSACDEF void InitPhysics(Vector2 gravity);            // Initializes physics values, pointers and creates physics loop thread
PHYSACDEF void ClosePhysics(void);                      // Unitializes physics pointers and closes physics loop thread

#endif // PHYSAC_H

/***********************************************************************************
*
*   PHYSAC IMPLEMENTATION
*
************************************************************************************/

#if defined(PHYSAC_IMPLEMENTATION)

#ifndef PHYSAC_NO_THREADS
    #include <pthread.h>        // Required for: pthread_t, pthread_create()
#endif

#include <math.h>               // Required for: clamp()
#include "C:\raylib\raylib\src\utils.h"     // Required for: TraceLog()

// Functions required to query time on Windows
int __stdcall QueryPerformanceCounter(unsigned long long int *lpPerformanceCount);
int __stdcall QueryPerformanceFrequency(unsigned long long int *lpFrequency);

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define     STATIC_DELTATIME        1.0/60.0

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// TODO: add internal structs

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static bool frameStepping = false;              // TODO: check variable utility
static bool canStep = false;                    // TODO: check variable utility
static bool physicsThreadEnabled = false;       // Physics thread enabled state
static int stepsCount = 0;                      // Total physics steps processed

static double currentTime = 0;                  // Current time in milliseconds
static double startTime = 0;                    // start time in milliseconds

static Vector2 gravityForce = { 0, 0 };         // Physics world gravity force
static int physicBodiesCount = 0;               // Physics world current bodies counter

//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------
static void *PhysicsLoop(void *arg);            // Physics loop thread function
static void PhysicsStep(void);                  // Physics steps calculations (dynamics, collisions and position corrections)

static double GetCurrentTime(void);             // Get current time in milliseconds

static void MathClamp(double *value, double min, double max);       // Clamp a value in a range

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
// Initializes physics values, pointers and creates physics loop thread
PHYSACDEF void InitPhysics(Vector2 gravity)
{
    TraceLog(WARNING, "[PHYSAC] physics module initialized successfully");
    
    // Initialize world gravity
    gravityForce = gravity;
    
    #ifndef PHYSAC_NO_THREADS
        // NOTE: if defined, user will need to create a thread for PhysicsThread function manually
        // Create physics thread using POSIXS thread libraries
        pthread_t tid;
        pthread_create(&tid, NULL, &PhysicsLoop, NULL);
    #endif
    
    // TODO: initialize dynamic memory allocations
}

// Unitializes physics pointers and exits physics loop thread
PHYSACDEF void ClosePhysics(void)
{
    TraceLog(WARNING, "[PHYSAC] physics module closed successfully");
    
    // Exit physics loop thread
    physicsThreadEnabled = false;
    
    // TODO: unitialize dynamic memory allocations
}

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Physics loop thread function
static void *PhysicsLoop(void *arg)
{
    TraceLog(WARNING, "[PHYSAC] physics thread created with successfully");
    
    // Initialize physics loop thread values
    physicsThreadEnabled = true;
    double accumulator = 0;
    
    // Initialize high resolution timer
    startTime = GetCurrentTime();
    
    // Physics update loop
    while (physicsThreadEnabled)
    {
        // Calculate current time
        currentTime = GetCurrentTime();
        
        // Store the time elapsed since the last frame began
        accumulator += currentTime - startTime;
        
        // Clamp accumulator to max time step to avoid bad performance
        MathClamp(&accumulator, 0.0, MAX_TIMESTEP);
        
        // Record the starting of this frame
        startTime = currentTime;
        
        // TODO: test static delta time (1.0/60.0) and then implement current fps based delta time
        while (accumulator >= STATIC_DELTATIME)
        {
            if (!frameStepping) PhysicsStep();
            else
            {
                if (canStep)
                {
                    PhysicsStep();
                    canStep = false;
                }
            }
            
            accumulator -= STATIC_DELTATIME;
        }
    }
    
    return NULL;
}

// Physics steps calculations (dynamics, collisions and position corrections)
static void PhysicsStep(void)
{
    stepsCount++;
    
    // TODO: calculate delta time
    
    // TODO: update dynamics
    
    // TODO: update collisions
    
    // TODO: other updates
    
    // TODO: test rendering interpolation for smooth rendering results
}

// Get current time in milliseconds
static double GetCurrentTime(void)
{
    double time = 0;
    
    unsigned long long int clockFrequency, currentTime;
    
    QueryPerformanceFrequency(&clockFrequency);
    QueryPerformanceCounter(&currentTime);
    
    // TraceLog(INFO, "currentTime: %f", (double)currentTime);
    // TraceLog(INFO, "clockFrequency: %i", clockFrequency);
    
    time = (double)((double)currentTime/clockFrequency)*1000;
    
    // TraceLog(INFO, "time: %f", time);

    return time;
}

// Clamp a value in a range
static void MathClamp(double *value, double min, double max)
{
    if (*value < min) *value = min;
    else if (*value > max) *value = max;
}

#endif  // PHYSAC_IMPLEMENTATION
