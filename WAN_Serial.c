#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/select.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h> 

#include "wanp.h"
#include "checksum.h"

#define WAN_HEADER_SIZE 	8

#define WAN_MSG_MAX_SIZE 	65536
// #define TIME_OUT 			30 	// in unit of second
#define TIME_OUT 			3 	// in unit of second
#define WAN_PORT 			33027
#define MAX_KERNEL_SIZE 	65536

#define DEFAULT_DATABIT CS8
#define DEFAULT_PAR ~PARENB
#define DEFAULT_INP ~INPCK
#define DEFAULT_STOPBIT ~CSTOPB
#define DEFAULT_INTERVAL 1


const char * Version = "1.0";
unsigned char SendBuffer[WAN_MSG_MAX_SIZE];
unsigned char RecvBuffer[WAN_MSG_MAX_SIZE];
unsigned char BufTmp[WAN_MSG_MAX_SIZE];
unsigned char KernelBuffer[MAX_KERNEL_SIZE];

extern const unsigned char WAN_EndFlag[];
extern const int WAN_EndFlagSize;
extern const Cmd Burn;
#ifdef WANP
int GotResponse = 1;
#endif

typedef struct OptionsData {
	char dev[256];
	unsigned int baud_rate;
	int use_test_server;
}OptionsData;

void SendCmd(int fd, unsigned char * cmd, unsigned int buf_size);
void * threadSend(void * arg);
// void Wan_InitHeader(uint8_t * cmd);
int RecvResponse(int fd, unsigned char *response, unsigned int buf_size, unsigned int time_out);
void * threadRecv(void * arg);
// uint16_t CheckSum(unsigned char * buff, unsigned int count);
int CreateSocket();
int CreateDevFd(const OptionsData * user_data);
void ReadOptions(int argc, char* argv[], OptionsData * user_data);
void PrintHelp();

static void SIGINT_handler();
static void SIGINT_handler()
{
	// fflush(stdout);
	fprintf(stderr, "\n# "); // Prompt
}


void main(int argc, char * argv[]) {
	int fd = -1;
	pthread_t send_thread;
	pthread_t recv_thread;
	signal(SIGINT, SIGINT_handler);
	OptionsData OpData;
	ReadOptions(argc, argv, &OpData);
	if(OpData.use_test_server) {
	 	printf("// WAN OS Tool Serial: v%s\n", Version);
	 	printf("// Connect to WAN server 127.0.0.1:33027\n");
	 	printf("\n");
	 	fd = CreateSocket();
	} else {
		fd = CreateDevFd(&OpData);
	}
	if(fd < 0) {
		fprintf(stderr, "connection failed\n");
		exit(2);
	}
	pthread_create(&send_thread, NULL, threadSend, (void *)&fd);
	sleep(1);
	pthread_create(&recv_thread, NULL, threadRecv, (void *)&fd);
	pthread_join(recv_thread, NULL);
	// printf("# "); // Prompt
	// while(1) {
	// 	sleep(1);
	// 	// SendCmd(fd, SendBuffer, sizeof(SendBuffer));
	// 	// // printf("r: %s", Buffer + WAN_HEADER_SIZE);
	// 	// RecvResponse(fd, RecvBuffer, sizeof(RecvBuffer), TIME_OUT);
	// }
}

