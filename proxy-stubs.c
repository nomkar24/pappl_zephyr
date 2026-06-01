//
//To exclude printer-proxy.c cleanly without breaking the build 
//empty stubs versions of proxy functions are defined 
//now it will build cleanly without linker errors
//

#include "pappl-private.h"

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
