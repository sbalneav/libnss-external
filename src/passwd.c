/*
 * nss_external: NSS module for providing NSS services from a external
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

#include <stdio.h>
#include <stdlib.h>
#include <nss.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "nss_external.h"

/*
 * Needed for getpwent() statefulness.
 */

static char **proc = NULL;
static char **pproc;

/*
 * buffer_to_pwstruct:
 *
 * Given a buffer containing a single passwd(5) line, populate a
 * password struct.  Note that the ':' in the buffer are converted
 * to '\0' by the call to split.
 */

static enum nss_status
buffer_to_pwstruct (struct passwd *pwstruct, char *newbuf, char *buffer,
		    size_t buflen, int *errnop)
{
  char **array;
  size_t count;

  if (pwstruct == NULL)
    {
      /* We weren't passed a valid pwstruct */
      *errnop = EAGAIN;
      return NSS_STATUS_TRYAGAIN;
    }

  if (strlen (newbuf) >= buflen)
    {
      *errnop = ERANGE;
      return NSS_STATUS_TRYAGAIN;
    }

  strncpy (buffer, newbuf, buflen);

  /*
   * split on ':'
   */

  array = split (buffer, ":");

  if (array == NULL)
    {
      /* couldn't split on the buffer */
      *errnop = EAGAIN;
      return NSS_STATUS_TRYAGAIN;
    }

  /*
   * sanity check: we should have 7 fields.  If not, it's
   * not a valid entry, so return NOTFOUND
   */

  for (count = 0; array[count] != NULL; count++);

  if (count != 7)
    {
      free (array);
      *errnop = ENOENT;
      return NSS_STATUS_NOTFOUND;
    }

  /*
   * Populate the pwstruct
   */

  pwstruct->pw_name   = array[0];
  pwstruct->pw_passwd = array[1];
  pwstruct->pw_uid    = (uid_t) atoi (array[2]);
  pwstruct->pw_gid    = (gid_t) atoi (array[3]);
  pwstruct->pw_gecos  = array[4];
  pwstruct->pw_dir    = array[5];
  pwstruct->pw_shell  = array[6];

  free (array);

  return NSS_STATUS_SUCCESS;
}

/*
 * search:
 *
 * When passed a command, get the result and populate.
 */

static enum nss_status
search (const char *command, char *arg, struct passwd *result, char *buffer,
	size_t buflen, int *errnop)
{
  char **proc;
  enum nss_status status;

  *errnop = 0;

  proc = cmdopen (command, arg);

  CHECKUNAVAIL(proc);
  CHECKLAST(proc[0]);

  status = buffer_to_pwstruct (result, proc[0], buffer, buflen, errnop);
  cmdclose (proc);

  return status;
}

/*
 * _nss_external_getpwuid_r
 */

enum nss_status
_nss_external_getpwuid_r (uid_t uid, struct passwd *result, char *buffer,
			  size_t buflen, int *errnop)
{
  char arg[CMDSIZ];

  CHECKDISABLED;

  *errnop = 0;

  if (uid < MINUID)
      return NSS_STATUS_NOTFOUND;

  /*
   * Will the arg overflow?
   */

  if (snprintf (arg, CMDSIZ, "%d", uid) >= CMDSIZ)
    {
      *errnop = ENOENT;
      return NSS_STATUS_UNAVAIL;
    }

  return search (PASSWDCMD, arg, result, buffer, buflen, errnop);
}

/*
 * _nss_external_getpwnam_r
 */

enum nss_status
_nss_external_getpwnam_r (const char *name, struct passwd *result,
			  char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status;

  CHECKDISABLED;

  *errnop = 0;

  status = search (PASSWDCMD, (char *) name, result, buffer, buflen, errnop);

  if (status == NSS_STATUS_SUCCESS)
      if (result->pw_uid < MINUID)
	  return NSS_STATUS_NOTFOUND;

  return status;
}

/*
 * _nss_external_setpwent
 *
 * Implements setpwent() functionality.
 */

enum nss_status
_nss_external_setpwent (void)
{
  CHECKDISABLED;

  if (proc != NULL)
      cmdclose (proc);

  proc = cmdopen (PASSWDCMD, "");
  pproc = proc;

  return NSS_STATUS_SUCCESS;
}

/*
 * _nss_external_getpwent_r
 *
 * Implements getpwent() functionality
 */

enum nss_status
_nss_external_getpwent_r (struct passwd *result, char *buffer, size_t buflen,
			  int *errnop)
{
  CHECKDISABLED;

  *errnop = 0;

  CHECKUNAVAIL(pproc);

  for (;;)
    {
      enum nss_status status;

      CHECKLAST(*pproc);
      status = buffer_to_pwstruct (result, *pproc, buffer, buflen, errnop);

      if (status == NSS_STATUS_TRYAGAIN)
	  return status;

      /* buffer was large enough, so increment pointer to next result */
      pproc++;

      if ((status != NSS_STATUS_SUCCESS) || (result->pw_uid >= MINUID))
	  return status;
    }
}

/*
 * _nss_external_endpwent
 *
 * Implements the endpwent() functionality.
 */

enum nss_status
_nss_external_endpwent (void)
{
  CHECKDISABLED;

  if (proc != NULL)
    {
      cmdclose (proc);
      proc = NULL;
      pproc = NULL;
    }

  return NSS_STATUS_SUCCESS;
}
