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
#include "detect.h"
#include "config.h"
#include "mail.h"
#include "tools.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>

struct email_t* mail_from_text(char* message, size_t length){
	
	struct email_t* mail = malloc(sizeof(struct email_t));;
	memset(mail, 0, sizeof(struct email_t));
	mail->message = message;
	mail->message_length = length;
	mail->submes_cnt = 0;
	mail->submes = NULL;
	mail->is_multipart = false;
	mail->boundary = NULL;
	mail->boundary_len = 0;
	
	redetect_body_head(mail);
	char* cont_type = search_header_key(mail, "Content-Type");
	if(cont_type == NULL){
		/* Halleluja, I've got nothing to do! WOO */
		mail->is_multipart = false;
		return mail;
	}else{
		size_t value_length = 0;
		char * mime_type = get_value_from_key(&value_length, 
			cont_type - mail->message, mail);
		if(mime_type != NULL){
			if(strncasecmp(mime_type, MULTIPART_MIME, 
				strlen(MULTIPART_MIME)) == 0){
				/* We have multiple messages cramped inside this
				 * one. Let's unravel em!
				 */
				mail->is_multipart = true;
				 size_t bd_len = 0;
				char* bd = get_multipart_boundary(
					mime_type, value_length, &bd_len);
				
				if(bd != NULL){
					mail->boundary = bd;
					mail->boundary_len = bd_len;
					
					if(verbose){
						printf("Received boundary: "
							"[%.*s]\n",
							(int)bd_len, bd);
					}
				}
				unravel_multipart_mail(mail);

			}
		}
	}

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
		fprintf(stderr, "Received message without header!\n");
		mail->header_len = 0;
		mail->body_offset = 0;
		return;
	}
	
	mail->body_offset = body_start - mail->message;
	mail->header_len = last_lf - mail->message - 1 /* for the CR */;
	return;
	
	
}

/* This function creates the submail-messages from the boundary defined for
 * multipart messages. This function doesN'T perform header checks for that
 * boundary!
 */
void unravel_multipart_mail(struct email_t* mail){
	
	if(mail == NULL || !mail->is_multipart || mail->boundary == NULL){
		return;
	}
	

	char ** mail_boundarys = NULL;
	size_t mb_cnt = 0;
	char* boundary = malloc(mail->boundary_len+1);
	memset(boundary, 0, mail->boundary_len+1);
	memcpy(boundary,mail->boundary, mail->boundary_len);
	
	char* newloc = mail->message + mail->body_offset;

	for(;newloc < mail->message + mail->message_length;){
		newloc = strstr(newloc, 
			boundary);
		if(newloc == NULL || newloc > 
			(mail->message + mail->message_length)){
			
			break;

		}else{
			mail_boundarys = realloc(mail_boundarys, 
				++mb_cnt*(sizeof(char*)));
			mail_boundarys[mb_cnt-1] = newloc;
		}
		newloc++;
	}
	
	if(verbose){
		printf("Found %lu boundarys inside mail, which means %li "
			"submessages\n", mb_cnt, mb_cnt-1);
	}
	
	free(mail_boundarys);
	free(boundary);
	return;

}

void free_submails(struct email_t* mail){
	
	if(!mail->is_multipart){
		return;
	}
	for(size_t i = 0; i < mail->submes_cnt; i++){
		free_submails(mail->submes[i]);
		free(mail->submes[i]);
	}
	free(mail->submes);
	return;

}

/* Message is required to be a null terminated string, length is the mail body.
 * One may leave something behind the body. len is without the '\0'
 * Attempts to replace files inside the email with links to it on a webserver
 */
char* attach_files(char* message, size_t len){
	
	char* mess;
	struct email_t *email = mail_from_text(message,len);
	
	/* Check if mails are signed/encrypted, and abort if nescessary */
	if(abort_on_pgp && detect_pgp(email)){
		printf("PGP detected, aborting...");
		goto finish;
	}
	/* Check if mails are signed/encrypted, and abort if nescessary */
	if(abort_on_dkim && detect_dkim(email)){
		printf("DKIM signature detected, aborting...");
		goto finish;
	}

	/* Now we can start the real work! */
	
	/* Announce our presence via header */
	if(append_header(email,"X-Mailattached", instance_id) < 0){
		fprintf(stderr, "Failed to attach header!\n");
		goto finish;
	}
finish:
	mess = email->message;
	free_submails(email);
	free(email);
	return mess;
}

