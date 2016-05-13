#ifndef CMD_H
#define CMD_H

#include "wanp.h"
#include "include.h"

#define HELP_HELLO 		100
#define HELP_BURN 		120
#define HELP_STARTOS 	140

int RespOK(uint8_t * buf, uint8_t * cmd, uint8_t * append_msg);
int RespErr(uint8_t * buf, uint8_t * cmd, uint8_t * reason);
int DoHello(uint8_t * msg);
int DoBurn(uint8_t * msg, uint8_t * kernel_name, uint8_t * addr, uint8_t * kernel_size, uint8_t * crc);
int DoStartos(uint8_t * msg);
int SealPacket(uint8_t *msg);

#endif
