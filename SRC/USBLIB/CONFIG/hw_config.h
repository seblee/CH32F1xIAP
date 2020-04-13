/********************************** (C) COPYRIGHT *******************************
* File Name          : hw_config.h
* Author             : WCH
* Version            : V1.0.0
* Date               : 2019/10/15
* Description        : Configuration file for USB.
*******************************************************************************/ 
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

#include "usb_type.h" 
 
#ifdef	DEBUG
#define printf_usb(X...) printf(X)
#else
#define printf_usb(X...)
#endif 


void Set_USBConfig(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void USB_Port_Set(u8 enable); 

#endif /* __HW_CONFIG_H */ 







