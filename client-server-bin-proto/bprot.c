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


	return BPROT_UNKNOWN;
}

/* Get Version from BPROT packet */
int bprot_get_version(char *buf, unsigned int len)
{


	return 0;
}

/* Get Flags from BPROT packet */
int bprot_check_flag(char *buf, unsigned int len, unsigned int flag)
{

	return 0;
}

/* Get Data Size from BPROT packet */
int bprot_get_data_size(char *buf, unsigned int len)
{


	return 0;
}

/* Get Data from BPROT packet */
char * bprot_get_data(char *buf, unsigned int len)
{

	return NULL;
}

/* Send BPROT PING message */
int bprot_send_ping(int socket, int flags, char *data, unsigned int data_size)
{

	return 0;
}

/* Send BPROT PONG message */
int bprot_send_pong(int socket, int flags, char *data, unsigned int data_size)
{

	return 0;
}

/* Send BPROT DATA message */
int bprot_send_data(int socket, int flags, char *data, unsigned int data_size)
{

	return 0;
}

/* Send BPROT ACK message */
int bprot_send_ack(int socket, int flags)
{

	return 0;
}

/* Send BPROT ERROR message */
int bprot_send_error(int socket, int flags)
{


	return 0;
}
