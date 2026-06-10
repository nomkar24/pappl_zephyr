//
// Command line utilities for the Printer Application Framework
//
// Copyright © 2020-2023 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//
// Zephyr/ESP32-S3 port:
//   - papplMainloop() is NOT supported (no argc/argv, no fork/exec on Zephyr).
//   - Startup is handled by main.c -> testpappl_main().
//   - Runtime commands are exposed via Zephyr Shell over USB-CDC.
//     Connect with:  screen /dev/ttyACM0 115200
//   - papplMainloopSetSystem() MUST be called after papplSystemCreate() to
//     register the active system pointer so shell commands can access it.
//
// Available shell commands (type in USB-CDC terminal):
//   pappl status     - Show system name and running state
//   pappl shutdown   - Gracefully shut down the PAPPL system
//   pappl printers   - List all registered printers
//   pappl jobs       - List all active/pending print jobs
//   pappl devices    - List available printer devices
//   pappl pause      - Pause a printer (usage: pappl pause <printer-name>)
//   pappl resume     - Resume a printer (usage: pappl resume <printer-name>)
//

#include "mainloop.h"
#include "printer.h"
#include "job.h"
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

//
// Globals...
//

// Global pointer to the running printer system.
// Set by papplMainloopSetSystem() right after papplSystemCreate().
static pappl_system_t *current_system = NULL;

//
// 'papplMainloopSetSystem()' - Register the active PAPPL system.
//
// Call this right after papplSystemCreate() in testpappl_main() so that
// all Zephyr shell commands have a reference to the live system.
//

void papplMainloopSetSystem(
    pappl_system_t *system) // I - Active system to register
{
  current_system = system;
}

//
// 'papplMainloopGetSystem()' - Get the registered PAPPL system pointer.
//
// Returns NULL if no system has been registered yet (system not started).
//

pappl_system_t *papplMainloopGetSystem(void) { return (current_system); }

//
// 'papplMainloop()' - Stub: NOT used on Zephyr.
//
// On Linux this function parses argc/argv and dispatches CLI sub-commands
// like "add", "server", "status", "shutdown", etc.
//
// On Zephyr, startup is handled directly in testpappl_main() which calls
// papplSystemCreate() and papplSystemRun() directly. This stub exists only
// for linker compatibility if any code path still references papplMainloop().
//

int                                       // O - Always returns 0 on Zephyr
papplMainloop(int argc,                   // I - Ignored on Zephyr
              char *argv[],               // I - Ignored on Zephyr
              const char *version,        // I - Ignored on Zephyr
              const char *footer_html,    // I - Ignored on Zephyr
              size_t num_drivers,         // I - Ignored on Zephyr
              pappl_pr_driver_t *drivers, // I - Ignored on Zephyr
              pappl_pr_autoadd_cb_t autoadd_cb, // I - Ignored on Zephyr
              pappl_pr_driver_cb_t driver_cb,   // I - Ignored on Zephyr
              const char *subcmd_name,          // I - Ignored on Zephyr
              pappl_ml_subcmd_cb_t subcmd_cb,   // I - Ignored on Zephyr
              pappl_ml_system_cb_t system_cb,   // I - Ignored on Zephyr
              pappl_ml_usage_cb_t usage_cb,     // I - Ignored on Zephyr
              void *data)                       // I - Ignored on Zephyr
{
  // papplMainloop() is not used on Zephyr.
  // Startup is handled by testpappl_main() -> papplSystemCreate() ->
  // papplMainloopSetSystem() -> papplSystemRun().
  (void)argc;
  (void)argv;
  (void)version;
  (void)footer_html;
  (void)num_drivers;
  (void)drivers;
  (void)autoadd_cb;
  (void)driver_cb;
  (void)subcmd_name;
  (void)subcmd_cb;
  (void)system_cb;
  (void)usage_cb;
  (void)data;

  return (0);
}

//
// 'papplMainloopShutdown()' - Gracefully shut down the running PAPPL system.
//
// On Linux this sends a signal to the server process.
// On Zephyr we call papplSystemShutdown() directly on the registered system.
//

void papplMainloopShutdown(void) {
  if (current_system)
    papplSystemShutdown(current_system);
}

static const char *papplPrinterGetStateString(pappl_printer_t *printer) {
  switch (papplPrinterGetState(printer)) {
  case IPP_PSTATE_IDLE:
    return "idle";
  case IPP_PSTATE_PROCESSING:
    return "processing";
  case IPP_PSTATE_STOPPED:
    return "stopped";
  default:
    return "unknown";
  }
}

