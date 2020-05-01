/*
 * mail.h - Mail editing toolset
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

#ifndef MAIL_H
#define MAIL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

struct type_file_info_t{
	char* name;
	char* mime_type;
};

struct email_t{
	char* message;
	/* From the below values you can say pretty much anything about the 
	 * message. The line delimiting body and header is left out. The header
	 * len includes the last \r\n of the header. If header len is zero,
	 * there was no clear distinction between header and body...
	 */
	size_t header_len, body_offset, message_length;

	/* Apparently emails consist of several smaller emails if you send an
	 * attachment alongside them. This also happens when you send an email
	 * as html. Therefore emails are recursive. Ask apple, their mail client
	 * is basically insane by this point...
	 */
	bool is_multipart;

	/* The boundary only needs to be defined if this is a multipart message 
	 */
	size_t boundary_len;
	char* boundary;
	
	/* The content type tells us the MIME type of this message part. May not
	 * be specified, so don't assume it is here! */
	size_t ct_len;
	char* content_type;
	
	/* NOTICE: the boundary, content_type and message fields may be updated
	 * when the message of the root object is modified. See the 
	 * propagate_root_pointer function for further information
	 */

	size_t submes_cnt;
	struct email_t** submes;
	struct email_t* parent;
	bool base64_encoded;
	struct type_file_info_t file_info;
	char* saved_filename;
};



int append_header(struct email_t* mail, const char* key, const char* value);
int append_to_header(struct email_t* mail, const char* pair);
int append_to_body(struct email_t* mail, const char* text);
int append_to_html_body(struct email_t* mail, const char* text);
int remove_mail(struct email_t* mail);

struct email_t* get_root_mail(struct email_t* mail);

void propagate_insert_delete(struct email_t* mail, char* change_p,  
	ssize_t change);
void propagate_root_pointer(struct email_t* mail, char* change_p, char* old_p);

struct type_file_info_t get_mime_file_info(struct email_t* mail);

#define MULTIPART_MIME "multipart/"
#define BASE64_ENC "base64"

#endif /* MAIL_H */
