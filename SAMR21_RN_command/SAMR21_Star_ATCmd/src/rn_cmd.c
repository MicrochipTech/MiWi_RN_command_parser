/**
* \file  rn_cmd.c
*
* \brief MiWi RN command parser and report implementation
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

/************************ HEADERS ****************************************/
#ifdef MIWI_RN_CMD
#include "string.h"
#include "asf.h"
#include "sio2host.h"
#include "rn_cmd.h"
#include "miwi_api.h"

/************************ LOCAL VARIABLES ****************************************/
extern void dataConfcb(uint8_t handle, miwi_status_t status, uint8_t* msgPointer);	//implemented in p2p_demo.c
extern void Connection_Confirm(miwi_status_t status);	//implemented in task.c
extern uint8_t msghandledemo;
extern CONNECTION_ENTRY connectionTable[CONNECTION_SIZE];
extern uint8_t myChannel;
#if defined(PROTOCOL_P2P)	
extern bool startNetwork;
#endif
uint16_t myPAN_ID = MY_PAN_ID;
bool freezer_enable = false;
bool manual_establish_network;
uint8_t enable_echo = 0;	//0: disable rx echo
uint8_t rn_cfg_mode = 1;	//1: configure mode, 0: command mode
uint8_t reboot_reported = 0;	//1: reboot reported
uint8_t phy_mod_user_setting = 0x00; //BPSK-20 for SAMR30 or 250kbps for SAMR21
uint8_t phy_txpwr_user_setting = 0;	//+3dbm for SAMR30 BPSK-40, BPSK-40-ALT, OQPSK-SIN-(250,500, 1000), 4dbm for SAMR21.

static uint8_t at_rx_data[RN_RX_BUF_SIZE];	//buffer receiving rx data input
uint8_t rx_cmd[APP_RX_CMD_SIZE];	//rx command, single command buffer, SAMR30 will reply AOK, host must wait for AOK before sending next command
uint8_t tx_cmd[APP_TX_CMD_SIZE];	//tx command, single command buffer
uint8_t *prx_cmd, *ptag1, *ptag2, *ptag3, *ptag4;

const char StrECHO[] = "echo";
const char StrExitECHO[] = "~echo";
const char StrCFG[] = "cfg";
const char StrExitCFG[] = "~cfg";
const char StrGET[] = "get";
const char StrSET[] = "set";
const char StrPAN[] = "pan";
const char StrCHANNEL[] = "channel";
const char StrRECONN[] = "reconn";
const char StrPHYMOD[] = "phymod";
const char StrTXPOWER[] = "txpower";
const char StrADDR[] = "addr";
const char StrVER[] = "ver";
const char StrSTART[] = "start";
const char StrJOIN[] = "join";
const char StrREMOVE[] = "remove";
const char StrCONN[] = "conn";
const char StrCONSIZE[] = "consize";
const char StrROLE[] = "role";
const char StrSEND[] = "send";
const char StrRECEIVE[] = "recv";
const char StrRESET[] = "reset";
const char StrEDSIZE[] = "edsize";
const char StrMYINDEX[] = "myindex";
const char StrEDS[] = "eds";
const char StrEVENT[] = "event";
const char StrERROR[] = "error";
const char StrAOK[] = "\nAOK\n\r";
const char StrAOK2[] = "AOK\r";
const char StrERR[] = "\nERR\n\r";
const char StrERR2[] = "ERR\r";
const char StrREBOOT[] = "Reboot\n\r";
#if defined(PROTOCOL_STAR)
 #if defined(PHY_AT86RF233)
 const char StrRET_VERSION[] = "\nver cmd06fw02d\n\r";	//SAMR21 Star
 const char StrRET_VERSION2[] = "ver cmd06fw02d\r";
 #else
 const char StrRET_VERSION[] = "\nver cmd06fw02b\n\r";	//SAMR30 Star
 const char StrRET_VERSION2[] = "ver cmd06fw02b\r";
 #endif
#else
 #if defined(PHY_AT86RF233)
 const char StrRET_VERSION[] = "\nver cmd06fw02c\n\r";	//SAMR21 P2P
 const char StrRET_VERSION2[] = "ver cmd06fw02c\r";
 #else
 const char StrRET_VERSION[] = "\nver cmd06fw02a\n\r";	//SAMR30 P2P
 const char StrRET_VERSION2[] = "ver cmd06fw02a\r";
 #endif
#endif

/************************ FUNCTION DEFINITIONS ****************************************/
void RNCmd_ByteReceived(uint8_t byte);
void RNCmd_RxCmdInit( void );
void RNCmd_TxCmdInit( void );
/*****************************************************************************
*****************************************************************************/
static bool samr30_phy_mode_parser(uint8_t phy_mod_set)
{
	switch(phy_mod_set)
	{
		case BPSK_20_RN:
		case BPSK_40_RN:
		case BPSK_40_ALT_RN:
		case OQPSK_SIN_RC_100_RN:
		case OQPSK_SIN_RC_200_RN:
		case OQPSK_SIN_RC_400_SCR_ON_RN:
		case OQPSK_SIN_RC_400_SCR_OFF_RN:
		case OQPSK_RC_100_RN:
		case OQPSK_RC_200_RN:
		case OQPSK_RC_400_SCR_ON_RN:
		case OQPSK_RC_400_SCR_OFF_RN:
		case OQPSK_SIN_250_RN:
		case OQPSK_SIN_500_RN:
		case OQPSK_SIN_500_ALT_RN:
		case OQPSK_SIN_1000_SCR_ON_RN:
		case OQPSK_SIN_1000_SCR_OFF_RN:
		case OQPSK_RC_250_RN:
		case OQPSK_RC_500_RN:
		case OQPSK_RC_500_ALT_RN:
		case OQPSK_RC_1000_SCR_ON_RN:
		case OQPSK_RC_1000_SCR_OFF_RN:
			return true;
		default:
			return false;
	}
}