void SendCmd(int fd, unsigned char * cmd, unsigned int buf_size)
{
	unsigned int i;
	uint16_t csum;
	uint16_t msg_size;
	uint16_t tmp;
	char kernel_name[128];
	char kernel_size[128];
	int i_kernel_size;
	int inspace = 0;
	uint8_t cmd_tmp[128];
	int tmp_fd;

	char * save_ptr;
	char * ptr;
	char * endptr;
	// Prefix the header
#ifdef WANP
	if(GotResponse) {
		GotResponse = 0;
		printf("\n# "); // Prompt
	} else {
		return;
	}
#else
	usleep(50000);
	printf("\n# "); // Prompt
#endif
#ifdef WANP
	Wan_InitHeader(cmd);
	for(i = WAN_HEADER_SIZE; i < buf_size - WAN_EndFlagSize - 1; i++) {
		*(cmd + i) = getchar();
		/* To gather multi-continuous blanks together as one */
		if(*(cmd + i) == ' ') {
			if(inspace == 0) {
				inspace = 1;
			} else {
				i--;
				continue;
			}
		} else {
			inspace = 0;
		}

		/* Get 'ENTER' and send the command message */
		if(*(cmd + i) == '\r' || *(cmd + i) == '\n') {
			*(cmd + i) = '\0';
			printf("\n");
			printf("%s\n", cmd+WAN_HEADER_SIZE);
			memcpy(cmd+i, WAN_EndFlag, WAN_EndFlagSize);
			msg_size = i + WAN_EndFlagSize;
			tmp = htons(msg_size);
			memcpy(&cmd[6], &tmp, sizeof(tmp));
			csum = htons(CheckSum(cmd, msg_size));
			memcpy(&cmd[3], &csum, sizeof(csum));
			printf("msg_size: %d\n", msg_size);
			if(write(fd, cmd, msg_size) < 0) {
				perror("write");
			}
			// if(send(fd, cmd, msg_size, 0) < 0) {
			// 	perror("send");
			// }
			return;
		}
	}
#else
	for(i = 0; i < buf_size - 1; i++) { // reserve 1 character for '\0'
		*(cmd + i) = getchar();
		/* To gather multi-continuous blanks together as one */
		if(*(cmd + i) == ' ') {
			if(inspace == 0) {
				inspace = 1;
			} else {
				i--;
				continue;
			}
		} else {
			inspace = 0;
		}

		/* Get 'ENTER' and send the command message */
		if(*(cmd + i) == '\n') {
			// printf("\n");
			// printf("%s\n", cmd);
			*(cmd+i+1) = '\0';
			msg_size = strlen(cmd);
			strcpy(BufTmp, cmd);
			GetCmd(BufTmp, cmd_tmp);

			*kernel_name = '\0';
			*kernel_size = '\0';
			if(strcmp(cmd_tmp, Burn.Name) == 0) {
				i_kernel_size = 0;
				ptr = strtok_r((char *)BufTmp, " \r", &save_ptr);
				while(ptr != NULL) {
					if(strcmp(ptr, "-f") == 0) {
						ptr = strtok_r(NULL, " \r", &save_ptr);
						if(NULL == ptr) {
							fprintf(stderr, "Can't find the kernel file\n");
							return;
						}
						strcpy(kernel_name, ptr);
					} else if(strcmp(ptr, "-s") == 0) {
						ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
						if(NULL == ptr) {
							fprintf(stderr, "Can't find the kernel size\n");
							return;
						}
						strcpy(kernel_size, ptr);
					} else {
						ptr = strtok_r(NULL, " \r", &save_ptr); // '\r' is the used for WAN message
					}
				}
				if('0' == kernel_size[0] && ('x' == kernel_size[1] || 'X' == kernel_size[1])) {
					i_kernel_size = strtol((const char *)kernel_size, &endptr, 16);
				} else {
					i_kernel_size = atoi((const char *)kernel_size);
				}
				if((tmp_fd = open(kernel_name, O_RDONLY)) < 0) {
					fprintf(stderr, "Can't open the kernel file\n");
					return;
				}
				read(tmp_fd, KernelBuffer, i_kernel_size);
				printf("%s=%d\n", kernel_name, i_kernel_size);
			}
			
			// printf("msg_size: %d\n", msg_size);
			if(write(fd, cmd, msg_size) < 0) {
				perror("write");
			}
			if(strlen(kernel_name) > 0 && i_kernel_size > 0) {
				printf("start to burning kernel...");
				sleep(1);
				if(write(fd, KernelBuffer, i_kernel_size) < 0) {
					perror("write");
				}
			}
			// if(send(fd, cmd, msg_size, 0) < 0) {
			// 	perror("send");
			// }
			return;
		}
	}

#endif
#ifdef WANP
	fprintf(stderr, "WAN_Serial: Command too long(longer than %d bytes)\n", buf_size - WAN_EndFlagSize - 1);
	/* Send a blank line instead */
	memcpy(cmd+WAN_HEADER_SIZE, WAN_EndFlag, WAN_EndFlagSize);
	msg_size = WAN_HEADER_SIZE + WAN_EndFlagSize;
	tmp = htons(msg_size);
	memcpy(&cmd[6], &tmp, sizeof(tmp));
	csum = htons(CheckSum(cmd, msg_size));
	memcpy(&cmd[3], &csum, sizeof(csum));
#else 
	fprintf(stderr, "WAN_Serial: Command too long(longer than %d bytes)\n", buf_size - 1);
#endif
	if(write(fd, cmd, msg_size) < 0) {
		perror("write");
	}
	// if(send(fd, cmd, msg_size, 0) < 0) {
	// 	perror("send");
	// }
	// write(fd, cmd, msg_size);

	/* Clear the characters left */
	while(getchar() != '\n') {
	}
	fflush(stdin);
	// fgets(Cmd + WAN_HEADER_SIZE, WAN_MSG_MAX_SIZE - WAN_HEADER_SIZE, stdin);
	// fgets(Cmd + WAN_HEADER_SIZE, 3, stdin);
	// printf("r:");
	// while((ch = *(cmd + WAN_HEADER_SIZE + (i++))) != '\0') {
	// 	printf("%x", ch);
	// }
	// printf("\n");
}