static const char *papplJobGetStateString(pappl_job_t *job) {
  switch (papplJobGetState(job)) {
  case IPP_JSTATE_PENDING:
    return "pending";
  case IPP_JSTATE_HELD:
    return "held";
  case IPP_JSTATE_PROCESSING:
    return "processing";
  case IPP_JSTATE_STOPPED:
    return "stopped";
  case IPP_JSTATE_CANCELED:
    return "canceled";
  case IPP_JSTATE_ABORTED:
    return "aborted";
  case IPP_JSTATE_COMPLETED:
    return "completed";
  default:
    return "unknown";
  }
}

static int cmd_pappl_status(const struct shell *sh, size_t argc, char **argv) {
  (void)argc;
  (void)argv;

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  char name[128];

  shell_print(sh, "=== PAPPL System Status ===");
  shell_print(sh, "System Name : %s",
              papplSystemGetName(current_system, name, sizeof(name)));
  shell_print(sh, "Is Running  : %s",
              papplSystemIsRunning(current_system) ? "Yes" : "No");
  shell_print(sh, "Server Port : %d", papplSystemGetHostPort(current_system));

  return (0);
}

//
// 'cmd_pappl_shutdown()' - Shut down the PAPPL system via shell.
//
// Shell usage:  pappl shutdown
//

static int cmd_pappl_shutdown(const struct shell *sh, size_t argc,
                              char **argv) {
  (void)argc;
  (void)argv;

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  shell_print(sh, "Shutting down PAPPL system...");
  papplSystemShutdown(current_system);

  return (0);
}

//
// 'cmd_pappl_printers()' - List all registered printers.
//
// Shell usage:  pappl printers
//

// Callback used by papplSystemIteratePrinters to print each printer
static void shell_list_printer_cb(pappl_printer_t *printer, void *data) {
  const struct shell *sh = (const struct shell *)data;

  shell_print(sh, "  [%d] %s  (state: %s)", papplPrinterGetID(printer),
              papplPrinterGetName(printer),
              papplPrinterGetStateString(printer));
}

static int cmd_pappl_printers(const struct shell *sh, size_t argc,
                              char **argv) {
  (void)argc;
  (void)argv;

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  shell_print(sh, "=== Registered Printers ===");
  papplSystemIteratePrinters(current_system, shell_list_printer_cb, (void *)sh);

  return (0);
}

//
// 'cmd_pappl_jobs()' - List all active or pending print jobs.
//
// Shell usage:  pappl jobs
//

// Callback used by papplPrinterIterateAllJobs to print each job
static void shell_list_job_cb(pappl_job_t *job, void *data) {
  const struct shell *sh = (const struct shell *)data;

  shell_print(sh, "  Job #%d: \"%s\"  (state: %s)", papplJobGetID(job),
              papplJobGetName(job), papplJobGetStateString(job));
}

// Printer iterator to iterate jobs across all printers
static void shell_jobs_per_printer_cb(pappl_printer_t *printer, void *data) {
  const struct shell *sh = (const struct shell *)data;

  shell_print(sh, "Printer: %s", papplPrinterGetName(printer));
  papplPrinterIterateAllJobs(printer, shell_list_job_cb, (void *)sh, 1, 0);
}

static int cmd_pappl_jobs(const struct shell *sh, size_t argc, char **argv) {
  (void)argc;
  (void)argv;

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  shell_print(sh, "=== Active/Pending Jobs ===");
  papplSystemIteratePrinters(current_system, shell_jobs_per_printer_cb,
                             (void *)sh);

  return (0);
}

//
// 'cmd_pappl_devices()' - List available printer devices.
//
// Shell usage:  pappl devices
//

// Device list callback — called for each discovered device
static bool shell_device_list_cb(const char *device_info,
                                 const char *device_uri, const char *device_id,
                                 void *data) {
  const struct shell *sh = (const struct shell *)data;

  shell_print(sh, "  Info : %s", device_info ? device_info : "(none)");
  shell_print(sh, "  URI  : %s", device_uri ? device_uri : "(none)");
  shell_print(sh, "  ID   : %s", device_id ? device_id : "(none)");
  shell_print(sh, "  ---");

  return (true); // continue listing
}

// Device error callback
static void shell_device_error_cb(void *data, const char *message) {
  const struct shell *sh = (const struct shell *)data;
  shell_error(sh, "Device error: %s", message);
}