/************************************************************************************
* Function:
*      static void channel2BCDStr(uint8_t channel, uint8_t *pTxt, uint8_t* txtSize)
*
* Summary:
*      this function convert a digit(channel number) to BCD Strings
*		number input is parameter channel
*		output is in parameter pTxt and txtSize
*
*****************************************************************************************/
static void channel2BCDStr(uint8_t channel, uint8_t *pTxt, uint8_t* txtSize)
{
	if(!pTxt)
		return;
	if(channel > 26)	//channel range: 0~10(RF212B), 11~26(RF233)
		return;
		
	if(channel < 10)
	{
		pTxt[0] = channel + 48;
		*txtSize = 1;
	}
	else if(channel < 20)
	{
		pTxt[0] = '1';
		pTxt[1] = channel + 38;
		*txtSize = 2;
	}
	else
	{
		pTxt[0] = '2';
		pTxt[1] = channel + 28;
		*txtSize = 2;
	}
}

/************************************************************************************
* Function:
*      static void channel2HexStr(uint8_t channel, uint8_t *pTxt, uint8_t* txtSize)
*
* Summary:
*      this function convert a digit(channel number) to HEX Strings
*		number input is parameter channel
*		output is in parameter pTxt and txtSize
*		similar to num2HexStr(), but will remove first 0 character if number is < 16
*
*****************************************************************************************/
static void channel2HexStr(uint8_t channel, uint8_t *pTxt, uint8_t* txtSize)
{
	if(!pTxt)
	return;
	if(channel > 26)	//channel range: 0~10(RF212B), 11~26(RF233)
	return;
	
	if(channel < 10)
	{
		pTxt[0] = channel + 48;
		*txtSize = 1;
	}
	else if(channel < 16)
	{
		pTxt[0] = channel + 87;
		*txtSize = 1;
	}
	else if(channel < 26)
	{
		pTxt[0] = '1';
		pTxt[1] = channel + 32;
		*txtSize = 2;
	}
	else
	{
		pTxt[0] = '1';
		pTxt[1] = channel + 71;
		*txtSize = 2;
	}
}

/************************************************************************************
* Function:
*      static void num2HexStr(uint8_t *pHex, uint8_t hexSize, uint8_t *pTxt, uint8_t txtSize)
*
* Summary:
*      this function convert a number of digits to HEX Strings
*		a number of digits is input by parameter pHex, and hexSize will define digit size by byte
*		HEX String is output by parameter pTxt and txtSize
*
*****************************************************************************************/
static void num2HexStr(uint8_t *pHex, uint8_t hexSize, uint8_t *pTxt, uint8_t txtSize)
{
	uint8_t i, datah, datal;
	
	if(!pTxt || !pHex)
		return;
	if(hexSize == 0 || hexSize > 8)	//hex number need to be equal or smaller than 8
		return;
	if(txtSize < 2*hexSize)	//text buffer size must be greater than 2 times of hex size
		return;
	
	for(i=0; i<hexSize; i++)
	{
		datah = *pHex++;
		datal = datah&0x0f;
		datah = datah>>4;
		if(datah>=0x0a)
			datah += 87;//55;
		else
			datah += 0x30;
		*pTxt++ = datah;
		if(datal>=0x0a)
			datal += 87;//55;
		else
			datal += 0x30;
		*pTxt++ = datal;
	}
}

/************************************************************************************
* Function:
*      static void num2Hex(uint8_t num, uint8_t *pTxt, uint8_t* txtSize)
*
* Summary:
*      this function convert digit to HEX Strings
*		it can only convert 1 byte digit, and string converted is only 1 or 2 characters.
*
*****************************************************************************************/
static void num2Hex(uint8_t num, uint8_t *pTxt, uint8_t* txtSize)
{
	if(!pTxt)
		return;
		
	if(num < 10)
	{
		pTxt[0] = num + 48;
		*txtSize = 1;
	}
	else if(num < 16)
	{
		pTxt[0] = num + 87;
		*txtSize = 1;
	}
	else if(num < 0xa0)
	{
		if(num%16 < 10)
		{
			pTxt[0] = num/16 + 48;
			pTxt[1] = num%16 + 48;
			*txtSize = 2;
		}
		else
		{
			pTxt[0] = num/16 + 48;
			pTxt[1] = num%16 + 87;
			*txtSize = 2;
		}
	}
	else
	{
		if(num%16 < 10)
		{
			pTxt[0] = num/16 + 87;
			pTxt[1] = num%16 + 48;
			*txtSize = 2;
		}
		else
		{
			pTxt[0] = num/16 + 87;
			pTxt[1] = num%16 + 87;
			*txtSize = 2;
		}
	}
}

/************************************************************************************
* Function:
*      static uint8_t char2byte(uint8_t charb)
*
* Summary:
*      sub function and only called by str2byte()
*
*****************************************************************************************/
static uint8_t char2byte(uint8_t charb)	//only used by str2byte()
{
	uint8_t a;
	if(charb >= 0x41 && charb <= 0x46 )
		a = charb - 0x41 + 10;
	else if(charb >= 0x61 && charb <= 0x66 )
		a = charb - 0x61 + 10;
	else if(charb >= 0x30 && charb <= 0x39 )
		a = charb - 0x30;
	else
		a = 0;
	return a;
}

/************************************************************************************
* Function:
*      static uint8_t str2byte(uint8_t* str)
*
* Summary:
*      this function convert String to unsigned byte
*		String can only be 1 or 2 characters
*
*****************************************************************************************/
static uint8_t str2byte(uint8_t* str)
{
	uint8_t strb[3];
	uint8_t bb;
	strb[0] = *str++;
	strb[1] = *str;
	strb[2] = 0;
	bb = char2byte(strb[0]);
	if(strb[1])
	{
		bb <<= 4;
		bb += char2byte(strb[1]);
	}
	return bb;
}

/************************************************************************************
* Function:
*      static void RNCmd_ResponseAOK( void )
*
* Summary:
*      implementation about "AOK"
*
*****************************************************************************************/
static void RNCmd_ResponseAOK( void )
{
	if(enable_echo)
		sio2host_tx((uint8_t *)StrAOK, sizeof(StrAOK));
	else
		sio2host_tx((uint8_t *)StrAOK2, sizeof(StrAOK2));
}

