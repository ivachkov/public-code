#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include <sp_lg.h>
#include <event_store.h>

/* 
 * The "event store" is effectively a linear structure that behaves like a
 * stack when a new element is added and like a linked list when an element
 * is being removed. Due to the need of the current process, only two 
 * operations are implemented:
 * - es_insert(): adds new element on the top;
 * - es_find_remove(): traverses the event store until if finds the required 
 * element, removes it and returns the event data;
 */

/* Event comparison routine */
int es_event_match(event_t *ev, event_match_t *data)
{	
	if (ev == NULL || data == NULL)
		return -1;

	// printf(".... Node offset: %u, Node size: %u, Match offset: %u, Match size: %u\n", ev->offset, ev->size, data->offset, data->size);
	
	if ((ev->offset == data->offset) && (ev->size == data->size))
		return 0;
	
	return 1;
}

/* Insert element in the event store */
int es_insert(event_store_t *store, event_t *ev)
{
	event_node_t *node = NULL;

	/* Allocate memory for new node */
	if ((node = malloc(sizeof(struct __event_node))) == NULL)
		return -1;

	memset(node, 0, sizeof(struct __event_node));

	/* Insert the node in the event store */
	node->event = ev;

	if (store->head == NULL) {
		store->head = node;
		store->head->next = NULL;
	} else {
		node->next = store->head;
		store->head = node;
	}
	
	store->count++;

	return 0;
}

/* Find and remove element from event store */
event_t * es_find_remove(event_store_t *store, int (*match_f)(event_t *, event_match_t *), event_match_t *data)
{
	event_node_t 	*c = NULL, *p = NULL;
	event_t		*e = NULL;
	
	if (store->head == NULL)
		return NULL;
	
	p = c = store->head;

	while (c != NULL) {		
		if (match_f(c->event, data) == 0) {
			if (c == store->head)
				store->head = c->next;
			else
				p->next = c->next;	

			e = c->event;
			free(c); 
			store->count--;
			return e;
		} else {
			p = c;
			c = c->next;
		}
	}

	return NULL;
}
