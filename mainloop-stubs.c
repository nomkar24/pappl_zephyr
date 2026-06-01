//
// Mainloop stubs and ported functions for the Printer Application Framework
//
//   This file provides definitions for every symbol declared in
//   mainloop-private.h.  The original implementations lived in three
//   POSIX-dependent files (mainloop.c, mainloop-subcommands.c,
//   mainloop-support.c) which are excluded from the Zephyr build.
//
//   _papplMainloopAddOptions() is the ONLY function called from outside
//   those files (used by job.c -> papplJobCreateWithFile).  It is fully
//   ported below because it is pure IPP attribute manipulation — no
//   POSIX, no fork/exec, no sockets.
//
//   Everything else returns error (1) or NULL to satisfy the linker.
//

#include "pappl-private.h"


//
// Globals...
//

char *_papplMainloopPath = NULL;		// Not used on Zephyr


//
// Local functions...
//

static int	get_length(const char *value);


//
// FULLY PORTED: _papplMainloopAddOptions()
//
// Called by papplJobCreateWithFile() in job.c to convert cups_option_t
// key/value pairs into IPP job template or printer description
// attributes on an IPP request.

void
_papplMainloopAddOptions(
    ipp_t         *request,		// I - IPP request
    size_t        num_options,		// I - Number of options
    cups_option_t *options,		// I - Options
    ipp_t         *supported)		// I - Supported attributes
{
  ipp_attribute_t *job_attrs;		// job-creation-attributes-supported
  int		is_default;		// Adding xxx-default attributes?
  ipp_tag_t	group_tag;		// Group to add to
  char		*end;			// End of value
  const char	*value;			// String value
  int		intvalue;		// Integer value
  const char	*media_left_offset = cupsGetOption("media-left-offset", num_options, options),
		*media_ready = cupsGetOption("media-ready", num_options, options),
		*media_source = cupsGetOption("media-source", num_options, options),
                *media_top_offset = cupsGetOption("media-top-offset", num_options, options),
		*media_tracking = cupsGetOption("media-tracking", num_options, options),
		*media_type = cupsGetOption("media-type", num_options, options);
					// media-xxx member values


  // Determine what kind of options we are adding...
  group_tag  = ippGetOperation(request) == IPP_OP_PRINT_JOB ? IPP_TAG_JOB : IPP_TAG_PRINTER;
  is_default = (group_tag == IPP_TAG_PRINTER);

  if (is_default)
  {
    // Add Printer Description attributes...
    if ((value = cupsGetOption("label-mode-configured", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "label-mode-configured", NULL, value);

    if ((value = cupsGetOption("label-tear-offset-configured", num_options, options)) != NULL)
      ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "label-tear-offset-configured", get_length(value));

    if ((value = cupsGetOption("media-ready", num_options, options)) != NULL)
    {
      size_t num_values;		// Number of values
      char	*values[PAPPL_MAX_SOURCE],
					// Pointers to size strings
	*cvalue,			// Copied value
	*cptr;				// Pointer into copy

      num_values = 0;
      cvalue     = strdup(value);
      cptr       = cvalue;

      while (num_values < PAPPL_MAX_SOURCE && (values[num_values] = strsep(&cptr, ",")) != NULL)
        num_values ++;

      if (num_values > 0)
        ippAddStrings(request, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "media-ready", num_values, NULL, (const char * const *)values);

      free(cvalue);
    }

    if ((value = cupsGetOption("printer-darkness-configured", num_options, options)) != NULL)
    {
      if ((intvalue = (int)strtol(value, &end, 10)) >= 0 && intvalue <= 100 && errno != ERANGE && !*end)
        ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "printer-darkness-configured", intvalue);
    }

    if ((value = cupsGetOption("printer-geo-location", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI, "printer-geo-location", NULL, value);

    if ((value = cupsGetOption("printer-location", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-location", NULL, value);

    if ((value = cupsGetOption("printer-organization", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-organization", NULL, value);

    if ((value = cupsGetOption("printer-organizational-unit", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-organizational-unit", NULL, value);
  }
  else
  {
    if ((value = cupsGetOption("compression", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "compression", NULL, value);

    if ((value = cupsGetOption("page-ranges", num_options, options)) != NULL)
    {
      int	first_page = 1,		// First page
		last_page = INT_MAX;	// Last page
      char	*valptr = (char *)value;// Pointer into value

      if (isdigit(*valptr & 255))
	first_page = (int)strtol(valptr, &valptr, 10);
      if (*valptr == '-')
	valptr ++;
      if (isdigit(*valptr & 255))
	last_page = (int)strtol(valptr, &valptr, 10);

      ippAddRange(request, IPP_TAG_JOB, "page-ranges", first_page, last_page);
    }
  }

  if ((value = cupsGetOption("copies", num_options, options)) == NULL)
    value = cupsGetOption("copies-default", num_options, options);
  if (value && (intvalue = (int)strtol(value, &end, 10)) >= 1 && intvalue <= 9999 && errno != ERANGE && !*end)
    ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? "copies-default" : "copies", intvalue);

  if ((value = cupsGetOption("finishings", num_options, options)) == NULL)
    value = cupsGetOption("finishings-default", num_options, options);
  if (value)
  {
    // Get finishings enum values...
    size_t	num_enumvalues = 0;	// Number of enum values
    int		enumvalues[32];		// Enum values
    char	keyword[128],		// Current keyword/value
		*kptr;			// Pointer into keyword/value

    while (*value && num_enumvalues < (sizeof(enumvalues) / sizeof(enumvalues[0])))
    {
      for (kptr = keyword; *value && *value != ','; value ++)
      {
        if (kptr < (keyword + sizeof(keyword) - 1))
          *kptr = *value;
      }

      *kptr = '\0';
      if (isdigit(keyword[0] & 255))
        enumvalues[num_enumvalues ++] = atoi(keyword);
      else if (keyword[0])
        enumvalues[num_enumvalues ++] = ippEnumValue("finishings", keyword);
    }

    if (num_enumvalues > 0)
      ippAddIntegers(request, group_tag, IPP_TAG_ENUM, is_default ? "finishings-default" : "finishings", num_enumvalues, enumvalues);
  }

  if (is_default && media_ready)
    value = media_ready;
  else
    value = cupsGetOption("media", num_options, options);

  if (media_left_offset || media_source || media_top_offset || media_tracking || media_type)
  {
    // Add media-col/media-col-default/media-col-ready
    ipp_t 	*media_col = ippNew();	// media-col value
    pwg_media_t *pwg = pwgMediaForPWG(value);
					// Size

    if (pwg)
    {
      ipp_t *media_size = ippNew();	// media-size value

      ippAddInteger(media_size, IPP_TAG_ZERO, IPP_TAG_INTEGER, "x-dimension", pwg->width);
      ippAddInteger(media_size, IPP_TAG_ZERO, IPP_TAG_INTEGER, "y-dimension", pwg->length);
      ippAddCollection(media_col, IPP_TAG_ZERO, "media-size", media_size);
      ippDelete(media_size);
    }

    if (media_left_offset)
      ippAddInteger(media_col, IPP_TAG_ZERO, IPP_TAG_INTEGER, "media-left-offset", get_length(media_left_offset));

    if (media_source)
      ippAddString(media_col, IPP_TAG_ZERO, IPP_TAG_KEYWORD, "media-source", NULL, media_source);

    if (media_top_offset)
      ippAddInteger(media_col, IPP_TAG_ZERO, IPP_TAG_INTEGER, "media-top-offset", get_length(media_top_offset));

    if (media_tracking)
      ippAddString(media_col, IPP_TAG_ZERO, IPP_TAG_KEYWORD, "media-tracking", NULL, media_tracking);

    if (media_type)
      ippAddString(media_col, IPP_TAG_ZERO, IPP_TAG_KEYWORD, "media-type", NULL, media_type);

    if (is_default)
    {
      ippAddCollection(request, group_tag, "media-col-default", media_col);
      if (media_ready)
	ippAddCollection(request, group_tag, "media-col-ready", media_col);
    }
    else
    {
      ippAddCollection(request, group_tag, "media-col", media_col);
    }

    ippDelete(media_col);
  }
  else if (value)
  {
    // Add media/media-default/media-ready
    if (is_default)
    {
      ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-default", NULL, value);
      if (media_ready)
        ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, "media-ready", NULL, value);
    }
    else
    {
      ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, "media", NULL, value);
    }
  }

  if ((value = cupsGetOption("orientation-requested", num_options, options)) == NULL)
    value = cupsGetOption("orientation-requested-default", num_options, options);
  if (value)
  {
    if ((intvalue = ippEnumValue("orientation-requested", value)) != 0)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "orientation-requested-default" : "orientation-requested", intvalue);
    else if ((intvalue = (int)strtol(value, &end, 10)) >= IPP_ORIENT_PORTRAIT && intvalue <= IPP_ORIENT_NONE && errno != ERANGE && !*end)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "orientation-requested-default" : "orientation-requested", intvalue);
  }

  if ((value = cupsGetOption("output-bin", num_options, options)) == NULL)
    value = cupsGetOption("output-bin-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "output-bin-default" : "output-bin", NULL, value);

  if ((value = cupsGetOption("print-color-mode", num_options, options)) == NULL)
    value = cupsGetOption("print-color-mode-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "print-color-mode-default" : "print-color-mode", NULL, value);

  if ((value = cupsGetOption("print-content-optimize", num_options, options)) == NULL)
    value = cupsGetOption("print-content-optimize-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "print-content-optimize-mode-default" : "print-content-optimize", NULL, value);

  if ((value = cupsGetOption("print-darkness", num_options, options)) == NULL)
    value = cupsGetOption("print-darkness-default", num_options, options);
  if (value && (intvalue = (int)strtol(value, &end, 10)) >= -100 && intvalue <= 100 && errno != ERANGE && !*end)
    ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? "print-darkness-default" : "print-darkness", intvalue);

  if ((value = cupsGetOption("print-quality", num_options, options)) == NULL)
    value = cupsGetOption("print-quality-default", num_options, options);
  if (value)
  {
    if ((intvalue = ippEnumValue("print-quality", value)) != 0)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "print-quality-default" : "print-quality", intvalue);
    else if ((intvalue = (int)strtol(value, &end, 10)) >= IPP_QUALITY_DRAFT && intvalue <= IPP_QUALITY_HIGH && errno != ERANGE && !*end)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "print-quality-default" : "print-quality", intvalue);
  }

  if ((value = cupsGetOption("print-scaling", num_options, options)) == NULL)
    value = cupsGetOption("print-scaling-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "print-scaling-default" : "print-scaling", NULL, value);

  if ((value = cupsGetOption("print-speed", num_options, options)) == NULL)
    value = cupsGetOption("print-speed-default", num_options, options);
  if (value)
    ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? "print-speed-default" : "print-speed", get_length(value));

  if ((value = cupsGetOption("printer-resolution", num_options, options)) == NULL)
    value = cupsGetOption("printer-resolution-default", num_options, options);

  if (value)
  {
    int		xres, yres;		// Resolution values
    char	units[32];		// Resolution units

    if (sscanf(value, "%dx%d%31s", &xres, &yres, units) != 3)
    {
      if (sscanf(value, "%d%31s", &xres, units) != 2)
      {
        xres = 300;

        cupsCopyString(units, "dpi", sizeof(units));
      }

      yres = xres;
    }

    ippAddResolution(request, group_tag, is_default ? "printer-resolution-default" : "printer-resolution", !strcmp(units, "dpi") ? IPP_RES_PER_INCH : IPP_RES_PER_CM, xres, yres);
  }

  if ((value = cupsGetOption("sides", num_options, options)) == NULL)
    value = cupsGetOption("sides-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "sides-default" : "sides", NULL, value);

  // Vendor attributes/options
  if ((job_attrs = ippFindAttribute(supported, "job-creation-attributes-supported", IPP_TAG_KEYWORD)) != NULL)
  {
    size_t	i,			// Looping var
		count;			// Count
    const char	*name;			// Attribute name
    char	defname[128],		// xxx-default name
		supname[128];		// xxx-supported name
    ipp_attribute_t *attr;		// Attribute

    for (i = 0, count = ippGetCount(job_attrs); i < count; i ++)
    {
      name = ippGetString(job_attrs, i, NULL);

      snprintf(defname, sizeof(defname), "%s-default", name);
      snprintf(supname, sizeof(supname), "%s-supported", name);

      if ((value = cupsGetOption(name, num_options, options)) == NULL)
        value = cupsGetOption(defname, num_options, options);

      if (!value)
        continue;

      if (!strcmp(name, "copies") || !strcmp(name, "finishings") || !strcmp(name, "media") || !strcmp(name, "multiple-document-handling") || !strcmp(name, "orientation-requested") || !strcmp(name, "output-bin") || !strcmp(name, "print-color-mode") || !strcmp(name, "print-content-optimize") || !strcmp(name, "print-darkness") || !strcmp(name, "print-quality") || !strcmp(name, "print-scaling") || !strcmp(name, "print-speed") || !strcmp(name, "printer-resolution") || !strcmp(name, "print-speed") || !strcmp(name, "sides"))
        continue;

      if ((attr = ippFindAttribute(supported, supname, IPP_TAG_ZERO)) != NULL)
      {
	switch (ippGetValueTag(attr))
	{
	  case IPP_TAG_BOOLEAN :
	      ippAddBoolean(request, group_tag, is_default ? defname : name, !strcmp(value, "true"));
	      break;

	  case IPP_TAG_INTEGER :
	  case IPP_TAG_RANGE :
	      intvalue = (int)strtol(value, &end, 10);
	      if (errno != ERANGE && !*end)
	        ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? defname : name, intvalue);
	      break;

	  case IPP_TAG_KEYWORD :
	      ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? defname : name, NULL, value);
	      break;

	  default :
	      break;
	}
      }
      else
      {
	ippAddString(request, group_tag, IPP_TAG_TEXT, is_default ? defname : name, NULL, value);
      }
    }
  }
}


// ===================================================================
// STUBS: Subcommand functions
//
// These are the POSIX CLI sub-commands (add, cancel, delete, modify,
// pause, resume, server, show-*, shutdown, submit).  They require
// fork/exec, UNIX domain sockets, and process management — none of
// which exist on Zephyr.  The equivalent functionality is provided
// by Zephyr Shell commands in mainloop.c.
// ===================================================================

int _papplMainloopAddPrinter(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopAutoAddPrinters(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopCancelJob(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopDeletePrinter(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopGetSetDefaultPrinter(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopModifyPrinter(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopPausePrinter(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopResumePrinter(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopRunServer(const char *base_name, const char *version, const char *footer_html, size_t num_drivers, pappl_pr_driver_t *drivers, pappl_pr_autoadd_cb_t autoadd_cb, pappl_pr_driver_cb_t driver_cb, size_t num_options, cups_option_t **options, pappl_ml_system_cb_t system_cb, void *data)
{
  (void)base_name; (void)version; (void)footer_html;
  (void)num_drivers; (void)drivers; (void)autoadd_cb; (void)driver_cb;
  (void)num_options; (void)options; (void)system_cb; (void)data;
  return (1);
}

int _papplMainloopShowDevices(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopShowDrivers(const char *base_name, size_t num_drivers, pappl_pr_driver_t *drivers, pappl_pr_autoadd_cb_t autoadd_cb, pappl_pr_driver_cb_t driver_cb, size_t num_options, cups_option_t *options, pappl_ml_system_cb_t system_cb, void *data)
{
  (void)base_name; (void)num_drivers; (void)drivers;
  (void)autoadd_cb; (void)driver_cb;
  (void)num_options; (void)options; (void)system_cb; (void)data;
  return (1);
}

int _papplMainloopShowJobs(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopShowOptions(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopShowPrinters(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopShowStatus(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopShutdownServer(const char *base_name, size_t num_options, cups_option_t *options)
{
  (void)base_name; (void)num_options; (void)options;
  return (1);
}

int _papplMainloopSubmitJob(const char *base_name, size_t num_options, cups_option_t *options, size_t num_files, char **files)
{
  (void)base_name; (void)num_options; (void)options;
  (void)num_files; (void)files;
  return (1);
}


// ===================================================================
// STUBS: Support functions (except _papplMainloopAddOptions above)
//
// These handle UNIX domain socket paths, process spawning, and
// IPP-over-HTTP connections to localhost — all POSIX-only concepts.
// ===================================================================

void
_papplMainloopAddPrinterURI(
    ipp_t      *request,		// I - IPP request
    const char *printer_name,		// I - Printer name
    char       *resource,		// I - Resource path buffer
    size_t     rsize)			// I - Size of buffer
{
  (void)request;
  (void)printer_name;

  if (resource && rsize > 0)
    resource[0] = '\0';
}

http_t *
_papplMainloopConnect(
    const char *base_name,		// I - Printer application name
    bool       auto_start)		// I - Auto-start server?
{
  (void)base_name;
  (void)auto_start;

  return (NULL);			// No UNIX domain sockets on Zephyr
}

http_t *
_papplMainloopConnectURI(
    const char *base_name,		// I - Base Name
    const char *printer_uri,		// I - Printer URI
    char       *resource,		// I - Resource path buffer
    size_t     rsize)			// I - Size of buffer
{
  (void)base_name;
  (void)printer_uri;

  if (resource && rsize > 0)
    resource[0] = '\0';

  return (NULL);
}

char *
_papplMainloopGetDefaultPrinter(
    http_t *http,			// I - HTTP connection
    char   *buffer,			// I - Buffer for printer name
    size_t bufsize)			// I - Size of buffer
{
  (void)http;

  if (buffer && bufsize > 0)
    buffer[0] = '\0';

  return (NULL);
}

char *
_papplMainloopGetServerPath(
    const char *base_name,		// I - Base name
    uid_t      uid,			// I - UID for server
    char       *buffer,			// I - Buffer for filename
    size_t     bufsize)			// I - Size of buffer
{
  (void)base_name;
  (void)uid;

  if (buffer && bufsize > 0)
    buffer[0] = '\0';

  return (buffer);
}

int
_papplMainloopGetServerPort(
    const char *base_name)		// I - Base name
{
  (void)base_name;

  return (0);
}


// ===================================================================
// Local helper: get_length()
//
// Used by _papplMainloopAddOptions() to parse length strings like
// "25.4mm", "1in", "2.5cm" into hundredths of millimeters.
// ===================================================================

static int				// O - Length value
get_length(const char *value)		// I - Length string
{
  double	n;			// Number
  char		*units;			// Pointer to units


  n = strtod(value, &units);

  if (units && !strcmp(units, "cm"))
    return ((int)(n * 1000.0));
  if (units && !strcmp(units, "in"))
    return ((int)(n * 2540.0));
  else if (units && !strcmp(units, "mm"))
    return ((int)(n * 100.0));
  else if (units && !strcmp(units, "m"))
    return ((int)(n * 100000.0));
  else
    return ((int)n);
}
