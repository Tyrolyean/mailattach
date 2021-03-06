/*
 * tools.c - Utility functions
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

#include "tools.h"
#include "mail.h"

#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>

/* Takes a string destination and inserts at the given point offset the string
 * in source. Really more like a stringcat with shifting the end. 
 * dest_orig_len is without '\0'*/
char* insert_string(char * destination, const char* source, 
	size_t dest_orig_len, size_t offset){

	if(source == NULL|| dest_orig_len < offset){
		return destination;
	}
	
	size_t src_len = strlen(source);
	size_t new_len = dest_orig_len + src_len + 1;

	char* result = realloc(destination, new_len);
	if(result == NULL){
		/* Out of memory... Failed me.. */
		return NULL;
	}
	memmove(result+offset+src_len, result+offset, 
		(dest_orig_len - offset+1));
	memcpy(result+offset, source, src_len);
	return result;
	
	
}

/* Takes a string string and removes from offset INCLUDING the character there
 * the following remove bytes. Len is without the NULL-termination
 */
bool remove_string(char * root, size_t len, size_t offset, size_t remove){

	if(remove+offset > len || root == NULL){
		return false;
	}
	
	memmove(root+offset, root+offset+remove, len - (offset+remove));
	root[len-remove] = 0;

	return true;	
	
}

/* Searches the given header for the key provided and, if found, returns the
 * pointer to that key NOT the value 
 */
char* search_header_key(const struct email_t* mail, const char* key){
	
	if(mail == NULL || key == NULL){
		return NULL;
	}
	size_t keylen = strlen(key);

	/* Also try that at the beginning of the message! */
	if(strncasecmp(mail->message, key, keylen) == 0){
		return mail->message;
	}

	for(size_t i = 0; (i+keylen) < mail->header_len; i++){
		if(mail->message[i] == '\n'){
			if(strncasecmp(&mail->message[i+1], key, keylen) == 0){
				return &mail->message[i+1];
			}
		}
	}

	return NULL;
}

/* If the key doesn't have a value, or nothing can be found, NULL is returned,
 * otherwise a pointer to the first character of the value is returned.
 */
char* get_value_from_key(size_t* val_len, size_t key_offset, 
	const struct email_t* mail){
	
	if(val_len == NULL || mail == NULL){

		return NULL;
	}

	size_t colon = 0;
	for(size_t i = key_offset; i < mail->header_len; i++){
		if(mail->message[i] == ':'){
			colon = i;
			break;
		}
	}

	colon++;
	char* val = NULL;
	for(size_t i = colon; i < (mail->header_len-1); i++){
		if(isalnum(mail->message[i]) && val == NULL){
			val = &mail->message[i];
		}

		if(mail->message[i] == '\n' && !isblank(mail->message[i+1])){
			if(val != NULL){
				*val_len = &mail->message[i] - 1 - val;
			}else{
				*val_len = 0;
			}
			return val;
		}
	}
	if(val != NULL){
		*val_len = &mail->message[mail->header_len] - val;
	}else{
		*val_len = 0;
	}

	return val;
}

char* get_value_equals(char* content_type, size_t content_len, 
	size_t* boundary_len, char* key){

	if(content_type == NULL || boundary_len == NULL){
		return NULL;
	}
	
	char* bd = key;
	char* boundary_begin = NULL;
	size_t bd_len = strlen(bd);
	long boundary_offset = -1;

	for(size_t i = 0; i < (content_len - bd_len);i++){
		if(strncasecmp(&content_type[i], bd, bd_len) == 0){
			boundary_offset = i+bd_len;
			break;
		}
	}
	
	if(boundary_offset < 0){
		*boundary_len = 0;
		return NULL;
	}
#define STATE_INIT   0
#define STATE_EQUALS 1
#define STATE_ALNUM  2
#define STATE_QUOTE  3

	size_t state = STATE_INIT;
	
	for(size_t i = boundary_offset; i < content_len;i++){
		if(state == STATE_INIT && content_type[i] == '='){
			state = STATE_EQUALS;
		}else if(state == STATE_EQUALS && !isspace(content_type[i])){
			if(content_type[i] == '"' && boundary_begin == NULL){
				boundary_begin = &content_type[++i];
				state = STATE_QUOTE;
			}else {
				boundary_begin = &content_type[i];
				state = STATE_ALNUM;
			}
		}else if(state == STATE_ALNUM){
			if(isspace(content_type[i])){
				*boundary_len = &content_type[i] - 
					boundary_begin ;
				return boundary_begin;
			}
		}else if(state == STATE_QUOTE && content_type[i] == '"'){
				*boundary_len = &content_type[i] - 
					boundary_begin ;
				return boundary_begin;

		}
	}
	if(state == STATE_ALNUM){
		*boundary_len = content_type+content_len - boundary_begin;
		return boundary_begin;

	}
	*boundary_len = 0;
	return NULL;
}

/* Returns a character pointer to the first character of the next line. */
const char* get_next_line(const char* message, size_t len){
	for(size_t i = 0; i < len-1; i++){
		if(message[i] == '\n'){
			return &message[i+1];
		}
	}

	return NULL;
}

/* Returns a character pointer to the last character of the last line which was 
 * NOT part of a cr lf. The len_neg attribute specifies how far we can go to
 * the left.
 */
const char* get_prev_line(const char* message, size_t len_neg){
	for(size_t i = 0; i < (len_neg-2); i++){
		if(message[-i] == '\n'){
			if(message[-(i+1)] == '\r'){
				return message-(i+1);

			}else{
				return message-(i);

			}
		}
	}

	return NULL;
}

/* Propagates a size change inside the body UP. This only  alters the complete
 * BODY length, the header len remains untouched. A positive change tells that
 * something was added, a negative one that something was removed. Call this
 * from the object you modified.
 */
void propagate_size_change(struct email_t *mail, ssize_t change){
	
	if(mail == NULL){
		return;
	}
	mail->message_length += change;
	if(mail->parent != NULL){
		propagate_size_change(mail->parent, change);
	}

	return;
}
