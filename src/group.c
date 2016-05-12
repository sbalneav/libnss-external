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
#include <grp.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "nss_external.h"

/*
 * Needed for getgrent() statefulness.
 */

static char **proc = NULL;
static char **gproc;

/*
 * buffer_to_grstruct:
 *
 * Given a buffer containing a single group(5) line, populate a
 * group struct.  Note that the ':' in the buffer are converted
 * to '\0' by the call to split.
 */

static enum nss_status
buffer_to_grstruct (struct group *grstruct, char *newbuf, char *buffer,
		    size_t buflen, int *errnop)
{
  char **array;
  size_t count;
  size_t loop;
  char *c;
  char *bufstart;
  char *groupmembers;
  char **arraystart = (char **) buffer;
  size_t offset;

  if (grstruct == NULL)
    {
      /* We weren't passed a valid grstruct */
      *errnop = EAGAIN;
      return NSS_STATUS_TRYAGAIN;
    }

  /*
   * Here's how we'll lay out our buffer:
   *
   * +---------------+----------+-------+-------+----------------------------+
   * | array of char | grname\0 | pwd\0 | gid\0 | grp membs terminated by \0 |
   * | pointers into |          |       |       |                            |
   * | grp memb with |          |       |       |                            |
   * | \0 term.      |          |       |       |                            |
   * +---------------+----------+-------+-------+----------------------------+
   */

  /*
   * First, count # of ','s in the alloc'd buffer, so we know how
   * much space for the char * array we need to reserve. Start off
   * with a minimum size of 2, since if we find no ','s, it may mean
   * we have one group member, and we need a NULL for a terminator.
   */

  count = 2;

  for (c = newbuf; *c != '\0'; c++)
      if (*c == ',')
	  count++;

  /*
   * So, we need to reserve (size of char ptr) * count
   * bytes at the beginning of the buffer.
   */

  offset = count * sizeof (char *);
  bufstart = buffer + offset;

  /*
   * Enough space?
   */

  if ((offset + strlen (newbuf) + 1) > buflen)
    {
      *errnop = ERANGE;
      return NSS_STATUS_TRYAGAIN;
    }

  /*
   * copy buffer into new location
   */

  strcpy (bufstart, newbuf);

  /*
   * Initialize arraystart to null pointers
   */

  for (loop = 0; loop < count; loop++)
      arraystart[loop] = NULL;

  /*
   * split on ':'
   */

  array = split (bufstart, ":");

  if (array == NULL)
    {
      /* couldn't split on the buffer */
      *errnop = EAGAIN;
      return NSS_STATUS_TRYAGAIN;
    }

  /*
   * sanity check: we should have 4 fields.  If not, not a valid group entry,
   * so return NOTFOUND.
   */

  for (loop = 0; array[loop] != NULL; loop++);

  if (loop != 4)
    {
      free (array);
      *errnop = ENOENT;
      return NSS_STATUS_NOTFOUND;
    }

  /*
   * Populate the grstruct
   */

  grstruct->gr_name   = array[0];
  grstruct->gr_passwd = array[1];
  grstruct->gr_gid    = (gid_t) atoi (array[2]);
  grstruct->gr_mem    = arraystart;
  groupmembers        = array[3];

  free (array);

  /*
   * groupmembers now contains the list
   * of comma separated group members.
   * Re split on ','
   */

  array = split (groupmembers, ",");

  if (array == NULL)
    {
      /* couldn't split on the buffer */
      *errnop = EAGAIN;
      return NSS_STATUS_TRYAGAIN;
    }

  /*
   * Copy array into the space reserved in the
   * buffer.  Make sure to NULL terminate at the end.
   */

  for (loop = 0; array[loop] != NULL; loop++)
      arraystart[loop] = array[loop];

  free (array);

  return NSS_STATUS_SUCCESS;
}

/*
 * search:
 *
 * When passed a command, get the result and populate.
 */

static enum nss_status
search (char *command, char *arg, struct group *result, char *buffer,
	size_t buflen, int *errnop)
{
  char **proc;
  enum nss_status status;

  *errnop = 0;

  proc = cmdopen (command, arg);

  CHECKUNAVAIL(proc);
  CHECKLAST(proc[0]);

  status = buffer_to_grstruct (result, proc[0], buffer, buflen, errnop);
  cmdclose (proc);

  return status;
}

/*
 * _nss_external_getgrgid_r
 */

enum nss_status
_nss_external_getgrgid_r (gid_t gid, struct group *result, char *buffer,
			  size_t buflen, int *errnop)
{
  char arg[CMDSIZ];

  CHECKDISABLED;

  if (gid < MINGID)
      return NSS_STATUS_NOTFOUND;

  /*
   * Will the arg overflow?
   */

  if (snprintf (arg, CMDSIZ, "%d", gid) >= CMDSIZ)
    {
      *errnop = ENOENT;
      return NSS_STATUS_UNAVAIL;
    }

  return search (GROUPCMD, arg, result, buffer, buflen, errnop);
}

/*
 * _nss_external_getgrnam_r
 */

enum nss_status
_nss_external_getgrnam_r (const char *name, struct group *result,
			  char *buffer, size_t buflen, int *errnop)
{
  enum nss_status status;

  CHECKDISABLED;

  *errnop = 0;

  status = search (GROUPCMD, (char *) name, result, buffer, buflen, errnop);

  if (status == NSS_STATUS_SUCCESS)
      if (result->gr_gid < MINGID)
	  return NSS_STATUS_NOTFOUND;

  return status;
}


/*
 * _nss_external_setgrent
 */

enum nss_status
_nss_external_setgrent (void)
{
  CHECKDISABLED;

  if (proc != NULL)
      cmdclose (proc);

  proc = cmdopen (GROUPCMD, "");
  gproc = proc;

  return NSS_STATUS_SUCCESS;
}

/*
 * _nss_external_getgrent_r
 */

enum nss_status
_nss_external_getgrent_r (struct group *result, char *buffer, size_t buflen,
			  int *errnop)
{
  enum nss_status status;

  CHECKDISABLED;

  *errnop = 0;

  CHECKUNAVAIL(gproc);

  for (;;)
    {
      CHECKLAST(*gproc);
      status = buffer_to_grstruct (result, *gproc, buffer, buflen, errnop);

      if (status == NSS_STATUS_TRYAGAIN)
          return status;

      /* buffer was large enough, so increment pointer to next result */
      gproc++;

      if ((status != NSS_STATUS_SUCCESS) || (result->gr_gid >= MINGID))
	  return status;
    }
}

/*
 * _nss_external_endgrent
 *
 * Implements the endgrent() functionality.
 */

enum nss_status
_nss_external_endgrent (void)
{
  CHECKDISABLED;

  if (proc != NULL)
    {
      cmdclose (proc);
      proc = NULL;
      gproc = NULL;
    }

  return NSS_STATUS_SUCCESS;
}
