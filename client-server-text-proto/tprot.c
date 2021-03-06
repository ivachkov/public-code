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

/* TPROT protocol definitions */
#include <tprot.h>

/* Get Method from TPROT Packet */
tprot_method_t tprot_get_method(char *buf, unsigned int len) 
{
	char **ap, *args[10], *line, *l;
	char *t = NULL;
	tprot_method_t r;

	if ((line = malloc(1024)) == NULL)
		goto err;
	l = line;
	memset(line, 0, 1024);

	if ((t = strstr(buf, "\r\n")) != NULL)
		memcpy(line, buf, t - buf);
	else
		goto err;

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;

	if (strncasecmp(args[0], "PING", sizeof("PING")) == 0)
		r = TPROT_PING;
	if (strncasecmp(args[0], "PONG", sizeof("PONG")) == 0)
		r = TPROT_PONG;
	if (strncasecmp(args[0], "SET_ARG", sizeof("SET_ARG")) == 0)
		r = TPROT_SET_ARG;
	if (strncasecmp(args[0], "ACK", sizeof("ACK")) == 0)
		r = TPROT_ACK;
	if (strncasecmp(args[0], "ERROR", sizeof("ERROR")) == 0)
		r = TPROT_ERROR;

	free(l);
	return r;
err:
	return TPROT_UNKNOWN;
}

/* Get Resource from TPROT Packet */
char * tprot_get_resource(char *buf, unsigned int len) 
{
	char **ap, *args[10], *line, *l;
	char *t = NULL, *u = NULL;

	if ((line = malloc(1024)) == NULL)
		goto err;
	l = line;
	memset(line, 0, 1024);

	if ((t = strstr(buf, "\r\n")) != NULL)
		memcpy(line, buf, t - buf);
	else
		goto err;

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;

	u = strdup(args[1]);
	free(l);
	return u;
err:
	return NULL;
}

/* Get SeqNo from TPROT Packet */
int tprot_get_seqno(char *buf, unsigned int len) 
{
	char **ap, *args[10], *line, *l;
	char *t = NULL, *s = NULL;
	long int seqno = 0;

	if ((line = malloc(1024)) == NULL)
		goto err;
	l = line;
	memset(line, 0, 1024);

	if ((t = strstr(buf, "SeqNo")) != NULL) {
		if ((s = strstr(t, "\r\n")) != NULL)
			memcpy(line, t, s - t);
	} else
		goto err;

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;

	seqno = strtol(args[1], (char **)NULL, 10);
	free(l);

	if (errno == ERANGE || errno == EINVAL)
		return -1;
	else
		return seqno;
err:
	return -1;
}

/* Get Timestamp from TPROT Packet */
time_t tprot_get_timestamp(char *buf, unsigned int len) 
{
	char **ap, *args[10], *line, *l;
	char *t = NULL, *s = NULL;
	long int ts_data = 0;

	if ((line = malloc(1024)) == NULL)
		goto err;
	l = line;
	memset(line, 0, 1024);

	if ((t = strstr(buf, "Timestamp")) != NULL) {
		if ((s = strstr(t, "\r\n")) != NULL)
			memcpy(line, t, s - t);
	} else
		goto err;

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;

	ts_data = strtol(args[1], (char **)NULL, 10);
	free(l);
	return (time_t)ts_data;
err:
	return (time_t)-1;
}

/* Get Data from TPROT Packet */
char * tprot_get_data(char *buf, unsigned int len) 
{
	char **ap, *args[2], *line, *l;
	char *t = NULL, *s = NULL, *u = NULL;

	if ((line = malloc(1024)) == NULL)
		goto err;
	l = line;
	memset(line, 0, 1024);

	if ((t = strstr(buf, "Data")) != NULL) {
		if ((s = strstr(t, "\r\n")) != NULL)
			memcpy(line, t, s - t);
	} else
		goto err;

	for (ap = args; (*ap = strsep(&line, " \t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[2])
				break;

	u = strdup(args[1]);
	free(l);
	return u;
err:
	return NULL;
}

/* Send TPROT_PING message */
int tprot_send_ping(int socket, char *resource, int seqno, time_t timestamp, char *data)
{
	char buf[MAXLEN];

	memset(buf, 0, MAXLEN);

	snprintf(buf, MAXLEN, "PING %s\r\nSeqNo: %d\r\nTimestamp: %ld\r\nData: %s\r\n", resource, seqno, timestamp, data);

	if (send(socket, buf, MAXLEN, 0) < strlen(buf))
		return -1;	

	return 0;
}

/* Send TPROT_PONG message */
int tprot_send_pong(int socket, char *resource, int seqno, time_t timestamp, char *data)
{
	char buf[MAXLEN];

	memset(buf, 0, MAXLEN);

	snprintf(buf, MAXLEN, "PONG %s\r\nSeqNo: %d\r\nTimestamp: %ld\r\nData: %s\r\n", resource, seqno, timestamp, data);

	if (send(socket, buf, MAXLEN, 0) < strlen(buf))
		return -1;	

	return 0;
}

/* Send TPROT_SET_ARG message */
int tprot_send_set_arg(int socket, char *resource, int seqno, time_t timestamp, char *data)
{
	char buf[MAXLEN];

	memset(buf, 0, MAXLEN);

	snprintf(buf, MAXLEN, "SET_ARG %s\r\nSeqNo: %d\r\nTimestamp: %ld\r\nData: %s\r\n", resource, seqno, timestamp, data);

	if (send(socket, buf, MAXLEN, 0) < strlen(buf))
		return -1;	

	return 0;
}

/* Send TPROT_ACK message */
int tprot_send_ack(int socket, char *resource, int seqno, time_t timestamp)
{
	char buf[MAXLEN];

	memset(buf, 0, MAXLEN);

	snprintf(buf, MAXLEN, "ACK %s\r\nSeqNo: %d\r\nTimestamp: %ld\r\n", resource, seqno, timestamp);

	if (send(socket, buf, MAXLEN, 0) < strlen(buf))
		return -1;	

	return 0;
}

/* Send TPROT_ERROR message */
int tprot_send_error(int socket, char *resource, int seqno, time_t timestamp)
{
	char buf[MAXLEN];

	memset(buf, 0, MAXLEN);

	snprintf(buf, MAXLEN, "ERROR %s\r\nSeqNo: %d\r\nTimestamp: %ld\r\n", resource, seqno, timestamp);

	if (send(socket, buf, MAXLEN, 0) < strlen(buf))
		return -1;	

	return 0;
}

