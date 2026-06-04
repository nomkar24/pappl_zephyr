//
// Utility functions for the Printer Application Framework (Zephyr Port)
//
// Copyright © 2019-2025 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "base-private.h"

// Local functions...
static cups_bool_t filter_cb(_pappl_ipp_filter_t *filter, ipp_t *dst, ipp_attribute_t *attr);

// '_papplCopyAttributes()' - Copy attributes from one message to another.
void
_papplCopyAttributes(
    ipp_t        *to,			// I - Destination request
    ipp_t        *from,			// I - Source request
    cups_array_t *ra,			// I - Requested attributes
    ipp_tag_t    group_tag,		// I - Group to copy
    bool         quickcopy)		// I - Do a quick copy?
{
  _pappl_ipp_filter_t	filter;		// Filter data

  filter.ra        = ra;
  filter.group_tag = group_tag;

  ippCopyAttributes(to, from, quickcopy, (ipp_copy_cb_t)filter_cb, &filter);
}

// 'papplCreatePipe()' - Create a pair of file descriptors in a pipe.
// Stubbed for Zephyr as subprocess command filtering is not used.
bool
papplCreatePipe(int  *fds,		// O - Array of 2 file descriptors
                bool text)		// I - `true` for a text pipe, `false` for binary data
{
  (void)text;

  if (fds)
  {
    fds[0] = -1;
    fds[1] = -1;
  }

  return (false);
}

// 'papplCreateTempFile()' - Create a temporary file.
int					// O - File descriptor or `-1` on error
papplCreateTempFile(
    char       *fname,			// I - Filename buffer
    size_t     fnamesize,		// I - Size of filename buffer
    const char *prefix,			// I - Prefix for filename
    const char *ext)			// I - Filename extension, if any
{
  int	fd,				// File descriptor
	tries = 0;			// Number of tries
  char	name[64],			// "Safe" filename
	*nameptr;			// Pointer into filename

  // Range check input...
  if (!fname || fnamesize < 256 || (prefix && strstr(prefix, "../")) || (ext && strstr(ext, "../")))
  {
    if (fname)
      *fname = '\0';

    return (-1);
  }

  if (prefix)
  {
    // Make a name from the prefix argument...
    for (nameptr = name; *prefix && nameptr < (name + sizeof(name) - 1); prefix ++)
    {
      if (isalnum(*prefix & 255) || *prefix == '-' || *prefix == '.')
      {
	*nameptr++ = (char)tolower(*prefix & 255);
      }
      else
      {
	*nameptr++ = '_';

	while (prefix[1] && !isalnum(prefix[1] & 255) && prefix[1] != '-' && prefix[1] != '.')
	  prefix ++;
      }
    }

    *nameptr = '\0';
  }
  else
  {
    // Use a prefix of "t"...
    cupsCopyString(name, "t", sizeof(name));
  }

  do
  {
    // Create a filename...
    if (ext)
      snprintf(fname, fnamesize, "%s/%s%08x.%s", papplGetTempDir(), name, cupsGetRand(), ext);
    else
      snprintf(fname, fnamesize, "%s/%s%08x", papplGetTempDir(), name, cupsGetRand());

    tries ++;
  }
  // Simplified open flags for Zephyr VFS compatibility (no O_NOFOLLOW / O_CLOEXEC / O_EXCL support needed)
  while ((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0 && tries < 100);

  return (fd);
}

// 'papplGetTempDir()' - Get the temporary directory.
const char *				// O - Temporary directory
papplGetTempDir(void)
{
  static char tmppath[1024] = "";	// Temporary directory buffer
  static cups_mutex_t tmpmutex = CUPS_MUTEX_INITIALIZER;
					// Mutex to control access

  cupsMutexLock(&tmpmutex);
  if (!tmppath[0])
  {
    // Return standard temporary path default for Zephyr targets
    cupsCopyString(tmppath, "/tmp", sizeof(tmppath));
  }
  cupsMutexUnlock(&tmpmutex);

  return (tmppath);
}

// '_papplIsEqual()' - Compare two strings for equality in constant time.
bool					// O - `true` on match, `false` on non-match
_papplIsEqual(const char *a,		// I - First string
              const char *b)		// I - Second string
{
  bool	result = true;			// Result

  // Loop through both strings, noting any differences...
  while (*a && *b)
  {
    result &= *a == *b;
    a ++;
    b ++;
  }

  // Return, capturing the equality of the last characters...
  return (result && *a == *b);
}

// 'filter_cb()' - Filter printer attributes based on the requested array.
static cups_bool_t			// O - `CUPS_BOOL_TRUE` to copy, `CUPS_BOOL_FALSE` to ignore
filter_cb(_pappl_ipp_filter_t *filter,	// I - Filter parameters
          ipp_t               *dst,	// I - Destination (unused)
	  ipp_attribute_t     *attr)	// I - Source attribute
{
  ipp_tag_t	group = ippGetGroupTag(attr);
					// Attribute group
  const char	*name = ippGetName(attr);
					// Attribute name

  (void)dst;

  // Filter out attributes in the wrong group or the "media-col-database" attribute unless requested...
  if ((filter->group_tag != IPP_TAG_ZERO && group != filter->group_tag && group != IPP_TAG_ZERO) || !name || (!strcmp(name, "media-col-database") && !cupsArrayFind(filter->ra, (void *)name)))
    return (CUPS_BOOL_FALSE);

  // Otherwise filter attributes by name...
  return (!filter->ra || cupsArrayFind(filter->ra, (void *)name) != NULL);
}
