#ifndef _PARSER_H_
#define _PARSER_H_

#include <sp_lg.h>

/* Parse log entry to event_t structure */
event_t * ev_entry_parser(char *line);

#endif
