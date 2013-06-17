#include <unistd.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <sp_lg.h>
#include <parser.h>
#include <event_store.h>

/* Generic usage() function */
void usage(char *progname)
{
	printf("\nUsage: \n");
	printf("\t%s -f log_file\n\n", progname);
	
	return;
}

/* Flush time unit */
int cb_tu_flush(time_unit_t tu)
{
	// check tu ...
	printf("TS: %lu, UTIL: %d, IOPS: %d, BPS: %lu, OPSZ: %f, LAT: %f, AQD: %f\n", (uint64_t)tu.ts.tv_sec, tu.t_utilization, tu.t_iops, tu.t_bps, (double)tu.t_bps / tu.t_iops, (double)tu.t_latency / tu.t_iops, (double)tu.t_queue_depth / tu.t_iops);
	
	return 0;
}

/* Update time unit */
int cb_tu_update(time_unit_t *tu)
{
	// check tu ... 
	return 0;
}

/* Main routine */
int main(int argc, char **argv)
{
	FILE 			*f = NULL;
	char 			line[LOG_LINE_LEN];
	char 			*log_file = NULL;
	char 			c = 0;
	
	int			tu_ptr = 0;
	int			tmp_ptr = 0;
	int			offset = 0;
	int			i = 0;
	
	event_t			*ev = NULL, *t = NULL;
	event_match_t		em;
	event_store_t		ev_store;

	time_unit_t		stat_data[MAX_OP_TIMEOUT];
	
	/* Check if enough parameters are present */
	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}

	/* Parse command line parameters */
	while ((c = getopt(argc, argv, "f:")) != -1) {
		switch (c) {
		case 'f':
			log_file = strndup(optarg, 256);
			break;
		default:
			usage(argv[0]);
			free(log_file); log_file = NULL;
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
		free(log_file); log_file = NULL;
		return -1;
	}

	/* Clean / Initialize complex structures */
	memset(line, 0, LOG_LINE_LEN);
	memset(&em, 0, sizeof(struct __event_match));
	memset(&ev_store, 0, sizeof(struct __event_store));
	memset(stat_data, 0, MAX_OP_TIMEOUT * sizeof(struct __time_unit));
	
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
			if (es_insert(&ev_store, ev) != 0) {
				printf("Error inserting event in the temporary store!\n");
				free(ev);
			}
			break;
		case EVT_COMPLETE:
			em.offset = ev->offset;
			em.size = ev->size;
			
			/* Find complementary ISSUE even from the event store */
			if ((t = es_find_remove(&ev_store, es_event_match, &em)) == NULL) {
				free(ev);
				break;
			}

			/* Updating current time unit; no change */
			if (t->ts.tv_sec == stat_data[tu_ptr].ts.tv_sec) {
				
				// printf("C1| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr);
				
				stat_data[tu_ptr].ts.tv_sec = t->ts.tv_sec;
				stat_data[tu_ptr].ts.tv_nsec = t->ts.tv_nsec;
				stat_data[tu_ptr].t_utilization += 1;
				stat_data[tu_ptr].t_iops += 1;
				stat_data[tu_ptr].t_bps += t->size;
				stat_data[tu_ptr].t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
				stat_data[tu_ptr].t_queue_depth = ev_store.count;
			}

			/* Updating time unit "from the past" */
			if (t->ts.tv_sec < stat_data[tu_ptr].ts.tv_sec) {
				
				// printf("C2| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr);

				/* If it's too old ignore it */
				if ((offset = stat_data[tu_ptr].ts.tv_sec - t->ts.tv_sec) >= MAX_OP_TIMEOUT) {
					free(t); free(ev);
					break;
				}
				
				/* Check if array boundary must be traversed */
				if ((tu_ptr - offset) >= 0) {

					printf("C2.1| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d, offset: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr, offset);
				
					tmp_ptr = tu_ptr - offset - 1; 
					
					// stat_data[tmp_ptr].ts.tv_sec = t->ts.tv_sec;
					// stat_data[tmp_ptr].ts.tv_nsec = t->ts.tv_nsec;
					stat_data[tmp_ptr].t_utilization += 1;
					stat_data[tmp_ptr].t_iops += 1;
					stat_data[tmp_ptr].t_bps += t->size;
					stat_data[tmp_ptr].t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
					stat_data[tmp_ptr].t_queue_depth = ev_store.count;	
				} else {

					printf("C2.2| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d, offset: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr, offset);

					tmp_ptr = abs(tu_ptr + offset - MAX_OP_TIMEOUT);
					
					// stat_data[tmp_ptr].ts.tv_sec = t->ts.tv_sec;
					// stat_data[tmp_ptr].ts.tv_nsec = t->ts.tv_nsec;
					stat_data[tmp_ptr].t_utilization += 1;
					stat_data[tmp_ptr].t_iops += 1;
					stat_data[tmp_ptr].t_bps += t->size;
					stat_data[tmp_ptr].t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
					stat_data[tmp_ptr].t_queue_depth = ev_store.count;					
				}
			}

			/* Updating time subsequent time unit */
			if (t->ts.tv_sec > stat_data[tu_ptr].ts.tv_sec) {

				// printf("C3| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr);

				/* Special case: possibly not handled properly!!! If MAX_OP_TIMEOUT seconds pass without activity this creates problems! */
				if ((offset = t->ts.tv_sec - stat_data[tu_ptr].ts.tv_sec) >= MAX_OP_TIMEOUT) {
					free(t); free(ev);
					break;
				}

				if ((tu_ptr + offset) < MAX_OP_TIMEOUT) {
					
					printf("C3.1| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d, offset: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr, offset);

					/* Calculate the offset to move forward */
					tmp_ptr = tu_ptr + offset;

					/* Store the time units we are about to override */
					for (i = tu_ptr + 1; i <= tmp_ptr; i++) {
						printf("I: %d, TS: %d\n", i, stat_data[i].ts.tv_sec);
						// tu_flush(stat_data[i]);
					}
					
					/* Update the stats with data from the latest event */
					// stat_data[tmp_ptr].ts.tv_sec = t->ts.tv_sec;
					// stat_data[tmp_ptr].ts.tv_nsec = t->ts.tv_nsec;
					stat_data[tmp_ptr].t_utilization += 1;
					stat_data[tmp_ptr].t_iops += 1;
					stat_data[tmp_ptr].t_bps += t->size;
					stat_data[tmp_ptr].t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
					stat_data[tmp_ptr].t_queue_depth = ev_store.count;

					/* Update the tu_ptr to the latest time unit */
					tu_ptr = tmp_ptr;
				} else {
				
					printf("C3.2| t.ts: %d, sd[tu].ts: %d, tu_ptr: %d, offset: %d\n", t->ts.tv_sec, stat_data[tu_ptr].ts.tv_sec, tu_ptr, offset);
					
					tmp_ptr = tu_ptr + offset - MAX_OP_TIMEOUT;
					
					for (i = tu_ptr + 1;; i++) {
						if (i >= MAX_OP_TIMEOUT - 1)
							i = 0;
												
						printf("xI: %d, TS: %d\n", i, stat_data[i].ts.tv_sec);
						// tu_flush(stat_data[i]);
						
						if (i == tmp_ptr)
							break;

					}
					
					// stat_data[tmp_ptr].ts.tv_sec = t->ts.tv_sec;
					// stat_data[tmp_ptr].ts.tv_nsec = t->ts.tv_nsec;
					stat_data[tmp_ptr].t_utilization += 1;
					stat_data[tmp_ptr].t_iops += 1;
					stat_data[tmp_ptr].t_bps += t->size;
					stat_data[tmp_ptr].t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
					stat_data[tmp_ptr].t_queue_depth = ev_store.count;										
					
					tu_ptr = tmp_ptr;
				}
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

	/* XXX: Flush the rest time units XXX */
	
	/* Free allocated resources */
	free(log_file); log_file = NULL;
	fclose(f);

	return 0;
}
