/*
 * @Author       : lixiangjun@abupdate.com
 * @Date         : 2022-06-06
 * @FilePath     : client.h
 * @Description  : Description
 */
#ifndef __CLIENT_H__
#define __CLIENT_H__

#define DATA_SIZE   512
struct tftp_info
{
    uint16_t opc;
	union{
		uint16_t code;
		uint16_t block;
		// For a RRQ and WRQ TFTP packet
		char filename[2];
	};
	char data[DATA_SIZE];
};


/* opcode */
#define OPCODE_RRQ     (short)1
#define OPCODE_WRQ     (short)2
#define OPCODE_DATA    (short)3
#define OPCODE_ACK     (short)4
#define OPCODE_ERROR   (short)5
#define OPCODE_OACK    (short)6

/* mode */
#define MODE_NETASCII   "netascii"
#define MODE_OCTET      "octet"
#define MODE_mail       "mail"

#endif /*__CLIENT_H__*/