/************************************************************************************
* Function:
*      static void RNCmd_ResponseERR( void )
*
* Summary:
*      implementation about "ERR"
*
*****************************************************************************************/
static void RNCmd_ResponseERR( void )
{
	if(enable_echo)
		sio2host_tx((uint8_t *)StrERR, sizeof(StrERR));
	else
		sio2host_tx((uint8_t *)StrERR2, sizeof(StrERR2));
}

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
void RNCmd_SendErrorCode( uint8_t error_code )
{
	uint8_t tx_data[10];
	uint8_t* ptx_data = tx_data;
	uint8_t tx_data_len = 0;
	uint8_t hex[2];
	uint8_t hex_len;
	
	//beginning is adding '\n' good for PC terminal display
	if(enable_echo)
	{
		*ptx_data++ = '\n';
		tx_data_len++;
	}
	
	//add command tag
	*ptx_data++ = 'e';
	*ptx_data++ = 'r';
	*ptx_data++ = 'r';
	*ptx_data++ = 'o';
	*ptx_data++ = 'r';
	*ptx_data++ = ' ';
	tx_data_len += 6;
	
	//add r1
	num2Hex(error_code, hex, &hex_len);
	if(hex_len == 1)
	{
		*ptx_data++ = hex[0];
		tx_data_len ++;
	}
	else if(hex_len == 2)
	{
		*ptx_data++ = hex[0];
		*ptx_data++ = hex[1];
		tx_data_len += 2;
	}
	
	//tail is adding 'r'
	if(enable_echo)
	{
		*ptx_data++ = '\n';
		*ptx_data++ = '\r';
		tx_data_len += 2;
	}
	else
	{
		*ptx_data++ = '\r';	//0x13, ENTER
		tx_data_len += 1;
	}
	
	//send data out and initialize
	sio2host_tx(tx_data, tx_data_len);
	RNCmd_TxCmdInit();
}

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
void RNCmd_SendStatusChange( uint8_t event_code )
{
	uint8_t tx_data[10];
	uint8_t* ptx_data = tx_data;
	uint8_t tx_data_len = 0;
	uint8_t hex[2];
	uint8_t hex_len;
	
	//beginning is adding '\n' good for PC terminal display
	if(enable_echo)
	{
		*ptx_data++ = '\n';
		tx_data_len++;
	}
	
	//add command tag
	*ptx_data++ = 's';
	*ptx_data++ = 't';
	*ptx_data++ = 'a';
	*ptx_data++ = 't';
	*ptx_data++ = 'u';
	*ptx_data++ = 's';
	*ptx_data++ = ' ';
	tx_data_len += 7;
	
	//add r1
	num2Hex(event_code, hex, &hex_len);
	if(hex_len == 1)
	{
		*ptx_data++ = hex[0];
		tx_data_len ++;
	}
	else if(hex_len == 2)
	{
		*ptx_data++ = hex[0];
		*ptx_data++ = hex[1];
		tx_data_len += 2;
	}
	
	//tail is adding 'r'
	if(enable_echo)
	{
		*ptx_data++ = '\n';
		*ptx_data++ = '\r';
		tx_data_len += 2;
	}
	else
	{
		*ptx_data++ = '\r';	//0x13, ENTER
		tx_data_len += 1;
	}
	
	//send data out and initialize
	sio2host_tx(tx_data, tx_data_len);
	RNCmd_TxCmdInit();
}

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
void RNCmd_SendConnectionChange( uint8_t index )
{
	uint8_t* pStr1 = tx_cmd;
	uint8_t str1Len = 0;
	uint8_t hex[2];
	uint8_t hex_len;
	uint8_t conn_sts;

	if(enable_echo)
	{
		*pStr1++ = '\n';
		str1Len++;
	}
	*pStr1++ = 'c';	//add "conn"
	*pStr1++ = 'o';
	*pStr1++ = 'n';
	*pStr1++ = 'n';
	*pStr1++ = ' ';
	str1Len += 5;
	
	//r1: connection index
	num2Hex(index, hex, &hex_len);
	if(hex_len == 1)
	{
		*pStr1++ = hex[0];
		*pStr1++ = ' ';
		str1Len += 2;
	}
	else if(hex_len == 2)
	{
		*pStr1++ = hex[0];
		*pStr1++ = hex[1];
		*pStr1++ = ' ';
		str1Len += 3;
	}
	
	//r2: connection valid/invalid, link status
#if 0	//add link status
	conn_sts = 0;
 #if defined(PROTOCOL_STAR)
 #if defined(ENABLE_LINK_STATUS)
	conn_sts = connectionTable[index].link_status;
 #endif
 #endif
	if( connectionTable[index].status.bits.isValid )
		conn_sts |= 0x80;
	num2Hex(conn_sts, hex, &hex_len);
	if(hex_len == 1)
	{
		*pStr1++ = hex[0];
		*pStr1++ = ' ';
		str1Len += 2;
	}
	else
	{
		*pStr1++ = hex[0];
		*pStr1++ = hex[1];
		*pStr1++ = ' ';
		str1Len += 3;
	}
#else
	if( connectionTable[index].status.bits.isValid )
	{
		*pStr1++ = '1';	//'1' means valid
		*pStr1++ = ' ';
		str1Len += 2;
	}
	else
	{
		*pStr1++ = '0';	//'0' means invalid
		*pStr1++ = ' ';
		str1Len += 2;
	}
#endif	
	
	//r3: connection long address
	num2HexStr(connectionTable[index].Address, MY_ADDRESS_LENGTH, pStr1, 2*MY_ADDRESS_LENGTH+1);
	pStr1 += 2*MY_ADDRESS_LENGTH;
	str1Len += 2*MY_ADDRESS_LENGTH;
	
	if(enable_echo)
	{
		*pStr1++ = '\n';
		*pStr1 = '\r';
		str1Len += 2;
	}
	else
	{
		*pStr1++ = '\r';	//0x13, ENTER
		str1Len += 1;
	}
	sio2host_tx(tx_cmd, str1Len);
	RNCmd_TxCmdInit();
}

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
void RNCmd_SendReceiveData( void )
{
	uint8_t* ptx_cmd = tx_cmd;
	uint16_t tx_cmd_len = 0;
	uint8_t temp;
	
	if(enable_echo)
	{
		*ptx_cmd++ = '\n';
		tx_cmd_len++;
	}
	
	*ptx_cmd++ = 'r';	//add "recv"
	*ptx_cmd++ = 'e';
	*ptx_cmd++ = 'c';
	*ptx_cmd++ = 'v';
	*ptx_cmd++ = ' ';
	tx_cmd_len += 5;
	
	//r1: security&broadcast(unicast)
	temp = 0;
	if( rxMessage.flags.bits.secEn )
		temp |= 0x02;
	if( rxMessage.flags.bits.broadcast )
		temp |= 0x01;
	num2HexStr(&temp, 1, ptx_cmd, 2);
	ptx_cmd += 2; 
	tx_cmd_len += 2;
	*ptx_cmd++ = ' ';
	tx_cmd_len ++;
	
	//r2: rssi
	temp = rxMessage.PacketRSSI;
	num2HexStr(&temp, 1, ptx_cmd, 2);
	ptx_cmd += 2;
	tx_cmd_len += 2;
	*ptx_cmd++ = ' ';
	tx_cmd_len ++;
	
	//r3: address
	if( rxMessage.flags.bits.srcPrsnt )
	{
		if( rxMessage.flags.bits.altSrcAddr )
		{
			num2HexStr(rxMessage.SourceAddress, 2, ptx_cmd, 4);
			ptx_cmd += 4;
			tx_cmd_len += 4;
		}
		else
		{
			num2HexStr(rxMessage.SourceAddress, MY_ADDRESS_LENGTH, ptx_cmd, 2*MY_ADDRESS_LENGTH);
			ptx_cmd += 2*MY_ADDRESS_LENGTH;
			tx_cmd_len += 2*MY_ADDRESS_LENGTH;
		}
	}
	else
	{
		//if no source address presented, use "ffff" to indicate this situation
		*ptx_cmd++ = 'f';
		*ptx_cmd++ = 'f';
		*ptx_cmd++ = 'f';
		*ptx_cmd++ = 'f';
		tx_cmd_len += 4;
	}
	*ptx_cmd++ = ' ';
	tx_cmd_len ++;
	
	for(temp = 0; temp < rxMessage.PayloadSize; temp++)
	{
		*ptx_cmd++ = rxMessage.Payload[temp];
		tx_cmd_len++;
	}
	
	if(enable_echo)
	{
		*ptx_cmd++ = '\n';
		*ptx_cmd++ = '\r';
		tx_cmd_len += 2;
	}
	else
	{
		*ptx_cmd++ = '\r';	//0x13, ENTER
		tx_cmd_len += 1;
	}
	
	sio2host_tx(tx_cmd, tx_cmd_len);
	RNCmd_TxCmdInit();
}

