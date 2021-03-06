/*
 * config.h - Configuration loading and data storage
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
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

extern uint16_t listen_port, forward_port;

/* Used as booleans, but integers for getops sake... */

extern int abort_on_pgp, abort_on_dkim, only_base64;

extern char* instance_id;

extern int verbose;

extern char* directory;
extern char* url_base;

extern long min_filesize;

#endif /* CONFIG_H */
