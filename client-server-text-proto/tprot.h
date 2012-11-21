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

#ifndef _TPROT_H_
#define _TPROT_H_

#include <time.h>

#define	MAXLEN		1500

/* TPROT Methods */
typedef enum {
	TPROT_UNKNOWN = 0,
	TPROT_PING,
	TPROT_PONG,
	TPROT_SET_ARG,
	TPROT_ACK,
	TPROT_ERROR
} tprot_method_t;

/* Packet parsing primitives */
tprot_method_t tprot_get_method(char *buf, unsigned int len);
char * tprot_get_resource(char *buf, unsigned int len);
int tprot_get_seqno(char *buf, unsigned int len);
time_t tprot_get_timestamp(char *buf, unsigned int len);
char * tprot_get_data(char *buf, unsigned int len);

/* Send TPROT_PING and TPROT_PONG messages */
int tprot_send_ping(int socket, char *resource, int seqno, time_t timestamp, char *data);
int tprot_send_pong(int socket, char *resource, int seqno, time_t timestamp, char *data);

/* Send TPROT_SET_ARG message */
int tprot_send_set_arg(int socket, char *resource, int seqno, time_t timestamp, char *data);

/* Send TPROT_ACK and TPROT_ERROR messages */
int tprot_send_ack(int socket, char *resource, int seqno, time_t timestamp);
int tprot_send_error(int socket, char *resource, int seqno, time_t timestamp);

#endif 

