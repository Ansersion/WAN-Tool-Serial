#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #define USE_STDPERIPH_DRIVER // to use stm32f10x_conf.h

// #ifdef 	DEBUG
// #define ASSERT_FAILED() 	(printf("Assert failed %s: line %d", __FILE__, __LINE__);while(1))
// #define ASSERT(expr) (expr ? 0 : ASSERT_FAILED())
// #else
// #define ASSERT(expr) ((void)0)
// #endif

#define TRUE 		1;
#define FALSE 		0;

typedef unsigned char  INT8U;			// Unsigned  8 bit quantity  
typedef unsigned short INT16U;			// Unsigned 16 bit quantity 
typedef unsigned int   INT32U;			// Unsigned 32 bit quantity

typedef char 	bool_t;

typedef unsigned char 	uint8_t;
typedef unsigned short 	uint16_t;
typedef unsigned int 	uint32_t;

typedef char  	sint8_t;
typedef short 	sint16_t;
typedef int 	sint32_t;

typedef unsigned int   OS_STK;			// Each stack entry is 32-bit wide(OS Stack)


// WanShell Response Signal

// Temp code
#define USART_RX_BUF_SIZE 	4

#endif
