/*
 * tools.h - Utility functions
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

#ifndef TOOLS_H
#define TOOLS_H

#include "mail.h"
#include <stddef.h>
#include <stdbool.h>

char* insert_string(char * destination, const char* source, 
	size_t dest_orig_len, size_t offset);

bool remove_string(char * root, size_t len, size_t offset, size_t remove);

char* search_header_key(const struct email_t* mail, const char* key);

char* get_value_from_key(size_t* val_len, size_t key_offset, 
	const struct email_t* mail);

char* get_value_equals(char* content_type, size_t content_len,
	size_t* boundary_len, char* key);

const char* get_next_line(const char* message, size_t len);
const char* get_prev_line(const char* message, size_t len_neg);

bool detect_base64(const struct email_t* mail);

void propagate_size_change(struct email_t *mail, ssize_t change);

#endif /* TOOLS_H */
