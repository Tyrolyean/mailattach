/*
 * file.h - File writing functionality
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

#ifndef FILE_H
#define FILE_H

#include "mail.h"

#include <stdbool.h>

char* generate_safe_dirname();

int base64_decode_file(const char* directory, const struct email_t* mail);

bool file_exists(const char* filename);

#endif /* FILE_H */
