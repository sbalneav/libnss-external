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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "nss_external.h"

/*
 * cmdopen:
 *
 * Sanity check and open command
 */

char **
cmdopen (const char *command, char *arg)
{
  struct stat sb;
  FILE *fcmd = NULL;
  char cmd[CMDSIZ];
  char **file = NULL;
  char *line = NULL;
  int c, len = 0, pos = 0, nlines = 0;

  /*
   * Do we have the command to execute?
   */

  if (!((stat (command, &sb) == 0) && (sb.st_mode > 0)
       && (S_IEXEC & sb.st_mode)))
      return NULL;

  /*
   * Make sure command doesn't overflow
   */

  if (snprintf (cmd, sizeof cmd, "%s=1 %s %s", DISABLE, command, arg) >= CMDSIZ)
      return NULL;

  /*
   * Call fflush before the popen command to make sure we're not
   * interfering with any buffered i/o currently in progress.
   */

  fflush (NULL);
  if ((fcmd = popen (cmd, "r")) == NULL)
      return NULL;

  /*
   * Read the output of the command, insert into a array of null-terminated
   * strings.
   */

  while ((c = fgetc (fcmd)) != EOF)
    {
      if (c != '\n')
	{
	  if (pos >= (len - 1))	/* leave room for NULL terminator */
	    {
	      char *tmpline;

	      len += CHUNKSIZ;
	      tmpline = realloc (line, len);
	      if (tmpline == NULL)
		  BAIL;
	      line = tmpline;
	    }
	  line[pos++] = c;
	}
      else
	{
	  char **tmpfile;

	  line[pos] = '\0';
	  tmpfile = realloc (file, (++nlines + 1) * sizeof (char *));	/* +1 for terminating null */
	  if (tmpfile == NULL)
              BAIL;
	  file = tmpfile;
	  file[nlines - 1] = line;
          file[nlines] = line = NULL;
	  pos = len = 0;
	}
    }

  pclose (fcmd);
  return file;
}

/*
 * cmdclose
 *
 * free all the strings generated from executing the command.
 */

void
cmdclose (char **f)
{
  char **fp;

  if (!f)
      return;

  for (fp = f; *fp != NULL; fp++)
      free (*fp);
  free (f);
}

/*
 * split:
 * When passed a buffer that contains newline separated strings,
 * and a character to split on, allocate an array of character
 * pointers, and initialize to the beginnings of each line.  Any
 * occurance of the "delim" character is turned into a '\0'
 */

char **
split (char *buffer, const char *delim)
{
  char **array, **ap;
  char *p;
  size_t size = 0;

  /*
   * Trivial case; buffer == NULL or *buffer = '\0', return a single
   * pointer array NULL terminated.
   */

  if ((buffer == NULL) || (*buffer == '\0'))
      return calloc (1, sizeof (char *));

  /*
   * Take a quick pass through the buffer and count the number
   * of delim characters.
   */

  for (p = buffer; *p != '\0'; p++)
      if (*p == *delim)
	  size++;

  /*
   * We'll need 2 more than the delim count: also want a null
   * terminator on the end of the array.
   */

  if ((array = calloc (size + 2, sizeof (char *))) == NULL)
      return NULL;

  /*
   * Delimit the array.
   */

  for (ap = array, p = buffer; (*ap = strsep (&p, delim)) != NULL; ap++);

  return array;
}
