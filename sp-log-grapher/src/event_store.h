#ifndef _EVENT_STORE_H_
#define _EVENT_STORE_H_

#include <stdint.h>

#include <sp_lg.h>

/* Event matching criteria */
typedef struct __event_match {
	uint32_t		offset;
	uint32_t		size;
} event_match_t;

/* Event nodes */
typedef struct __event_node {
	event_t			*event;
	struct __event_node	*next;
} event_node_t; 

/* Event Store descriptor */
typedef struct __event_store {
	unsigned int 		count;
	struct __event_node	*head;
} event_store_t;

/* Event comparison routine */
int es_event_match(event_t *ev, event_match_t *data);
/* Insert element in the event store */
int es_insert(event_store_t *store, event_t *ev);
/* Find and remove element from event store */
event_t * es_find_remove(event_store_t *store, int (*match_f)(event_t *, event_match_t *), event_match_t *data);

#endif
