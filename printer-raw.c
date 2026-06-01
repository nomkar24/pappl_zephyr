//
// Stubs for Infrastructure Proxy and Raw Printing features
// To exclude printer-proxy.c and printer-raw.c from the Zephyr build cleanly.
// 

#include "pappl-private.h"

// 
// Infrastructure Proxy Stubs (printer-proxy.c)
// 

void *
_papplPrinterRunProxy(pappl_printer_t *printer)
{
  (void)printer;
  return (NULL);
}

void
_papplPrinterUpdateProxy(
    pappl_printer_t *printer,
    http_t          *http,
    const char      *resource)
{
  (void)printer;
  (void)http;
  (void)resource;
}

void
_papplPrinterUpdateProxyDocument(
    pappl_printer_t *printer,
    pappl_job_t     *job,
    int             doc_number)
{
  (void)printer;
  (void)job;
  (void)doc_number;
}

void
_papplPrinterUpdateProxyJobNoLock(
    pappl_printer_t *printer,
    pappl_job_t     *job)
{
  (void)printer;
  (void)job;
}

http_t *
_papplPrinterConnectProxyNoLock(
    pappl_printer_t *printer,
    char            *resource,
    size_t          ressize)
{
  (void)printer;
  (void)resource;
  (void)ressize;
  return (NULL);
}

//
// Raw Socket Printer Stubs (printer-raw.c)
// 

bool
_papplPrinterAddRawListeners(pappl_printer_t *printer)
{
  (void)printer;
  return (false);
}

void *
_papplPrinterRunRaw(pappl_printer_t *printer)
{
  (void)printer;
  return (NULL);
}
