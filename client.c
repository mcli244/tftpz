/*
 * @Author       : lixiangjun@abupdate.com
 * @Date         : 2022-06-06
 * @FilePath     : client.c
 * @Description  : Description
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include "client.h"


struct tftp_info_typdef
{
    uint8_t *tx_pkg;
    uint8_t *rx_pkg;
    uint64_t cur_block;
    uint16_t block_size;
    uint16_t tsize;
    uint16_t timeout;   // 单位：秒
    char filename[256];
};

#define RX_BUF_SIZE 2048
#define TX_BUF_SIZE 512

uint8_t g_rx_buf[RX_BUF_SIZE] = {0};
uint8_t g_tx_buf[TX_BUF_SIZE] = {0};
static struct tftp_info_typdef tftp_inf;

void start_tftp_get(int sock_fd, struct sockaddr_in *server, struct tftp_info_typdef *inf)
{
    uint8_t *pkt = (uint8_t *)inf->tx_pkg;
	uint8_t *xp;
	int len = 0;
	ushort *s;

    /*
      +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
      |  opc  |filename| 0 |  mode  | 0 | blksize| 0 | #octets| 0 |
      +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
    */
    xp = pkt;
    s = (ushort *)pkt;
    *s++ = htons(OPCODE_RRQ);
    pkt = (uint8_t *)s;
    strcpy((char *)pkt, inf->filename);
    pkt += strlen(inf->filename) + 1;
    strcpy((char *)pkt, "octet");
    pkt += 5 /*strlen("octet")*/ + 1;
    strcpy((char *)pkt, "timeout");
    pkt += 7 /*strlen("timeout")*/ + 1;
    sprintf((char *)pkt, "%u", inf->timeout);
    pkt += strlen((char *)pkt) + 1;
    pkt += sprintf((char *)pkt, "tsize%c%u%c",
            0, inf->tsize, 0);
    /* try for more effic. blk size */
    pkt += sprintf((char *)pkt, "blksize%c%d%c",
            0, inf->block_size, 0);
    len = pkt - xp;

    sendto(sock_fd, inf->tx_pkg, len, 0, (struct sockaddr *)server, sizeof(struct sockaddr_in));
}

int main(int argc, char **argv)
{
    int sock_fd = -1;
    struct sockaddr_in server;
    struct sockaddr_in sender;
    socklen_t addr_len;
	uint8_t *xp, *pkt;
	int len = 0;
    ssize_t r_size = -1;


    if(argc != 4)
    {
        printf("Usage: client [serverip] [port] [filename]\r\n"
                "\teg:client 172.18.101.121 69 test.txt\r\n");
        return 0;
    }
    
    addr_len = sizeof(struct sockaddr_in);
    // UDP常用socket 创建
    // SOCK_DGRAM 数据报 套接字
    // IPPROTO_UDP 协议UDP
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock_fd < 0)
    {
        perror("socket");
        return -1;
    }

    // 绑定网卡
    struct ifreq ifr;
    memset(&ifr, 0x00, sizeof(ifr));
    strncpy(ifr.ifr_name, "ens38", strlen("ens38"));
    setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr));
    
    /* 设置接收超时时间 */
    struct timeval tv = {10, 0};
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
    
    // 服务器信息
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);

    tftp_inf.block_size = 512;
    tftp_inf.cur_block = 0;
    tftp_inf.timeout = 5;
    tftp_inf.tx_pkg = g_tx_buf;
    tftp_inf.rx_pkg = g_rx_buf;
    tftp_inf.tsize = 0;
    strcpy(tftp_inf.filename, argv[3]);

    printf("tftp start\r\n");
    printf("serverip:%s port:%d\r\n",  inet_ntoa(server.sin_addr), ntohs(server.sin_port));
    printf("filename:%s\r\n", tftp_inf.filename);
    start_tftp_get(sock_fd, &server, &tftp_inf);

    while(1)
    {
        r_size = recvfrom(sock_fd, tftp_inf.rx_pkg, RX_BUF_SIZE, MSG_WAITFORONE, (struct sockaddr *)&sender, &addr_len);
        if(r_size <= 2)
        {
            printf("tftp recv timeout\r\n");
            goto exit;
        }
        
        uint16_t *s = (uint16_t *)tftp_inf.rx_pkg;
        // printf("ntohs(*s):%d block:%d r_size:%ld\r\n", ntohs(*s), ntohs(*(s+1)), r_size);
        switch (ntohs(*s))
        {
        case OPCODE_RRQ     : 
            break;
        case OPCODE_WRQ     : 
            break;
        case OPCODE_OACK    :
            /* 发送ACK */
            pkt = (uint8_t *)tftp_inf.tx_pkg;
            xp = pkt;
            s = (ushort *)pkt;
            s[0] = htons(OPCODE_ACK);
            s[1] = htons(tftp_inf.cur_block);
            pkt = (uint8_t *)(s + 2);
            len = pkt - xp;
            sendto(sock_fd, tftp_inf.tx_pkg, len, 0, (struct sockaddr *)&sender, sizeof(struct sockaddr_in));
            tftp_inf.cur_block++;
            printf("#");
            fflush(stdout);
            break;
        case OPCODE_DATA    :
            if(*(s+1) != htons(tftp_inf.cur_block))
                break;
            if((tftp_inf.cur_block % 512) == 1)
            {
                printf("#");
                fflush(stdout);
            }
                
            /* 发送ACK */
            pkt = (uint8_t *)tftp_inf.rx_pkg;
            xp = pkt;
            s = (ushort *)pkt;
            s[0] = htons(OPCODE_ACK);
            s[1] = htons(tftp_inf.cur_block);
            pkt = (uint8_t *)(s + 2);
            len = pkt - xp;
            sendto(sock_fd, tftp_inf.rx_pkg, len, 0, (struct sockaddr *)&sender, sizeof(struct sockaddr_in));
            if(r_size - 4 < tftp_inf.block_size)
            {
                tftp_inf.cur_block = 0;
                printf("recv succ\r\n");
                goto exit;
            }else{
                tftp_inf.cur_block++;
            }
            
            break;
        case OPCODE_ACK     : 
            break;
        case OPCODE_ERROR   : 
            printf("OPCODE_ERROR\r\n");
            break;
        default:
            break;
        }
    }
    
exit:
    close(sock_fd);
    return 0;
}

























