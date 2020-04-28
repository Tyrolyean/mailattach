/*
 * attach.c - Core mechanix behind attachment replacing
 * I know how mechanics is spelt, let me have my fun :D
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

#include "attach.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

struct email_t mail_from_text(char* message, size_t length){
	
	struct email_t mail;
	memset(&mail, 0, sizeof(mail));
	mail.message = message;
	mail.message_length = length;
	
	redetect_body_head(&mail);

	return mail;
}

void redetect_body_head(struct email_t* mail){
	
	char* last_lf = NULL;
	char* body_start = NULL;
	bool found_alnum = false;
	for(size_t i = 0; i < mail->message_length; i++){
		if(mail->message[i] == '\n' && !found_alnum){
			body_start = &(mail->message[i+1]);
			break;
			
		}else if(mail->message[i] == '\n'){
			last_lf = &(mail->message[i]);
			found_alnum = false;
		}else if(isalnum(mail->message[i])){
			found_alnum = true;
		}
	}

	/* In that case we only have a body and no header, or something along
	 * those lines... In any case, if postfix delivers that to us, something
	 * is probably wrong!
	 */

	if(body_start == NULL) {
		fprintf(stderr, "Received message without header!");
		mail->header_len = 0;
		mail->body_offset = 0;
		return;
	}
	
	mail->body_offset = body_start - mail->message;
	mail->header_len = last_lf - mail->message;
	return;
	
	
}

char* attach_files(char* message, size_t len){

	struct email_t email = mail_from_text(message,len);
	
	printf("Received message header: [%.*s]\n", email.header_len, 
		email.message);
	printf("Received message body: [%.*s]\n", 
		email.message_length-email.body_offset,
		email.message + email.body_offset);

	/* Now we have a null terminated body which we can edit! */

	return email.message;
}

