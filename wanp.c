#include <string.h>
#include <stdarg.h>
#include "wanp.h"
#include "checksum.h"

// const unsigned char * WAN_EndFlag 	= "\r\n\r\n";
const unsigned char WAN_EndFlag[] 	= {'\r', '\n', '\r', '\n', '\0'};
const unsigned char WAN_RespTag[] 	= {'=', '\0'};
const unsigned char WAN_RespOK[] 	= {'=', 'O', 'K', '\0'}; 
const unsigned char WAN_RespErr[] 	= {'=', 'E', 'R', 'R', '\0'};
const int WAN_EndFlagSize = 4; // strlen(WAN_EndFlag);
const int WAN_RespTagSize = 1; // strlen(WAN_RespTag);

extern const Cmd Hello;
extern const Cmd Burn;
extern const Cmd Startos;
// const Cmd * CmdSet[] = {&Hello, &Burn, &Startos, NULL};
// const int CmdNum = sizeof(CmdSet) / sizeof(Cmd *);

uint16_t Wan_htons(uint16_t hostshort)
{
    uint16_t ret;
#ifndef BIG_END
    ret = hostshort & 0x00ff;
    ret <<= 8;
    ret |= (hostshort & 0xff00) >> 8;
#else
	ret = hostshort;
#endif
    return ret;
}



int Wan_InitHeader(uint8_t * msg)
{
	if(!msg) return -1;
	msg[0] = 'W'; // WAN Protocal magic
	msg[1] = 'A'; // WAN Protocal magic
	msg[2] = 'N'; // WAN Protocal magic
	msg[3] = 0; // next to store checksum 
	msg[4] = 0; // next to store checksum
	msg[5] = 0; // reserved
	msg[6] = 0; // next to store msg size
	msg[7] = 0; // next to store msg size
	return 0;
}

int Wan_CheckMsg(uint8_t * msg, uint32_t size)
{
	if(!msg) {
		return -1;
	}
	// least msg size: WAN_HEADER_SIZE + WAN_EndFlag
	if(size < WAN_HEADER_SIZE + WAN_EndFlagSize) {
#ifdef DEBUG
		printf("Wan_CheckMsg: size < WAN_HEADER_SIZE + WAN_EndFlag\n");
#endif
		return -1;
	}
	if(Wan_DoMagicCheck(msg) != 0) {
#ifdef DEBUG
		printf("Wan_CheckMsg: !Wan_DoMagicCheck\n");
		printf("magic: %x%x%x\n", msg[0], msg[1], msg[2]);
#endif
		return -1;
	}
	if(Wan_DoChecksum(msg, size) != 0) {
#ifdef DEBUG
		printf("Wan_CheckMsg: !Wan_DoChecksum\n");
#endif
		return -1;
	}
	return 0;
}

int Wan_DoMagicCheck(uint8_t * msg)
{
	if(!msg) return -1;
	if( 'W' != msg[0] || 
		'A' != msg[1] ||
		'N' != msg[2] ) {
			return -1;
		}
	return 0;
}

int Wan_DoChecksum(uint8_t * msg, uint32_t msg_size)
{
	if(!msg) return -1;
	if(CheckSum(msg, msg_size) != 0) {
		return -1;
	}
	return 0;
}

// int Wan_GetSize(const uint8_t * msg, uint32_t * size)
int Wan_GetSize(const uint8_t * msg, uint16_t * size)
{
	if(!msg || !size) return -1;
	*size = msg[6];
	*size <<= 8;
	*size += msg[7];
	return 0;
}

int Wan_Get_ReqCmd(uint8_t * req_msg, uint8_t * cmd)
{
	if(!req_msg || !cmd) return -1;
	req_msg += WAN_HEADER_SIZE;
	while((*cmd = *req_msg)) {
#ifdef DEBUG
		printf("%x ", *cmd);
#endif
		if(' ' == *cmd || '\r' == *cmd) {
			*cmd = '\0';
			break;
		}
		cmd++;
		req_msg++;
	}
	printf("\n");
	return 0;
}

