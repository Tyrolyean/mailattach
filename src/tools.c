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

#include <string.h>
#include <stdlib.h>
#include <strings.h>

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

