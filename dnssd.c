//
// DNS-SD support stub for the Printer Application Framework (Zephyr Port)
//
// Copyright © 2019-2026 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// On Zephyr RTOS, the cups_dnssd_* API (which wraps Avahi/mDNSResponder on
// Linux/macOS) is not available. Instead, mDNS advertisement is handled
// natively by the Zephyr DNS-SD subsystem (CONFIG_MDNS_RESPONDER=y) directly
// in the application's main.c.
//
// These stubs allow PAPPL to build and run without crashing. The actual mDNS
// advertisement of the printer service (_ipp._tcp on port 631) must be done
// in the application layer using Zephyr's native dns_sd_register_tcp_service().
//

#include "pappl-private.h"


//
// '_papplPrinterRegisterDNSSDNoLock()' - Register a printer's DNS-SD service (stub).
//
// On Zephyr, mDNS registration is handled by the application layer using
// Zephyr's CONFIG_MDNS_RESPONDER subsystem. This stub prevents PAPPL from
// crashing when it tries to register DNS-SD services.
//

bool
_papplPrinterRegisterDNSSDNoLock(
    pappl_printer_t *printer)		// I - Printer
{
  (void)printer;

  // mDNS registration is handled by the application via Zephyr's
  // dns_sd_register_tcp_service() in main.c.
  // See: zephyr/include/zephyr/net/dns_sd.h
  return (true);
}


//
// '_papplPrinterUnregisterDNSSDNoLock()' - Unregister a printer's DNS-SD service (stub).
//

void
_papplPrinterUnregisterDNSSDNoLock(
    pappl_printer_t *printer)		// I - Printer
{
  (void)printer;
}


//
// '_papplSystemRegisterDNSSDNoLock()' - Register a system's DNS-SD service (stub).
//

bool
_papplSystemRegisterDNSSDNoLock(
    pappl_system_t *system)		// I - System
{
  (void)system;

  return (true);
}


//
// '_papplSystemUnregisterDNSSDNoLock()' - Unregister a system's DNS-SD service (stub).
//

void
_papplSystemUnregisterDNSSDNoLock(
    pappl_system_t *system)		// I - System
{
  (void)system;
}
