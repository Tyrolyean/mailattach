/*
 * file.c - File writing functionality
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

#include "file.h"
#include "config.h"
#include "base64.h"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Generate a safe directory name to store ONE emails files into. This is
 * done to prevent someone from guessing the directory names. THe first part
 * is the date in ISO format, in case you want to have a shell script cleaning
 * the directory every once in a while.
 */
char* generate_safe_dirname(){
	
	/* Get time */
        time_t rawtime;
        struct tm *info;
        time( &rawtime );
        info = localtime( &rawtime );

#define TIME_LEN 30
        char datestr[TIME_LEN];
	memset(datestr, 0, TIME_LEN);
        strftime(datestr, TIME_LEN, "%FT%T%z", info);

	/* Get data from urandom to be  secure. I mean from what I
	 * know it should be, but I'm not a crypto expert. If you have doubts
	 * and know how I should do that, PLEASE TELL ME! The man pages told me
	 * to do so!
	 */
	int randie[3];
	int random_fd = open("/dev/random", O_RDONLY);
	if (random_fd < 0){
		perror("Failed to open /dev/urandom");
		return NULL;
	}else{
	    size_t randie_len = 0;
	    while (randie_len < sizeof(randie)){
		ssize_t result = read(random_fd, randie + randie_len, sizeof(randie) - randie_len);
		if (result < 0){

			perror("Failed to read from /dev/urandom");
			close(random_fd);
			return NULL;
		}
		randie_len += result;
	    }
	    close(random_fd);
	}
	size_t dir_len = TIME_LEN + 50 + strlen(directory);
	char * dir_id = malloc(dir_len+1);
	memset(dir_id, 0, dir_len+1 );

	snprintf(dir_id,dir_len, "%s/%s%i%i%i/",directory, datestr,
		randie[0], randie[1], randie[2]);
#undef TIME_LEN
	return dir_id;
}

int base64_decode_file(const char* directory, const struct email_t* mail){
	
	if(!mail->base64_encoded){
		return -1;
	}

	if(directory == NULL || mail == NULL || mail->file_info.name == NULL){

		/* I don't know how I should call that file! */
		return 0;
	}
	size_t dec_len = 0;
	unsigned char* decoded =  base64_decode(
		((unsigned char*)mail->message+mail->body_offset),
		(mail->message_length-mail->body_offset),
		&dec_len);
	
	if(decoded == NULL){
		fprintf(stderr, "Failed to decode base64 message\n");
		return -1;
	}


	size_t fn_len = strlen(mail->file_info.name) + strlen(directory) + 1;
	
	char* filename = malloc(fn_len);
	if(filename == NULL){
		free(decoded);
		return -1;
	}

	memset(filename, 0, fn_len);
	strcat(filename, directory);
	strcat(filename, mail->file_info.name);

	bool exists = false;
	for(size_t i = 0; i < 10; i++){
		exists = file_exists(filename);
		if(exists){
			free(filename);
			fn_len = strlen(mail->file_info.name) + 
				strlen(directory) + 20;
			filename = malloc(fn_len);
			snprintf(filename, fn_len, "%s%i-%s",directory,
				rand(), mail->file_info.name);
		}
	}
	if(exists){
		/* What?*/
		fprintf(stderr,"Failed to create unique file name!\n");
		free(filename);
		free(decoded);
		return -1;
	}

	FILE* outfile = fopen(filename, "w+");
	if(outfile == NULL){
		perror("Failed to open base64 out file");
		free(filename);
		free(decoded);
		return -1;
	}
	if(verbose){
		printf("Storing base64 file len %lu top [%s]\n",
			dec_len, decoded);
	}

	fwrite(decoded, dec_len, 1, outfile);
	fclose(outfile);
	free(filename);
	free(decoded);
	return 0;
	
}

bool file_exists(const char* filename){
	struct stat buffer;
	int exist = stat(filename,&buffer);
	if(exist >= 0){
		return true;
	} else{
		return false;
	}
}
