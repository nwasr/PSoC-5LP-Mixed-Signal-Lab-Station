/*******************************************************************************
* File Name: WaveTimer.h
* Version 2.80
*
*  Description:
*     Contains the function prototypes and constants available to the timer
*     user module.
*
*   Note:
*     None
*
********************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#if !defined(CY_TIMER_WaveTimer_H)
#define CY_TIMER_WaveTimer_H

#include "cytypes.h"
#include "cyfitter.h"
#include "CyLib.h" /* For CyEnterCriticalSection() and CyExitCriticalSection() functions */

extern uint8 WaveTimer_initVar;

/* Check to see if required defines such as CY_PSOC5LP are available */
/* They are defined starting with cy_boot v3.0 */
#if !defined (CY_PSOC5LP)
    #error Component Timer_v2_80 requires cy_boot v3.0 or later
#endif /* (CY_ PSOC5LP) */


/**************************************
*           Parameter Defaults
**************************************/

#define WaveTimer_Resolution                 16u
#define WaveTimer_UsingFixedFunction         0u
#define WaveTimer_UsingHWCaptureCounter      0u
#define WaveTimer_SoftwareCaptureMode        0u
#define WaveTimer_SoftwareTriggerMode        0u
#define WaveTimer_UsingHWEnable              0u
#define WaveTimer_EnableTriggerMode          0u
#define WaveTimer_InterruptOnCaptureCount    0u
#define WaveTimer_RunModeUsed                0u
#define WaveTimer_ControlRegRemoved          0u

#if defined(WaveTimer_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG)
    #define WaveTimer_UDB_CONTROL_REG_REMOVED            (0u)
#elif  (WaveTimer_UsingFixedFunction)
    #define WaveTimer_UDB_CONTROL_REG_REMOVED            (0u)
#else 
    #define WaveTimer_UDB_CONTROL_REG_REMOVED            (1u)
#endif /* End WaveTimer_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG */


/***************************************
*       Type defines
***************************************/


/**************************************************************************
 * Sleep Wakeup Backup structure for Timer Component
 *************************************************************************/
typedef struct
{
    uint8 TimerEnableState;
    #if(!WaveTimer_UsingFixedFunction)

        uint16 TimerUdb;
        uint8 InterruptMaskValue;
        #if (WaveTimer_UsingHWCaptureCounter)
            uint8 TimerCaptureCounter;
        #endif /* variable declarations for backing up non retention registers in CY_UDB_V1 */

        #if (!WaveTimer_UDB_CONTROL_REG_REMOVED)
            uint8 TimerControlRegister;
        #endif /* variable declaration for backing up enable state of the Timer */
    #endif /* define backup variables only for UDB implementation. Fixed function registers are all retention */

}WaveTimer_backupStruct;


/***************************************
*       Function Prototypes
***************************************/

void    WaveTimer_Start(void) ;
void    WaveTimer_Stop(void) ;

void    WaveTimer_SetInterruptMode(uint8 interruptMode) ;
uint8   WaveTimer_ReadStatusRegister(void) ;
/* Deprecated function. Do not use this in future. Retained for backward compatibility */
#define WaveTimer_GetInterruptSource() WaveTimer_ReadStatusRegister()

#if(!WaveTimer_UDB_CONTROL_REG_REMOVED)
    uint8   WaveTimer_ReadControlRegister(void) ;
    void    WaveTimer_WriteControlRegister(uint8 control) ;
#endif /* (!WaveTimer_UDB_CONTROL_REG_REMOVED) */

uint16  WaveTimer_ReadPeriod(void) ;
void    WaveTimer_WritePeriod(uint16 period) ;
uint16  WaveTimer_ReadCounter(void) ;
void    WaveTimer_WriteCounter(uint16 counter) ;
uint16  WaveTimer_ReadCapture(void) ;
void    WaveTimer_SoftwareCapture(void) ;

