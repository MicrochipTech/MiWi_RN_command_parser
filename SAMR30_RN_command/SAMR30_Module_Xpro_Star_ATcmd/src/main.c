/**
* \file  main.c
*
* \brief Main file of Simple Example Star.
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

/**
* \mainpage
* \section preface Preface
* The simple example application code focuses on the simplicity of the
* MiWi™ DE protocol stack application programming interfaces.
* It provides a clean and straightforward wireless communication between
* two devices with less than 30 lines of effective C code to run the stack
* in application layer for both devices. In this application, following
* features of MiWi™ DE protocol stack have been demonstrated:
* <P>• Establish connection automatically between two devices.</P>
* <P>• Broadcast a packet.</P>
* <P>• Unicast a packet from one node to another through Pan Coordinator.</P>
* <P>• Apply security to the transmitted packet.</P>
*/

/************************ HEADERS ****************************************/
#include "task.h"
#include "asf.h"
#include "sio2host.h"
#ifdef MIWI_RN_CMD
#include "rn_cmd.h"
#endif

#if defined(ENABLE_NETWORK_FREEZER)
#include "pdsDataServer.h"
#include "wlPdsTaskManager.h"
#endif

#if ((BOARD == SAMR30_XPLAINED_PRO) || (BOARD == SAMR21_XPLAINED_PRO))
#include "edbg-eui.h"
#endif

/************************** DEFINITIONS **********************************/
#if (BOARD == SAMR21ZLL_EK)
#define NVM_UID_ADDRESS   ((volatile uint16_t *)(0x00804008U))
#endif
#if (BOARD == SAMR30_MODULE_XPLAINED_PRO)
#define NVM_UID_ADDRESS   ((volatile uint16_t *)(0x0080400AU))
#endif

#ifdef MIWI_RN_CMD
uint8_t main_state = 0;	//0: configure mode, 1: demo init and running
extern bool freezer_enable;
extern uint16_t myPAN_ID;
#endif
/************************** PROTOTYPES **********************************/
void ReadMacAddress(void);

/*********************************************************************
* Function:         void main(void)
*
* PreCondition:     none
*
* Input:            none
*
* Output:            none
*
* Side Effects:        none
*
* Overview:            This is the main function that runs the simple
*                   example demo. The purpose of this example is to
*                   demonstrate the simple application programming
*                   interface for the MiWi(TM) Development
*                   Environment. By virtually total of less than 30
*                   lines of code, we can develop a complete
*                   application using MiApp interface. The
*                   application will first try to establish a
*                   link with another device and then process the
*                   received information as well as transmit its own
*                   information.
*                   MiWi(TM) DE also support a set of rich
*                   features. Example code FeatureExample will
*                   demonstrate how to implement the rich features
*                   through MiApp programming interfaces.
*
* Note:
**********************************************************************/
int main ( void )
{
#ifndef MIWI_RN_CMD	//remove it	
    bool freezer_enable = false;
#endif	//remove	
    irq_initialize_vectors();

#if SAMD || SAMR21 || SAML21 || SAMR30
    system_init();
    delay_init();
#else
    sysclk_init();
    board_init();
#endif

    cpu_irq_enable();

#ifndef MIWI_RN_CMD	//remove it
#if defined (ENABLE_LCD)
    LCD_Initialize();
#endif
#endif //remove

    sio2host_init();

#ifdef MIWI_RN_CMD
	RNCmd_CmdInit();
#endif

    // Read the MAC address from either flash or EDBG
    ReadMacAddress();

    // Initialize system Timer
    SYS_TimerInit();

#ifndef MIWI_RN_CMD	//remove it >>
    // Demo Start Message
    DemoOutput_Greeting();

#if defined (CONF_BOARD_JOYSTICK)
    Joystick_Init();
#endif

#if (defined EXT_BOARD_OLED1_XPLAINED_PRO)
    Buttons_init();
#endif
#endif		//remove <<

#if defined(ENABLE_NETWORK_FREEZER)
    SYS_TimerInit();
    nvm_init(INT_FLASH);
    PDS_Init();
 #ifndef MIWI_RN_CMD	//remove it			
    demo_output_freezer_options();
//#endif	//unused code

//#if defined(ENABLE_NETWORK_FREEZER)
    // User Selection to commission a network or use Freezer
    freezer_enable = freezer_feature();
 #else		//remove
	freezer_enable = false;	//AT command page init	
 #endif //ifndef MIWI_RN_CMD	
#endif

    // Commission the network
#ifndef MIWI_RN_CMD		
    Initialize_Demo(freezer_enable);
#endif		

    while(1)
    {
#ifdef MIWI_RN_CMD
		if(main_state==0)
		{
			RNCmdTask();
			if(!RNCmd_IsCfgMode())
			{
				main_state = 1;
				Initialize_Demo(freezer_enable);
			}
		}
		else
		{
			Run_Demo();
			RNCmdTask();
		}
#else		
        Run_Demo();
#endif			
    }
}

/*********************************************************************
* Function:         void ReadMacAddress()
*
* PreCondition:     none
*
* Input:            none
*
* Output:            Reads MAC Address from MAC Address EEPROM
*
* Side Effects:        none
*
* Overview:            Uses the MAC Address from the EEPROM for addressing
*
* Note:
**********************************************************************/
void ReadMacAddress(void)
{
#if ((BOARD == SAMR21ZLL_EK) || (BOARD == SAMR30_MODULE_XPLAINED_PRO))
   uint8_t i = 0, j = 0;
   for (i = 0; i < 8; i += 2, j++)
   {
     myLongAddress[i] = (NVM_UID_ADDRESS[j] & 0xFF);
     myLongAddress[i + 1] = (NVM_UID_ADDRESS[j] >> 8);
   }
#elif ((BOARD == SAMR30_XPLAINED_PRO) || (BOARD == SAMR21_XPLAINED_PRO))
   uint8_t* peui64 = edbg_eui_read_eui64();
   for(uint8_t i=0; i<MY_ADDRESS_LENGTH; i++)
   {
       myLongAddress[i] = peui64[MY_ADDRESS_LENGTH-i-1];
   }
#endif
}
