/*
 * mail.c - Mail editing toolset
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

#include "mail.h"
#include "tools.h"
#include "attach.h"

#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int append_header(struct email_t* mail, const char* key, const char* value){
	
	if(mail == NULL || key == NULL || value == NULL){
		return -1;
	}
	const char* in_between = ": ";
	size_t len = strlen(key) + strlen(value) + strlen(in_between) + 1;
	char* buffer = malloc(len);
	
	if(buffer == NULL){
		return -1;
	}
	
	memset(buffer, 0, len);
	strcat(buffer, key);
	strcat(buffer, in_between);
	strcat(buffer, value);
	if(append_to_header(mail, buffer) < 0){
		free(buffer);
		return -1;
	}
	
	free(buffer);

	return 0;

}


int append_to_header(struct email_t* mail, const char* pair){

	if(pair == NULL || mail == NULL){
		return -1;
	}

	char *buffer = malloc(strlen(pair) + 3);
	if(buffer == NULL){
		return -1;
	}
	memset(buffer, 0, strlen(pair) + 3);
	buffer[0] = '\r';
	buffer[1] = '\n';
	strcat(buffer, pair);
	struct email_t* root = get_root_mail(mail);
	size_t root_offset = mail->header_len + (mail->message - root->message);

	mail->message=insert_string(mail->message, buffer, mail->message_length, 
		mail->header_len);
	
	propagate_insert_delete(root, root->message+root_offset, 
		strlen(buffer));
	
	free(buffer);
	redetect_body_head(mail);

	return 0;

}

struct email_t* get_root_mail(struct email_t* mail){
	
	if(mail == NULL){
		return NULL;
	}
	
	if(mail->parent == NULL){
		return mail;
	}else{
		return get_root_mail(mail->parent);
	}

}

/* This propagates a size change inside the root email down to all submails
 * Call this function with the root, changes will propagate down via recursion!
 * The change pointer should point to the last character IN FRONT of the change.
 * an insert has a positive amount of chane, a delete a negative one.
 */
void propagate_insert_delete(struct email_t* mail, char* change_p,
	ssize_t change){
	
	if(mail == NULL || change_p == NULL || change == 0){
		/* MISSION ABORT!! */
		return;
	}

	if(mail->message >= change_p){
		mail->message += change;
	}

	if(mail->is_multipart){
		for(size_t i = 0; i < mail->submes_cnt; i++){
			propagate_insert_delete(mail->submes[i], change_p, 
				change);
		}
	}
	
	return;
}