#if(!WaveTimer_UsingFixedFunction) /* UDB Prototypes */
    #if (WaveTimer_SoftwareCaptureMode)
        void    WaveTimer_SetCaptureMode(uint8 captureMode) ;
    #endif /* (!WaveTimer_UsingFixedFunction) */

    #if (WaveTimer_SoftwareTriggerMode)
        void    WaveTimer_SetTriggerMode(uint8 triggerMode) ;
    #endif /* (WaveTimer_SoftwareTriggerMode) */

    #if (WaveTimer_EnableTriggerMode)
        void    WaveTimer_EnableTrigger(void) ;
        void    WaveTimer_DisableTrigger(void) ;
    #endif /* (WaveTimer_EnableTriggerMode) */


    #if(WaveTimer_InterruptOnCaptureCount)
        void    WaveTimer_SetInterruptCount(uint8 interruptCount) ;
    #endif /* (WaveTimer_InterruptOnCaptureCount) */

    #if (WaveTimer_UsingHWCaptureCounter)
        void    WaveTimer_SetCaptureCount(uint8 captureCount) ;
        uint8   WaveTimer_ReadCaptureCount(void) ;
    #endif /* (WaveTimer_UsingHWCaptureCounter) */

    void WaveTimer_ClearFIFO(void) ;
#endif /* UDB Prototypes */

/* Sleep Retention APIs */
void WaveTimer_Init(void)          ;
void WaveTimer_Enable(void)        ;
void WaveTimer_SaveConfig(void)    ;
void WaveTimer_RestoreConfig(void) ;
void WaveTimer_Sleep(void)         ;
void WaveTimer_Wakeup(void)        ;


/***************************************
*   Enumerated Types and Parameters
***************************************/

/* Enumerated Type B_Timer__CaptureModes, Used in Capture Mode */
#define WaveTimer__B_TIMER__CM_NONE 0
#define WaveTimer__B_TIMER__CM_RISINGEDGE 1
#define WaveTimer__B_TIMER__CM_FALLINGEDGE 2
#define WaveTimer__B_TIMER__CM_EITHEREDGE 3
#define WaveTimer__B_TIMER__CM_SOFTWARE 4



/* Enumerated Type B_Timer__TriggerModes, Used in Trigger Mode */
#define WaveTimer__B_TIMER__TM_NONE 0x00u
#define WaveTimer__B_TIMER__TM_RISINGEDGE 0x04u
#define WaveTimer__B_TIMER__TM_FALLINGEDGE 0x08u
#define WaveTimer__B_TIMER__TM_EITHEREDGE 0x0Cu
#define WaveTimer__B_TIMER__TM_SOFTWARE 0x10u


/***************************************
*    Initialial Parameter Constants
***************************************/

#define WaveTimer_INIT_PERIOD             998u
#define WaveTimer_INIT_CAPTURE_MODE       ((uint8)((uint8)1u << WaveTimer_CTRL_CAP_MODE_SHIFT))
#define WaveTimer_INIT_TRIGGER_MODE       ((uint8)((uint8)0u << WaveTimer_CTRL_TRIG_MODE_SHIFT))
#if (WaveTimer_UsingFixedFunction)
    #define WaveTimer_INIT_INTERRUPT_MODE (((uint8)((uint8)1u << WaveTimer_STATUS_TC_INT_MASK_SHIFT)) | \
                                                  ((uint8)((uint8)0 << WaveTimer_STATUS_CAPTURE_INT_MASK_SHIFT)))
#else
    #define WaveTimer_INIT_INTERRUPT_MODE (((uint8)((uint8)1u << WaveTimer_STATUS_TC_INT_MASK_SHIFT)) | \
                                                 ((uint8)((uint8)0 << WaveTimer_STATUS_CAPTURE_INT_MASK_SHIFT)) | \
                                                 ((uint8)((uint8)0 << WaveTimer_STATUS_FIFOFULL_INT_MASK_SHIFT)))