#ifdef WANP
int RecvResponse(int fd, unsigned char *response, unsigned int buf_size, unsigned int time_out)
{
	int i;
	unsigned int msg_size = 0;
	unsigned int tmp = 0;
	fd_set 		 rset;
	struct timeval tv;

	int recv_size;

	tv.tv_sec = time_out;
	tv.tv_usec = 0;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	while(!FD_ISSET(fd, &rset)) {
		switch(select(fd + 1, &rset, NULL, NULL, &tv) < 0) {
			case -1:
				exit(1);
				break;
			case 0:
				printf("timeout\n");
				return -1;
				break;
			default: 
				break;
		}
	}
	// sleep(1);

	// 1. Read Header
	// recv_size = recv(fd, response, WAN_HEADER_SIZE, MSG_WAITALL);
	recv_size = Recv(fd, response, WAN_HEADER_SIZE);
	// printf("recv: %d\n", recv_size);
	// for(i = 0; i < WAN_HEADER_SIZE; i++) {
	// 	printf("response[%d]=%x\n", i, response[i]);
	// }
	msg_size = response[6];
	msg_size = msg_size << 8;
	msg_size += response[7];
	// printf("%c%c%c\n", response[0], response[1], response[2]);
	// printf("msg_size: %d\n", msg_size);
	// printf("msg_size: %d\n", msg_size);
	// 2. Read all and get msg size
	// // sleep(1);
	// recv_size = recv(fd, response + WAN_HEADER_SIZE, msg_size - WAN_HEADER_SIZE, MSG_WAITALL);
	recv_size = Recv(fd, response + WAN_HEADER_SIZE, msg_size - WAN_HEADER_SIZE);
	// printf("recv: %d\n", recv_size);
	// printf("recv: %d\n", recv(fd, response + WAN_HEADER_SIZE, msg_size - WAN_HEADER_SIZE, MSG_WAITALL));
	// for(i = 0; i < msg_size; i++) {
	// 	printf("response[%d]=%x\n", i, response[i]);
	// }
	// tmp = WAN_HEADER_SIZE;
	// while(tmp > WAN_HEADER_SIZE + 10) {
	// 	printf("%2x ", response[tmp]);
	// 	tmp++;
	// }
	// msg_size > buf_size => error;
	if(Wan_CheckMsg(response, msg_size) != 0) {
		fprintf(stderr, "Peer Error: Not support WAN Protocol or internal error\n");
		return -2;
	}
	response[msg_size - WAN_EndFlagSize] = '\0';
	return 0;
	// printf("%d: %s\n", msg_size, &response[WAN_HEADER_SIZE]);
	// print msg if exist;
}
#else

