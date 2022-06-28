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

#include "ccask_server.h"
#include "util.h"

// TODO: implement configuration of server parameters
#define MAX_CONN 5
#define MAX_CMD_SIZE 1024 // 1 KB limit for now

struct ccask_server {
	unsigned int sd;
	struct sockaddr_un local;
};

ccask_server* ccask_server_init(ccask_server* srv, char* path) {
	if (srv && path) {
		srv->sd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (srv->sd == -1) {
			perror("socket");
			return 0;
		}

		srv->local.sun_family = AF_UNIX;
		strcpy(srv->local.sun_path, path);
		unlink(srv->local.sun_path);
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
		srv->sd = 0;
		free(srv->local.sun_path);
		srv->local = (struct sockaddr_un){ 0 };
	}
}

void ccask_server_delete(ccask_server* srv) {
	ccask_server_destroy(srv);
	free(srv);
}

void ccask_server_print(ccask_server* srv) {
	printf("sd: %d\npath: %s\n", srv->sd, srv->local.sun_path);
}

ccask_server* ccask_server_bind(ccask_server* srv) {
	if (!srv) return 0;
	size_t len = strlen(srv->local.sun_path) + sizeof(srv->local.sun_family);
	if (bind(srv->sd, (struct sockaddr*)&(srv->local), len) == -1) {
		perror("bind");
		return 0;
	}

	return srv;
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

int ccask_server_run(ccask_server* srv) {
	srv = ccask_server_bind(srv);
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
