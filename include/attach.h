/*
 * attach.h - Core mechanix behind attachment replacing
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

#ifndef ATTACH_H
#define ATTACH_H

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

struct email_t mail_from_text(char* message, size_t length);
void redetect_body_head(struct email_t* mail);


char* attach_files(char* message, size_t len);

#endif /* ATTACH_H */
