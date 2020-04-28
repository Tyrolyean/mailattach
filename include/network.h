/*
 * network.h - Netowrking interface to and from postfix
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

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <poll.h>

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

void receive_mail(struct mail_recv_t* rec);

int init_net();

void loop_clients();

extern int parentfd;

#endif /* NETWORK_H */
