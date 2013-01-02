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
#include <pthread.h>

#include <server-posix-threads.h>

int main(int argc, char **argv)
{
	/* Server and client sockets */
	int server_sock = -1;
	int client_sock = -1;

	/* Port the server listens on */
	unsigned int server_port = 0;

	/* Server and client addresses */
	struct sockaddr_in pth_server_addr;
	struct sockaddr_in pth_client_addr;
	unsigned int pth_client_addr_len = 0;

	/* POSIX Threads */
	pthread_t thread;
	thread_data_t *thread_data = NULL;
	
	/* Misc. */
	char prog_name[64];
	unsigned int debug = 0;
	unsigned int daemonize = 0;
	unsigned int yes = 1;
	char ch = '\0';
	int data = 0;

	/* Inialize */
	memset(prog_name, 0, 64);
	strncpy(prog_name, argv[0], 64);

	/* Check for minimum number of parameters 
	 *
	 * Format: 
	 * 	./server-text-proto -p $port [-v]
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
	
	/* Initialise TPROT Server Socket */
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(-1);
	}

	/* Set protocol family, address to bind to and port to use */
	pth_server_addr.sin_family = AF_INET;
	pth_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	pth_server_addr.sin_port = htons(server_port);

	/* Quickly reuse address and port */
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("setsockopt");
		exit(-1);
	}

	/* Bind application to specified address and port */
	if (bind(server_sock, (struct sockaddr *)&pth_server_addr, sizeof(struct sockaddr)) < 0) {
		perror("bind");
		exit(-1);
	}

	/* Set maximum size of listen queue */
	listen(server_sock, 1024);

	if (debug != 0)
		printf("POSIX Thread Server: Server boots up ...\n");

	/* Main Service Loop */
	for(;;data++) {
		/* Accept client connection */
		pth_client_addr_len = sizeof(struct sockaddr);

		if ((client_sock = accept(server_sock, (struct sockaddr *)&pth_client_addr, &pth_client_addr_len)) == -1)
			continue;

		if (debug != 0)
			printf("POSIX Thread Server: Incoming connection from: %s\n", inet_ntoa(pth_client_addr.sin_addr));

                /* Prepare thread parameters here */
		if ((thread_data = malloc(sizeof(thread_data_t))) == NULL) {
			if (debug != 0)
				printf("POSIX Thread Server: Unable to allocate memory for thread parameters!\n");
	
			close(client_sock);
			continue;
		}
		
		/* Set thread parameters */
		thread_data->client_sock = client_sock;
		snprintf(thread_data->data, 16, "%015d", data);

		/* Create new thread for client */
		if (pthread_create(&thread, NULL, worker_thread, (void *)thread_data) != 0) {
			if (debug != 0)
				printf("POSIX Thread Server: Unable to create new thread!\n");

			close(client_sock);
			free(thread_data); 
			thread_data = NULL;
			continue;
		}
        }

        /* Close network socket */
	close(server_sock);

	if (debug != 0)
		printf("POSIX Thread Server: Server shuts down ...\n");

	return 0;
}
