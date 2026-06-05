//
// Stubs for Infrastructure Proxy and Raw Printing features
// To exclude printer-proxy.c and printer-raw.c from the Zephyr build cleanly.
// 

#include "pappl-private.h"

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
