/*
 * network.c - Netowrking interface to and from postfix
 * The author licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <pthread.h>

#include "version.h"
#include "config.h"
#include "attach.h"
#include "detect.h"
#include "network.h"

int parentfd = 0;

/* Used to ingest data from a client */
void receive_mail(struct mail_recv_t* rec){


	if(!rec->in_body){
		/* As long as we are outside the real mail body
		 * we can basically passthrough the commands
		 */
		rec->in_len += rec->n;
		rec->input_buffer = realloc(rec->input_buffer, rec->in_len);
		strncat(rec->input_buffer,rec->buf, rec->n);
		rec->body_p = detect_start_of_body(rec->input_buffer);
		if(rec->body_p != NULL){
			/* We reached the beginning of the body
			 * now! */
			rec->body_offs = rec->body_p - rec->input_buffer;
			rec->in_body = true;
			write((rec->fds[1].fd),
				rec->input_buffer+(rec->in_len-rec->n),
				rec->body_offs-(rec->in_len-rec->n));
			printf("Beginning of message found! Awaiting end...\n");

		}else{
			write((rec->fds[1].fd), rec->buf, rec->n);

		}
	} else if(!rec->after_body){
		/* We keep the body until we have it completely
		 */
		rec->in_len += rec->n;
		rec->input_buffer = realloc(rec->input_buffer, rec->in_len);
		strncat(rec->input_buffer, rec->buf, rec->n);
		rec->body_p = detect_end_of_body(rec->input_buffer+
			rec->body_offs);

		if(rec->body_p != NULL){

			printf("Data found, interpreting...\n");
			size_t body_len = rec->body_p-
				(rec->input_buffer+rec->body_offs);

			/* Copy the after body message and reduce original
			 * pointer to mail size to allow reallocs */
			 size_t abody_len = rec->in_len-
				(rec->body_offs+body_len);

			void * abody = malloc(abody_len);
			memcpy(abody,
				rec->input_buffer+rec->body_offs+body_len,
				abody_len);

			/* reduce the input buffer to the body */
			memmove(rec->input_buffer,
				rec->input_buffer+rec->body_offs, body_len);
			rec->input_buffer = realloc(rec->input_buffer,
				body_len + 1);
			rec->input_buffer[body_len] = 0;
			rec->in_len = body_len;

			char* new_body = attach_files(
				rec->input_buffer,
				rec->in_len);

			if(new_body != NULL){
				/* Write the replacement */
				write((rec->fds[1].fd), new_body,
					strlen(new_body));
				free(new_body);
				rec->input_buffer = NULL;
				rec->in_len = 0;
			}else{
				/* Write the original */
				write((rec->fds[1].fd),
					rec->input_buffer+rec->body_offs,
					body_len);

			}

			/* Rest of conversation after mail itself. Most likely
			 * just a CRLF.CRLF abd QUIT */

			write((rec->fds[1].fd),
				abody,
				abody_len);
			free(abody);

			rec->after_body = true;

		}
	}else{
		write((rec->fds[1].fd), rec->buf, rec->n);

	}
	return;

}

/* Handle an incoming client connection. Passes every command through to the
 * server on the other end, apart from the E-Mail message itself, identified by
 * the DATA command. If the data command fails, then we just error out by
 * disconnecting both sides of this tunnel.
 */
void* client_handle_async(void* params){

	struct client_info * cli = params;
	struct sockaddr_in serveraddr; /* server's addr */
	int remote_fd;

	remote_fd =  socket(AF_INET, SOCK_STREAM, 0);
	if (remote_fd < 0){
		perror("error opening client socket");
		close(cli->fd);
		free(cli);
		return NULL;
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serveraddr.sin_port = htons(forward_port);

	struct mail_recv_t rec;
	memset(&rec,0,sizeof(rec));
	
	/* connect: create a connection with the server */
	if (connect(remote_fd,(struct sockaddr *) &serveraddr,
		sizeof(serveraddr)) < 0){

		perror("ERROR connecting client socket");
		goto closeup;
	}


	rec.fds[0].fd = cli->fd;
	rec.fds[0].events = POLLIN;
	rec.fds[1].fd = remote_fd;
	rec.fds[1].events = POLLIN;

	int timeout = (10 * 1000); /* 10 seconds */

	rec.input_buffer = malloc(1);
	rec.input_buffer[0]=0;
	rec.in_len = 1;
	rec.in_body = false;
	rec.after_body = false;
	rec.body_p = NULL;
	rec.body_offs = 0;

	while(poll(rec.fds, 2, timeout) > 0){
		for(size_t i = 0; i < 2; i++){

			if(!(rec.fds[i].revents & POLLIN)){
				continue;

			}

			bzero(rec.buf, sizeof(rec.buf));
			rec.n = read(rec.fds[i].fd, rec.buf, sizeof(rec.buf)-1);
			if (rec.n <= 0) {
				goto closeup;
			}

			if(i == 0){
				receive_mail(&rec);

			}else{
				write((rec.fds[!i].fd), rec.buf, rec.n);

			}

		}
	}

closeup:

	printf("Disconnecting...\n");
	close(cli->fd);
	close(remote_fd);
	free(cli);
	free(rec.input_buffer);

	return NULL;
}

int init_net(){

	struct sockaddr_in serveraddr;
	int optval;

	parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0){
		perror("ERROR opening socket");
		return -1;
	}

	optval = 1;
	setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serveraddr.sin_port = htons((unsigned short)listen_port);

	if (bind(parentfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0) {
		perror("ERROR on binding");
		return -1;
	}

	return 0;
}

void loop_clients(){

	/*
	* listen: make this socket ready to accept connection requests
	*/
	if (listen(parentfd, 5) < 0){
		perror("Failed to listen on socket...");
	}

	int childfd;
	struct sockaddr_in clientaddr;
	
	socklen_t clientlen = sizeof(clientaddr);
	printf("Listening for clients on port %u\n",  listen_port);
	
	while (1) {

		childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0){
			perror("accept failed");
		}


		struct client_info* cli = malloc(sizeof(struct client_info));
		memset(cli, 0, sizeof(struct client_info));
		cli->fd = childfd;
		pthread_t executing_thread;
		
		int status = pthread_create(&executing_thread, NULL,
			client_handle_async, cli);
		
		if(status < 0){
			fprintf(stderr, "Failed to create thread: %s\n", 
				strerror(status));
		}
	}


	return;

}

