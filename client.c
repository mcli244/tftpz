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
#include "client.h"



int creat_rrq(char buf, char *filename)
{

}

int main(int argc, char **argv)
{
    int sock_fd = -1;
    struct sockaddr_in server;
    struct sockaddr_in sender;

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
    
    // 服务器信息
    server.sin_family = AF_INET;
    server.sin_port = htons(69);
    server.sin_addr.s_addr = inet_addr("172.18.101.121");

    // 发送获取请求
    struct tftp_info send_pkg, recv_pkg;
    int len = 0;
    socklen_t addr_len;
    ssize_t r_size = -1;
    int block = 0;
    int blocksize = 1468;
    
    
    /*
      +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
      |  opc  |filename| 0 |  mode  | 0 | blksize| 0 | #octets| 0 |
      +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
    */
    memset(&send_pkg, 0xff, sizeof(send_pkg));
    send_pkg.opc = htons(OPCODE_RRQ);
    sprintf(send_pkg.filename, "%s%c%s%c%s%c%u%c%s%c%u%c%s%c%d%c", 
            "time_test.out", 0, 
            MODE_OCTET, 0, 
            "timeout", 0, 5, 0,
            "tsize", 0, 0, 0,
            "blksize", 0, blocksize, 0);

    char *ptmp = (char *)&send_pkg;
    for(int i=0; i<sizeof(send_pkg); i++)
    {
        printf("0x%x,", *(ptmp + i));
        if(*(ptmp + i) == 0xffffffff)
            break;
        len ++;
    }
    len --;
    sendto(sock_fd, &send_pkg, len, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
    printf("send.. len:%d\r\n", len);
    while(1)
    {
        r_size = recvfrom(sock_fd, &recv_pkg, sizeof(struct tftp_info), MSG_DONTWAIT, (struct sockaddr *)&sender, &addr_len);
        // if (r_size > 0 && r_size < 4)
        // {
        //     printf("Bad packet: r_size=%ld\n", r_size);
        // }
        // if(r_size >= 4 && recv_pkg.opc == htons(OPCODE_DATA) && recv_pkg.block == htons(block))
        // {
        //     // send ack
        //     printf("recv block:%d\r\n", ntohs(recv_pkg.block));
        //     send_pkg.opc = htons(OPCODE_ACK);
        //     send_pkg.block = htons(block);
        //     sendto(sock_fd, &send_pkg, sizeof(struct tftp_info), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
        //     block++;
        // }
        
        
        if(r_size <= 2)
        {
            usleep(10*1000);
            continue;
        }
        printf("ntohs(recv_pkg.opc):%d r_size:%ld\r\n", ntohs(recv_pkg.opc), r_size);
           
        switch (ntohs(recv_pkg.opc))
        {
        case OPCODE_RRQ     : 
            break;
        case OPCODE_WRQ     : 
            break;
        case OPCODE_DATA    :
            if(r_size >= 4 && recv_pkg.block == htons(block))
            {   
                memset(&send_pkg, 0, sizeof(send_pkg));
                send_pkg.opc = htons(OPCODE_ACK);
                send_pkg.block = recv_pkg.block;
                sendto(sock_fd, &send_pkg, 4, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
                if(r_size < blocksize)
                {
                    printf("tftp succ\r\n");
                }
                block++;
            }
            break;
        case OPCODE_ACK     : 
            break;
        case OPCODE_ERROR   : 

            break;
        case OPCODE_OACK    : 
            printf("recv oack\r\n");
            if(block == 0)
            {
                send_pkg.opc = htons(OPCODE_ACK);
                send_pkg.block = recv_pkg.block;
                sendto(sock_fd, &send_pkg, 4, 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
            }
            break;
        default:
            break;
        }

        usleep(10*1000);
    }
    



    close(sock_fd);
    return 0;
}

























