#ifndef CYGONCE_HAL_PLF_INTR_H
#define CYGONCE_HAL_PLF_INTR_H

//==========================================================================
//
//      plf_intr.h
//
//      rtl819x Interrupt and clock support
//
//==========================================================================
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    nickg
// Contributors: nickg, jskov,
//               gthomas, jlarmour, dmoseley, michael anburaj <michaelanburaj@hotmail.com>
// Date:         2000-06-06
// Purpose:      Define Interrupt support
// Description:  The macros defined here provide the HAL APIs for handling
//               interrupts and the clock for the rtl819x board.
//              
// Usage:
//              #include <cyg/hal/plf_intr.h>
//              ...
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/hal.h>

#include <cyg/infra/cyg_type.h>

#include <cyg/hal/bspchip.h>


//--------------------------------------------------------------------------
// Interrupt vectors.

#ifndef CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

// These are decoded via the IP bits of the cause
// register when an external interrupt is delivered.
#define CYGNUM_HAL_INTERRUPT_HW1              32
#define CYGNUM_HAL_INTERRUPT_HW2              33
#define CYGNUM_HAL_INTERRUPT_HW3              34
#define CYGNUM_HAL_INTERRUPT_HW4              35
#define CYGNUM_HAL_INTERRUPT_HW5              36
#define CYGNUM_HAL_SPURIOUS_INTERRUPT         37

#define CYGNUM_HAL_INTERRUPT_UART0            BSP_UART0_IRQ
#define CYGNUM_HAL_INTERRUPT_1                1
#define CYGNUM_HAL_INTERRUPT_SWCORE           BSP_SW_IRQ
#define CYGNUM_HAL_INTERRUPT_PCIE             BSP_PCIE_IRQ
#define CYGNUM_HAL_INTERRUPT_TC0              BSP_TC0_IRQ


// Min/Max ISR numbers
#define CYGNUM_HAL_ISR_MIN                 0
#define CYGNUM_HAL_ISR_MAX                 (BSP_IRQ_CPU_NUM+BSP_IRQ_ICTL_NUM)
#define CYGNUM_HAL_ISR_COUNT               (CYGNUM_HAL_ISR_MAX - CYGNUM_HAL_ISR_MIN + 1)

// The vector used by the Real time clock
#ifdef CONFIG_RTL_819X_INTERNAL_TIMER
#define CYGNUM_HAL_INTERRUPT_RTC           BSP_SITIMER_IRQ
#else
#define CYGNUM_HAL_INTERRUPT_RTC           BSP_TC0_IRQ
#endif

#define CYGHWR_HAL_INTERRUPT_VECTORS_DEFINED

#endif

//--------------------------------------------------------------------------
// Interrupt controller access
// The default code here simply uses the fields present in the CP0 status
// and cause registers to implement this functionality.
// Beware of nops in this code. They fill delay slots and avoid CP0 hazards
// that might otherwise cause following code to run in the wrong state or
// cause a resource conflict.

#ifndef CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED
#define HAL_INTERRUPT_MASK( _vector_ )                                  \
{                                                                       \
    if( (_vector_) < CYGNUM_HAL_INTERRUPT_HW1 )                         \
    {                                                                   \
        REG32(BSP_GIMR) &= ~(1 << (_vector_));                          \
    }                                                                   \
    else if( (_vector_) <= CYGNUM_HAL_INTERRUPT_HW5 )                   \
    {                                                                   \
        cyg_uint32 __vector;                                            \
                                                                        \
        __vector = _vector_ - (CYGNUM_HAL_INTERRUPT_HW1-1);             \
                                                                        \
        asm volatile (                                                  \
            "mfc0   $3,$12\n"                                           \
            "la     $2,0x00000400\n"                                    \
            "sllv   $2,$2,%0\n"                                         \
            "nor    $2,$2,$0\n"                                         \
            "and    $3,$3,$2\n"                                         \
            "mtc0   $3,$12\n"                                           \
            :                                                           \
            : "r"(__vector)                                             \
            : "$2", "$3"                                                \
            );                                                          \
    }                                                                   \
}

