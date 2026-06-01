//
// Private mainloop header file for the Printer Application Framework
//
// Copyright © 2020-2023 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Zephyr/ESP32-S3 port:
//   - The original mainloop-private.h declares internal functions that lived
//     in mainloop.c, mainloop-subcommands.c, and mainloop-support.c.
//   - Those three POSIX-dependent files are excluded from the Zephyr build.
//   - This ported header retains all function declarations so that any .c file
//     including pappl-private.h still compiles cleanly.
//   - _papplMainloopAddOptions() is the ONLY function actually called from
//     outside the excluded files (used by job.c -> papplJobCreateWithFile).
//     It is fully implemented in mainloop-stubs.c because it is pure IPP
//     attribute logic with no POSIX dependencies.
//   - All other functions are no-op stubs returning error/NULL.
//

#ifndef _PAPPL_MAINLOOP_PRIVATE_H_
#  define _PAPPL_MAINLOOP_PRIVATE_H_
#  include "mainloop.h"
#  include "base-private.h"
#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus


//
// Globals...
//

extern char *_papplMainloopPath _PAPPL_PRIVATE;


//
// Subcommand functions (all stubbed on Zephyr — POSIX CLI not available)
//

extern int	_papplMainloopAddPrinter(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopAutoAddPrinters(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopCancelJob(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopDeletePrinter(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopGetSetDefaultPrinter(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopModifyPrinter(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopPausePrinter(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopResumePrinter(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopRunServer(const char *base_name, const char *version, const char *footer_html, size_t num_drivers, pappl_pr_driver_t *drivers, pappl_pr_autoadd_cb_t autoadd_cb, pappl_pr_driver_cb_t driver_cb, size_t num_options, cups_option_t **options, pappl_ml_system_cb_t system_cb, void *data) _PAPPL_PRIVATE;
extern int	_papplMainloopShowDevices(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopShowDrivers(const char *base_name, size_t num_drivers, pappl_pr_driver_t *drivers, pappl_pr_autoadd_cb_t autoadd_cb, pappl_pr_driver_cb_t driver_cb, size_t num_options, cups_option_t *options, pappl_ml_system_cb_t system_cb, void *data) _PAPPL_PRIVATE;
extern int	_papplMainloopShowJobs(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopShowOptions(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopShowPrinters(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopShowStatus(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopShutdownServer(const char *base_name, size_t num_options, cups_option_t *options) _PAPPL_PRIVATE;
extern int	_papplMainloopSubmitJob(const char *base_name, size_t num_options, cups_option_t *options, size_t num_files, char **files) _PAPPL_PRIVATE;


//
// Support functions
// _papplMainloopAddOptions() is FULLY IMPLEMENTED (used by job.c).
// All others are stubbed.
//

extern void	_papplMainloopAddOptions(ipp_t *request, size_t num_options, cups_option_t *options, ipp_t *supported) _PAPPL_PRIVATE;
extern void	_papplMainloopAddPrinterURI(ipp_t *request, const char *printer_name, char *resource,size_t rsize) _PAPPL_PRIVATE;
extern http_t	*_papplMainloopConnect(const char *base_name, bool auto_start) _PAPPL_PRIVATE;
extern http_t	*_papplMainloopConnectURI(const char *base_name, const char *printer_uri, char  *resource, size_t rsize) _PAPPL_PRIVATE;
extern char	*_papplMainloopGetDefaultPrinter(http_t *http, char *buffer, size_t bufsize) _PAPPL_PRIVATE;
extern char	*_papplMainloopGetServerPath(const char *base_name, uid_t uid, char *buffer, size_t bufsize) _PAPPL_PRIVATE;
extern int	_papplMainloopGetServerPort(const char *base_name) _PAPPL_PRIVATE;


#  ifdef __cplusplus
}
#  endif // __cplusplus
#  endif // !_PAPPL_MAINLOOP_PRIVATE_H_
