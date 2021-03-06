#ifndef CYGONCE_HAL_CLOCK_H
#define CYGONCE_HAL_CLOCK_H

//=============================================================================
//
//      hal_clock.h
//
//      HAL clock support
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   nickg, gthomas, hmt
// Contributors:        nickg, gthomas, hmt
// Date:        1999-01-28
// Purpose:     Define clock support
// Description: The macros defined here provide the HAL APIs for handling
//              the clock.
//              
// Usage:
//              #include <cyg/hal/hal_intr.h> // which includes this file
//              ...
//              
//
//####DESCRIPTIONEND####
//
//=============================================================================

#include <cyg/hal/hal_io.h>

//-----------------------------------------------------------------------------
// Clock control

// in erc32 simulator:
//  4 = UART A
//  5 = UART B
//  7 = UART error
// 12 = GPT (general purpose timer)
// 13 = TI1 (realtime clock)
// 15 = watchdog

// in erc32 simulator:

// The vector used by the Real time clock is defined in hal_xpic.h

// We could place conditional code here to choose one clock or the other
// depending on the selected interrupt vector... but pro tem: (pun intended)

#define SPARC_LEON_TIMER_SCALE (24)

/* These must be accessed word-wide to work! */

#define SPARC_LEON_REG              (0x80000000)

#define SPARC_LEON_TI1_COUNTER    (SPARC_LEON_REG + 0x40)
#define SPARC_LEON_TI1_CRELOAD    (SPARC_LEON_REG + 0x44)
#define SPARC_LEON_SCALER     (SPARC_LEON_REG + 0x60)
#define SPARC_LEON_SRELOAD    (SPARC_LEON_REG + 0x64)
                                                       
/* LEON timer control register bits */
#define SPARC_LEON_TI1_EN  1      /* System Enable */     
#define SPARC_LEON_TI1_RL  2      /* Continuous Running */
#define SPARC_LEON_TI1_LD  4      /* Counter Load */      

#define SPARC_LEON_TI1_CONTROL    (SPARC_LEON_REG + 0x48)

externC cyg_int32 cyg_hal_sparc_clock_period;

#define HAL_CLOCK_INITIALIZE( _period_ ) CYG_MACRO_START              \
  cyg_hal_sparc_clock_period = (_period_);                        \
  HAL_WRITE_UINT32( SPARC_LEON_TI1_CRELOAD, (_period_) );              \
  HAL_WRITE_UINT32( SPARC_LEON_TI1_COUNTER, (_period_) );              \
  HAL_WRITE_UINT32( SPARC_LEON_TI1_CONTROL,                            \
                           (SPARC_LEON_TI1_EN |                     \
                            SPARC_LEON_TI1_RL |                     \
                            SPARC_LEON_TI1_LD) );                   \
CYG_MACRO_END

#define HAL_CLOCK_RESET( _vector_, _period_ ) /* nowt, it is freerunning */

#define HAL_CLOCK_READ( _pvalue_ ) CYG_MACRO_START                    \
   cyg_uint32 _read_;                                                 \
   HAL_READ_UINT32( SPARC_LEON_TI1_COUNTER, _read_ );                  \
   *((cyg_uint32 *)(_pvalue_)) =                                      \
                 (cyg_hal_sparc_clock_period - _read_ );          \
CYG_MACRO_END


#ifdef CYGVAR_KERNEL_COUNTERS_CLOCK_LATENCY
#define HAL_CLOCK_LATENCY( _pvalue_ )         HAL_CLOCK_READ( _pvalue_ )
#endif

//-----------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_CLOCK_H
// End of hal_clock.h
