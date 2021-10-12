/**
* \file  rn_cmd.h
*
* \brief MiWi RN command parser and report interface
*
* Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
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
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/

#ifdef MIWI_RN_CMD
#ifndef RN_CMD_H
#define RN_CMD_H

#define RN_RX_BUF_SIZE         64//100
#define APP_RX_CMD_SIZE         150//200
#define APP_TX_CMD_SIZE			200//64

typedef enum {
	BPSK_20_RN = 0x0,
	BPSK_40_RN = 0x4,
	BPSK_40_ALT_RN = 0x14,
	OQPSK_SIN_RC_100_RN = 0x08,
	OQPSK_SIN_RC_200_RN = 0x09,
	OQPSK_SIN_RC_400_SCR_ON_RN = 0x2A,
	OQPSK_SIN_RC_400_SCR_OFF_RN = 0x0A,
	OQPSK_RC_100_RN = 0x18,
	OQPSK_RC_200_RN = 0x19,
	OQPSK_RC_400_SCR_ON_RN = 0x3A,
	OQPSK_RC_400_SCR_OFF_RN = 0x1A,
	OQPSK_SIN_250_RN = 0x0C,
	OQPSK_SIN_500_RN = 0x0D,
	OQPSK_SIN_500_ALT_RN = 0x0F,
	OQPSK_SIN_1000_SCR_ON_RN = 0x2E,
	OQPSK_SIN_1000_SCR_OFF_RN = 0x0E,
	OQPSK_RC_250_RN = 0x1C,
	OQPSK_RC_500_RN = 0x1D,
	OQPSK_RC_500_ALT_RN = 0x1F,
	OQPSK_RC_1000_SCR_ON_RN = 0x3E,
	OQPSK_RC_1000_SCR_OFF_RN = 0x1E
	} SAMR30_PHY_MOD_RN;
extern uint8_t phy_mod_user_setting;
extern uint8_t phy_txpwr_user_setting;

/************************************************************************************
* Function:
*      void RNCmd_CmdInit( void )
*
* Summary:
*      Initialize RN command module.
*
*****************************************************************************************/
void RNCmd_CmdInit( void );

/************************************************************************************
* Function:
*      void RNCmdTask(void)
*
* Summary:
*      RN command task called by application main loop.
*      It send Reboot to host after initialization, and process data input from host and echo.
*
*****************************************************************************************/
void RNCmdTask(void);

/************************************************************************************
* Function:
*      uint8_t RNCmd_IsCfgMode(void)
*
* Summary:
*      This function is called to check if currently it is in configuration mode or not
*      Return TRUE when configuration mode, return FALSE when action mode.
*
*****************************************************************************************/
uint8_t RNCmd_IsCfgMode(void);

/************************************************************************************
* Function:
*      void RNCmd_SendReceiveData( void )
*
* Summary:
*      implementation about "recv", which reprot MiWi received data to host
*		recv r1 r2 r3 r4
*		r1: 0: unicast&unsecured, 1: unicast&secured, 2: broadcast&unsecured, 3: broadcast&secured
*		r2: rssi value
*		r3: address(8bytes long address or 2bytes short address
*		r4: received data
*		** there is no connection index or received data size as parameters **
*
*****************************************************************************************/
void RNCmd_SendReceiveData( void );

/************************************************************************************
* Function:
*      void RNCmd_SendErrorCode( uint8_t error_code )
*
* Summary:
*      implementation about "error", which report error code to host
*		and information to host.
*		error r1
*		r1: 0: connection failure
*		1: reconnection failure
*		2: link failure, BUSY, NO ACK, ...
*
*****************************************************************************************/
void RNCmd_SendErrorCode( uint8_t error_code );

/************************************************************************************
* Function:
*      void RNCmd_SendStatusChange( uint8_t event_code )
*
* Summary:
*      implementation about "status", which report some status change to host
*		and information to host.
*		event r1
*		r1: 0: connection table change, for Star PAN or P2P all devices
*		1: shared connection table change, for Star End Devices(reserved now)
*
*****************************************************************************************/
void RNCmd_SendStatusChange( uint8_t event_code );

/************************************************************************************
* Function:
*      void RNCmd_SendConnectionChange( uint8_t index )
*
* Summary:
*      implementation about "conn", which report connection index and
*		and information to host.
*		conn r1 r2 r3
*		r1: index, valid from 0
*		r2: 1: valid connection. 0: invalid connection
*		r3: 8bytes long address
*
*****************************************************************************************/
void RNCmd_SendConnectionChange( uint8_t index );
#endif
#endif
