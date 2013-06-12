#include <unistd.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sp_lg.h>
#include <parser.h>
#include <event_store.h>

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
	
	unsigned int		tu_ptr = 0;
	
	event_t			*ev = NULL, *t = NULL;
	event_match_t		em;
	event_store_t		ev_store;
	time_unit_t		*tu = NULL;

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

			/* Add operation data to the temporary time unit */
			tu->ts.tv_sec = t->ts.tv_sec;
			tu->ts.tv_nsec = t->ts.tv_nsec;
			tu->t_utilization = 1;
			tu->t_iops = 1;
			tu->t_bps = t->size;
			tu->t_latency = (((ev->ts.tv_sec - t->ts.tv_sec) * 1000000000) + (ev->ts.tv_nsec - t->ts.tv_nsec));
			tu->t_queue_depth = ev_store.count;
			
			
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
			
			
			
			
			
			
			
			
			
			
			/* Free both the retrieved entry (ISSUE) and the current one (COMPLETE) */
			free(t); free(ev);
			
			break;
		default:
			/* Fee the event memory, not supported type */
			free(ev);
			
			break;
		}
	}

	/* XXX: Flush the rest time units */
	
	/* Free allocated resources */
	free(log_file); log_file = NULL;
	fclose(f);

	return 0;
}

