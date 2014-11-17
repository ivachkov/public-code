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

/* BPROT protocol definitions */
#include <bprot.h>

/* Test BPROT Client Application */
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
	char *data = NULL;

	/* User inputs */
	char line[MAXLEN];
	char choice = '\0';
	
	/* Test data */
	char test_data[] = { 'A', 'B', 'C', 'D', '\0' };
	char invalid_data[] = { 'I', 'N', 'V', 'A', 'L', 'I', 'D', '\0' };

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
	 * 	./client-bin-proto -s $address -p $port
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

	/* Initialise BPROT Client Socket */
	if ((client_sock = socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)) < 0) {
		perror("socket");
		exit(-1);
	}

	/* Connect to BPROT Server */
	if (connect(client_sock, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
		perror("connect");
		exit(-1);
	}

	for (;;) {
		memset(buf, 0, MAXLEN);

		printf("Please choose:\n");
		printf("\t1. Send PING to server\n");
		printf("\t2. Send valid DATA to server\n");
		printf("\t3. Send invalid DATA to server\n");
		printf("\t4. Send erroneous message (BPROT_ERROR) to server\n");
		printf("\t9. Exit\n");

		/* Securely read from input stream */
		if (fgets(line, MAXLEN, stdin) != NULL) {
			/* Replace \n with \0 */
			line[strlen(line) - 1] = '\0';
			/* Get the first letter */
			choice = line[0];
			
			switch (choice) {
			case '1':
				/* Sending request to server */
				printf("Sending BPROT_PING to server ...\n");
				if (bprot_send_ping(client_sock, test_data, sizeof(test_data)) < 0) {
					printf("Sending failed!\n");
					goto termination;
				}

				/* Getting response from server */
				printf("Reading response from server (expecting PONG) ...\n");
				if (recv(client_sock, buf, MAXLEN, 0) <= 0) {
					printf("Reading failed!\n");
					goto termination;
				}

				/* Checking versions */
				if (bprot_get_version(buf, MAXLEN) != BPROT_VERSION) {
					printf("Received wrong version!\n");
					goto termination;
				}
				
				/* Checking type */
				if (bprot_get_type(buf, MAXLEN) != BPROT_PONG) {
					printf("Received wrong type!\n");
					goto termination;
				}
				
				/* Checking flags */
				if (bprot_check_flag(buf, MAXLEN, BPROT_F2) != 1) {
					printf("Received wrong flags!\n");
					goto termination;
				}
				
				/* Checking data */
				if ((data = bprot_get_data(buf, MAXLEN)) != NULL) {
					printf("Received data: %s\n", data);
					free(data); data = NULL;
				}

				printf("Successfull processing of BPROT_PING message!\n");
				
				break;
			case '2':
				/* Sending request to server */
				printf("Sending BPROT_DATA to server with valid data ...\n");
				if (bprot_send_data(client_sock, test_data, sizeof(test_data)) < 0) {
					printf("Sending failed!\n");
					goto termination;
				}

				/* Getting response from server */
				printf("Reading response from server (expecting ACK) ...\n");
				if (recv(client_sock, buf, MAXLEN, 0) <= 0) {
					printf("Reading failed!\n");
					goto termination;
				}
				
				/* Checking versions */
				if (bprot_get_version(buf, MAXLEN) != BPROT_VERSION) {
					printf("Received wrong version!\n");
					goto termination;
				}
				
				/* Checking type */
				if (bprot_get_type(buf, MAXLEN) != BPROT_ACK) {
					printf("Received wrong type!\n");
					goto termination;
				}
				
				/* Checking flag */
				if (bprot_check_flag(buf, MAXLEN, BPROT_F3) != 1) {
					printf("Received wrong flags!\n");
					goto termination;
				}

				printf("Successfull processing of BPROT_DATA message!\n");

				break;
			case '3':
				/* Sending request to server */
				printf("Sending BPROT_DATA to server with invalid data ...\n");
				if (bprot_send_data(client_sock, invalid_data, sizeof(invalid_data)) < 0) {
					printf("Sending failed!\n");
					goto termination;
				}

				/* Getting response from server */
				printf("Reading response from server (expecting ERROR) ...\n");
				if (recv(client_sock, buf, MAXLEN, 0) <= 0) {
					printf("Reading failed!\n");
					goto termination;
				}
				
				/* Checking versions */
				if (bprot_get_version(buf, MAXLEN) != BPROT_VERSION) {
					printf("Received wrong version!\n");
					goto termination;
				}
				
				/* Checking type */
				if (bprot_get_type(buf, MAXLEN) != BPROT_ERROR) {
					printf("Received wrong type!\n");
					goto termination;
				}
				
				/* Checking flag */
				if (bprot_check_flag(buf, MAXLEN, BPROT_F4) != 1) {
					printf("Received wrong flags!\n");
					goto termination;
				}

				printf("Successfull processing of BPROT_DATA message!\n");

				break;
			case '4':
				/* Sending request to server */
				printf("Sending invalid request (BPROT_ERROR) to server ...\n");
				if (bprot_send_error(client_sock) < 0) {
					printf("Sending failed!\n");
					goto termination;
				}

				/* Getting response from server */
				printf("Reading response from server (expecting ERROR) ...\n");
				if (recv(client_sock, buf, MAXLEN, 0) <= 0) {
					printf("Reading failed!\n");
					goto termination;
				}

				/* Checking versions */
				if (bprot_get_version(buf, MAXLEN) != BPROT_VERSION) {
					printf("Received wrong version!\n");
					goto termination;
				}
				
				/* Checking type */
				if (bprot_get_type(buf, MAXLEN) != BPROT_ERROR) {
					printf("Received wrong type!\n");
					goto termination;
				}
				
				/* Checking flag */
				if (bprot_check_flag(buf, MAXLEN, BPROT_F4) != 1) {
					printf("Received wrong flags!\n");
					goto termination;
				}
				
				printf("Successfull processing of BPROT_ERROR message!\n");

				break;
			case '9':
				goto termination;
			default:
				break;
			}

			/* Cleaning-up resources */
			if (data != NULL) {
				free(data);
				data = NULL;
			}
			
			continue;
		} else {
			break;
		}
	}

/* Exit point */
termination:

	/* Close network socket */
	close(client_sock);
	
	/* Free allocated resources */
	if (server_addr != NULL)	
		free(server_addr);
	if (server_port != NULL)
		free(server_port);
	if (addr_info != NULL)
		freeaddrinfo(addr_info);	

	return 0;
}
