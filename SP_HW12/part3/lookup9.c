/*
 * lookup9 : does no looking up locally, but instead asks
 * a server for the answer. Communication is by Internet UDP Sockets
 * The name of the server is passed as resource. PORT is defined in dict.h
 */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "dict.h"


int lookup(Dictrec * sought, const char * resource) {
	static int sockfd;
	static struct sockaddr_in server;
	struct hostent *host;
	static int first_time = 1;
	ssize_t n;
	
	if (first_time) {  /* Set up server address & create local UDP socket */
		first_time = 0;

		if ((host = gethostbyname(resource)) == NULL)DIE("gethostname");
		/* Set up destination address.
		 * Fill in code. */
		memset(&server, '\0', sizeof(server));
		server.sin_family = AF_INET;
		server.sin_port = htons(PORT);
		memcpy(&server.sin_addr, host->h_addr_list[0], host->h_length);
		/* Allocate a socket.
		 * Fill in code. */
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd == -1)DIE("socket");


	}

	/* Send a datagram & await reply
	 * Fill in code. */
	if (sendto(sockfd, sought, sizeof(Dictrec), 0, (struct sockaddr *)&server, sizeof(server)) < 0)DIE("sendto");
	n = recvfrom(sockfd, sought, sizeof(Dictrec), 0, NULL, NULL);

	if (strcmp(sought->text,"XXXX") != 0) {
		return FOUND;
	}

	return NOTFOUND;
}
