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

struct email_t{
	char* message;
	/* From the below values you can say pretty much anything about the 
	 * message. The line delimiting body and header is left out. The header
	 * len includes the last \r\n of the header. If header len is zero,
	 * there was no clear distinction between header and body...
	 */
	size_t header_len, body_offset, message_length;
};

int append_header(struct email_t* mail, const char* key, const char* value);
int append_to_header(struct email_t* mail, const char* pair);

#endif /* MAIL_H */