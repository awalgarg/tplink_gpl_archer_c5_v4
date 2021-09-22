/*
 * brief: receiving ate commands from PC with TCP and set it using iwpriv.
 * history: create, 2017.07.17, xiejiabai@tp-link.com.cn
 * note:
 */

/**************************************************************
 * headers
 *************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>  
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <signal.h>

#include "ated_iwpriv_common.h"
#include "ated_iwpriv_cmd.h"

#define DEBUG


/**************************************************************
 * typedef
 *************************************************************/
typedef unsigned char   boolean;
typedef unsigned char	u8;
typedef int				s32;
typedef unsigned int	u32;
typedef unsigned short	u16;

/**************************************************************
 * macro
 *************************************************************/

/* recv select time 
 * if idle for more than 20s, we should quit.
 * Should be long enough. 
 */
#define SOCKET_SELECT_TIME 60 /* second */

/* buffer to hold input packet */
#define INPUT_BUFFER_LEN 2048

/* we will add # to the tail of the packet, so leave some space */
#define PACKET_TAIL_RESERVED_LEN 10

/* the maxium of 802.3 is 1500+14(header). IP+tcp header is 20+20. do not fragment*/
#define SEND_PACKET_LEN 1460

/* shell command length */
#define CMD_LEN 255

/* listening queue of tcp. NOT the maxium clients! Only the queue len of waiting for connecting. */
#define LISTEN_QUEUE_LEN 5

/* if no port option, use it */
#define FIRST_DEFAULT_PORT 5000

/* try the first and then the second */
#define SECOND_DEFAULT_PORT 5001

#define CONSOLE_NAME "/dev/console"

#define DEBUG_PRINTF(...) {do{if(l_debug && l_debugStream)fprintf(l_debugStream, ##__VA_ARGS__);}while(0);}
/**************************************************************
 * extern functions
 *************************************************************/
extern int process_iwpriv_cmd(char *input, char *buf, int buflen);
extern int init_iwpriv(void);



/**************************************************************
 * static functions declaration
 *************************************************************/
static int WaitForCmd(int sock);
static int ProcessCmd(int connSock, u8 *inpkt, int len);
static int SendReply(int connSockk, const u8 *packet, int len);
static int OpenSocket(int *sock);
/**************************************************************
 * static variables
 *************************************************************/
/* port of listening, read from options*/
u16 l_port=0;

/* socket of listening */
int l_sock = 0;

/* socket of connecting. use static data here and we can close the socket if user kill the process. */
int l_connSock = 0;

int l_debug = 0;

FILE *l_debugStream = NULL;
/**************************************************************
 * static functions
 *************************************************************/
/*
 * brief: wait for cmd in L2
 * IN sock: listening socket
 * note:    
 */
static int WaitForCmd(int sock)
{
	int count = 0;
	struct timeval tv;
	fd_set readfds;
    int ret = 0;
    u8 buffer[SEND_PACKET_LEN];
    
    
	while (1) 
	{
        ret = listen(sock, LISTEN_QUEUE_LEN);
        if (ret < 0)
        {
            perror("socket listen error\n");
            return -1;
        }
        
        l_connSock = accept(sock, NULL, NULL);
        if (l_connSock < 0)
        {
            perror("socket accept error\n");
            return -1;
        }
        while (1)
        {
            FD_ZERO(&readfds);
            FD_SET(l_connSock,&readfds);

            tv.tv_sec = SOCKET_SELECT_TIME;
            tv.tv_usec=0;
            
            count = select(l_connSock+1,&readfds,NULL,NULL,&tv);
            
            if (count < 0)
            {
                /* interrupt, again */
                if (errno == EINTR)
                    continue;
                
                perror("socket select error\n");
                return -1;
            }
            else if (count == 0)
            {
                /* time out */
                /* just continue? What if the client hangup? */
#if 0
                continue;
#else
                /* quit and goto listen again */
                perror("socket timeout, return to listen\n");
                close(l_connSock);
                break;
#endif
            }
            else
            {
                memset(buffer, 0, sizeof(buffer));
                count = recvfrom(l_connSock, buffer, sizeof(buffer), 0, NULL, NULL);
                if (count > 0)
                {      
                    ProcessCmd(l_connSock, buffer, count);
                }
                else if (count == 0)
                {
                    /* peer stop, goto listen again */
                    printf("socket close, return to listen\n");
                    close(l_connSock);
                    break;
                }
                else
                {
                    perror("socket recv error\n");
                    return -1;
                }
                
            }
            
        }
    }

    /* never reach */
	return 0;
}

/*
 * brief: open socket
 * OUT:     listening socket
 * return: 0 - OK, -1 - ERROR
 */
static int OpenSocket(int *sock)
{
    /* server data */
    struct sockaddr_in server;
    
	int ret = 0;
	memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(l_port);
    
    /*create socket*/
	int listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if(listenSock < 0)
	{
		perror("socket open error\n");
		goto err;
	}
	/*bind to a sockaddr*/
	ret = bind(listenSock, (struct sockaddr*)&server, sizeof(server));
	if (ret == -1)
	{
        if (l_port == FIRST_DEFAULT_PORT)
        {
            /* try the second port */
            printf("first default port used, use second default port %d\n", SECOND_DEFAULT_PORT);
            l_port = SECOND_DEFAULT_PORT;
            server.sin_port = htons(l_port);
            ret = bind(listenSock, (struct sockaddr*)&server, sizeof(server));
        }
        if (ret == -1)
        {
            perror("socket bind error\n");
            goto err;
        }
    }
    
    int use_addr = 1;
    /* set SO_REUSEADDR, or the port will be still occupied after the process killed */
    ret = setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&use_addr, sizeof(use_addr));
    if (ret == -1)
    {
        perror("socket setsockopt error\n");
        goto err;
    }
        
    *sock =  listenSock;
	return 0;

