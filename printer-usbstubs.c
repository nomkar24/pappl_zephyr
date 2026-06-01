//
// USB Printer Stubs for the Printer Application Framework
//
// Used to exclude printer-usb.c from the Zephyr build cleanly.
//

#include "pappl-private.h"

void *
_papplPrinterRunUSB(pappl_printer_t *printer)
{
  // Set usb_active to true to prevent system.c from hanging on startup
  _papplRWLockWrite(printer);
  printer->usb_active = true;
  _papplRWUnlock(printer);

  while (!papplPrinterIsDeleted(printer) && !_papplSystemIsShutdownNoLock(printer->system))
  {
    // Sleep to mimic a running thread with low CPU usage
    usleep(1000000); 
  }

  _papplRWLockWrite(printer);
  printer->usb_active = false;
  _papplRWUnlock(printer);

  return (NULL);
}
