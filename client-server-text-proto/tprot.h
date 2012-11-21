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

typedef struct __tprot_packet {
	tprot_method_t method;
	char resource[128];
	unsigned int seqno;
	char data[1024];
} tprot_packet_t;

/* Packet parsing primitives */
tprot_method_t tprot_get_method(char *buf, unsigned int len);
unsigned int tprot_get_seqno(char *buf, unsigned int len);
time_t tprot_get_timestamp(char *buf, unsigned int len);
char * tprot_get_data(char *buf, unsigned int len);

int tprot_send_ping(char *resource);
int tprot_send_pong();
int tprot_send_set_arg(char *resource, char *data);
int tprot_send_ack();
int tprot_send_error();

#endif 

