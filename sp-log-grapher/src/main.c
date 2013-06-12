#include <unistd.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sp_lg.h>
#include <parser.h>
#include <event_store.h>
#include <stat_data_buf.h>

/* Generic usage() function */
void usage(char *progname)
{
	printf("\nUsage: \n");
	printf("\t%s -f log_file\n\n", progname);
}

/* Main routine */
int main(int argc, char **argv)
{
	FILE 			*f = NULL;
	char 			line[LOG_LINE_LEN];
	char 			*log_file = NULL;
	char 			c = 0;
	
	unsigned int		i = 0;
	
	event_t			*ev = NULL, *t = NULL;
	event_match_t		em;
	event_store_t		ev_store;
	stat_buf_t		sd_store;
	time_unit_t		tu[MAX_OP_TIMEOUT + 1];
	
	/* Check if enough parameters are present */
	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}

	/* Parse command line parameters */
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			log_file = strdup(optarg);
			break;
		default:
			usage(argv[0]);
			return -1;
		}
	}
        argc -= optind;
        argv += optind;

	/* Check if log file is passed as parameter */
	if (log_file == NULL) {
		usage(argv[0]);
		return -1;
	}

	/* Try opening the log file */
	if ((f = fopen(log_file, "r")) == NULL) {
		printf("%s: Unable to open the log_file!\n", argv[0]);
		return -1;
	}

	/* Clean / Initialize complex structures */
	memset(line, 0, LOG_LINE_LEN);
	memset(&em, 0, sizeof(struct __event_match));
	memset(&ev_store, 0, sizeof(struct __event_store));
	memset(&sd_store, 0, sizeof(struct __stat_data_buf));
		
	/* Going through the log file line at a time */
	while (!feof(f)) {
		memset(line, 0, LOG_LINE_LEN);
		
		/* Get event from the log file */
		if (fgets(line, LOG_LINE_LEN, f) == NULL)
			continue;

		/* Parse the event line */
		if ((ev = ev_entry_parser(line)) == NULL)
			continue;

		switch (ev->evt) {
		case EVT_ISSUE:
			// XXX: printf("---- (%d) ISSUE: Inserting event (o: %u, s: %u) ...\n", ev_store.count, ev->offset, ev->size);			
			if (es_insert(&ev_store, ev) != 0) {
				printf("Error inserting event in the temporary store!\n");
				free(ev);
			}
			break;
		case EVT_COMPLETE:
			// XXX: printf("---- (%d) COMPLETE: Processing event (o: %u, s: %u) ...\n", ev_store.count, ev->offset, ev->size);			
			em.offset = ev->offset;
			em.size = ev->size;
			
			/* Find complementary ISSUE even from the event store */
			if ((t = es_find_remove(&ev_store, es_event_match, &em)) == NULL) {
				free(ev);
				break;
			}

			/* Allocate new time unit */
			if ((tu = malloc(sizeof(__time_unit))) == NULL) {
				free(ev);
				break;
			}
			memset(tu, 0, sizeof(__time_unit));
			
			/* Add operation data to the temporary time unit */
			tu->ts.tv_sec = t->ts.tv_sec;
			tu->ts.tv_nsec = t->ts.tv_nsec;
			tu->t_utilization = 1;
			tu->t_iops = 1;
			tu->t_bps = t_size;
			tu->t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
			tu->t_queue_depth = ev_store.count;
			
			/* Update the appropriate time unit stats in the stats buffer */
			if (sb_insert_update(&sb_store, tu) != 0) 
				;;

			
			/* XXX:
			 * iops++
			 * bps += t->size
			 * total_lat += cur_lat
			 * utilization++
			 * 
			 * q_depth = ev_store.count
			 * opsize = bps / iops
			 * lat = total_lat / iops
			op_data[t->ts.tv_sec].t_utilization = 1;
			op_data[t->ts.tv_sec].t_iops++;
			op_data[t->ts.tv_sec].t_bps += t->size; 
			op_data[t->ts.tv_sec].t_latency += (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
			op_data[t->ts.tv_sec].t_queue_depth += ev_store.count;
			 */
			
			/* XXX:
			 * if (list_peak_head().time > MAX_OP_TIMEOUT)
			 * 	list_get_head
			 * 	calc_avgs
			 * 	store
			 * list_add_update(d, t->ts.tv-sec, t->size, (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec)), ev_store.count)
			 */
			
			/* Peak (get the pointer without removing the node) the oldest node */
			if ((tu = sb_peak_head(&sd_store)) == NULL)
				;;
			
			/* If the oldest is older than the specified timeout, store it */
			if (ev->ts.tv_sec - MAX_OP_TIMEOUT > tu->ts.tv_sec) {
				
				
				
				
			}
			
			
			
			
			
			
			
			
			
			
			/* Free both the retrieved entry (ISSUE) and the current one (COMPLETE) */
			free(t); free(ev);
			
			break;
		default:
			/* Fee the event memory, not supported type */
			free(ev);
			
			break;
		}
	}


	// Flush the rest time units 
	
	/* XXX: calculate averages */
	for (i = 0; i < 86400; i++) {
		if (op_data[i].t_utilization == 1) {
			op_data[i].a_op_size = op_data[i].t_bps / op_data[i].t_iops;
			op_data[i].a_latency = op_data[i].t_latency / op_data[i].t_iops;
			op_data[i].a_queue_depth = op_data[i].t_queue_depth / op_data[i].t_iops;
		}
	}
	
	/* Free allocated resources */
	free(log_file); log_file = NULL;

	return 0;
}
