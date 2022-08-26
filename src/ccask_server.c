#define _POSIX_C_SOURCE 200112L

#include <asm-generic/errno.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ccask_server.h"
#include "util.h"

// TODO: implement configuration of server parameters
#define MAX_CONN 5
#define MAX_CMD_SIZE 1024 // 1 KB limit for now

// TODO: finish work on my scratch file netw_sockets (~/scratch/netw_sockets)
// and then move that implementation here

// helpers

/*@brief get_in_addr ripped directly from beej's guide :+1:*/
void *get_in_addr(struct sockaddr* sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	} 

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

unsigned int get_listener_socket(char* port) {
	if (!port) return -1;
	unsigned int listener;
	int yes = 1;
	int rv;

	struct addrinfo hints, *ai, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
		fprintf(stderr, "server: %s\n", gai_strerror(rv));
		return -1;
	}

	for (p = ai; p != 0; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			// error on socket()
			perror("server: socket");
			continue;
		}

		// drop "address already in use" message
		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			// error on setsockopt()
			perror("setsockopt");
			continue;
		}

		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
			// error on bind()
			close(listener);
			perror("server: bind");
			continue;
		}
		
		// if we made it here, we have successfully bound our listener socket
		break;
	}

	// done with the addrinfo
	freeaddrinfo(ai);

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return -1;
	}

	if (listen(listener, MAX_CONN) == -1) {
		fprintf(stderr, "server: listen failed\n");
		return -1;
	}

	return listener;
}

struct ccask_server {
	int sd;	
	unsigned int fd_count;
	unsigned int fd_size;
	struct pollfd *pfds;
};

ccask_server* ccask_server_init(ccask_server* srv, char* port) {
	// 1) get our addr info using the passed port
	// 2) bind our listening socket based on that
	if (!port) port = "54569"; // a random default
	if (srv && port) {
		if((srv->sd = get_listener_socket(port)) == -1) {
			*srv = (ccask_server){ 0 };
			srv->sd = -1;
			return srv;
		};
		srv->fd_count = 0;
		srv->fd_size = 5;
		srv->pfds = malloc(sizeof(*srv->pfds) * srv->fd_size);
	} else {
		*srv = (ccask_server){ 0 };
		srv->sd = -1;
	}
	return srv;
}

ccask_server* ccask_server_new(char* path) {
	ccask_server* srv = malloc(sizeof(ccask_server));
	srv = ccask_server_init(srv, path);
	return srv;
}

void ccask_server_destroy(ccask_server* srv) {
	if (srv) {
		free(srv->pfds);
		*srv = (ccask_server){ 0 };
	}
}

void ccask_server_delete(ccask_server* srv) {
	ccask_server_destroy(srv);
	free(srv);
}

void ccask_server_print(ccask_server* srv) {
	printf("sd: %d\nfd_count: %d\nfd_size: %d\n", srv->sd, srv->fd_count, srv->fd_size);
}

/**@brief recv_until reads len bytes into buf or returns -1 or -2 to indicate an error*/
int recv_until(int sockfd, uint8_t* buf, size_t len, int flags) {
	if (!buf) return -1;

	size_t total_read = 0;
	size_t read_size = 0;


	while(total_read < len && (read_size = recv(sockfd, buf, len-total_read, 0)) > 0) {
		buf += read_size;
		total_read += read_size;
	}

	if (read_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
		perror("recv");
		return -1;
	}

	if (total_read < len) {
		return -2;
	}

	return total_read;
}

/**@brief recv_cmd receives a command of maximum size MAX_CMD_SIZE from the non-blocking socket sockfd. returns -1 on error or full command not received*/
int recv_cmd(int sockfd, uint8_t* buf, size_t bufsz, int flags) {
	if (!buf) return -1;
	size_t len = 0;
	size_t len_size = 4; // bytes the msg length header

	int recvd = recv(sockfd, buf, len_size, 0);

	if (recvd < len_size) {
		return -1;
	}

	len = NWK_BYTE_ARR_U32(buf);
	buf += recvd;

	if (len == 0 || len > MAX_CMD_SIZE || len > bufsz-recvd) return -1;

	recvd = recv_until(sockfd, buf, len-recvd, 0);

	if (recvd < 0 || recvd < len-len_size) return -1;
	
	return len;
}


// TODO: re-write ccask_server_run to use in-struct
// 		 pfds. Then wire up to ccask query interface from ccask_db
int ccask_server_run(ccask_server* srv) {
	if( !srv ) return 1;

	struct sockaddr_un remote;
	socklen_t t;
	int sd2, fd_count;

	if (listen(srv->sd, MAX_CONN) == -1) {
		perror("listen");
		return 1;
	}

	struct pollfd pfds[MAX_CONN+1];

	pfds[0].fd = srv->sd; // first polled socket is the listen socket
	pfds[0].events = POLLIN;

	fd_count = 1;

	for (;;) {
		int poll_count = poll(pfds, fd_count, -1);

		if (poll_count == -1) {
			perror("poll");
			return 0;
		}

		for (int i = 0; i < fd_count; i++) {
			if (pfds[i].revents & POLLIN) {
				if (pfds[i].fd == srv->sd) {
					sd2 = accept(srv->sd, (struct sockaddr *)&remote, &t);

					if (sd2 == -1) {
						perror("accept");
					} else {
						// handle the new connection
						// add it to our polling list
					}
				} else {
					// handle reading from a client socket
					// or send a message to a client socket
				}
			}
		}
	}
	return 0;
}
