/*******************************************************************************
* File Name: WaveClock.h
* Version 2.20
*
*  Description:
*   Provides the function and constant definitions for the clock component.
*
*  Note:
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(CY_CLOCK_WaveClock_H)
#define CY_CLOCK_WaveClock_H

#include <cytypes.h>
#include <cyfitter.h>


/***************************************
* Conditional Compilation Parameters
***************************************/

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component cy_clock_v2_20 requires cy_boot v3.0 or later
#endif /* (CY_PSOC5LP) */


/***************************************
*        Function Prototypes
***************************************/

void WaveClock_Start(void) ;
void WaveClock_Stop(void) ;

#if(CY_PSOC3 || CY_PSOC5LP)
void WaveClock_StopBlock(void) ;
#endif /* (CY_PSOC3 || CY_PSOC5LP) */

void WaveClock_StandbyPower(uint8 state) ;
void WaveClock_SetDividerRegister(uint16 clkDivider, uint8 restart) 
                                ;
uint16 WaveClock_GetDividerRegister(void) ;
void WaveClock_SetModeRegister(uint8 modeBitMask) ;
void WaveClock_ClearModeRegister(uint8 modeBitMask) ;
uint8 WaveClock_GetModeRegister(void) ;
void WaveClock_SetSourceRegister(uint8 clkSource) ;
uint8 WaveClock_GetSourceRegister(void) ;
#if defined(WaveClock__CFG3)
void WaveClock_SetPhaseRegister(uint8 clkPhase) ;
uint8 WaveClock_GetPhaseRegister(void) ;
#endif /* defined(WaveClock__CFG3) */

#define WaveClock_Enable()                       WaveClock_Start()
#define WaveClock_Disable()                      WaveClock_Stop()
#define WaveClock_SetDivider(clkDivider)         WaveClock_SetDividerRegister(clkDivider, 1u)
#define WaveClock_SetDividerValue(clkDivider)    WaveClock_SetDividerRegister((clkDivider) - 1u, 1u)
#define WaveClock_SetMode(clkMode)               WaveClock_SetModeRegister(clkMode)
#define WaveClock_SetSource(clkSource)           WaveClock_SetSourceRegister(clkSource)
#if defined(WaveClock__CFG3)
#define WaveClock_SetPhase(clkPhase)             WaveClock_SetPhaseRegister(clkPhase)
#define WaveClock_SetPhaseValue(clkPhase)        WaveClock_SetPhaseRegister((clkPhase) + 1u)
#endif /* defined(WaveClock__CFG3) */


/***************************************
*             Registers
***************************************/

/* Register to enable or disable the clock */
#define WaveClock_CLKEN              (* (reg8 *) WaveClock__PM_ACT_CFG)
#define WaveClock_CLKEN_PTR          ((reg8 *) WaveClock__PM_ACT_CFG)

/* Register to enable or disable the clock */
#define WaveClock_CLKSTBY            (* (reg8 *) WaveClock__PM_STBY_CFG)
#define WaveClock_CLKSTBY_PTR        ((reg8 *) WaveClock__PM_STBY_CFG)

/* Clock LSB divider configuration register. */
#define WaveClock_DIV_LSB            (* (reg8 *) WaveClock__CFG0)
#define WaveClock_DIV_LSB_PTR        ((reg8 *) WaveClock__CFG0)
#define WaveClock_DIV_PTR            ((reg16 *) WaveClock__CFG0)

/* Clock MSB divider configuration register. */
#define WaveClock_DIV_MSB            (* (reg8 *) WaveClock__CFG1)
#define WaveClock_DIV_MSB_PTR        ((reg8 *) WaveClock__CFG1)

/* Mode and source configuration register */
#define WaveClock_MOD_SRC            (* (reg8 *) WaveClock__CFG2)
#define WaveClock_MOD_SRC_PTR        ((reg8 *) WaveClock__CFG2)

#if defined(WaveClock__CFG3)
/* Analog clock phase configuration register */
#define WaveClock_PHASE              (* (reg8 *) WaveClock__CFG3)
#define WaveClock_PHASE_PTR          ((reg8 *) WaveClock__CFG3)
#endif /* defined(WaveClock__CFG3) */


/**************************************
*       Register Constants
**************************************/

/* Power manager register masks */
#define WaveClock_CLKEN_MASK         WaveClock__PM_ACT_MSK
#define WaveClock_CLKSTBY_MASK       WaveClock__PM_STBY_MSK

/* CFG2 field masks */
#define WaveClock_SRC_SEL_MSK        WaveClock__CFG2_SRC_SEL_MASK
#define WaveClock_MODE_MASK          (~(WaveClock_SRC_SEL_MSK))

#if defined(WaveClock__CFG3)
/* CFG3 phase mask */
#define WaveClock_PHASE_MASK         WaveClock__CFG3_PHASE_DLY_MASK
#endif /* defined(WaveClock__CFG3) */

#endif /* CY_CLOCK_WaveClock_H */


/* [] END OF FILE */
