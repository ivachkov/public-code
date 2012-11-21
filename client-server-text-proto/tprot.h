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

