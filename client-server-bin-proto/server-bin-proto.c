/*
 * Copyright (c) 2012, Ivo Vachkov
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

/* BPROT protocol definitions */
#include <bprot.h>

/* Test BPROT Server Application */
int main(int argc, char **argv) {
	/* Server and client sockets */
	int server_sock = -1;
	int client_sock = -1;

	/* Port the server listens on */
	unsigned int server_port = 0;

	/* Server and client addresses */
	struct sockaddr_in bprot_server_addr;
	struct sockaddr_in bprot_client_addr;
	unsigned int bprot_client_addr_len = 0;

	/* Read packet buffer */
	char buf[MAXLEN];

	/* select() specific variables */
	fd_set master_fds;
	fd_set read_fds;
	int fd_max = -1;

	/* TPROT specific variables */
	bprot_type_t bprot_type = BPROT_UNKNOWN;
	unsigned int bprot_version = 0;
	unsigned int bprot_flags = 0;
	unsigned int bprot_data_size = 0;
	char *bprot_data = NULL;

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
	 * 	./server-bin-proto -p $port [-v]
	 *
	 * Description:
	 * -p $port 	: Listen on port $port
	 * -v 		: Enable verbose output
	 * -d		: Enable background mode
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
	
	/* Initialise BPROT Server Socket */
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	/* Set protocol family, address to bind to and port to use */
	bprot_server_addr.sin_family = AF_INET;
	bprot_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bprot_server_addr.sin_port = htons(server_port);

	/* Quickly reuse address and port */
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("setsockopt");
		exit(-1);
	}

	/* Bind application to specified address and port */
	if (bind(server_sock, (struct sockaddr *)&bprot_server_addr, sizeof(struct sockaddr)) < 0) {
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
		/* Wait until connecion is created */
		read_fds = master_fds;
		if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) < 0) {
			perror("select");
			exit(-1);
		}

		/* Go through all open descriptors to process the requests */
		for (i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == server_sock) {
					/* Accept client connection */
					bprot_client_addr_len = sizeof(struct sockaddr);
					if ((client_sock = accept(server_sock, (struct sockaddr *)&bprot_client_addr, &bprot_client_addr_len)) < 0) {
						perror("accept");
						exit(-1);
					}

					/* Insert the new socket in the select() pool */
					FD_SET(client_sock, &master_fds);
					if (client_sock > fd_max)
						fd_max = client_sock;

					if (debug != 0)
						printf("TPROT Server: Incoming connection from: %s\n", inet_ntoa(bprot_client_addr.sin_addr));
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
							printf("BPROT Server: Packet received:\n--------\n%s\n--------\n", buf);

						/* Get BPROT method (mandatory) */
						if ((bprot_type = bprot_get_type(buf, MAXLEN)) == BPROT_UNKNOWN) {
							if (debug != 0)
								printf("BPROT Server: Unknown BPROT method!\n");
							continue;
						}

						/* Get BPROT */

						/* Get BPROT  */

						/* Get BPROT  */

						/* BPROT state machine */
						switch (bprot_type) {
						case BPROT_PING: /* BPROT_PING received, return BPROT_PONG */
							// if (bprot_send_pong(i, data) < 0)
								if (debug != 0)
									printf("BPROT Server: Problem returning BPROT_PONG!\n");

							break;
						case BPROT_DATA:
						case BPROT_PONG:
						case BPROT_ACK:
						case BPROT_ERROR:
						default:
							if (bprot_send_error(i, 42) < 0)
								if (debug != 0)
									printf("BPROT Server: Problem returning BPROT_ERROR!\n");

							break;
						}

						/* Free allocated resources */
						if (bprot_data != NULL)
							free(bprot_data);
					}
				}
			}
		}
	}

	/* Close network socket */
	close(server_sock);

	if (debug != 0)
		printf("TPROT Server: Server shuts down ...\n");

	return 0;
}
