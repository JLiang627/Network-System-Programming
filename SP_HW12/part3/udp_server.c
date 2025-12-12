/*
 * udp_server : listen on a UDP socket ;reply immediately
 * argv[1] is the name of the local datafile
 * PORT is defined in dict.h
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#include "dict.h"

int main(int argc, char **argv) {
	static struct sockaddr_in server,client;
	int sockfd;
	Dictrec dr, *tryit = &dr;

	if (argc != 2) {
		fprintf(stderr,"Usage : %s <datafile>\n",argv[0]);
		exit(errno);
	}

	/* Initialize address.
	 * Fill in code. */
	memset(&server, '\0', sizeof(server));
	
	/* Create a UDP socket.
	 * Fill in code. */
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);

	/* Name and activate the socket.
	 * Fill in code. */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)DIE("socket");

	if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) == -1)DIE("bind");

	for (;;) { /* await client packet; respond immediately */
		socklen_t client_len = sizeof(client);

		ssize_t n = recvfrom(sockfd, tryit, sizeof(Dictrec), 0, (struct sockaddr *)&client, &client_len);

		/* Wait for a request.
		 * Fill in code. */
		if (n < 0){
			if (errno == EINTR) continue;
			DIE("recvfrom");
		}

			/* Lookup request and respond to user. */
		switch(lookup(tryit,argv[1]) ) {
			case FOUND:
				if (sendto(sockfd, tryit, sizeof(Dictrec), 0, (struct sockaddr *)&client, client_len) < 0)
				DIE("sendto"); 
				break;
			case NOTFOUND : 
				if (sendto(sockfd, tryit, sizeof(Dictrec), 0, (struct sockaddr *)&client, client_len) < 0)
				DIE("sendto");
				break;
			case UNAVAIL:
				DIE(argv[1]);
		} /* end lookup switch */
	} /* end forever loop */
} /* end main */
