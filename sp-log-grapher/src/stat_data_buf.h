#ifndef _STAT_DATA_BUF_H_
#define _STAT_DATA_BUF_H_

#include <sp_lg.h>

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

/* Statistics nodes */
typedef struct __stat_dat_node {
	time_unit_t		*tu;
	struct __time_unit	*next;
} stat_data_node_t; 

/* Statistics data buffer */
typedef struct __stat_data_buf {
	struct __stat_data_node	*first;
	struct __stat_data_node	*last;
} stat_buf_t;


#endif