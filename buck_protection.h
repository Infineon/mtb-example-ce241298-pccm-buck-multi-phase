/*******************************************************************************
* File Name: buck_protection.h
*
* Description:
* This is a user-defined header file for call back functions with protection
* conditions. These functions will get called from control loop in the generated
* code.
*******************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#ifndef BUCK_PROTECTION_H
#define BUCK_PROTECTION_H
#include "cybsp.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* buck converter states */
typedef enum Ifx_buck_states
{
    Ifx_BUCK_STATE_IDLE      = 0,
    Ifx_BUCK_STATE_RAMP      = 1,
    Ifx_BUCK_STATE_RUN       = 2,
    Ifx_BUCK_STATE_TEST      = 3,
    Ifx_BUCK_STATE_FAULT     = 4
}Ifx_buck_states;

/* Number of samples for averaging the parameters used for overload protection */
#define AVERAGING_SAMPLES     (8U)

/* input voltage */
#define VIN_COUNT             (1906)       /* ADC count for input voltage - 24v*/

/* Counters values for LED operation */
#define CLR_LED    (0)
#define SET_LED    (10000)
#define TOGGLE_LED (5000)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Variables used for Protection for converter 1*/
extern float32_t buck1_iout1_adc_res;
extern float32_t buck1_iout2_adc_res;
extern float32_t buck1_temp_adc_res;
extern float32_t vin_adc_res;
extern float32_t buck1_iout1_avg;
extern float32_t buck1_iout2_avg;
extern float32_t buck1_temp_avg;
extern float32_t vin_avg;

/* state variable */
extern Ifx_buck_states buck_state;

/* Interrupt configuration structure of button GPIO. */
extern cy_stc_sysint_t button_press_intr_config;

/*******************************************************************************
* Function Name: fault_processing
*********************************************************************************
* Summary:
* This function is executes when a fault is detected. It disables
* the buck converter and changes the state.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
__STATIC_INLINE void fault_processing(void)
{
    /* Result variable */
    cy_rslt_t result;

    /* Disable the buck converter when protection condition passed. */
    /* Stops the buck converter. */
    result = BUCK1_disable();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Disable the transient pulses. If it is running. */
    Cy_TCPWM_TriggerStopOrKill_Single(PWM_LOAD_HW, PWM_LOAD_NUM);

    /* Turn on Fault LED. */
    Cy_GPIO_Clr(FAULT_LED_PORT, FAULT_LED_NUM);

    /* Stop Run LED */
    Cy_TCPWM_PWM_SetCompare0Val(PWM_ACT_LED_HW, PWM_ACT_LED_NUM, CLR_LED);

    /* Enables button IRQ after fault event*/
    NVIC_EnableIRQ(button_press_intr_config.intrSrc);

    /* Reset buck converter state to idle. */
    buck_state = Ifx_BUCK_STATE_FAULT;
}

/*******************************************************************************
* Function Name: Buck 1 fault callback
*********************************************************************************
* Summary:
* This function is executes when a buck 1 vout fault is detected. It disables
* the buck converter and changes the state.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
__STATIC_INLINE void buck1_fault_callback(void)
{
    /*Fault processing after detection of the fault*/
    fault_processing();
}

/*******************************************************************************

* Function Name: buck1_scheduled_adc_callback
*********************************************************************************
* Summary:
* This is the scheduled adc callback from the buck1 scheduled ISR. In this
* function, protection logic for buck1 is implemented.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/

__STATIC_INLINE void buck1_scheduled_adc_callback(void)
{
    /* Read result from ADC result register. */
    vin_adc_res        = BUCK1_Vin_get_result();
    buck1_iout1_adc_res = BUCK1_Iout1_get_result();
    buck1_iout2_adc_res = BUCK1_Iout2_get_result();
    buck1_temp_adc_res = BUCK1_Temp_get_result();

    /*Moving Average calculation*/
    buck1_iout1_avg = (float32_t)((buck1_iout1_avg - ((buck1_iout1_avg - buck1_iout1_adc_res) / AVERAGING_SAMPLES)));
    buck1_iout2_avg = (float32_t)((buck1_iout2_avg - ((buck1_iout2_avg - buck1_iout2_adc_res) / AVERAGING_SAMPLES)));
    buck1_temp_avg = (float32_t)((buck1_temp_avg - ((buck1_temp_avg - buck1_temp_adc_res) / AVERAGING_SAMPLES)));
    vin_avg        = (float32_t)((vin_avg        - ((vin_avg        - vin_adc_res)        / AVERAGING_SAMPLES)));


    /* Check for vin voltage, output current and temperature range */
    if((vin_avg < BUCK1_Vin_MIN) || (vin_avg > BUCK1_Vin_MAX) ||
       (buck1_iout1_avg > BUCK1_Iout1_MAX) ||
       (buck1_iout2_avg > BUCK1_Iout2_MAX) ||
       (buck1_temp_avg > BUCK1_Temp_MAX))
    {
        /*Fault processing after detection of the fault*/
        fault_processing();
    }
}

#endif  /* BUCK_PROTECTION_H */
/* [] END OF FILE */