int RecvResponse(int fd, unsigned char *response, unsigned int buf_size, unsigned int time_out)
{
	int i;
	int readResult;
	unsigned int msg_size = 0;
	unsigned int tmp = 0;
	fd_set 		 rset;
	struct timeval tv;
	int recv_size;
	char ch;

	tv.tv_sec = time_out;
	tv.tv_usec = 0;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	while(!FD_ISSET(fd, &rset)) {
		switch(select(fd + 1, &rset, NULL, NULL, &tv) < 0) {
			case -1:
#ifdef DEBUG
				printf("select -1\n");
#endif
				exit(1);
				break;
			case 0:
				printf("timeout\n");
				return -1;
				break;
			default: 
				break;
		}
	}
	// if(recv(fd, &ch, 1, 0) != 1) {
	// 	perror("recv");
	// 	close(fd);
	// 	break;
	// }
	readResult = read(fd, &ch, 1);
	if(readResult < 0) {
		perror("Recv");
		close(fd);
		return -2;
	} else if(readResult == 0) {
#ifdef DEBUG
		printf("readResult == 0\n");
#endif
		return 0;
	}
	// printf("%c-\n", ch);
	printf("%c", ch);
	return 0;

	// for(i = 0; i < buf_size; i++) {
	// 	if(recv(fd, response+i, 1, 0) != 1) {
	// 		perror("recv");
	// 		close(fd);
	// 		break;
	// 	}
	// 	if('\n' == response[i]) {
	// 		msg_size = i + 1;
	// 		break;
	// 	}
	// }
	// printf("%d: %s\n", msg_size, response);
}

#endif

int CreateSocket()
{
	int sock_fd;
	int 				reuse = 1;
	struct sockaddr_in ser_addr; 
	memset(&ser_addr, 0, sizeof(ser_addr));  
	ser_addr.sin_family = AF_INET;  
	inet_aton("127.0.0.1", (struct in_addr *)&ser_addr.sin_addr);  
	ser_addr.sin_port = htons(WAN_PORT);  
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);  
	if(sock_fd < 0) {
		perror("socket");
		return -1;
	}
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopet");
		exit(1);
	}
	if(connect(sock_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0) {
		perror("connect");
		return -1;
	}
	return sock_fd;
}