#define HAL_INTERRUPT_UNMASK( _vector_ )                                \
{                                                                       \
    if( (_vector_) < CYGNUM_HAL_INTERRUPT_HW1 )                         \
    {                                                                   \
        REG32(BSP_GIMR) |= (1 << (_vector_));                           \
    }                                                                   \
    else if( (_vector_) <= CYGNUM_HAL_INTERRUPT_HW5 )                   \
    {                                                                   \
        cyg_uint32 __vector;                                            \
                                                                        \
        __vector = _vector_ - (CYGNUM_HAL_INTERRUPT_HW1-1);             \
                                                                        \
        asm volatile (                                                  \
            "mfc0   $3,$12\n"                                           \
            "la     $2,0x00000400\n"                                    \
            "sllv   $2,$2,%0\n"                                         \
            "or     $3,$3,$2\n"                                         \
            "mtc0   $3,$12\n"                                           \
            :                                                           \
            : "r"(__vector)                                             \
            : "$2", "$3"                                                \
            );                                                          \
    }                                                                   \
}

#ifdef CONFIG_RTL_819X_INTERNAL_TIMER
#define HAL_INTERRUPT_ACKNOWLEDGE( _vector_ )           \
{                                                       \
}
#else
#define HAL_INTERRUPT_ACKNOWLEDGE( _vector_ )           \
{                                                       \
    if( _vector_ == CYGNUM_HAL_INTERRUPT_RTC )          \
        REG32(BSP_TCIR) |= BSP_TC0IP;                   \
}
#endif

#define HAL_INTERRUPT_CONFIGURE( _vector_, _level_, _up_ )   \
{                                                            \
}

#define HAL_INTERRUPT_SET_LEVEL( _vector_, _level_ )         \
{                                                            \
}

#define CYGHWR_HAL_INTERRUPT_CONTROLLER_ACCESS_DEFINED

#endif


//--------------------------------------------------------------------------
// Clock control
#ifndef CONFIG_RTL_819X_INTERNAL_TIMER
#ifndef CYGHWR_HAL_CLOCK_CONTROL_DEFINED

#define HAL_CLOCK_INITIALIZE( _period_ )				\
{									\
	REG32(BSP_TCIR) = (BSP_TC0IP | BSP_TC1IP);			\
	REG32(BSP_TCCNR) = 0;						\
	REG32(BSP_CDBR)=(CYGNUM_HAL_RTC_DIV_FACTOR) << BSP_DIVF_OFFSET;	\
	REG32(BSP_TC0DATA) = (_period_) << BSP_TCD_OFFSET;		\
	REG32(BSP_TCCNR) = BSP_TC0EN | BSP_TC0MODE_TIMER;		\
	REG32(BSP_TCIR) = BSP_TC0IE;					\
}

#define HAL_CLOCK_RESET( _vector_, _period_ )				\
{									\
}

#define HAL_CLOCK_READ( _pvalue_ )					\
{									\
	*(_pvalue_) = REG32(BSP_TC0CNT);				\
	*(_pvalue_) = (REG32(BSP_TC0CNT) >> 4) & 0x0fffffff;		\
}

#define CYGHWR_HAL_CLOCK_CONTROL_DEFINED

#endif

#if defined(CYGVAR_KERNEL_COUNTERS_CLOCK_LATENCY) && \
    !defined(HAL_CLOCK_LATENCY)
#define HAL_CLOCK_LATENCY( _pvalue_ )    HAL_CLOCK_READ( _pvalue_ )
#endif
#endif

//--------------------------------------------------------------------------
// Control-C support.

#if defined(CYGDBG_HAL_MIPS_DEBUG_GDB_CTRLC_SUPPORT)

#define CYGHWR_HAL_GDB_PORT_VECTOR CYGNUM_HAL_INTERRUPT_UART0

externC cyg_uint32 hal_ctrlc_isr(CYG_ADDRWORD vector, CYG_ADDRWORD data);

#define HAL_CTRLC_ISR hal_ctrlc_isr

#endif


//----------------------------------------------------------------------------
// Reset.
#ifndef CYGHWR_HAL_RESET_DEFINED
extern void hal_rtl819x_reset( void );
#define CYGHWR_HAL_RESET_DEFINED
#define HAL_PLATFORM_RESET()             hal_rtl819x_reset()

#define HAL_PLATFORM_RESET_ENTRY 0xbfc00000

#endif // CYGHWR_HAL_RESET_DEFINED
//--------------------------------------------------------------------------
#endif // ifndef CYGONCE_HAL_PLF_INTR_H
// End of plf_intr.h
