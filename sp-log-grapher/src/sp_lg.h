#ifndef _SP_LG_H_
#define _SP_LG_H_

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

/* Maximum log entry length */
#define LOG_LINE_LEN	256

/* Maximum operation delay (seconds between issue and complete entries */
#define MAX_OP_TIMEOUT	60

/* Event types */
typedef enum __event_type {
	EVT_UNKNOWN,
	EVT_ISSUE,
	EVT_COMPLETE
} event_type_t;

/* Operation types */
typedef enum __op_type {
	OPT_UNKNOWN,
	OPT_READ,
	OPT_WRITE,
	OPT_WRITE_SYNC
} op_type_t;

/* Event entry */
typedef struct __event {
	struct timespec 	ts;
	event_type_t		evt;
	op_type_t		opt;
	uint32_t		offset;
	uint32_t		size;	/* in sectors, multiply by 512 for bytes */
	uint32_t		pid;
	char			comment[256];
} event_t;

/* Time granularity units */
typedef struct __time_unit {
	struct timespec		ts;
	uint32_t		t_utilization;
	uint32_t		t_iops;
	uint32_t		t_bps;
	uint32_t		t_latency;
	uint32_t		t_queue_depth;
	float			a_op_size;
	float			a_latency;
	float			a_queue_depth;
} time_unit_t;

#endif