int CreateDevFd(const OptionsData * user_data)
{
	struct termios term_opt;
	int fd;
	int i;
	int speed_arr[] = {B38400, B19200, B9600, B4800, B2400, B1200, B300,
		B38400, B19200, B9600, B4800, B2400, B1200, B300};
	int name_arr[] = {38400, 19200, 9600, 4800, 2400, 1200, 300, 38400, 19200, 9600, 4800, 2400, 1200, 300};

	printf("Try to open stty: %s\n", user_data->dev);
	printf("baud rate: %d\n", user_data->baud_rate);

	fd = open(user_data->dev, O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd < 0) {
		perror("open");
		return -1;
	}
	if(fcntl(fd, F_SETFL, 0) < 0) {
		perror("fcntl");
		close(fd);
		return -1;
	}
	if(!isatty(fd)) {
		fprintf(stderr, "%s is not a tty\n", user_data->dev);
		close(fd);
		return -1;
	}

	/* Set options for fd */
	if(tcgetattr(fd, &term_opt) != 0) {
		close(fd);
		return -1;
	}
	bzero(&term_opt, sizeof(term_opt));
	for (i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
		if(user_data->baud_rate == name_arr[i]) {       
			cfsetispeed(&term_opt, speed_arr[i]); 
			cfsetospeed(&term_opt, speed_arr[i]);  
		}
	}     
	tcsetattr(fd, TCSANOW, &term_opt);

	term_opt.c_cflag |= CLOCAL | CREAD;
	term_opt.c_cflag &= ~CSIZE;
	term_opt.c_cflag |= DEFAULT_DATABIT;
	term_opt.c_cflag &= DEFAULT_PAR;
	term_opt.c_iflag &= DEFAULT_INP;
	term_opt.c_cflag &= DEFAULT_STOPBIT;

	tcflush(fd, TCIOFLUSH);
	term_opt.c_cc[VTIME] = DEFAULT_INTERVAL;
	term_opt.c_cc[VMIN] = 0;
	if(tcsetattr(fd, TCSANOW, &term_opt) != 0) {
		close(fd);
		return -1;
	}

	return fd;
}

void ReadOptions(int argc, char* argv[], OptionsData * user_data)
{
	int i = 0;

	if(!user_data) return;
	// Default configure
	strcpy(user_data->dev, "/dev/ttyS0");
	user_data->baud_rate = 9600;
	user_data->use_test_server = 0;

	if (argc <= 1) {
		return;
	}
	while(++i < argc) {
		if (strcmp(argv[i], "--help") == 0 ) {
			PrintHelp();
			exit(0);
		} else if(strcmp(argv[i], "-h") == 0) {
			PrintHelp();
			exit(0);
		} else if ( strncmp( argv[i], "-b", strlen("-b")) == 0 ) {
			if(i++ < argc) {
				user_data->baud_rate = atoi(argv[i]);
			} else {
				printf("No option with '-b'\n");
				exit(1);
			}
		} else if ( strncmp( argv[i], "-d", strlen("-d")) == 0 ) {
			if(i++ < argc) {
				strcpy(user_data->dev, argv[i]);
			} else {
				printf("No option with '-d'\n");
				exit(1);
			}
		} else if(strncmp(argv[i], "-s", strlen("-s")) == 0) {
			user_data->use_test_server = 1;
		}
	}
}

void PrintHelp()
{
	printf("\nOptions:");
	printf(" --help      	print this message.\n");
	printf(" -h 		  	like '--help'.\n");
	printf(" -d <stty> 		set stty, default: '/dev/ttyS0'\n" );
	printf(" -b <baud rate> set baud rate for stty, default: '9600'\n");
	printf("\n");
}

int Recv(int fd, void * buf, size_t len)
{
	size_t bytesRead = 0;
	size_t readLen = len;
	int readResult;

	if(fd < 0 || !buf) {
		return -1;
	}
	while(bytesRead < len) {
		readResult = read(fd, buf, readLen);
		if(readResult < 0) {
			perror("read");
			return -1;
		} else if(0 == readResult) {
			printf("socket closed\r\n");
			return bytesRead;
		}
		bytesRead += readResult;
		readLen -= readResult;
	}
	return bytesRead;
}

void * threadSend(void * arg)
{
	int * fd;
	if(!arg) {
		pthread_exit("error argument");
		return NULL;
	}

	fd = (int *)arg;
	while(1) {
		SendCmd(*fd, SendBuffer, sizeof(SendBuffer));
	}
}

void * threadRecv(void * arg)
{
	int fd;
	int Err = 0;
	if(!arg) {
		pthread_exit("error argument");
		return NULL;
	}

	fd = *((int *)arg);
	while(!Err) {
		Err = RecvResponse(fd, RecvBuffer, sizeof(RecvBuffer), TIME_OUT);
#ifdef WANP
		if(!Err) {
			GotResponse = 1;
		}
#endif
	}
}
