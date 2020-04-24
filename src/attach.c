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

#include <string.h>
#include <stdlib.h>

char* attach_files(const unsigned char* original_message, 
	size_t original_length){

	const char* new_body = malloc(original_length + 1);
	if(new_body == NULL){
		return NULL;
	}
	memset(new_body,0,  original_length + 1);
	
}

