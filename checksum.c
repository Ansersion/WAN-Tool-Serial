#include "include.h"
#include "wanp.h"
#include "checksum.h"

uint16_t CheckSum(unsigned char * buff, unsigned int count)
{
	register unsigned long sum = 0;
	unsigned short * addr = (unsigned short *)buff;
	while(count > 1) {
		sum += *addr++;
		count -= 2;
	}
	if(count > 0) sum += *((unsigned char *)addr);
	while(sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	
	return (unsigned short)(~sum);
}

