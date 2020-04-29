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

/* Searches the given header for the key provided and, if found, returns the
 * pointer to that key NOT the value 
 */
char* search_header_key(struct email_t* mail, const char* key){
	
	if(mail == NULL || key == NULL){
		return NULL;
	}
	size_t keylen = strlen(key);

	for(size_t i = 0; i < (mail->header_len - keylen); i++){
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
	struct email_t* mail){
	
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
		if(isalnum(mail->message[i])){
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