// int Wan_Get_ReqOpt(uint8_t * req_msg, uint8_t * cmd, uint8_t * opt_flag, uint8_t * opt)
// {
// 	int i, j;
// 	const char * req = (const char *)req_msg;
// 	if(!req || !cmd || !opt_flag || !opt) return -1;
// 	req += WAN_HEADER_SIZE;
// 	for(i = 0; i < CmdNum; i++) {
// 		if(strcmp((const char *)cmd, CmdSet[i]->Name) == 0) break;
// 	}
// 	if(CmdSet[i] == NULL) {
// 		// printf("not found command\r\n");
// 		return -1;
// 	}
// 
// 	for(j = 0; CmdSet[i]->Opts[j] != NULL; j++) {
// 		if(strcmp((const char *)opt_flag, CmdSet[i]->Opts[j]) == 0) break;
// 	}
// 	if(CmdSet[i]->Opts[j] == NULL) {
// 		// printf("not found option\r\n");
// 		return -1;
// 	}
// 	// FIXME: 
// 	if(!(req = strstr(req, CmdSet[i]->Name))) {
// 		// printf("not found command in msg\r\n");
// 		return -1;
// 	}
// 	if(!(req = strstr(req, CmdSet[i]->Opts[j]))) {
// 		// printf("not found option in msg\r\n");
// 		return -1;
// 	}
// 	req += strlen(CmdSet[i]->Opts[j]) + 1; // '1' is the size of a blank
// 	// printf("req_msg: %s\n", req_msg);
// 
// 	while((*opt = *req)) {
// 		if(' ' == *opt || '\r' == *opt) {
// 			*opt = '\0';
// 			break;
// 		}
// 		opt++;
// 		req++;
// 	}
// 	return 0;
// }

int Wan_Set_RespTag(uint8_t * resp_msg)
{
	const unsigned char * resp_tag = WAN_RespTag;
	if(!resp_msg) return -1;
	resp_msg += WAN_HEADER_SIZE;
	while((*resp_msg++ = *resp_tag++) != '\0')
		;
	return 0;
}

int Wan_Set_RespCmd(uint8_t * resp_msg, uint8_t * cmd)
{
	if(!resp_msg || !cmd) return -1;
	if(Wan_Set_RespTag(resp_msg) != 0) return -1;
	resp_msg += WAN_HEADER_SIZE;
	while(*++resp_msg != '\0')
		;
	while((*resp_msg++ = *cmd++) != '\0')
		;
	return 0;
}

int Wan_Set_RespOK(uint8_t * resp_msg)
{
	const unsigned char * resp_ok = WAN_RespOK;
	if(!resp_msg) return -1;
	resp_msg += WAN_HEADER_SIZE;
	while(*++resp_msg != '\0')
		;
	while((*resp_msg++ = *resp_ok++) != '\0')
		;
	return 0;
}

int Wan_Set_RespErr(uint8_t * resp_msg)
{
	const unsigned char * resp_err = WAN_RespErr;
	if(!resp_msg) return -1;
	resp_msg += WAN_HEADER_SIZE;
	while(*++resp_msg != '\0')
		;
	while((*resp_msg++ = *resp_err++) != '\0')
		;
	return 0;
}

int Wan_Set_RespAppendMsg(uint8_t * resp_msg, uint8_t * append_msg)
{
	if(!resp_msg || !append_msg) return -1;
	resp_msg += WAN_HEADER_SIZE;
	while(*++resp_msg != '\0')
		;
	*resp_msg++ = '\r';
	*resp_msg++ = '\n';
	while((*resp_msg++ = *append_msg++) != '\0')
		;
	return 0;
}

int Wan_Set_RespEndFlag(uint8_t * resp_msg)
{
	const unsigned char * resp_end_flag = WAN_EndFlag;
	if(!resp_msg) return -1;
	resp_msg += WAN_HEADER_SIZE;
	while(*++resp_msg != '\0')
		;
	while((*resp_msg++ = *resp_end_flag++) != '\0')
		;
	return 0;
}

