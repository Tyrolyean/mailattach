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

char* insert_string(char * destination, const char* source, 
	size_t dest_orig_len, size_t offset);

char* search_header_key(struct email_t* mail, const char* key);

char* get_value_from_key(size_t* val_len, size_t key_offset, 
	struct email_t* mail);

#endif /* TOOLS_H */
