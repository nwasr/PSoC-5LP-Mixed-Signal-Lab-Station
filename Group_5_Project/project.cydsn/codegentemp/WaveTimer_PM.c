/*******************************************************************************
* File Name: WaveTimer_PM.c
* Version 2.80
*
*  Description:
*     This file provides the power management source code to API for the
*     Timer.
*
*   Note:
*     None
*
*******************************************************************************
* Copyright 2008-2017, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
********************************************************************************/

#include "WaveTimer.h"

static WaveTimer_backupStruct WaveTimer_backup;


/*******************************************************************************
* Function Name: WaveTimer_SaveConfig
********************************************************************************
*
* Summary:
*     Save the current user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  WaveTimer_backup:  Variables of this global structure are modified to
*  store the values of non retention configuration registers when Sleep() API is
*  called.
*
*******************************************************************************/
void WaveTimer_SaveConfig(void) 
{
    #if (!WaveTimer_UsingFixedFunction)
        WaveTimer_backup.TimerUdb = WaveTimer_ReadCounter();
        WaveTimer_backup.InterruptMaskValue = WaveTimer_STATUS_MASK;
        #if (WaveTimer_UsingHWCaptureCounter)
            WaveTimer_backup.TimerCaptureCounter = WaveTimer_ReadCaptureCount();
        #endif /* Back Up capture counter register  */

        #if(!WaveTimer_UDB_CONTROL_REG_REMOVED)
            WaveTimer_backup.TimerControlRegister = WaveTimer_ReadControlRegister();
        #endif /* Backup the enable state of the Timer component */
    #endif /* Backup non retention registers in UDB implementation. All fixed function registers are retention */
}


/*******************************************************************************
* Function Name: WaveTimer_RestoreConfig
********************************************************************************
*
* Summary:
*  Restores the current user configuration.
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  WaveTimer_backup:  Variables of this global structure are used to
*  restore the values of non retention registers on wakeup from sleep mode.
*
*******************************************************************************/
void WaveTimer_RestoreConfig(void) 
{   
    #if (!WaveTimer_UsingFixedFunction)

        WaveTimer_WriteCounter(WaveTimer_backup.TimerUdb);
        WaveTimer_STATUS_MASK =WaveTimer_backup.InterruptMaskValue;
        #if (WaveTimer_UsingHWCaptureCounter)
            WaveTimer_SetCaptureCount(WaveTimer_backup.TimerCaptureCounter);
        #endif /* Restore Capture counter register*/

        #if(!WaveTimer_UDB_CONTROL_REG_REMOVED)
            WaveTimer_WriteControlRegister(WaveTimer_backup.TimerControlRegister);
        #endif /* Restore the enable state of the Timer component */
    #endif /* Restore non retention registers in the UDB implementation only */
}


/*******************************************************************************
* Function Name: WaveTimer_Sleep
********************************************************************************
*
* Summary:
*     Stop and Save the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  WaveTimer_backup.TimerEnableState:  Is modified depending on the
*  enable state of the block before entering sleep mode.
*
*******************************************************************************/
void WaveTimer_Sleep(void) 
{
    #if(!WaveTimer_UDB_CONTROL_REG_REMOVED)
        /* Save Counter's enable state */
        if(WaveTimer_CTRL_ENABLE == (WaveTimer_CONTROL & WaveTimer_CTRL_ENABLE))
        {
            /* Timer is enabled */
            WaveTimer_backup.TimerEnableState = 1u;
        }
        else
        {
            /* Timer is disabled */
            WaveTimer_backup.TimerEnableState = 0u;
        }
    #endif /* Back up enable state from the Timer control register */
    WaveTimer_Stop();
    WaveTimer_SaveConfig();
}


/*******************************************************************************
* Function Name: WaveTimer_Wakeup
********************************************************************************
*
* Summary:
*  Restores and enables the user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
* Global variables:
*  WaveTimer_backup.enableState:  Is used to restore the enable state of
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void WaveTimer_Wakeup(void) 
{
    WaveTimer_RestoreConfig();
    #if(!WaveTimer_UDB_CONTROL_REG_REMOVED)
        if(WaveTimer_backup.TimerEnableState == 1u)
        {     /* Enable Timer's operation */
                WaveTimer_Enable();
        } /* Do nothing if Timer was disabled before */
    #endif /* Remove this code section if Control register is removed */
}


/* [] END OF FILE */
