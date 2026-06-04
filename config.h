//
// Configuration header file for the Printer Application Framework
//

#ifndef _PAPPL_CONFIG_H_
#define _PAPPL_CONFIG_H_

#include <stddef.h>
#include <stdio.h>
#include <cups/cups.h>

// Version numbers (matching PAPPL 2.0)
#define PAPPL_VERSION "2.0.0"
#define PAPPL_VERSION_MAJOR 2
#define PAPPL_VERSION_MINOR 0

// Location of PAPPL state and spool data
// Mapping to Zephyr's VFS mount point (LittleFS usually mounted at /lfs)
#define PAPPL_STATEDIR		"/lfs/var/pappl"
#define PAPPL_SOCKDIR		"/lfs/run"

// Zephyr POSIX features
#define HAVE_PTHREAD_H 1
#define HAVE_GETADDRINFO 1
#define HAVE_POLL 1
#define HAVE_STRDUP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRLCPY 1
#define HAVE_STRLCAT 1

// Zephyr supports statvfs via <zephyr/fs/fs.h>
#define HAVE_STATVFS 1
#define HAVE_SYS_STATVFS_H 1

// Security/TLS (Using MbedTLS)
#define HAVE_MBEDTLS 1

// Resource limits for ESP32 (Avoid memory overflow)
#define PAPPL_MAX_PRINTERS 1
#define PAPPL_MAX_JOBS 5

// Libraries
#undef HAVE_LIBJPEG
#undef HAVE_LIBPNG
#undef HAVE_LIBUSB
#undef HAVE_LIBPAM
#undef HAVE_PDFIO

// POSIX/CUPS Shims for Zephyr
#define realpath(x, y) (y)

static inline FILE *pappl_stub_popen(const char *c, const char *t) { return NULL; }
#define popen pappl_stub_popen

static inline int pappl_stub_pclose(FILE *f) { return 0; }
#define pclose pappl_stub_pclose

static inline size_t pappl_stub_cupsAddDest(const char *name, const char *instance, size_t num_dests, cups_dest_t **dests) { return 0; }
#define cupsAddDest pappl_stub_cupsAddDest

static inline cups_dest_t *pappl_stub_cupsGetDest(const char *name, const char *instance, size_t num_dests, cups_dest_t *dests) { return NULL; }
#define cupsGetDest pappl_stub_cupsGetDest

// Missing mkstemp on some Zephyr versions
#ifndef HAVE_MKSTEMP
static inline int pappl_stub_mkstemp(char *template) { return -1; }
#  define mkstemp pappl_stub_mkstemp
#endif

#endif // _PAPPL_CONFIG_H_
