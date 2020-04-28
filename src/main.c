/*
 * Mailattach - a program to remove attachments and replace them with links.
 * Licensed under the Apache 2.0 License. Parts taken from the sendmail 
 * libmilter sample. License restrictions from their license may apply.
 */


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

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}


struct client_info {
	int fd;
};

void* client_handle_async(void* params){
	
	struct client_info * cli = params;
	struct sockaddr_in serveraddr; /* server's addr */
	int remote_fd;

	remote_fd =  socket(AF_INET, SOCK_STREAM, 0);
	if (remote_fd < 0){
		error("ERROR opening client socket");
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serveraddr.sin_port = htons(forward_port);

	/* connect: create a connection with the server */
	if (connect(remote_fd,(struct sockaddr *) &serveraddr, 
		sizeof(serveraddr)) < 0){
		error("ERROR connecting");
	}

	int n; /* message byte size */
	char buf[1000];
	struct pollfd fds[2];
	memset(fds, 0 , sizeof(fds));
	fds[0].fd = cli->fd;
	fds[0].events = POLLIN;
	fds[1].fd = remote_fd;
	fds[1].events = POLLIN;
	
	int timeout = (10 * 1000); /* 10 seconds */

	char * input_buffer = malloc(1);
	input_buffer[0]=0;
	size_t in_len = 1;
	bool in_body = false, after_body = false;
	char* body_p = NULL;
	size_t body_offs = 0;

	while(poll(fds, 2, timeout) > 0){
		for(size_t i = 0; i < 2; i++){
			
			if(!(fds[i].revents & POLLIN)){
				continue;

			}

			bzero(buf, sizeof(buf));
			n = read(fds[i].fd, buf, sizeof(buf)-1);
			if (n <= 0) {
				goto closeup;
			}

			if(i==0 && !in_body){
				/* As long as we are outside the real mail body 
				 * we can basically passthrough the commands 
				 */
				in_len += n;
				input_buffer = realloc(input_buffer, in_len);
				strncat(input_buffer,buf, n);
				body_p = detect_start_of_body(input_buffer);
				if(body_p != NULL){
					/* We reached the beginning of the body
					 * now! */
					body_offs = body_p - input_buffer;
					in_body = true;
					write((fds[!i].fd), 
						input_buffer+(in_len-n), 
						body_offs-(in_len-n));
					printf("Beginning of message found! "
						"Awaiting end...\n");
					
				}else{
					write((fds[!i].fd), buf, n);

				}
			} else if(i==0 && !after_body){
				/* We keep the body until we have it completely
				 */
				in_len += n;
				input_buffer = realloc(input_buffer, in_len);
				strncat(input_buffer, buf, n);
				body_p = detect_end_of_body(input_buffer+ 
					body_offs);

				if(body_p != NULL){
					
					printf("Data found, interpreting...\n");
					size_t body_len = body_p-
						(input_buffer+body_offs);
					
					char* new_body = attach_files(
						input_buffer+body_offs, 
						body_len);
					
					if(new_body != NULL){
						/* Write the replacement */
						write((fds[!i].fd), new_body, 
							strlen(new_body));
						free(new_body);
					}else{
						/* Write the original */
						write((fds[!i].fd), 
							input_buffer+body_offs, 
							body_len);
						
					}

					/* Rest of conversation after message */

					write((fds[!i].fd), 
						input_buffer+body_offs+body_len, 
						in_len-(body_offs+body_len));

					after_body = true;
					
				}
				
			}else{
				write((fds[!i].fd), buf, n);

			}
			
		}
	}

closeup:

	printf("Disconnecting clients\n");
	close(cli->fd);
	close(remote_fd);
	free(cli);
	free(input_buffer);
	
	return NULL;
}

int main(int argc, char* argv[]){
	
	printf("INIT\n");
	printf("Arguments passed on:\n");
	for(int i = 0; i < argc; i++){
		printf("\t%i:%s\n",i, argv[i]);
	}
	
	int parentfd; /* parent socket */
	int childfd; /* child socket */
	socklen_t clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */

	/* 
	* socket: create the parent socket 
	*/
	parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0){
		error("ERROR opening socket");
	}

	/* setsockopt: Handy debugging trick that lets 
	* us rerun the server immediately after we kill it; 
	* otherwise we have to wait about 20 secs. 
	* Eliminates "ERROR on binding: Address already in use" error. 
	*/
	optval = 1;
	setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

	/*
	* build the server's Internet address
	*/
	bzero((char *) &serveraddr, sizeof(serveraddr));

	/* this is an Internet address */
	serveraddr.sin_family = AF_INET;

	/* let the system figure out our IP address */
	serveraddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	/* this is the port we will listen on */
	serveraddr.sin_port = htons((unsigned short)listen_port);

	/* 
	* bind: associate the parent socket with a port 
	*/
	if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) {
		error("ERROR on binding");
	}

	/* 
	* listen: make this socket ready to accept connection requests 
	*/
	if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
	error("ERROR on listen");

	/* 
	* main loop: wait for a connection request, echo input line, 
	* then close connection.
	*/
	clientlen = sizeof(clientaddr);
	char hostaddr_str[20];
	inet_ntop(AF_INET,(struct sockaddr *) &serveraddr, hostaddr_str, 20);
	printf("Listening for clients on port %u\n",  listen_port);
	while (1) {

		/* 
		* accept: wait for a connection request 
		*/
		childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0) 
			error("ERROR on accept");

		/* 
		* gethostbyaddr: determine who sent the message 
		*/
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
				  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server established connection with %s (%s) fd %i\n", 
		   hostp->h_name, hostaddrp, childfd);
		
		struct client_info* cli = malloc(sizeof(struct client_info));
		memset(cli, 0, sizeof(struct client_info));
		cli->fd = childfd;
		pthread_t executing_thread;
		int status = pthread_create(&executing_thread, NULL,
			client_handle_async, cli);
		if(status < 0){
			error("Failed to create thread");
		}
	}


	return EX_TEMPFAIL;

}