static int cmd_pappl_devices(const struct shell *sh, size_t argc, char **argv) {
  (void)argc;
  (void)argv;

  shell_print(sh, "=== Available Devices ===");
  shell_print(sh, "(Scanning for network/USB devices...)");

  papplDeviceList(PAPPL_DEVTYPE_ALL, shell_device_list_cb, (void *)sh,
                  shell_device_error_cb, (void *)sh);

  return (0);
}

//
// Helper structure to find a printer by its friendly name
struct find_printer_data {
  const char *name;
  pappl_printer_t *printer;
};

// Callback to compare printer names
static void find_printer_cb(pappl_printer_t *printer, void *data) {
  struct find_printer_data *find_data = (struct find_printer_data *)data;
  if (strcmp(papplPrinterGetName(printer), find_data->name) == 0) {
    find_data->printer = printer;
  }
}

// Find a printer by its name inside the system
static pappl_printer_t *find_printer_by_name(pappl_system_t *system, const char *name) {
  struct find_printer_data find_data = {name, NULL};
  papplSystemIteratePrinters(system, find_printer_cb, &find_data);
  return (find_data.printer);
}

//
// 'cmd_pappl_pause()' - Pause a named printer.
//
// Shell usage:  pappl pause <printer-name>
//

static int cmd_pappl_pause(const struct shell *sh, size_t argc, char **argv) {
  pappl_printer_t *printer;

  if (argc < 2) {
    shell_error(sh, "Usage: pappl pause <printer-name>");
    return (-EINVAL);
  }

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  printer = find_printer_by_name(current_system, argv[1]);
  if (!printer) {
    shell_error(sh, "Printer '%s' not found.", argv[1]);
    return (-ENOENT);
  }

  papplPrinterPause(printer);
  shell_print(sh, "Printer '%s' paused.", argv[1]);

  return (0);
}

//
// 'cmd_pappl_resume()' - Resume a named printer.
//
// Shell usage:  pappl resume <printer-name>
//

static int cmd_pappl_resume(const struct shell *sh, size_t argc, char **argv) {
  pappl_printer_t *printer;

  if (argc < 2) {
    shell_error(sh, "Usage: pappl resume <printer-name>");
    return (-EINVAL);
  }

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  printer = find_printer_by_name(current_system, argv[1]);
  if (!printer) {
    shell_error(sh, "Printer '%s' not found.", argv[1]);
    return (-ENOENT);
  }

  papplPrinterResume(printer);
  shell_print(sh, "Printer '%s' resumed.", argv[1]);

  return (0);
}

//
// 'cmd_pappl_set_device()' - Set the device URI of a named printer.
//
// Shell usage:  pappl setdevice <printer-name> <device-uri>
//

static int cmd_pappl_set_device(const struct shell *sh, size_t argc, char **argv) {
  pappl_printer_t *printer;

  if (argc < 3) {
    shell_error(sh, "Usage: pappl setdevice <printer-name> <device-uri>");
    return (-EINVAL);
  }

  if (!current_system) {
    shell_error(sh, "PAPPL system is not running.");
    return (-ENOENT);
  }

  printer = find_printer_by_name(current_system, argv[1]);
  if (!printer) {
    shell_error(sh, "Printer '%s' not found.", argv[1]);
    return (-ENOENT);
  }

  papplPrinterSetDeviceURI(printer, argv[2]);
  shell_print(sh, "Printer '%s' device URI set to '%s'.", argv[1], argv[2]);

  return (0);
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    pappl_cmds,
    SHELL_CMD(status, NULL, "Show PAPPL system name and running state",
              cmd_pappl_status),
    SHELL_CMD(shutdown, NULL, "Gracefully shut down the PAPPL system",
              cmd_pappl_shutdown),
    SHELL_CMD(printers, NULL, "List all registered printers",
              cmd_pappl_printers),
    SHELL_CMD(jobs, NULL, "List active/pending print jobs", cmd_pappl_jobs),
    SHELL_CMD(devices, NULL, "List available printer devices",
              cmd_pappl_devices),
    SHELL_CMD(pause, NULL, "Pause a printer: pappl pause <name>",
              cmd_pappl_pause),
    SHELL_CMD(resume, NULL, "Resume a printer: pappl resume <name>",
              cmd_pappl_resume),
    SHELL_CMD(setdevice, NULL, "Set printer device URI: pappl setdevice <name> <uri>",
              cmd_pappl_set_device),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(pappl, &pappl_cmds, "PAPPL printer system commands", NULL);
