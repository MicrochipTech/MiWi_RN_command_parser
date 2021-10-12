/**
* \file  star_demo.c
*
* \brief Demo Application for MiWi Star Implementation
*
* Copyright (c) 2018 - 2019 Microchip Technology Inc. and its subsidiaries.
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products.
* It is your responsibility to comply with third party license terms applicable
* to your use of third party software (including open source software) that
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/

/************************ HEADERS ****************************************/
#include "miwi_api.h"
#include "miwi_p2p_star.h"
#include "task.h"
#include "star_demo.h"
#include "mimem.h"
#include "asf.h"
#if defined(ENABLE_SLEEP_FEATURE)
#include "sleep_mgr.h"
#endif
#if defined (ENABLE_CONSOLE)
#include "sio2host.h"
#endif

#ifdef MIWI_RN_CMD
#include "rn_cmd.h"
#endif

#if defined(ENABLE_NETWORK_FREEZER)
#include "pdsDataServer.h"
#include "wlPdsTaskManager.h"
#endif

#if defined(PROTOCOL_STAR)
/************************ LOCAL VARIABLES ****************************************/
uint8_t i;
uint8_t TxSynCount = 0;    // Maintain the Count on TX's Done
uint8_t TxSynCount2 = 0; // Maintain the Count on TX's Done
uint8_t TxNum = 0;         // Maintain the Count on TX's Done
uint8_t RxNum = 0;         // Maintain the Count on RX's Done
/* check for selections made by USER */
bool chk_sel_status;
uint8_t NumOfActiveScanResponse;
bool update_ed;
uint8_t select_ed;
uint8_t msghandledemo = 0;
extern uint8_t myChannel;
/* Connection Table Memory */
extern CONNECTION_ENTRY connectionTable[CONNECTION_SIZE];
bool display_connections;
MIWI_TICK t1 , t2;

/************************ FUNCTION DEFINITIONS ****************************************/
/*********************************************************************
* Function: static void dataConfcb(uint8_t handle, miwi_status_t status)
*
* Overview: Confirmation Callback for MiApp_SendData
*
* Parameters:  handle - message handle, miwi_status_t status of data send
****************************************************************************/
#ifdef MIWI_RN_CMD
void dataConfcb(uint8_t handle, miwi_status_t status, uint8_t* msgPointer)
#else
static void dataConfcb(uint8_t handle, miwi_status_t status, uint8_t* msgPointer)
#endif
{
    if (SUCCESS == status)
    {
#ifndef MIWI_RN_CMD			//remove it >>		
        /* Update the TX NUM and Display it on the LCD */
        DemoOutput_UpdateTxRx(++TxNum, RxNum);
        /* Delay for Display */
        delay_ms(100);
#endif //<<			
    }
#ifdef MIWI_RN_CMD	
	else
	{
		RNCmd_SendErrorCode(status);		//return status code to host if not success
	}
#endif	
#ifndef MIWI_RN_CMD			//remove it >>		
    /* After Displaying TX and RX Counts , Switch back to showing Demo Instructions */
    STAR_DEMO_OPTIONS_MESSAGE (role);
#endif //remove		
}

/*********************************************************************
* Function: void run_star_demo(void)
*
* Overview: runs the demo based on input
*
* Parameters: None
*********************************************************************/
void run_star_demo(void)
{
#if defined(ENABLE_SLEEP_FEATURE)
        if ((role == END_DEVICE) && Total_Connections())
        {
            uint32_t timeToSleep = 0;
            /* Check whether the stack allows to sleep, if yes, put the device
            into sleep */
            if(MiApp_ReadyToSleep(&timeToSleep))
            {
#if defined (ENABLE_CONSOLE)
                /* Disable UART */
                sio2host_disable();
#endif
                /* Put the MCU into sleep */
                sleepMgr_sleep(timeToSleep);
                //printf("\r\nDevice is sleeping");
#if defined (ENABLE_CONSOLE)
                /* Enable UART */
                sio2host_enable();
#endif
            }
        }
#endif	//ENABLE_SLEEP_FEATURE
}
#endif	//PROTOCOL_STAR

/*********************************************************************
* Function: void ReceivedDataIndication (RECEIVED_MESSAGE *ind)
*
* Overview: Process a Received Message
*
* PreCondition: MiApp_ProtocolInit
*
* Input:  RECEIVED_MESSAGE *ind - Indication structure
********************************************************************/
void ReceivedDataIndication (RECEIVED_MESSAGE *ind)
{
#ifdef MIWI_RN_CMD
	RNCmd_SendReceiveData();
#endif

#if !defined(ENABLE_SLEEP_FEATURE)
    /* Toggle LED2 to indicate receiving a packet */
    LED_Toggle(LED0);
#endif
}