/*******************************************************************************
* File Name: IDAC_1.c
* Version 2.0
*
* Description:
*  This file provides the power management source code to API for the
*  IDAC8.
*
* Note:
*  None
*
********************************************************************************
* Copyright 2008-2012, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions, 
* disclaimers, and limitations in the end user license agreement accompanying 
* the software package with which this file was provided.
*******************************************************************************/


#include "IDAC_1.h"

static IDAC_1_backupStruct IDAC_1_backup;


/*******************************************************************************
* Function Name: IDAC_1_SaveConfig
********************************************************************************
* Summary:
*  Save the current user configuration
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void IDAC_1_SaveConfig(void) 
{
    if (!((IDAC_1_CR1 & IDAC_1_SRC_MASK) == IDAC_1_SRC_UDB))
    {
        IDAC_1_backup.data_value = IDAC_1_Data;
    }
}


/*******************************************************************************
* Function Name: IDAC_1_RestoreConfig
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
*******************************************************************************/
void IDAC_1_RestoreConfig(void) 
{
    if (!((IDAC_1_CR1 & IDAC_1_SRC_MASK) == IDAC_1_SRC_UDB))
    {
        if((IDAC_1_Strobe & IDAC_1_STRB_MASK) == IDAC_1_STRB_EN)
        {
            IDAC_1_Strobe &= (uint8)(~IDAC_1_STRB_MASK);
            IDAC_1_Data = IDAC_1_backup.data_value;
            IDAC_1_Strobe |= IDAC_1_STRB_EN;
        }
        else
        {
            IDAC_1_Data = IDAC_1_backup.data_value;
        }
    }
}


/*******************************************************************************
* Function Name: IDAC_1_Sleep
********************************************************************************
* Summary:
*  Stop and Save the user configuration
*
* Parameters:
*  void:
*
* Return:
*  void
*
* Global variables:
*  IDAC_1_backup.enableState: Is modified depending on the enable 
*  state of the block before entering sleep mode.
*
*******************************************************************************/
void IDAC_1_Sleep(void) 
{
    if(IDAC_1_ACT_PWR_EN == (IDAC_1_PWRMGR & IDAC_1_ACT_PWR_EN))
    {
        /* IDAC8 is enabled */
        IDAC_1_backup.enableState = 1u;
    }
    else
    {
        /* IDAC8 is disabled */
        IDAC_1_backup.enableState = 0u;
    }

    IDAC_1_Stop();
    IDAC_1_SaveConfig();
}


/*******************************************************************************
* Function Name: IDAC_1_Wakeup
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
*  IDAC_1_backup.enableState: Is used to restore the enable state of 
*  block on wakeup from sleep mode.
*
*******************************************************************************/
void IDAC_1_Wakeup(void) 
{
    IDAC_1_RestoreConfig();
    
    if(IDAC_1_backup.enableState == 1u)
    {
        /* Enable IDAC8's operation */
        IDAC_1_Enable();
        
        /* Set the data register */
        IDAC_1_SetValue(IDAC_1_Data);
    } /* Do nothing if IDAC8 was disabled before */    
}


/* [] END OF FILE */
