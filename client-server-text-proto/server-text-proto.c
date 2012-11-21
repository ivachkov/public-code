#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <tprot.h>

/* Test TPROT Server Application */
int main(int argc, char **argv) {
	/* Server and client sockets */
	int server_sock = -1;
	int client_sock = -1;

	/* Port the server listens on */
	unsigned int server_port = 0;

	/* Server and client addresses */
	struct sockaddr_in tprot_server_addr;
	struct sockaddr_in tprot_client_addr;
	unsigned int tprot_client_addr_len = 0;

	/* Read packet buffer */
	char buf[MAXLEN];

	/* select() specific variables */
	fd_set master_fds;
	fd_set read_fds;
	int fd_max = -1;

	/* TPROT specific variables */
	tprot_method_t tprot_method = TPROT_UNKNOWN;
	char *tprot_resource = NULL;
	int tprot_seqno = -1;
	time_t tprot_timestamp = -1;
	char *tprot_data = NULL;

	/* Misc. */
	char prog_name[64];
	unsigned int debug = 0;
	unsigned int daemonize = 0;
	unsigned int yes = 1;
	char ch = '\0';
	int i = -1;

	/* Inialize */
	memset(buf, 0, MAXLEN);
	memset(prog_name, 0, 64);
	strncpy(prog_name, argv[0], 64);

	FD_ZERO(&master_fds);
	FD_ZERO(&read_fds);	


	/* Check for minimum number of parameters 
	 *
	 * Format: 
	 * 	./server-text-proto -p $port [-v]
	 *
	 * Description:
	 * -p $port 	: Listen on port $port
	 * -v 		: Enable verbose output
	 *
	 */
	if (argc < 2) {
		printf("\n\tUsage: %s -p $port [-v] [-d]\n", prog_name);
		printf("\t\t-p $port: Use $port port\n");
		printf("\t\t-v: Output Debug Information\n");
		printf("\t\t-d: Daemonize Process\n");
		printf("\n");
		exit(-1);
        }

	/* Parse command line parameters */
        while ((ch = getopt(argc, argv, "p:vd")) != -1) {
                switch (ch) {
                case 'v':
                        debug = 1;
                        break;
                case 'd':
                        daemonize = 1;
			break;
                case 'p':
                        server_port = atoi(optarg);
                        break;
                default:
			printf("\n\tUsage: %s -p $port [-v] [-d]\n", prog_name);
			printf("\t\t-p $port: Use $port port\n");
			printf("\t\t-v: Output Debug Information\n");
			printf("\t\t-d: Daemonize Process\n");
			printf("\n");
                        exit(-1);
                }
        }
        argc -= optind;
        argv += optind;

	/* Daemon processes can not write debug info on console */
	if (daemonize == 1 && debug == 1) {
		printf("%s: Daemon processes can not write debug information on console!\n", prog_name);
		printf("\tPlease use either -v or -d, but not both!\n");
		exit(-1);
	}

	/* Check if server port is below 1024 and it is possible to open */
	if (server_port <= 1024 && getpid() != 0) {
		printf("%s: Please use port above 1024 or run the application as root!\n", prog_name);
		exit(-1);
	}

	/* Daemonize, if specified */
	if (daemonize != 0)
		if (daemon(0, 0) != 0) {
			perror("daemon");
			exit(-1);
		}
	
	/* Initialise TPROT Server Socket */
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	/* Set protocol family, address to bind to and port to use */
	tprot_server_addr.sin_family = AF_INET;
	tprot_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	tprot_server_addr.sin_port = htons(server_port);

	/* Quickly reuse address and port */
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("setsockopt");
		exit(-1);
	}

	/* Bind application to specified address and port */
	if (bind(server_sock, (struct sockaddr *)&tprot_server_addr, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(-1);
	}

	/* Set maximum size of listen queue */
	listen(server_sock, 1024);

	/* Add server socket to the select() fd set */
	FD_SET(server_sock, &master_fds);
	fd_max = server_sock;

	if (debug != 0)
		printf("TPROT Server: Server boots up ...\n");

	
	/* Main server routine */
	for (;;) {
		read_fds = master_fds;
		if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(-1);
		}

		for (i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == server_sock) {
					/* Accept client connection */
					tprot_client_addr_len = sizeof(struct sockaddr);
					if ((client_sock = accept(server_sock, (struct sockaddr *)&tprot_client_addr, &tprot_client_addr_len)) < 0) {
						perror("accept");
						exit(-1);
					}

					/* Insert the new socket in the select() pool */
					FD_SET(client_sock, &master_fds);
					if (client_sock > fd_max)
						fd_max = client_sock;

					if (debug != 0)
						printf("TPROT Server: Incoming connection from: %s\n", inet_ntoa(tprot_client_addr.sin_addr));
				} else {
					/* Read data from client */
					memset(buf, 0, MAXLEN);
					if (recv(i, buf, MAXLEN, 0) <= 0) {
						/* Closed socket or error on read - close anyway */
						close(i);
						/* Remove from select() pool */
						FD_CLR(i, &master_fds);
					} else {
						/* Data is available for processing */
						if (debug != 0)
							printf("TPROT Server: Packet received:\n--------\n%s\n--------\n", buf);

						/* Get TPROT method (mandatory) */
						if ((tprot_method = tprot_get_method(buf, MAXLEN)) == TPROT_UNKNOWN) {
							if (debug != 0)
								printf("TPROT Server: Unknown TPROT method!\n");
							continue;
						}

						/* Get TPROT resource (mandatory) */
						if ((tprot_resource = tprot_get_resource(buf, MAXLEN)) == NULL) {
							if (debug != 0)
								printf("TPROT Server: Missing resource ID!\n");
							continue;
						}

						/* Get TPROT sequence number (mandatory) */
						if ((tprot_seqno = tprot_get_seqno(buf, MAXLEN)) == -1) {
							if (debug != 0)
								printf("TPROT Server: Missing or bad sequence number!\n");

							if (tprot_resource != NULL)
								free(tprot_resource);

							continue;
						}

						/* Get TPROT timestamp (mandatory) */
						if ((tprot_timestamp = tprot_get_timestamp(buf, MAXLEN)) == -1) {
							if (debug != 0)
								printf("TPROT Server: Missing or bad timestamp!\n");

							if (tprot_resource != NULL)
								free(tprot_resource);

							continue;
						}

						/* Get TPROT data (optional) */
						if ((tprot_data = tprot_get_data(buf, MAXLEN)) != NULL)
							if (debug != 0)
								printf("TPROT Server: Packet data available:\n--------\n%s\n--------\n", buf);


						/* TPROT state machine */
						switch (tprot_method) {
						case TPROT_PING: /* TPROT_PING received, return TPROT_PONG */
							if (tprot_send_pong(i, tprot_resource, ++tprot_seqno, time(NULL), tprot_data) < 0)
								if (debug != 0)
									printf("TPROT Server: Problem returning TPROT_PONG!\n");

							break;
						case TPROT_SET_ARG: /* TPROT_SET_ARG received, return TPROT_ACK or TPROT_ERROR */
							if (strncmp(tprot_resource, "/allowed", strlen("/allowed")) == 0) {
								/* Correct resource name */
								if (tprot_send_ack(i, tprot_resource, ++tprot_seqno, time(NULL)) < 0)
									if (debug != 0)
										printf("TPROT Server: Problem returning TPROT_ACK!\n");
							} else {
								/* Incorrect resource name */
								if (tprot_send_error(i, tprot_resource, ++tprot_seqno, time(NULL)) < 0)
									if (debug != 0)
										printf("TPROT Server: Problem returning TPROT_ERROR!\n");
							}

							break;
						case TPROT_PONG: /* For all other cases, return TPROT_ERROR */
						case TPROT_ACK:
						case TPROT_ERROR:
						case TPROT_UNKNOWN:
						default:
							if (tprot_send_error(i, tprot_resource, ++tprot_seqno, time(NULL)) < 0)
								if (debug != 0)
									printf("TPROT Server: Problem returning TPROT_ERROR!\n");

							break;
						}

						/* Free allocated resources */
						if (tprot_resource != NULL)
							free(tprot_resource);
						if (tprot_data != NULL)
							free(tprot_data);
					}
				}
			}
		}
	}

	close(server_sock);

	if (debug != 0)
		printf("TPROT Server: Server shuts down ...\n");

	return 0;
}

