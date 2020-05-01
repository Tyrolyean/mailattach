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
#include "file.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>

/* Generates an email struct from the given eml text. If the content is
 * multipart, it will be slit up into several email_ts. This expansion is done
 * recursively. For the root message set parent_mail to NULL
 */
struct email_t* mail_from_text(char* message, size_t length, 
	struct email_t* parent_mail){
	
	struct email_t* mail = malloc(sizeof(struct email_t));;
	memset(mail, 0, sizeof(struct email_t));
	mail->message = message;
	mail->message_length = length;
	mail->submes_cnt = 0;
	mail->submes = NULL;
	mail->is_multipart = false;
	mail->boundary = NULL;
	mail->boundary_len = 0;
	mail->ct_len = 0;
	mail->content_type = 0;
	mail->parent = parent_mail;
	mail->saved_filename = NULL;
	
	redetect_body_head(mail);
	char* cont_type = search_header_key(mail, "Content-Type");
	if(cont_type == NULL){
		/* Halleluja, I've got nothing to do! WOO */
		if(verbose){
			printf("Mail found without content type!\n");
		}
		mail->is_multipart = false;
		return mail;
	}else{
		size_t value_length = 0;
		char * mime_type = get_value_from_key(&value_length, 
			cont_type - mail->message, mail);
		if(mime_type != NULL){
			mail->ct_len = value_length;
			mail->content_type = mime_type;

			if(verbose){
				printf("Found content type: %.*s\n",
					(int)value_length, mime_type);
			}
			
			if(strncasecmp(mime_type, MULTIPART_MIME, 
				strlen(MULTIPART_MIME)) == 0){
				/* We have multiple messages cramped inside this
				 * one. Let's unravel em!
				 */
				mail->is_multipart = true;
				 size_t bd_len = 0;
				char* bd = get_value_equals(
					mime_type, value_length, &bd_len, 
					"boundary");
				
				if(bd != NULL){
					mail->boundary = bd;
					mail->boundary_len = bd_len;
					
					if(verbose){
						printf("Received boundary: "
							"[%.*s]\n",
							(int)bd_len, bd);
					}
				}
				/* Expands the mail message to multiple ones */
				unravel_multipart_mail(mail);

			}
		}
	}
	mail->base64_encoded = detect_base64(mail);
	
	mail->file_info = get_mime_file_info(mail);

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

	/* Really this is something that SHOULD NOT HAPPEN during transport, but
	 * enigmail produces a weirdly formatted signing here which has ONLY a
	 * header and no body. So we need that for multipart messages.
	 */

	if(body_start == NULL) {
		mail->header_len = mail->message_length;
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
	
	for(ssize_t i = 0; i < ((ssize_t)mb_cnt)-1; i++){
		const char * begin_pointer = get_next_line(mail_boundarys[i], 
			mail->message_length - 
			(mail->message - mail_boundarys[i]));

		if(begin_pointer == NULL){
			continue;
		}
		
		const char* end_pointer = get_prev_line(mail_boundarys[i+1],
			(mail_boundarys[i+1] - begin_pointer));

		if(end_pointer == NULL){
			continue;
		}
		struct email_t *submail = mail_from_text((char*)begin_pointer, 
			end_pointer - begin_pointer, mail);
		mail->submes = realloc(mail->submes, ++mail->submes_cnt * 
			(sizeof(struct email_t)));
		mail->submes[mail->submes_cnt - 1] = submail;
	}
	
	free(mail_boundarys);
	free(boundary);
	return;

}

void free_submails(struct email_t* mail){
	
	free(mail->file_info.name);
	free(mail->file_info.mime_type);

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

void print_mail_structure(struct email_t *email, unsigned int level){
	
	for(unsigned int i = 0; i < level; i++){
		printf("  ");
	}
	
	if(email->base64_encoded){
		printf("Base64 encoded ");
	}
	
	if(email->is_multipart){
		printf("multipart Message with %lu submessages:\n", 
			email->submes_cnt);
		for(size_t i = 0; i < email->submes_cnt; i++){
			print_mail_structure(email->submes[i], level+1);
		}
	}else{
		if( email->file_info.mime_type == NULL){
	
			printf("final message with length %lu and type "
				"[%.*s] \n", 
				email->message_length, (int)email->ct_len, 
				email->content_type);
		}else{
			printf("final message with length %lu and type [%s] \n", 
				email->message_length, 
				email->file_info.mime_type);
			
		}
	}

	return;

}

/* Message is required to be a null terminated string, length is the mail body.
 * One may leave something behind the body. len is without the '\0'
 * Attempts to replace files inside the email with links to it on a webserver
 */
char* attach_files(char* message, size_t len){
	
	char* mess;
	struct email_t *email = mail_from_text(message,len, NULL);
	if(email == NULL){
		return NULL;
	}
	if(verbose){
		print_mail_structure(email, 0);
	}
	/* Check if mails are signed/encrypted, and abort if nescessary */
	if(abort_on_pgp && detect_pgp(email)){
		printf("PGP detected, aborting...\n");
		goto finish;
	}
	/* Check if mails are signed/encrypted, and abort if nescessary */
	if(abort_on_dkim && detect_dkim(email)){
		printf("DKIM signature detected, aborting...\n");
		goto finish;
	}

	/* Now we can start the real work! */
	char* storage_dir = generate_safe_dirname();
	if(storage_dir == NULL){
		fprintf(stderr,"Failed to get mail storage directory!\n");
		goto finish;
	}
	if(verbose){
		printf("Storing mail messages into directory [%s]\n",
			storage_dir);
	}
	bool dir_cre = false;
	if(replace_files(email, storage_dir, &dir_cre) < 0){
		fprintf(stderr, "Failed to store base64 messages!\n");
		goto finish;
	}
	
	free(storage_dir);

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

/* This function returns the first message where it makes sense to append the
 * attachment message to. To differentiate between text and html the mime type
 * is sent along
 */
struct email_t* get_appendable_mail(struct email_t* mail, 
	const char* mime_type){
	
	if(mail->is_multipart){
		struct email_t* ret = NULL;
		for(size_t i = 0; i < mail->submes_cnt; i++){
			struct email_t* ret2 = 
				get_appendable_mail(mail->submes[i], mime_type);
				if(ret2 != NULL){
					ret = ret2;
				}
		}
		return ret;

	}

	if(mail->base64_encoded || mail->file_info.mime_type == NULL || 
		mail->file_info.name != NULL){
		return NULL;
	}

	if(strcasecmp(mail->file_info.mime_type, mime_type) == 0){
		return mail;
	}else{
		return NULL;
	}
	

}

/* This function RECURSIVELY replaces ALL base 64 encoded messages above the
 * threshold set in the config file with links to that file in the HTML format.
 * Call this function with the mail ROOT object if you want to replace all
 * base64 files, or only one when you want to replace only one. Created is a
 * boolean wether the directory to store files has already been created.
 */
int replace_files(struct email_t* mail, const char* dirname, bool* created){

	if(mail->is_multipart){
	
		for(size_t i = 0; i < mail->submes_cnt; i++){
			int n = replace_files(mail->submes[i], dirname, 
				created) ;
			if(n < 0){

				return -1;
			} else if(n == 1){
				i--;
			}
		}
		return 0;

	}
	if(mail->file_info.name == NULL){
		return 0;
	}
	if((mail->message_length - mail->body_offset) < (unsigned long)
		min_filesize ){
		return 0;
	}
	/* Create the directory for the file */
	if(!(*created)){
		*created = true;
		if(mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) 
			< 0){

			perror("Failed to create storage directory!");
			return -1;

		}

	}
	struct email_t* root = get_root_mail(mail);
	if(root == NULL){
		return -1;
	}
	struct email_t* txt_app = get_appendable_mail(root, "text/plain");
	struct email_t* html_app = get_appendable_mail(root, "text/html");
	if(txt_app == NULL && html_app == NULL){
		return -1;
	}
	char* chosen_filename = NULL;
	if(mail->base64_encoded){
		if(base64_decode_file(dirname, mail, &chosen_filename) < 0){
			fprintf(stderr, "Failed to decode base64 file!\n");
			return -1;
		}
	}else{
		if(decode_file(dirname, (mail->message+mail->body_offset),
			 (mail->message_length - mail->body_offset), 
			 mail->file_info.name, &chosen_filename) < 0){
			
			fprintf(stderr, "Failed to decode regular file!\n");
			return -1;
		}

	}
	/* Insert a message telling you what has happened to the attachment */

	/* Delete old attachment */
	if(mail->parent != NULL){
		if(remove_mail(mail) < 0){
			fprintf(stderr, "Failed to remove old attachment!\n");
			free(chosen_filename);
			return -1;
		}
	}
	static const char* html_filler_pref = 
	"<br><h1>MAIL ATTACHED</h1> The following attachment of this mail has "
	"been remotely stored: <br>\r\n"
	"<p> File %s of Type %s as <a href=\"%s%s\">%s%s</a></p>\r\n";
	
	static const char* text_filler_pref = 
	" --- MAIL ATTACHED ---\r\n The following attachment of this mail has "
	"been remotely stored:\r\n"
	"File %s of Type %s as %s%s\r\n";
	
	size_t directory_len = strlen(directory);
	size_t url_len = strlen(url_base);
	size_t mime_len = 0;
	if(mail->file_info.mime_type != NULL){
		mime_len = strlen(mail->file_info.mime_type);
	}

	size_t html_buffer_len = strlen(html_filler_pref) + 2*url_len + 100 +
		mime_len + 2*strlen(mail->file_info.name);
	
	size_t text_buffer_len = strlen(html_filler_pref) + url_len + 50 +
		mime_len + strlen(mail->file_info.name);

	char* html_buffer = malloc(html_buffer_len);
	memset(html_buffer, 0, html_buffer_len);
	
	char* text_buffer = malloc(text_buffer_len);
	memset(text_buffer, 0, text_buffer_len);
	
	snprintf(html_buffer, html_buffer_len - 1, html_filler_pref,  
		mail->file_info.name,
		mail->file_info.mime_type, 
		url_base, chosen_filename+directory_len,
		url_base, chosen_filename+directory_len);
	
	snprintf(text_buffer, text_buffer_len - 1, text_filler_pref,  
		mail->file_info.name,
		mail->file_info.mime_type, 
		url_base, chosen_filename+directory_len);
	
	
	if(txt_app != NULL){
		 append_to_body(txt_app, text_buffer);
	}
	if(html_app != NULL){
		 append_to_body(html_app, html_buffer);
		
	}
	
	free(html_buffer);
	free(text_buffer);
	free(chosen_filename);
	free_submails(mail);
	free(mail);
	return 1;
}