#endif /* (WaveTimer_UsingFixedFunction) */
#define WaveTimer_INIT_CAPTURE_COUNT      (2u)
#define WaveTimer_INIT_INT_CAPTURE_COUNT  ((uint8)((uint8)(1u - 1u) << WaveTimer_CTRL_INTCNT_SHIFT))


/***************************************
*           Registers
***************************************/

#if (WaveTimer_UsingFixedFunction) /* Implementation Specific Registers and Register Constants */


    /***************************************
    *    Fixed Function Registers
    ***************************************/

    #define WaveTimer_STATUS         (*(reg8 *) WaveTimer_TimerHW__SR0 )
    /* In Fixed Function Block Status and Mask are the same register */
    #define WaveTimer_STATUS_MASK    (*(reg8 *) WaveTimer_TimerHW__SR0 )
    #define WaveTimer_CONTROL        (*(reg8 *) WaveTimer_TimerHW__CFG0)
    #define WaveTimer_CONTROL2       (*(reg8 *) WaveTimer_TimerHW__CFG1)
    #define WaveTimer_CONTROL2_PTR   ( (reg8 *) WaveTimer_TimerHW__CFG1)
    #define WaveTimer_RT1            (*(reg8 *) WaveTimer_TimerHW__RT1)
    #define WaveTimer_RT1_PTR        ( (reg8 *) WaveTimer_TimerHW__RT1)

    #if (CY_PSOC3 || CY_PSOC5LP)
        #define WaveTimer_CONTROL3       (*(reg8 *) WaveTimer_TimerHW__CFG2)
        #define WaveTimer_CONTROL3_PTR   ( (reg8 *) WaveTimer_TimerHW__CFG2)
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */
    #define WaveTimer_GLOBAL_ENABLE  (*(reg8 *) WaveTimer_TimerHW__PM_ACT_CFG)
    #define WaveTimer_GLOBAL_STBY_ENABLE  (*(reg8 *) WaveTimer_TimerHW__PM_STBY_CFG)

    #define WaveTimer_CAPTURE_LSB         (* (reg16 *) WaveTimer_TimerHW__CAP0 )
    #define WaveTimer_CAPTURE_LSB_PTR       ((reg16 *) WaveTimer_TimerHW__CAP0 )
    #define WaveTimer_PERIOD_LSB          (* (reg16 *) WaveTimer_TimerHW__PER0 )
    #define WaveTimer_PERIOD_LSB_PTR        ((reg16 *) WaveTimer_TimerHW__PER0 )
    #define WaveTimer_COUNTER_LSB         (* (reg16 *) WaveTimer_TimerHW__CNT_CMP0 )
    #define WaveTimer_COUNTER_LSB_PTR       ((reg16 *) WaveTimer_TimerHW__CNT_CMP0 )


    /***************************************
    *    Register Constants
    ***************************************/

    /* Fixed Function Block Chosen */
    #define WaveTimer_BLOCK_EN_MASK                     WaveTimer_TimerHW__PM_ACT_MSK
    #define WaveTimer_BLOCK_STBY_EN_MASK                WaveTimer_TimerHW__PM_STBY_MSK

    /* Control Register Bit Locations */
    /* Interrupt Count - Not valid for Fixed Function Block */
    #define WaveTimer_CTRL_INTCNT_SHIFT                  0x00u
    /* Trigger Polarity - Not valid for Fixed Function Block */
    #define WaveTimer_CTRL_TRIG_MODE_SHIFT               0x00u
    /* Trigger Enable - Not valid for Fixed Function Block */
    #define WaveTimer_CTRL_TRIG_EN_SHIFT                 0x00u
    /* Capture Polarity - Not valid for Fixed Function Block */
    #define WaveTimer_CTRL_CAP_MODE_SHIFT                0x00u
    /* Timer Enable - As defined in Register Map, part of TMRX_CFG0 register */
    #define WaveTimer_CTRL_ENABLE_SHIFT                  0x00u

    /* Control Register Bit Masks */
    #define WaveTimer_CTRL_ENABLE                        ((uint8)((uint8)0x01u << WaveTimer_CTRL_ENABLE_SHIFT))

    /* Control2 Register Bit Masks */
    /* As defined in Register Map, Part of the TMRX_CFG1 register */
    #define WaveTimer_CTRL2_IRQ_SEL_SHIFT                 0x00u
    #define WaveTimer_CTRL2_IRQ_SEL                      ((uint8)((uint8)0x01u << WaveTimer_CTRL2_IRQ_SEL_SHIFT))

    #if (CY_PSOC5A)
        /* Use CFG1 Mode bits to set run mode */
        /* As defined by Verilog Implementation */
        #define WaveTimer_CTRL_MODE_SHIFT                 0x01u
        #define WaveTimer_CTRL_MODE_MASK                 ((uint8)((uint8)0x07u << WaveTimer_CTRL_MODE_SHIFT))
    #endif /* (CY_PSOC5A) */
    #if (CY_PSOC3 || CY_PSOC5LP)
        /* Control3 Register Bit Locations */
        #define WaveTimer_CTRL_RCOD_SHIFT        0x02u
        #define WaveTimer_CTRL_ENBL_SHIFT        0x00u
        #define WaveTimer_CTRL_MODE_SHIFT        0x00u

        /* Control3 Register Bit Masks */
        #define WaveTimer_CTRL_RCOD_MASK  ((uint8)((uint8)0x03u << WaveTimer_CTRL_RCOD_SHIFT)) /* ROD and COD bit masks */
        #define WaveTimer_CTRL_ENBL_MASK  ((uint8)((uint8)0x80u << WaveTimer_CTRL_ENBL_SHIFT)) /* HW_EN bit mask */
        #define WaveTimer_CTRL_MODE_MASK  ((uint8)((uint8)0x03u << WaveTimer_CTRL_MODE_SHIFT)) /* Run mode bit mask */

        #define WaveTimer_CTRL_RCOD       ((uint8)((uint8)0x03u << WaveTimer_CTRL_RCOD_SHIFT))
        #define WaveTimer_CTRL_ENBL       ((uint8)((uint8)0x80u << WaveTimer_CTRL_ENBL_SHIFT))
    #endif /* (CY_PSOC3 || CY_PSOC5LP) */

    /*RT1 Synch Constants: Applicable for PSoC3 and PSoC5LP */
    #define WaveTimer_RT1_SHIFT                       0x04u
    /* Sync TC and CMP bit masks */
    #define WaveTimer_RT1_MASK                        ((uint8)((uint8)0x03u << WaveTimer_RT1_SHIFT))
    #define WaveTimer_SYNC                            ((uint8)((uint8)0x03u << WaveTimer_RT1_SHIFT))
    #define WaveTimer_SYNCDSI_SHIFT                   0x00u
    /* Sync all DSI inputs with Mask  */
    #define WaveTimer_SYNCDSI_MASK                    ((uint8)((uint8)0x0Fu << WaveTimer_SYNCDSI_SHIFT))
    /* Sync all DSI inputs */
    #define WaveTimer_SYNCDSI_EN                      ((uint8)((uint8)0x0Fu << WaveTimer_SYNCDSI_SHIFT))

    #define WaveTimer_CTRL_MODE_PULSEWIDTH            ((uint8)((uint8)0x01u << WaveTimer_CTRL_MODE_SHIFT))
    #define WaveTimer_CTRL_MODE_PERIOD                ((uint8)((uint8)0x02u << WaveTimer_CTRL_MODE_SHIFT))
    #define WaveTimer_CTRL_MODE_CONTINUOUS            ((uint8)((uint8)0x00u << WaveTimer_CTRL_MODE_SHIFT))

    /* Status Register Bit Locations */
    /* As defined in Register Map, part of TMRX_SR0 register */
    #define WaveTimer_STATUS_TC_SHIFT                 0x07u
    /* As defined in Register Map, part of TMRX_SR0 register, Shared with Compare Status */
    #define WaveTimer_STATUS_CAPTURE_SHIFT            0x06u
    /* As defined in Register Map, part of TMRX_SR0 register */
    #define WaveTimer_STATUS_TC_INT_MASK_SHIFT        (WaveTimer_STATUS_TC_SHIFT - 0x04u)
    /* As defined in Register Map, part of TMRX_SR0 register, Shared with Compare Status */
    #define WaveTimer_STATUS_CAPTURE_INT_MASK_SHIFT   (WaveTimer_STATUS_CAPTURE_SHIFT - 0x04u)

    /* Status Register Bit Masks */
    #define WaveTimer_STATUS_TC                       ((uint8)((uint8)0x01u << WaveTimer_STATUS_TC_SHIFT))
    #define WaveTimer_STATUS_CAPTURE                  ((uint8)((uint8)0x01u << WaveTimer_STATUS_CAPTURE_SHIFT))
    /* Interrupt Enable Bit-Mask for interrupt on TC */
    #define WaveTimer_STATUS_TC_INT_MASK              ((uint8)((uint8)0x01u << WaveTimer_STATUS_TC_INT_MASK_SHIFT))
    /* Interrupt Enable Bit-Mask for interrupt on Capture */
    #define WaveTimer_STATUS_CAPTURE_INT_MASK         ((uint8)((uint8)0x01u << WaveTimer_STATUS_CAPTURE_INT_MASK_SHIFT))

