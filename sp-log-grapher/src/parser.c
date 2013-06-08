#include <sys/types.h>
#include <sys/time.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <sp_lg.h>
#include <parser.h>

/* Parse time stampt from text to struct timespec */
int ev_ts_parser(char *cts, struct timespec *ts)
{
	char **ap, *args[3];

	if (cts == NULL || strlen(cts) == 0 || ts == NULL)
		return -1;
	
	for (ap = args; (*ap = strsep(&cts, ".")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[3])
				break;

	if (strlen(args[0]) == 0 || strlen(args[1]) == 0)
		return -1;

	ts->tv_sec = strtol(args[0], NULL, 10);
	ts->tv_nsec = strtol(args[1], NULL, 10);

	return 0;
}

/* Parse log entry to event_t structure */
event_t * ev_entry_parser(char *line)
{
	char **ap, *args[10];
	event_t *ev = NULL;

	/* Skip comments, empty lines and lines not long enough (magically set to 16 characters) */
	if (line[0] == '\n' || strlen(line) < 16)
		return NULL;
	
	/* Parse the line */
	for (ap = args; (*ap = strsep(&line, " ,\t\n")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &args[10])
				break;

	/* Allocate new event entry */
	if ((ev = malloc(sizeof(struct __event))) == NULL)
		return NULL;

	memset(ev, 0, sizeof(struct __event));

	/* Parse the timestamp */
	if (ev_ts_parser(args[0], &ev->ts) != 0) {
		free(ev);
		return NULL;
	}

	/* Parse the event type */
	switch (args[1][0]) {
	case 'i':
		ev->evt = EVT_ISSUE;
		break;
	case 'c':
		ev->evt = EVT_COMPLETE;
		break;
	default:
		ev->evt = EVT_UNKNOWN;
		break;
	}

	/* Parse operation type */
	if (strncmp(args[2], "ws", strlen("ws")) == 0) {
		ev->opt = OPT_WRITE_SYNC;
		goto opt_exit;
	} else if (strncmp(args[2], "w", strlen("w")) == 0) {
		ev->opt = OPT_WRITE;
		goto opt_exit;
	} else if (strncmp(args[2], "r", strlen("r")) == 0) {
		ev->opt = OPT_READ;
		goto opt_exit;
	} else {
		ev->opt = OPT_UNKNOWN;
	}
opt_exit:

	/* Parse the rest */
	ev->offset = strtol(args[3], NULL, 10);
	ev->size = strtol(args[4], NULL, 10); ev->size *= 512;
	ev->pid = strtol(args[5], NULL, 10);
	memcpy(ev->comment, args[6], LOG_LINE_LEN - 1);

	/* Sanity check */
	/* XXX SUCH ENTRIES EXIST AND ARE CURRENTLY IGNORED XXX
	 * 	5.036441596,c,ws,0,unknown,3,0
	 *	5.036654679,c,ws,0,unknown,3,0
	 */
	if (ev->offset == 0 && ev->size == 0) {
		free(ev);
		return NULL;
	}

	return ev;
}
