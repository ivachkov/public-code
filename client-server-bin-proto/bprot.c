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

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <errno.h>

/* BPROT protocol definitions */
#include <bprot.h>

/* Get Type from BPROT packet */
bprot_type_t bprot_get_type(char *buf, unsigned int len)
{
	bprot_header_t	*header = (bprot_header_t *)buf;
	
	/* Sanity check */
	if (header->version != BPROT_VERSION)
		return BPROT_UNKNOWN;
	
	if (header->type < BPROT_PING || header->type > BPROT_ERROR)
		return BPROT_UNKNOWN;
	else
		return (bprot_type_t)header->type;
}

/* Get Version from BPROT packet */
int bprot_get_version(char *buf, unsigned int len)
{
	bprot_header_t	*header = (bprot_header_t *)buf;
	
	/* Sanity check */
	if (header->version != BPROT_VERSION)
		return -1;
	else
		return header->version;
}

/* Get Flags from BPROT packet */
int bprot_check_flag(char *buf, unsigned int len, unsigned int flag)
{
	bprot_header_t	*header = (bprot_header_t *)buf;
	
	/* Sanity check */
	if (header->version != BPROT_VERSION)
		return -1;

	/* Check if the specified  flag is turned on */
	if (header->flags & flag)
		return 1;
	
	return 0;
}

/* Get Data Size from BPROT packet */
int bprot_get_data_size(char *buf, unsigned int len)
{
	bprot_header_t	*header = (bprot_header_t *)buf;
	
	/* Sanity check */
	if (header->version != BPROT_VERSION)
		return -1;

	return header->data_size;
}

/* Get Data from BPROT packet */
char * bprot_get_data(char *buf, unsigned int len)
{
	bprot_header_t	*header = (bprot_header_t *)buf;
	char *data = NULL;
	
	/* Sanity check */
	if (header->version != BPROT_VERSION)
		return NULL;

	/* Allocate storage for the packet data */
	if ((data = malloc(header->data_size)) == NULL)
		return NULL;
	
	/* Copy the data starting right after the header */
	memcpy(data, header + sizeof(header), header->data_size);
	
	return data;
}

/* Send BPROT PING message */
int bprot_send_ping(int socket, char *data, unsigned int data_size)
{
	char buf[MAXLEN];
	bprot_header_t	*header = (bprot_header_t *)buf;

	memset(buf, 0, MAXLEN);
	
	header->version = BPROT_VERSION;
	header->flags = BPROT_F1;
	header->type = BPROT_PING;
	
	if (data != NULL && data_size > 0) {
		header->data_size = data_size;
		memcpy(buf + sizeof(bprot_header_t), data, data_size);
	}
	
	if (send(socket, buf, sizeof(bprot_header_t) + header->data_size, 0) < sizeof(bprot_header_t) + header->data_size)
		return -1;

	return 0;
}

/* Send BPROT PONG message */
int bprot_send_pong(int socket, char *data, unsigned int data_size)
{
	char buf[MAXLEN];
	bprot_header_t	*header = (bprot_header_t *)buf;

	memset(buf, 0, MAXLEN);

	header->version = BPROT_VERSION;
	header->flags = BPROT_F2;
	header->type = BPROT_PONG;
	
	if (data != NULL && data_size > 0) {
		header->data_size = data_size;
		memcpy(buf + sizeof(bprot_header_t), data, data_size);
	}
	
	if (send(socket, buf, sizeof(bprot_header_t) + header->data_size, 0) < sizeof(bprot_header_t) + header->data_size)
		return -1;
		
	return 0;
}

/* Send BPROT DATA message */
int bprot_send_data(int socket, char *data, unsigned int data_size)
{
	char buf[MAXLEN];
	bprot_header_t	*header = (bprot_header_t *)buf;

	memset(buf, 0, MAXLEN);

	header->version = BPROT_VERSION;
	header->flags = BPROT_F3;
	header->type = BPROT_PING;
	
	if (data != NULL && data_size > 0) {
		header->data_size = data_size;
		memcpy(buf + sizeof(bprot_header_t), data, data_size);
	} else
		return -1;
	
	if (send(socket, buf, sizeof(bprot_header_t) + header->data_size, 0) < sizeof(bprot_header_t) + header->data_size)
		return -1;
		
	return 0;
}

/* Send BPROT ACK message */
int bprot_send_ack(int socket)
{
	char buf[MAXLEN];
	bprot_header_t	*header = (bprot_header_t *)buf;

	memset(buf, 0, MAXLEN);

	header->version = BPROT_VERSION;
	header->flags = BPROT_F3;
	header->type = BPROT_ACK;
	header->data_size = 0;

	if (send(socket, buf, sizeof(bprot_header_t), 0) < sizeof(bprot_header_t))
		return -1;

	return 0;
}

/* Send BPROT ERROR message */
int bprot_send_error(int socket)
{
	char buf[MAXLEN];
	bprot_header_t	*header = (bprot_header_t *)buf;

	memset(buf, 0, MAXLEN);

	header->version = BPROT_VERSION;
	header->flags = BPROT_F4;
	header->type = BPROT_ERROR;
	header->data_size = 0;
	
	if (send(socket, buf, sizeof(bprot_header_t), 0) < sizeof(bprot_header_t))
		return -1;

	return 0;
}
