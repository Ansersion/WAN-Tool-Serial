#ifndef WANP_H
#define WANP_H

#include "include.h"

#define WAN_HEADER_SIZE 	8
#define CMD_MAX_OPTS 		8

typedef struct Cmd {
	char * Name;
	char * Opts[CMD_MAX_OPTS];
}Cmd;

typedef struct Opt {
	char * OptFlag;
	char * Opt;
}Opt;

uint16_t Wan_htons(uint16_t hostshort);

int Wan_CheckMsg(uint8_t * msg, uint32_t size);

// Initializing WAN message header
int Wan_InitHeader(uint8_t * msg);
// Check message magic "WAN";
int Wan_DoMagicCheck(uint8_t * msg);
// Checksum
int Wan_DoChecksum(uint8_t * msg, uint32_t size);
// Get the 'size' of WAN message header
// int Wan_GetSize(const uint8_t * msg, uint32_t * size);
int Wan_GetSize(const uint8_t * msg, uint16_t * size);

// int Wan_GetSize(uint8_t * msg, uint16_t * size);
int Wan_GetChecksum(uint8_t * msg, uint16_t * csum);
int Wan_GetCmd(uint8_t * msg, char * cmd);

int Wan_Get_ReqCmd(uint8_t * req_msg, uint8_t * cmd);
// int Wan_Get_ReqCmd(const char * req_msg, uint8_t * cmd);
int Wan_Get_ReqOpt(uint8_t * req_msg, uint8_t * cmd, uint8_t * opt_flag, uint8_t * opt);

int Wan_Set_RespTag(uint8_t * resp_msg);
int Wan_Set_RespCmd(uint8_t * resp_msg, uint8_t * cmd);
int Wan_Set_RespOK(uint8_t * resp_msg);
int Wan_Set_RespErr(uint8_t * resp_msg);
int Wan_Set_RespAppendMsg(uint8_t * buf, uint8_t * append_msg);
int Wan_Set_RespEndFlag(uint8_t * resp_msg);

// void Wan_Response_OK(const char * msg_append);
// void Wan_Response_OptError(const char * msg_append);

// 
// int strcmp(const char *str1,const char *str2);
	
#endif

