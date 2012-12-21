/*
 * Copyright (c) 2012, Ivo Vachkov
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BPROT_H_
#define _BPROT_H_

/* Global constants */
#define MAXLEN		1500
#define BPROT_VERSION	13

/* BPROT flags */
#define BPROT_F1	0x01
#define BPROT_F2	0x02
#define BPROT_F3	0x04
#define BPROT_F4	0x08

/* BPROT Packet Types */
typedef enum {
	BPROT_UNKNOWN = 0,
	BPROT_PING,
	BPROT_PONG,
	BPROT_DATA,
	BPROT_ACK,
	BPROT_ERROR
} bprot_type_t;

typedef struct _header_t {
	unsigned int version:4;
	unsigned int flags:4;
	unsigned char type;
	unsigned short data_size;
} bprot_header_t;

typedef union {
	bprot_header_t h_fields;
	unsigned int h_word;
} bprot_packet_t;

/* Packet parsing primitives */
int bprot_get_version(char *buf, unsigned int len);
int bprot_check_flag(char *buf, unsigned int len, unsigned int flag);
bprot_type_t bprot_get_type(char *buf, unsigned int len);
int bprot_get_data_size(char *buf, unsigned int len);
char * bprot_get_data(char *buf, unsigned int len);

/* Send BPROT_PING and BPROT_PONG messages */
int bprot_send_ping(int socket, char *data, unsigned int data_size);
int bprot_send_pong(int socket, char *data, unsigned int data_size);

/* Send BPROT_DATA message */
int bprot_send_data(int socket, char *data, unsigned int data_size);

/* Send BPROT_ACK and BPROT_ERROR messages */
int bprot_send_ack(int socket);
int bprot_send_error(int socket);

#endif
