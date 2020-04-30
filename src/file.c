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
#include <sys/random.h>
#include <ctype.h>

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
        strftime(datestr, TIME_LEN, "%FT%H%M", info);

	/* Get data from urandom to be  secure. I mean from what I
	 * know it should be, but I'm not a crypto expert. If you have doubts
	 * and know how I should do that, PLEASE TELL ME! The man pages told me
	 * to do so!
	 */
	int randie[2];
	
	if(getrandom(randie, sizeof(randie), 0) <= 0){
		perror("Failed to get random seed! Aborting");
		return NULL;
	}

	size_t dir_len = TIME_LEN + 50 + strlen(directory);
	char * dir_id = malloc(dir_len+1);
	memset(dir_id, 0, dir_len+1 );

	snprintf(dir_id,dir_len, "%s/%s%i%i/",directory, datestr,
		randie[0], randie[1]);
#undef TIME_LEN
	return dir_id;
}

int base64_decode_file(const char* directory, const struct email_t* mail,
	char** dest_filename){
	
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
	
	int n = decode_file(directory,(char*) decoded, dec_len, 
		mail->file_info.name, dest_filename);
	
	free(decoded);

	return n;
	
}

int decode_file(const char* directory, const char * message, size_t len, 
	char* name, char** dest_filename){

	if(directory == NULL || message == NULL || name == NULL){

		/* I don't know how I should call that file! */
		return 0;
	}
	char* sane_name = sanitizie_filename(name);
	
	size_t fn_len = strlen(sane_name) + strlen(directory) + 1;
	
	char* filename = malloc(fn_len);
	if(filename == NULL){
		return -1;
	}

	memset(filename, 0, fn_len);
	strcat(filename, directory);
	strcat(filename, sane_name);
	
	
	bool exists = false;
	for(size_t i = 0; i < 10; i++){
		exists = file_exists(filename);
		if(exists){
			free(filename);
			fn_len = strlen(sane_name) + 
				strlen(directory) + 20;
			filename = malloc(fn_len);
			snprintf(filename, fn_len, "%s%i-%s",directory,
				rand(), sane_name);
		}
	}
	
	free(sane_name);
	
	if(exists){
		/* What?*/
		fprintf(stderr,"Failed to create unique file name!\n");
		free(filename);
		return -1;
	}

	FILE* outfile = fopen(filename, "w+");
	if(outfile == NULL){
		perror("Failed to open base64 out file");
		free(filename);
		return -1;
	}
	if(verbose){
		printf("Storing file len %lu to [%s]\n",
			len, filename);
	}

	fwrite(message, len, 1, outfile);
	fclose(outfile);
	*dest_filename = filename;
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

/* Generates a filename which should fit into a URL without URL-encode and which
 * doesn't allow for  */
char* sanitizie_filename(char* filename){
	
	if(filename == NULL){
		return NULL;
	}
	size_t filename_len = strlen(filename);
	size_t new_len = 0;
	char* new_name = malloc(filename_len + 1);
	memset(new_name, 0, filename_len+1);
	for(size_t i = 0; i < filename_len; i++){
		if(isalnum(filename[i]) || filename[i] == '-' || 
			filename[i] == '.'){
			new_name[new_len++] = filename[i];
		}
	}
	return new_name;
}
