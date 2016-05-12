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

#include <stdio.h>
#include <stdlib.h>
#include <nss.h>
#include <shadow.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "nss_external.h"

/*
 * Needed for getpwent() statefulness.
 */

static char **proc = NULL;
static char **sproc;

/*
 * buffer_to_spwdstruct:
 *
 * Given a buffer containing a single shadow(5) line, populate a
 * shadow passwd struct.  Note that the ':' in the buffer are converted
 * to '\0' by the call to split.
 */

static enum nss_status
buffer_to_spwdstruct (struct spwd *spwdstruct, char *newbuf, char *buffer,
		      size_t buflen, int *errnop)
{
  char **array;
  size_t count;

  if (spwdstruct == NULL)
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
   * sanity check: we should have 9 fields.  If not, it's not a valid
   * spwd entry, so return NOTFOUND.
   */

  for (count = 0; array[count] != NULL; count++);

  if (count != 9)
    {
      free (array);
      *errnop = ENOENT;
      return NSS_STATUS_NOTFOUND;
    }

  /*
   * Populate the spwdstruct
   */

  spwdstruct->sp_namp   = array[0];
  spwdstruct->sp_pwdp   = array[1];
  spwdstruct->sp_lstchg = *array[2] == '\0' ? -1 : atol (array[2]);
  spwdstruct->sp_min    = *array[3] == '\0' ? -1 : atol (array[3]);
  spwdstruct->sp_max    = *array[4] == '\0' ? -1 : atol (array[4]);
  spwdstruct->sp_warn   = *array[5] == '\0' ? -1 : atol (array[5]);
  spwdstruct->sp_inact  = *array[6] == '\0' ? -1 : atol (array[6]);
  spwdstruct->sp_expire = *array[7] == '\0' ? -1 : atol (array[7]);
  spwdstruct->sp_flag   = *array[8] == '\0' ? -1 : strtoul (array[8], NULL, 10);

  free (array);

  return NSS_STATUS_SUCCESS;
}

/*
 * search:
 *
 * When passed a command, get the result and populate.
 */

static enum nss_status
search (const char *command, char *arg, struct spwd *result, char *buffer,
	size_t buflen, int *errnop)
{
  char **proc;
  enum nss_status status;

  *errnop = 0;

  proc = cmdopen (command, arg);

  CHECKUNAVAIL(proc);
  CHECKLAST(proc[0]);

  status = buffer_to_spwdstruct (result, proc[0], buffer, buflen, errnop);
  cmdclose (proc);

  return status;
}

/*
 * _nss_external_getspnam_r
 */

enum nss_status
_nss_external_getspnam_r (const char *name, struct spwd *result, char *buffer,
			  size_t buflen, int *errnop)
{
  CHECKDISABLED;
  CHECKROOT;

  *errnop = 0;

  return search (SHADOWCMD, (char *) name, result, buffer, buflen, errnop);
}

/*
 * _nss_external_setspent
 */

enum nss_status
_nss_external_setspent (void)
{
  CHECKDISABLED;

  if (proc != NULL)
      cmdclose (proc);

  proc = cmdopen (SHADOWCMD, "");
  sproc = proc;

  return NSS_STATUS_SUCCESS;
}

/*
 * _nss_external_getspent_r
 */

enum nss_status
_nss_external_getspent_r (struct spwd *result, char *buffer, size_t buflen,
			  int *errnop)
{
  enum nss_status status;

  CHECKDISABLED;
  CHECKROOT;

  *errnop = 0;

  CHECKUNAVAIL(sproc);
  CHECKLAST(*sproc);

  status = buffer_to_spwdstruct (result, *sproc, buffer, buflen, errnop);

  if (status == NSS_STATUS_TRYAGAIN)
      return status;

  sproc++;

  return status;
}

/*
 * _nss_external_endspent
 */

enum nss_status
_nss_external_endspent (void)
{
  CHECKDISABLED;

  if (proc != NULL)
    {
      cmdclose (proc);
      proc = NULL;
      sproc = NULL;
    }

  return NSS_STATUS_SUCCESS;
}