#else   /* UDB Registers and Register Constants */


    /***************************************
    *           UDB Registers
    ***************************************/

    #define WaveTimer_STATUS              (* (reg8 *) WaveTimer_TimerUDB_rstSts_stsreg__STATUS_REG )
    #define WaveTimer_STATUS_MASK         (* (reg8 *) WaveTimer_TimerUDB_rstSts_stsreg__MASK_REG)
    #define WaveTimer_STATUS_AUX_CTRL     (* (reg8 *) WaveTimer_TimerUDB_rstSts_stsreg__STATUS_AUX_CTL_REG)
    #define WaveTimer_CONTROL             (* (reg8 *) WaveTimer_TimerUDB_sCTRLReg_SyncCtl_ctrlreg__CONTROL_REG )
    
    #if(WaveTimer_Resolution <= 8u) /* 8-bit Timer */
        #define WaveTimer_CAPTURE_LSB         (* (reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define WaveTimer_CAPTURE_LSB_PTR       ((reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define WaveTimer_PERIOD_LSB          (* (reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define WaveTimer_PERIOD_LSB_PTR        ((reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define WaveTimer_COUNTER_LSB         (* (reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
        #define WaveTimer_COUNTER_LSB_PTR       ((reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
    #elif(WaveTimer_Resolution <= 16u) /* 8-bit Timer */
        #if(CY_PSOC3) /* 8-bit addres space */
            #define WaveTimer_CAPTURE_LSB         (* (reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define WaveTimer_CAPTURE_LSB_PTR       ((reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define WaveTimer_PERIOD_LSB          (* (reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define WaveTimer_PERIOD_LSB_PTR        ((reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define WaveTimer_COUNTER_LSB         (* (reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
            #define WaveTimer_COUNTER_LSB_PTR       ((reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
        #else /* 16-bit address space */
            #define WaveTimer_CAPTURE_LSB         (* (reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__16BIT_F0_REG )
            #define WaveTimer_CAPTURE_LSB_PTR       ((reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__16BIT_F0_REG )
            #define WaveTimer_PERIOD_LSB          (* (reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__16BIT_D0_REG )
            #define WaveTimer_PERIOD_LSB_PTR        ((reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__16BIT_D0_REG )
            #define WaveTimer_COUNTER_LSB         (* (reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__16BIT_A0_REG )
            #define WaveTimer_COUNTER_LSB_PTR       ((reg16 *) WaveTimer_TimerUDB_sT16_timerdp_u0__16BIT_A0_REG )
        #endif /* CY_PSOC3 */
    #elif(WaveTimer_Resolution <= 24u)/* 24-bit Timer */
        #define WaveTimer_CAPTURE_LSB         (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define WaveTimer_CAPTURE_LSB_PTR       ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
        #define WaveTimer_PERIOD_LSB          (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define WaveTimer_PERIOD_LSB_PTR        ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
        #define WaveTimer_COUNTER_LSB         (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
        #define WaveTimer_COUNTER_LSB_PTR       ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
    #else /* 32-bit Timer */
        #if(CY_PSOC3 || CY_PSOC5) /* 8-bit address space */
            #define WaveTimer_CAPTURE_LSB         (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define WaveTimer_CAPTURE_LSB_PTR       ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__F0_REG )
            #define WaveTimer_PERIOD_LSB          (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define WaveTimer_PERIOD_LSB_PTR        ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__D0_REG )
            #define WaveTimer_COUNTER_LSB         (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
            #define WaveTimer_COUNTER_LSB_PTR       ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
        #else /* 32-bit address space */
            #define WaveTimer_CAPTURE_LSB         (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__32BIT_F0_REG )
            #define WaveTimer_CAPTURE_LSB_PTR       ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__32BIT_F0_REG )
            #define WaveTimer_PERIOD_LSB          (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__32BIT_D0_REG )
            #define WaveTimer_PERIOD_LSB_PTR        ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__32BIT_D0_REG )
            #define WaveTimer_COUNTER_LSB         (* (reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__32BIT_A0_REG )
            #define WaveTimer_COUNTER_LSB_PTR       ((reg32 *) WaveTimer_TimerUDB_sT16_timerdp_u0__32BIT_A0_REG )
        #endif /* CY_PSOC3 || CY_PSOC5 */ 
    #endif

    #define WaveTimer_COUNTER_LSB_PTR_8BIT       ((reg8 *) WaveTimer_TimerUDB_sT16_timerdp_u0__A0_REG )
    
    #if (WaveTimer_UsingHWCaptureCounter)
        #define WaveTimer_CAP_COUNT              (*(reg8 *) WaveTimer_TimerUDB_sCapCount_counter__PERIOD_REG )
        #define WaveTimer_CAP_COUNT_PTR          ( (reg8 *) WaveTimer_TimerUDB_sCapCount_counter__PERIOD_REG )
        #define WaveTimer_CAPTURE_COUNT_CTRL     (*(reg8 *) WaveTimer_TimerUDB_sCapCount_counter__CONTROL_AUX_CTL_REG )
        #define WaveTimer_CAPTURE_COUNT_CTRL_PTR ( (reg8 *) WaveTimer_TimerUDB_sCapCount_counter__CONTROL_AUX_CTL_REG )
    #endif /* (WaveTimer_UsingHWCaptureCounter) */


    /***************************************
    *       Register Constants
    ***************************************/

    /* Control Register Bit Locations */
    #define WaveTimer_CTRL_INTCNT_SHIFT              0x00u       /* As defined by Verilog Implementation */
    #define WaveTimer_CTRL_TRIG_MODE_SHIFT           0x02u       /* As defined by Verilog Implementation */
    #define WaveTimer_CTRL_TRIG_EN_SHIFT             0x04u       /* As defined by Verilog Implementation */
    #define WaveTimer_CTRL_CAP_MODE_SHIFT            0x05u       /* As defined by Verilog Implementation */
    #define WaveTimer_CTRL_ENABLE_SHIFT              0x07u       /* As defined by Verilog Implementation */

    /* Control Register Bit Masks */
    #define WaveTimer_CTRL_INTCNT_MASK               ((uint8)((uint8)0x03u << WaveTimer_CTRL_INTCNT_SHIFT))
    #define WaveTimer_CTRL_TRIG_MODE_MASK            ((uint8)((uint8)0x03u << WaveTimer_CTRL_TRIG_MODE_SHIFT))
    #define WaveTimer_CTRL_TRIG_EN                   ((uint8)((uint8)0x01u << WaveTimer_CTRL_TRIG_EN_SHIFT))
    #define WaveTimer_CTRL_CAP_MODE_MASK             ((uint8)((uint8)0x03u << WaveTimer_CTRL_CAP_MODE_SHIFT))
    #define WaveTimer_CTRL_ENABLE                    ((uint8)((uint8)0x01u << WaveTimer_CTRL_ENABLE_SHIFT))

    /* Bit Counter (7-bit) Control Register Bit Definitions */
    /* As defined by the Register map for the AUX Control Register */
    #define WaveTimer_CNTR_ENABLE                    0x20u

    /* Status Register Bit Locations */
    #define WaveTimer_STATUS_TC_SHIFT                0x00u  /* As defined by Verilog Implementation */
    #define WaveTimer_STATUS_CAPTURE_SHIFT           0x01u  /* As defined by Verilog Implementation */
    #define WaveTimer_STATUS_TC_INT_MASK_SHIFT       WaveTimer_STATUS_TC_SHIFT
    #define WaveTimer_STATUS_CAPTURE_INT_MASK_SHIFT  WaveTimer_STATUS_CAPTURE_SHIFT
    #define WaveTimer_STATUS_FIFOFULL_SHIFT          0x02u  /* As defined by Verilog Implementation */
    #define WaveTimer_STATUS_FIFONEMP_SHIFT          0x03u  /* As defined by Verilog Implementation */
    #define WaveTimer_STATUS_FIFOFULL_INT_MASK_SHIFT WaveTimer_STATUS_FIFOFULL_SHIFT

    /* Status Register Bit Masks */
    /* Sticky TC Event Bit-Mask */
    #define WaveTimer_STATUS_TC                      ((uint8)((uint8)0x01u << WaveTimer_STATUS_TC_SHIFT))
    /* Sticky Capture Event Bit-Mask */
    #define WaveTimer_STATUS_CAPTURE                 ((uint8)((uint8)0x01u << WaveTimer_STATUS_CAPTURE_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define WaveTimer_STATUS_TC_INT_MASK             ((uint8)((uint8)0x01u << WaveTimer_STATUS_TC_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define WaveTimer_STATUS_CAPTURE_INT_MASK        ((uint8)((uint8)0x01u << WaveTimer_STATUS_CAPTURE_SHIFT))
    /* NOT-Sticky FIFO Full Bit-Mask */
    #define WaveTimer_STATUS_FIFOFULL                ((uint8)((uint8)0x01u << WaveTimer_STATUS_FIFOFULL_SHIFT))
    /* NOT-Sticky FIFO Not Empty Bit-Mask */
    #define WaveTimer_STATUS_FIFONEMP                ((uint8)((uint8)0x01u << WaveTimer_STATUS_FIFONEMP_SHIFT))
    /* Interrupt Enable Bit-Mask */
    #define WaveTimer_STATUS_FIFOFULL_INT_MASK       ((uint8)((uint8)0x01u << WaveTimer_STATUS_FIFOFULL_SHIFT))

    #define WaveTimer_STATUS_ACTL_INT_EN             0x10u   /* As defined for the ACTL Register */

    /* Datapath Auxillary Control Register definitions */
    #define WaveTimer_AUX_CTRL_FIFO0_CLR             0x01u   /* As defined by Register map */
    #define WaveTimer_AUX_CTRL_FIFO1_CLR             0x02u   /* As defined by Register map */
    #define WaveTimer_AUX_CTRL_FIFO0_LVL             0x04u   /* As defined by Register map */
    #define WaveTimer_AUX_CTRL_FIFO1_LVL             0x08u   /* As defined by Register map */
    #define WaveTimer_STATUS_ACTL_INT_EN_MASK        0x10u   /* As defined for the ACTL Register */

#endif /* Implementation Specific Registers and Register Constants */

#endif  /* CY_TIMER_WaveTimer_H */


/* [] END OF FILE */
