/*
 * nss_external: NSS module for providing NSS services from an external
 * command.
 *
 * Copyright (C) 2016 Scott Balneaves <sbalneav@ltsp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Config
 */

#define CONFDIR "/etc/nss-external"
#define PASSWDCMD CONFDIR "/passwd"
#define GROUPCMD  CONFDIR "/group"
#define SHADOWCMD CONFDIR "/shadow"

/*
 * Minimum UID and GID we'll return
 */

#define MINUID 500
#define MINGID 500

/*
 * Size of the commands passed to popen
 * and chunksize for lines.
 */

#define CMDSIZ   BUFSIZ
#define CHUNKSIZ 128

/*
 * Environment variables.
 */

#define DISABLE  "NSS_EXTERNAL_DISABLE"

/*
 * Quick macros
 */

#define CHECKDISABLED   { if (getenv (DISABLE)) return NSS_STATUS_NOTFOUND; }
#define CHECKROOT       { if (geteuid () != 0) { *errnop = EPERM; return NSS_STATUS_UNAVAIL; }}
#define CHECKUNAVAIL(p) { if (p == NULL) { *errnop = ENOENT; return NSS_STATUS_UNAVAIL; }}
#define CHECKLAST(p)    { if (p == '\0') { *errnop = ENOENT; return NSS_STATUS_NOTFOUND; }}
#define BAIL            { free (line); cmdclose (file); file = NULL; break; }

/*
 * Prototypes
 */

char **cmdopen (const char *command, char *arg);
void cmdclose (char **f);
char **split (char *buffer, const char *delim);
