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
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXLEN 1500

int main(int argc, char **argv)
{
	/* Client socket */
	int client_sock = -1;

	/* Port the server listens on */
	char *server_port = NULL;
	char *server_addr = NULL;

	/* Server address information */
	struct addrinfo hints;
	struct addrinfo *addr_info = NULL;

	/* Read packet buffer */
	char buf[MAXLEN];

	/* User inputs */
	char line[MAXLEN];

	/* Misc. */
	char prog_name[64];
	char ch = '\0';

	/* Inialize */
	memset(buf, 0, MAXLEN);
	memset(line, 0, MAXLEN);
	memset(&hints, 0, sizeof(struct addrinfo));
	memset(prog_name, 0, 64);
	strncpy(prog_name, argv[0], 64);

	/* Check for minimum number of parameters 
	 *
	 * Format: 
	 * 	./client-text-proto -s $address -p $port
	 *
	 * Description:
	 * -s $address	: Server address to connect to
	 * -p $port 	: Server port to connect to
	 *
	 */
	if (argc < 2) {
		printf("\n\tUsage: %s -s $address -p $port\n", prog_name);
		printf("\t\t-s $address: Server address\n");
		printf("\t\t-p $port: Server port\n");
		printf("\n");
		exit(-1);
        }

	/* Parse command line parameters */
        while ((ch = getopt(argc, argv, "s:p:")) != -1) {
                switch (ch) {
                case 's':
                        server_addr = strndup(optarg, 128);
			break;
                case 'p':
                        server_port = strndup(optarg, 8);
                        break;
                default:
			printf("\n\tUsage: %s -s $address -p $port\n", prog_name);
			printf("\t\t-s $address: Server address\n");
			printf("\t\t-p $port: Server port\n");
			printf("\n");
                        exit(-1);
                }
        }
        argc -= optind;
        argv += optind;

	/* Get server details */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	if (getaddrinfo(server_addr, server_port, &hints, &addr_info) < 0) {
		perror("getaddrinfo");
		exit(-1);
	}

	/* Initialise Client Socket */
	if ((client_sock = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)) < 0) {
		perror("socket");
		exit(-1);
	}

	/* Connect to POSIX Thread Server */
	if (connect(client_sock, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
		perror("connect");
		exit(-1);
	}

	/* ... */

	return 0;
}