/************************************************************************************
* Function:
*      void RNCmd_ProcessCommand( void )
*
* Summary:
*      detailed process to parser RN command and every filed in RN command
*
*****************************************************************************************/
void RNCmd_ProcessCommand( void )
{
	uint8_t temp, index, data_size;
	uint8_t str1[APP_TX_CMD_SIZE];	//can be replaced by tx_cmd[] global array
	uint8_t* pStr1;
	uint8_t str1Len;
	uint8_t pana[2];
	uint8_t conn_sts;

//process this command
	switch(ptag1[0])
	{
		case 'c':
		//case 'C':
		if(strcmp(StrCFG, (const char*)ptag1) == 0)
		{
			if(!rn_cfg_mode)	//not in configure mode
			RNCmd_ResponseERR();
			else if(strcmp(StrPAN, (const char*)ptag2) == 0)		//command: cfg pan r1
			{
				RNCmd_ResponseAOK();
				myPAN_ID = (uint16_t)str2byte(ptag3);
				ptag3+=2;
				myPAN_ID <<= 8;
				myPAN_ID += (uint16_t)str2byte(ptag3);
				//printf("%x", myPAN_ID);	//debug
			}
			else if(strcmp(StrRECONN, (const char*)ptag2) == 0)		//command: cfg reconn r1(0/1/2)
			{
				temp = str2byte(ptag3);
				if(temp == 0 || temp == 1 || temp == 2)
				{
					//0: new network with auto running.
					//1: network reconnection, if no reconnection available, then auto run to establish network.
					//2: wait for "start" and "join" to establish new network.
					RNCmd_ResponseAOK();
					if(temp == 0 || temp == 2)
					freezer_enable = false;
					else
					freezer_enable = true;
					if(temp == 2)
					manual_establish_network = true;
					else
					manual_establish_network = false;
				}
				else
				{
					RNCmd_ResponseERR();
					freezer_enable = 0;
					manual_establish_network = false;
				}
				//printf("%d", freezer_enable);	//debug
			}
			else if(strcmp(StrCHANNEL, (const char*)ptag2) == 0)	//command: cfg channel r1
			{
				temp = str2byte(ptag3);
#if defined(PHY_AT86RF233)
				if(temp > 26 || temp < 11)		//SAMR21
#else
				if(temp > 10)					//SAMR30
#endif				
					RNCmd_ResponseERR();
				else
				{
					RNCmd_ResponseAOK();
					myChannel = str2byte(ptag3);
				}
				//printf("%d", myChannel);	//debug
			}
			else if(strcmp(StrPHYMOD, (const char*)ptag2) == 0)	//command: cfg phymod r1
			{
				temp = str2byte(ptag3);
#if defined(PHY_AT86RF233)
				if(temp > 3)		//SAMR21 phy mode: 0:250kbps, 1:500kbps, 2:1000kbps, 3:2000kbps
				{
					RNCmd_ResponseERR();
				}
				else
				{
					RNCmd_ResponseAOK();
					phy_mod_user_setting = temp;
				}
#else
				temp &= 0x3f;		//only valid last 5bits.
				if(samr30_phy_mode_parser(temp))	//use const table to validate the value, referring to datasheet.
				{
					RNCmd_ResponseAOK();
					phy_mod_user_setting = temp;
				}
				else
					RNCmd_ResponseERR();
#endif		
			}
			else if(strcmp(StrTXPOWER, (const char*)ptag2) == 0)	//command: cfg txpower r1
			{
				temp = str2byte(ptag3);
#if defined(PHY_AT86RF233)
				if(temp > 0xf)		//SAMR21 tx power, valid value 0~0xf, from +4dbm to -17dbm, referring to datasheet.
				{
					RNCmd_ResponseERR();
				}
				else
				{
					RNCmd_ResponseAOK();
					phy_txpwr_user_setting = str2byte(ptag3);
				}
#else
				//not checking tx power setting for SAMR30
				RNCmd_ResponseAOK();
				phy_txpwr_user_setting = str2byte(ptag3);
#endif				
			}
			else
			{
				RNCmd_ResponseERR();
			}
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		case 'g':
		//case 'G':
		if(strcmp(StrGET, (const char*)ptag1) == 0)
		{
			if(strcmp(StrADDR, (const char*)ptag2) == 0)	//command: get addr
			{
				memset(str1, 0, sizeof(str1));
				pStr1 = str1;
				str1Len = 0;
				if(enable_echo)
				{
				*pStr1++ = '\n';
				str1Len++;
				}
			*pStr1++ = 'a';	//add "addr"
			*pStr1++ = 'd';
			*pStr1++ = 'd';
			*pStr1++ = 'r';
			*pStr1++ = ' ';
			str1Len += 5;
			num2HexStr(myLongAddress, MY_ADDRESS_LENGTH, pStr1, 2*MY_ADDRESS_LENGTH+1);
			pStr1 += 2*MY_ADDRESS_LENGTH;
			str1Len += 2*MY_ADDRESS_LENGTH;
				if(enable_echo)
				{
				*pStr1++ = '\n';
				*pStr1 = '\r';
				str1Len += 2;
				}
				else
				{
				*pStr1++ = '\r';	//0x13, ENTER
				str1Len += 1;
				}
				sio2host_tx(str1, str1Len);
			}
			else if(strcmp(StrCHANNEL, (const char*)ptag2) == 0)	//command: get channel
			{
				memset(str1, 0, sizeof(str1));
				pStr1 = str1;
				str1Len = 0;
				if(enable_echo)
				{
				*pStr1++ = '\n';
				str1Len++;
				}
				*pStr1++ = 'c';	//add "channel"
				*pStr1++ = 'h';
				*pStr1++ = 'a';
				*pStr1++ = 'n';
				*pStr1++ = 'n';
				*pStr1++ = 'e';
				*pStr1++ = 'l';
				*pStr1++ = ' ';
				str1Len += 8;
			
				if(!rn_cfg_mode)	//afer exiting from config mode, use API to read channel; in config mode, just use variable myChannel
					MiApp_Get(CHANNEL, &myChannel);
				//channel2BCDStr(myChannel, pStr1, &temp);
				channel2HexStr(myChannel, pStr1, &temp);
				pStr1 += temp;
				str1Len += temp;
			
				if(enable_echo)
				{
				*pStr1++ = '\n';
				*pStr1 = '\r';
				str1Len += 2;
				}
				else
				{
				*pStr1++ = '\r';	//0x13, ENTER
				str1Len += 1;
				}
				sio2host_tx(str1, str1Len);
			}
			else if(strcmp(StrPAN, (const char*)ptag2) == 0)	//command: get pan
			{
				memset(str1, 0, sizeof(str1));
				pStr1 = str1;
				str1Len = 0;
				if(enable_echo)
				{
				*pStr1++ = '\n';
				str1Len++;
				}
				*pStr1++ = 'p';	//add "pan"
				*pStr1++ = 'a';
				*pStr1++ = 'n';
				*pStr1++ = ' ';
				str1Len += 4;
				
				if(!rn_cfg_mode)	//afer exiting from config mode, use API to read PAN ID; in config mode, just use variable myPAN_ID
					MiApp_Get(PANID, &myPAN_ID);
				pana[0] = (uint8_t)(myPAN_ID>>8);
				pana[1] = (uint8_t)myPAN_ID;
				num2HexStr(pana, 2, pStr1, 5);
				pStr1 += 4;
				str1Len += 4;
				
				if(enable_echo)
				{
				*pStr1++ = '\n';
				*pStr1 = '\r';
				str1Len += 2;
				}
				else
				{
					*pStr1++ = '\r';	//0x13, ENTER
					str1Len += 1;
				}
				sio2host_tx(str1, str1Len);
			}
			else if(!rn_cfg_mode && (strcmp(StrROLE, (const char*)ptag2) == 0))	//command: get role
			{
				memset(str1, 0, sizeof(str1));
				pStr1 = str1;
				str1Len = 0;
				if(enable_echo)
				{
					*pStr1++ = '\n';
					str1Len++;
				}
				*pStr1++ = 'r';	//add "role"
				*pStr1++ = 'o';
				*pStr1++ = 'l';
				*pStr1++ = 'e';
				*pStr1++ = ' ';
				str1Len += 5;
			
				temp = 0;
#if defined(PROTOCOL_P2P)				
				if(startNetwork)
					temp |= 0x01;
#endif					
#if defined(PROTOCOL_STAR)
				if(role == PAN_COORD)
					temp |= 0x02;
#endif		
				num2HexStr(&temp, 1, pStr1, 2);
				pStr1 += 2;
				str1Len += 2;		

				if(enable_echo)
				{
					*pStr1++ = '\n';
					*pStr1 = '\r';
					str1Len += 2;
				}
				else
				{
					*pStr1++ = '\r';	//0x13, ENTER
					str1Len += 1;
				}
				sio2host_tx(str1, str1Len);
			}
			else if(!rn_cfg_mode && (strcmp(StrCONSIZE, (const char*)ptag2) == 0))	//command: get consize
			{
				memset(str1, 0, sizeof(str1));
				pStr1 = str1;
				str1Len = 0;
				if(enable_echo)
				{
					*pStr1++ = '\n';
					str1Len++;
				}
				*pStr1++ = 'c';	//add "consize"
				*pStr1++ = 'o';
				*pStr1++ = 'n';
				*pStr1++ = 's';
				*pStr1++ = 'i';
				*pStr1++ = 'z';
				*pStr1++ = 'e';
				*pStr1++ = ' ';
				str1Len += 8;
			
				temp = Total_Connections();
				num2HexStr(&temp, 1, pStr1, 2);
				pStr1 += 2;
				str1Len += 2;
			
				if(enable_echo)
				{
				*pStr1++ = '\n';
				*pStr1 = '\r';
				str1Len += 2;
				}
				else
				{
				*pStr1++ = '\r';	//0x13, ENTER
				str1Len += 1;
				}
				sio2host_tx(str1, str1Len);
			}
			else if(!rn_cfg_mode && (strcmp(StrCONN, (const char*)ptag2) == 0))	//command: get conn r1
			{
				temp = str2byte(ptag3);
				if(temp >= Total_Connections())
					RNCmd_ResponseERR();
				else
				{
					memset(str1, 0, sizeof(str1));
					pStr1 = str1;
					str1Len = 0;
					if(enable_echo)
					{
					*pStr1++ = '\n';
					str1Len++;
					}
					*pStr1++ = 'c';	//add "conn"
					*pStr1++ = 'o';
					*pStr1++ = 'n';
					*pStr1++ = 'n';
					*pStr1++ = ' ';
					str1Len += 5;
			
					//r1: connection index
					num2Hex(temp, pana, &data_size);
					if(data_size == 1)
					{
						*pStr1++ = pana[0];
						*pStr1++ = ' ';
						str1Len += 2;
					}
					else if(data_size == 2)
					{
						*pStr1++ = pana[0];
						*pStr1++ = pana[1];
						*pStr1++ = ' ';
						str1Len += 3;
					}
			
					//r2: connection valid/invalid, link status
#if 0	//add link status					
					conn_sts = 0;
 #if defined(PROTOCOL_STAR)
 #if defined(ENABLE_LINK_STATUS)					
					conn_sts = connectionTable[temp].link_status;
 #endif
 #endif
					if( connectionTable[temp].status.bits.isValid )
						conn_sts |= 0x80;
					num2Hex(conn_sts, pana, &data_size);
					if(data_size == 1)
					{
						*pStr1++ = pana[0];
						*pStr1++ = ' ';
						str1Len += 2;
					}
					else
					{
						*pStr1++ = pana[0];
						*pStr1++ = pana[1];
						*pStr1++ = ' ';
						str1Len += 3;
					}
#else
					if( connectionTable[temp].status.bits.isValid )
					{
						*pStr1++ = '1';	//'1' means valid
						*pStr1++ = ' ';
						str1Len += 2;
					}
					else
					{
						*pStr1++ = '0';	//'0' means invalid
						*pStr1++ = ' ';
						str1Len += 2;
					}
#endif					
				
					//r3: connection IEEE long address
					num2HexStr(connectionTable[temp].Address, MY_ADDRESS_LENGTH, pStr1, 2*MY_ADDRESS_LENGTH+1);
					pStr1 += 2*MY_ADDRESS_LENGTH;
					str1Len += 2*MY_ADDRESS_LENGTH;
			
					if(enable_echo)
					{
						*pStr1++ = '\n';
						*pStr1 = '\r';
						str1Len += 2;
					}
					else
					{
						*pStr1++ = '\r';	//0x13, ENTER
						str1Len += 1;
					}
					sio2host_tx(str1, str1Len);
				}
			}
			else if(strcmp(StrVER, (const char*)ptag2) == 0)	//command: get ver
			{
				if(enable_echo)
				sio2host_tx((uint8_t *)StrRET_VERSION, sizeof(StrRET_VERSION));
				else
				sio2host_tx((uint8_t *)StrRET_VERSION2, sizeof(StrRET_VERSION2));
			}
			else if(!rn_cfg_mode && (strcmp(StrEDSIZE, (const char*)ptag2) == 0))	//command: get edsize
			{
#if defined(PROTOCOL_STAR)
				if(role == END_DEVICE)
				{
					memset(str1, 0, sizeof(str1));
					pStr1 = str1;
					str1Len = 0;
					if(enable_echo)
					{
						*pStr1++ = '\n';
						str1Len++;
					}
					*pStr1++ = 'e';	//add "edsize"
					*pStr1++ = 'd';
					*pStr1++ = 's';
					*pStr1++ = 'i';
					*pStr1++ = 'z';
					*pStr1++ = 'e';
					*pStr1++ = ' ';
					str1Len += 7;
					
					num2Hex(end_nodes, pStr1, &temp);
					if(temp == 1)
					{
						pStr1++;
						str1Len ++;
					}
					else
					{
						pStr1 += 2;
						str1Len += 2;
					}
					if(enable_echo)
					{
						*pStr1++ = '\n';
						*pStr1 = '\r';
						str1Len += 2;
					}
					else
					{
						*pStr1++ = '\r';	//0x13, ENTER
						str1Len += 1;
					}
					sio2host_tx(str1, str1Len);
				}
				else
				{
					RNCmd_ResponseERR();
				}
#else
				RNCmd_ResponseERR();
#endif				
			}
			else if(!rn_cfg_mode && (strcmp(StrMYINDEX, (const char*)ptag2) == 0))	//command: get myindex
			{
#if defined(PROTOCOL_STAR)
				if(role == END_DEVICE)
				{
					memset(str1, 0, sizeof(str1));
					pStr1 = str1;
					str1Len = 0;
					if(enable_echo)
					{
						*pStr1++ = '\n';
						str1Len++;
					}
					*pStr1++ = 'm';	//add "edsize"
					*pStr1++ = 'y';
					*pStr1++ = 'i';
					*pStr1++ = 'n';
					*pStr1++ = 'd';
					*pStr1++ = 'e';
					*pStr1++ = 'x';
					*pStr1++ = ' ';
					str1Len += 8;
					
					num2Hex(myConnectionIndex_in_PanCo, pStr1, &temp);
					if(temp == 1)
					{
						pStr1++;
						str1Len ++;
					}
					else
					{
						pStr1 += 2;
						str1Len += 2;
					}
					if(enable_echo)
					{
						*pStr1++ = '\n';
						*pStr1 = '\r';
						str1Len += 2;
					}
					else
					{
						*pStr1++ = '\r';	//0x13, ENTER
						str1Len += 1;
					}
					sio2host_tx(str1, str1Len);
				}
				else
				{
					RNCmd_ResponseERR();
				}
#else
				RNCmd_ResponseERR();
#endif
			}
			else if(!rn_cfg_mode && (strcmp(StrEDS, (const char*)ptag2) == 0))	//command: get eds r1 r2
			//r1 is start index of eds reading, r2 is end index of eds reading.
			{
#if defined(PROTOCOL_STAR)
				if(role == END_DEVICE)
				{
					if(ptag3 && ptag4)
					{
						temp = str2byte(ptag3);	//get start index of eds reading
						data_size = str2byte(ptag4);	//get end index of eds reading
						if(temp > data_size || temp >= end_nodes || data_size >= end_nodes)
							RNCmd_ResponseERR();
						else
						{
						memset(str1, 0, sizeof(str1));
						pStr1 = str1;
						str1Len = 0;
						if(enable_echo)
						{
							*pStr1++ = '\n';
							str1Len++;
						}
						*pStr1++ = 'e';	//add "eds"
						*pStr1++ = 'd';
						*pStr1++ = 's';
						*pStr1++ = ' ';
						str1Len += 4;
						
						for(index=temp; index<=data_size; index++)
						{
							num2HexStr(&END_DEVICES_Short_Address[index].Address[0], 3, pStr1, 6);
							pStr1+=6;
							str1Len += 6;
							num2HexStr(&END_DEVICES_Short_Address[index].connection_slot, 1, pStr1, 2);
							pStr1+=2;
							str1Len += 2;
						}
						
						if(enable_echo)
						{
							*pStr1++ = '\n';
							*pStr1 = '\r';
							str1Len += 2;
						}
						else
						{
							*pStr1++ = '\r';	//0x13, ENTER
							str1Len += 1;
						}
						sio2host_tx(str1, str1Len);
						}	//start index, end index, end_nodes value check
					}
					else //r1, r2 parameters is incomplete
					{
						RNCmd_ResponseERR();
					}
				}
				else
				{
					RNCmd_ResponseERR();
				}
#else
				RNCmd_ResponseERR();
#endif				
			}
			else
			{
				RNCmd_ResponseERR();
			}
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		case 's':
		//case 'S':
		if(!rn_cfg_mode && (strcmp(StrSTART, (const char*)ptag1) == 0))	//command: start
		{
			RNCmd_ResponseAOK();
#if defined(PROTOCOL_P2P)				
			startNetwork = true;
#endif			
			MiApp_StartConnection(START_CONN_DIRECT, 10, (1L << myChannel), Connection_Confirm);			
		}
		else if(!rn_cfg_mode && (strcmp(StrSEND, (const char*)ptag1) == 0))	//command: send r1 r2 r3
		{
			if(ptag2 && ptag3 && ptag4)
			{
				temp = strlen(ptag2);
				data_size = str2byte(ptag3);
				if(temp == 1 || temp == 2)	//unicast, by peer device index
				{
					index = str2byte(ptag2);
					if(index >= Total_Connections())
						RNCmd_ResponseERR();
					else
					{
						if(!data_size)	//if r2=0, count r3 bytes and use counted number
						{
							if(MiApp_SendData(LONG_ADDR_LEN, connectionTable[index].Address, strlen(ptag4), ptag4, msghandledemo++, true, dataConfcb))
								RNCmd_ResponseAOK();
							else
								RNCmd_ResponseERR();
						}
						else
						{
							if(MiApp_SendData(LONG_ADDR_LEN, connectionTable[index].Address, data_size, ptag4, msghandledemo++, true, dataConfcb))
								RNCmd_ResponseAOK();
							else
								RNCmd_ResponseERR();
						}
					}
				}
				else if(temp == 4)	//broadcast, by 0xFFFF broadcast address
				{
					uint16_t broadcastAddress = 0xFFFF;
					if((strcmp(ptag2, "ffff") == 0) || strcmp(ptag2, "FFFF") == 0)
					{
						if(!data_size)	//if r2=0, count r3 bytes and use counted number
						{
							if(MiApp_SendData(SHORT_ADDR_LEN, (uint8_t *)&broadcastAddress, strlen(ptag4), ptag4, msghandledemo++, false, dataConfcb))
								RNCmd_ResponseAOK();
							else
								RNCmd_ResponseERR();
						}
						else
						{
							if(MiApp_SendData(SHORT_ADDR_LEN, (uint8_t *)&broadcastAddress, data_size, ptag4, msghandledemo++, false, dataConfcb))
								RNCmd_ResponseAOK();
							else
								RNCmd_ResponseERR();
						}
					}
					else
					{
						RNCmd_ResponseERR();
					}
				}
				else if(temp == 6)	//unicast, Star edx -> edy only
				{
#if defined(PROTOCOL_STAR)
					if(role == END_DEVICE)
					{
						uint8_t desShortAddress[3];
						pStr1 = ptag2;
						desShortAddress[0] = str2byte(pStr1);
						pStr1+=2;
						desShortAddress[1] = str2byte(pStr1);
						pStr1+=2;
						desShortAddress[2] = str2byte(pStr1);
						if(!data_size)	//if r2=0, count r3 bytes and use counted number
						{
							if(MiApp_SendData(3, desShortAddress, strlen(ptag4), ptag4, msghandledemo++, true, dataConfcb))
								RNCmd_ResponseAOK();
							else
								RNCmd_ResponseERR();
						}
						else
						{
							if(MiApp_SendData(3, desShortAddress, data_size, ptag4, msghandledemo++, true, dataConfcb))
								RNCmd_ResponseAOK();
							else
								RNCmd_ResponseERR();
						}
					}
					else
					{
						RNCmd_ResponseERR();
					}
#else
					RNCmd_ResponseERR();
#endif
				}
				else if(temp == 16)	//unicast, by 8bytes IEEE long address
				{
					uint8_t destLongAddress[8];
					pStr1 = ptag2;
					destLongAddress[0] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[1] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[2] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[3] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[4] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[5] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[6] = str2byte(pStr1);
					pStr1+=2;
					destLongAddress[7] = str2byte(pStr1);
					if(!data_size)	//if r2=0, count r3 bytes and use counted number
					{
						if(MiApp_SendData(LONG_ADDR_LEN, destLongAddress, strlen(ptag4), ptag4, msghandledemo++, true, dataConfcb))
							RNCmd_ResponseAOK();
						else
							RNCmd_ResponseERR();
					}
					else
					{
						if(MiApp_SendData(LONG_ADDR_LEN, destLongAddress, data_size, ptag4, msghandledemo++, true, dataConfcb))
							RNCmd_ResponseAOK();
						else
							RNCmd_ResponseERR();
					}
				}
				else
				{
					RNCmd_ResponseERR();
				}
			}
			else
			{
				RNCmd_ResponseERR();
			}
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		case 'j':
		//case 'J':
		if(!rn_cfg_mode && (strcmp(StrJOIN, (const char*)ptag1) == 0))	//command: join
		{
			uint16_t broadcastAddr = 0xFFFF;
			RNCmd_ResponseAOK();
#if defined(PROTOCOL_P2P)				
			startNetwork = false;
#endif			
			MiApp_EstablishConnection(myChannel, 2, (uint8_t*)&broadcastAddr, 0, Connection_Confirm);
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		case 'r':
		//case 'R':
		if(strcmp(StrRESET, (const char*)ptag1) == 0)		//command: reset
		{
			RNCmd_ResponseAOK();
			MiApp_ResetToFactoryNew();
		}
		else if(!rn_cfg_mode &&  (strcmp(StrREMOVE, (const char*)ptag1) == 0))		//command: remove r1
		{
			temp = str2byte(ptag2);
			if(temp >= Total_Connections())
				RNCmd_ResponseERR();
			else
			{
				RNCmd_ResponseAOK();
				MiApp_RemoveConnection(temp);
			}
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		case 'e':
		//case 'E':
		if(strcmp(StrECHO, (const char*)ptag1) == 0)		//command: echo
		{
			RNCmd_ResponseAOK();
			enable_echo = 1;
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		case '~':
		if(strcmp(StrExitCFG, (const char*)ptag1) == 0)	//command: ~cfg
		{
			RNCmd_ResponseAOK();
			rn_cfg_mode = 0;	//go to action mode
		}
		else if(strcmp(StrExitECHO, (const char*)ptag1) == 0)	//command: ~echo
		{
			RNCmd_ResponseAOK();
			enable_echo = 0;
		}
		else
		{
			RNCmd_ResponseERR();
		}
		break;
		
		default:
		break;
	}

	//initialize rx command and its pointers
	RNCmd_RxCmdInit();
}

/************************************************************************************
* Function:
*      void RNCmd_ByteReceived(uint8_t byte)
*
* Summary:
*      Sub-function to identify RN command ENDING, will mark pointer position when find SPACE
*
*****************************************************************************************/
void RNCmd_ByteReceived(uint8_t byte)
{
	if(byte == 0x0D)	//ENTER character
	{
		//get to command end
		RNCmd_ProcessCommand();		
		return;
	}
	if((byte == 0x20) && (!ptag2 || !ptag3 || !ptag4))	//SPACE check
	{
		byte = 0;		//replace SPACE with NULL
		*prx_cmd++ = byte;
		if(!ptag2)
			ptag2 = prx_cmd;
		else if(!ptag3)
			ptag3 = prx_cmd;
		else if(!ptag4)
			ptag4 = prx_cmd;
	}
	else
		*prx_cmd++ = byte;
}

/************************************************************************************
* Function:
*      void RNCmd_RxCmdInit( void )
*
* Summary:
*      Initialize RN rx buffer and variables for decoding rx data
*
*****************************************************************************************/
void RNCmd_RxCmdInit( void )
{
	prx_cmd = rx_cmd;
	ptag1 = rx_cmd;
	ptag2 = 0;
	ptag3 = 0;
	ptag4 = 0;
	memset(rx_cmd, 0, APP_RX_CMD_SIZE);
}

/************************************************************************************
* Function:
*      void RNCmd_TxCmdInit( void )
*
* Summary:
*      Initialize RN tx buffer.
*
*****************************************************************************************/
void RNCmd_TxCmdInit( void )
{
	memset(tx_cmd, 0, APP_TX_CMD_SIZE);
}

/************************************************************************************
* Function:
*      void RNCmd_CmdInit( void )
*
* Summary:
*      Initialize RN command module.
*
*****************************************************************************************/
void RNCmd_CmdInit( void )
{
	reboot_reported = 0;
	rn_cfg_mode = 1;
	enable_echo = 0;
	manual_establish_network = false;
	RNCmd_RxCmdInit();
	RNCmd_TxCmdInit();
	phy_mod_user_setting = BPSK_20_RN;	//BPSK-20 for SAMR30 or 250kbps for SAMR21
	phy_txpwr_user_setting = 0;	//+3dbm for SAMR30 BPSK-40, BPSK-40-ALT, OQPSK-SIN-(250,500, 1000), +4dbm for SAMR21.
#if defined(PHY_AT86RF212B)
	if(myChannel)
	{
		phy_mod_user_setting = BPSK_40_ALT_RN;//default to BPSK-40-ALT if it is not channel 0
	}
	else
	{
		phy_txpwr_user_setting = 0xCB;	//+3dbm for 868.3MHZ EU band, Channel 0, BPSK-20
	}
#endif	
}

/************************************************************************************
* Function:
*      void RNCmdTask(void)
*
* Summary:
*      RN command task called by application main loop.
*      It send Reboot to host after initialization, and process data input from host and echo.
*
*****************************************************************************************/
void RNCmdTask(void)
{
	uint16_t bytes;
	if(reboot_reported == 0)
	{
		reboot_reported = 1;
		sio2host_tx((uint8_t *)StrREBOOT, sizeof(StrAOK));
	}
	if ((bytes = sio2host_rx(at_rx_data, RN_RX_BUF_SIZE)) > 0) {
		if(enable_echo)
			sio2host_tx(at_rx_data, bytes);		//echo back
		for (uint16_t i = 0; i < bytes; i++) {
			RNCmd_ByteReceived(at_rx_data[i]);
		}
	}
}

/************************************************************************************
* Function:
*      uint8_t RNCmd_IsCfgMode(void)
*
* Summary:
*      This function is called to check if currently it is in configuration mode or not
*      Return TRUE when configuration mode, return FALSE when action mode.
*
*****************************************************************************************/
uint8_t RNCmd_IsCfgMode(void)
{
	return rn_cfg_mode;
}
#endif
