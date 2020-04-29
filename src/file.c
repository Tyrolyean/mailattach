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



char* generate_safe_dirname(){
	
	/* Get time */
        time_t rawtime;
        struct tm *info;
        time( &rawtime );
        info = localtime( &rawtime );

#define TIME_LEN 30
        char datestr[TIME_LEN];
	if(datestr == NULL){
		return NULL;
	}
        strftime(datestr, TIME_LEN, "%FT%T%z", info);
	
	
#undef TIME_LEN


}

#endif /* FILE_H */
