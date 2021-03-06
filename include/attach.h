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

#include "mail.h"

struct email_t* mail_from_text(char* message, size_t length, 
	struct email_t* parent_mail);
void redetect_body_head(struct email_t* mail);

void unravel_multipart_mail(struct email_t* mail);
void free_submails(struct email_t* mail);

char* attach_files(char* message, size_t len);

int replace_files(struct email_t* mail, const char* dirname, bool* created);

#endif /* ATTACH_H */
