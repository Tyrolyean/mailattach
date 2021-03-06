/*
 * detect.c - Detect content
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

#include <stddef.h>
#define _GNU_SOURCE
#include <string.h>

#include "detect.h"
#include "tools.h"

char* pgp_signatures[] = 
{
	"application/pgp-encrypted",
	"application/pgp-signature",
	"-----BEGIN PGP SIGNATURE-----",
	"-----BEGIN PGP MESSAGE-----"
};

char* dkim_signatures[] = 
{
	"DKIM-Signature:"
};

bool detect_pgp(struct email_t* mail){
		
	size_t points = 0;
	
	for(size_t i = 0; i < (sizeof(pgp_signatures)/sizeof(char*));i++){
		if(strcasestr(mail->message,
			pgp_signatures[i]) != NULL){
			points++;
		}
		
	}
	

	return points >= 2;
}

bool detect_dkim(struct email_t* mail){
		
	size_t points = 0;
	
	for(size_t i = 0; i < (sizeof(dkim_signatures)/sizeof(char*));i++){
		if(strcasestr(mail->message, dkim_signatures[i]) != NULL
				&& strcasestr(mail->message, dkim_signatures[i])
				<= (mail->message+mail->header_len)){
			/* DKIM only happens inside the header, not the body! */
			points++;
		}
		
	}
	

	return points >= 1;
}

/* If body hasn't started yet, it returns NULL, if it has started, it returns
 * the pointer to the beginning of the newline.
 */
char* detect_start_of_body(char* message){
	
	const char* data = "DATA";

	char* data_cmd = strcasestr(message, data);
	for(;data_cmd != NULL; data_cmd = strcasestr(data_cmd+1, data)){
		
		if(data_cmd == NULL){
			return NULL;
		}

		if(data_cmd == message){
			/* You don't start an smtp conversation with data eh? */
			return NULL;
		}

		char before = data_cmd[-1];
		char after = data_cmd[strlen(data)+1];
		if(before == '\n' || after == '\r'){
			/* Find the end of that line, otherwise fail... */
			for(size_t i = 0; data_cmd[i] != '\0'; i++){
				if(data_cmd[i] == '\n'){
					return data_cmd+i+1;
				}
			}
		}	
		

	}
	return NULL;
	
}

char* detect_end_of_body(char* message){
	
	const char* data = "\r\n.\r\n";

	return strstr(message, data);
	
}

bool detect_base64(const struct email_t* mail){
	
	char* encoding = search_header_key(mail, "Content-Transfer-Encoding");
	
	if(encoding == NULL){
		return false;
	}else{
		
		size_t value_length = 0;
		char * type = get_value_from_key(&value_length, 
			encoding - mail->message, mail);

		if(type != NULL){
			
			if(strncasecmp(type, BASE64_ENC, strlen(BASE64_ENC)) 
				== 0){

				return true;

			}else{
				return false;
			}

		}else{
			return false;
		}
	}


}

