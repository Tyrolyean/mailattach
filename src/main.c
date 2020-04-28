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

struct mail_recv_t{
	char* input_buffer;
	size_t in_len;
	bool in_body;
	bool after_body;
	char* body_p;
	size_t body_offs;
	struct pollfd fds[2];
	int n;
	char buf[1000];

};

struct client_info {
	int fd;
};

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
			rec->input_buffer[body_len+1] = 0;
			rec->in_len = body_len;

			char* new_body = attach_files(
				rec->input_buffer+rec->body_offs, 
				body_len);
			
			if(new_body != NULL){
				/* Write the replacement */
				write((rec->fds[1].fd), new_body, 
					strlen(new_body));
				free(new_body);
			}else{
				/* Write the original */
				write((rec->fds[1].fd), 
					rec->input_buffer+rec->body_offs, 
					body_len);
				
			}

			/* Rest of conversation after message */

			write((rec->fds[1].fd), 
				abody, 
				abody_len);

			rec->after_body = true;
			
		}
	}else{
		write((rec->fds[1].fd), rec->buf, rec->n);

	}
	return;
	
}

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

	struct mail_recv_t rec;
	memset(&rec,0,sizeof(rec));
	
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

	printf("Disconnecting clients\n");
	close(cli->fd);
	close(remote_fd);
	free(cli);
	free(rec.input_buffer);
	
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
