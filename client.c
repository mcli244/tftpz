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
uint8_t rx_buf[2048] = {0};
uint8_t tx_buf[512] = {0};
static struct tftp_info_typdef tftp_inf;

int main(int argc, char **argv)
{
    int sock_fd = -1;
    struct sockaddr_in server;
    struct sockaddr_in sender;
    
    uint8_t *pkt = (uint8_t *)&tx_buf;
	uint8_t *xp;
	int len = 0;
	ushort *s;

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
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
    
    // 服务器信息
    server.sin_family = AF_INET;
    server.sin_port = htons(69);
    server.sin_addr.s_addr = inet_addr("172.18.101.121");

    tftp_inf.block_size = 512;
    tftp_inf.cur_block = 0;
    tftp_inf.timeout = 5;
    tftp_inf.tx_pkg = tx_buf;
    tftp_inf.rx_pkg = rx_buf;
    tftp_inf.tsize = 0;


    // 发送获取请求
    struct tftp_info send_pkg, recv_pkg;
    socklen_t addr_len;
    addr_len = sizeof(struct sockaddr_in);

    ssize_t r_size = -1;
    int block = 0;
    int tftp_block_size_option = 1468;
    
    /*
      +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
      |  opc  |filename| 0 |  mode  | 0 | blksize| 0 | #octets| 0 |
      +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
    */
    memset(&tx_buf, 0x0, sizeof(tx_buf));

    xp = pkt;
    s = (ushort *)pkt;
    *s++ = htons(OPCODE_RRQ);
    pkt = (uint8_t *)s;
    strcpy((char *)pkt, "Image");
    pkt += strlen("Image") + 1;
    strcpy((char *)pkt, "octet");
    pkt += 5 /*strlen("octet")*/ + 1;
    strcpy((char *)pkt, "timeout");
    pkt += 7 /*strlen("timeout")*/ + 1;
    sprintf((char *)pkt, "%u", 5);
    pkt += strlen((char *)pkt) + 1;
    pkt += sprintf((char *)pkt, "tsize%c%u%c",
            0, 0, 0);
    /* try for more effic. blk size */
    pkt += sprintf((char *)pkt, "blksize%c%d%c",
            0, tftp_block_size_option, 0);
    len = pkt - xp;

    sendto(sock_fd, &tx_buf, len, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
    printf("send.. len:%d\r\n", len);

    
    while(1)
    {
        r_size = recvfrom(sock_fd, &rx_buf, sizeof(rx_buf), MSG_WAITFORONE, (struct sockaddr *)&sender, &addr_len);
        if(r_size <= 2)
        {
            printf("TIMEPUT\r\n");
            usleep(10*1000);
            continue;
        }
        
        uint16_t *s = (uint16_t *)&rx_buf;
        printf("ntohs(*s):%d block:%d r_size:%ld\r\n", ntohs(*s), ntohs(*(s+1)), r_size);
        switch (ntohs(*s))
        {
        case OPCODE_RRQ     : 
            break;
        case OPCODE_WRQ     : 
            break;
        case OPCODE_OACK    :
            printf("OPCODE_OACK\r\n");
            /* 发送ACK */
            pkt = (uint8_t *)&tx_buf;
            xp = pkt;
            s = (ushort *)pkt;
            s[0] = htons(OPCODE_ACK);
            s[1] = htons(block);
            pkt = (uint8_t *)(s + 2);
            len = pkt - xp;
            sendto(sock_fd, &tx_buf, len, 0, (struct sockaddr *)&sender, sizeof(struct sockaddr_in));
            block++;
            break;
        case OPCODE_DATA    :
            if(*(s+1) != htons(block))
                break;

            /* 发送ACK */
            pkt = (uint8_t *)&tx_buf;
            xp = pkt;
            s = (ushort *)pkt;
            s[0] = htons(OPCODE_ACK);
            s[1] = htons(block);
            pkt = (uint8_t *)(s + 2);
            len = pkt - xp;
            sendto(sock_fd, &tx_buf, len, 0, (struct sockaddr *)&sender, sizeof(struct sockaddr_in));
            if(r_size - 4 < tftp_block_size_option)
            {
                block = 0;
                printf("recv succ\r\n");
                goto exit;
            }else{
                block++;
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

        //usleep(10*1000);
    }
    


exit:
    close(sock_fd);
    return 0;
}

