err:
	close(listenSock);
	listenSock = -1;
	return (-1);
}


/*
 * brief: send reply
 * IN packet: packet to send
 * IN len:  packet len
 * return:  sent len
 */
static int SendReply(int connSock, const u8 *packet, int len)
{
	
	int res = 0;
	
	res = send(connSock, packet, len, 0);
	
    return res;

}



/* 
 * brief: process the incoming commands
 * IN inpkt:    input packet
 * IN len:  input packet lenhth
 * return: 0-OK, -1:ERROR
 */
static int ProcessCmd(int connSock, u8 *inpkt, int len)
{

    u8 outpkt[SEND_PACKET_LEN];
	char *argv[MAX_ARG_NUM];
    char *inputStr = NULL;
    int index = 0;
    int ret = 0;
    char *in = (char *)inpkt;
    char *out = (char *)outpkt;
    int length = 0;
    /* we support multi commands. only return the last result*/
    int argc = SplitString(in, argv, sizeof(argv), "\n");
    if ((argc == 0)) 
    {
        perror("found no string, return!\n");
        return -1;
    }
    
    
    for (index=0;index<argc;index++)
    {
        inputStr = argv[index];
        
        if ((strlen(inputStr) > 0) && (inputStr[strlen(inputStr)-1] == '\r'))
        {
            inputStr[strlen(inputStr)-1] = '\0';
        }
        
        if (strlen(inputStr) == 0)
            continue;

        DEBUG_PRINTF("recv:%s\n", inputStr);

        memset(outpkt, 0, sizeof(outpkt));
        ret = process_iwpriv_cmd(inputStr, out, sizeof(outpkt) - PACKET_TAIL_RESERVED_LEN);
        
        /* add # here as end-of-line indicator to be recognised by PC Client */
        if (ret != 0)
        {
            fprintf(stderr, "%s", (char *)outpkt);
            snprintf(out + strlen(out), sizeof(outpkt)-strlen(out) -1, "# error");
        }
        else
        {
            snprintf(out + strlen(out), sizeof(outpkt)-strlen(out) -1, "# ");
        }
        
        length = strlen(out);

        DEBUG_PRINTF("packet length: %d, send:%s\n", length, out);
        
        if (ret != 0)
            break;
    }
    
    /* send packet */
    if (length == SendReply(connSock, outpkt, length))
        return 0;
    else
        return -1;
	
}


static void usage()
{
    printf("usage:\n"
    "-p: listening port, i.e. 5000. default is 5000 and 5001\n"
    "-h: help info\n"
    "-d: enable debug message\n"
    );
}


static int get_option(int argc, char *argv[])
{
	l_port = 0;
	
    static const struct option arg_options[] = {
		{"port",	required_argument,	0, 'p'},
		{"help",		no_argument,		0, 'h'},
        {"debug",  no_argument, 0, 'd'},
		{0, 0, 0, 0}
	};	
	
    int c = 0;
    /* get options */
	while (1) 
    {
		int option_index = 0;
		c = getopt_long(argc, argv, "p:hd", arg_options, &option_index);
		if (c == -1) break;
		/* optarg is a global variable */
		switch (c) 
        {
		case 'p':
            if (sscanf(optarg, "%hd", &l_port) != 1)
            {
                goto erropt;
            }
			break;
            
            
        case 'h':
            usage();
            exit(0);
			break;
            
        case 'd':
            l_debug = 1;
			break;
            
        default:
			goto erropt;
			break;
        }
    }
    
    if (l_port == 0)
	{
		printf("use default port %d\n", FIRST_DEFAULT_PORT);
        l_port = FIRST_DEFAULT_PORT;
	}
	
	return 0;

erropt:
	perror("input parameter wrong!\n");
	usage();
	exit(-1);
    return -1;

}

/* 
 * brief:       start socket
 * return:      0:OK, -1:ERROR
 */
static int start_socket(void)
{
    
	if (OpenSocket(&l_sock) != 0)
	{
		return -1;
	}
	printf("ated_iwpriv start\n");
	
	WaitForCmd(l_sock);
    
    if (l_sock > 0)
        close(l_sock);
    
    if (l_connSock >0)
        close(l_connSock);
    
    
    return 0;
}


/* 
 * brief:   sigterm handler, close socket to release port
 * IN:      signal num 
 */
static void sigterm_handler(int sig)
{
	if (l_sock > 0)
        close(l_sock);
    
    if (l_connSock >0)
        close(l_connSock);
    
    if (l_debugStream)
        fclose(l_debugStream);
    
    exit(0);
    return;
}


static int register_sig()
{
    struct sigaction sa;
	/* delete locks before quit */
	sa.sa_handler = sigterm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
    
	if (sigaction(SIGTERM, &sa, NULL) == -1) {
		perror("sig register error, quit!\n");
        exit(-1);
	}
	
	return 0;
    
}

static int init_debug(void)
{
    if (l_debug)
    {
        l_debugStream = fopen(CONSOLE_NAME, "a");
        if (l_debugStream == NULL)
        {
            perror("debug stream open error, quit!\n");
            exit(-1);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
	setsid();
	
	get_option(argc, argv);
	
    register_sig();
    
    init_debug();
    
    /* init wl */
    init_iwpriv();
    
    start_socket();
  
        
exit:
	sigterm_handler(0);
	return 0;
}