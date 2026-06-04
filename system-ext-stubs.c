//
// Zephyr RTOS stubs for external command support
//
// Copyright © 2022-2026 by Michael R Sweet.
//
// Licensed under Apache License v2.0. See the file "LICENSE" for more
// information.
//

#include "pappl-private.h"

//
// 'papplSystemAddExtCommandPath()' - Stub
//
void
papplSystemAddExtCommandPath(
    pappl_system_t *system,
    const char     *path)
{
  (void)system;
  (void)path;
}

//
// 'papplSystemAddExtReadOnlyPath()' - Stub
//
void
papplSystemAddExtReadOnlyPath(
    pappl_system_t *system,
    const char     *path)
{
  (void)system;
  (void)path;
}

//
// 'papplSystemAddExtReadWritePath()' - Stub
//
void
papplSystemAddExtReadWritePath(
    pappl_system_t *system,
    const char     *path)
{
  (void)system;
  (void)path;
}

//
// 'papplSystemRunExtCommand()' - Stub (always returns 0 / unsupported)
//
int
papplSystemRunExtCommand(
    pappl_system_t  *system,
    pappl_printer_t *printer,
    pappl_job_t     *job,
    const char      **args,
    const char      **env,
    int             infd,
    int             outfd,
    bool            allow_networking)
{
  (void)system;
  (void)printer;
  (void)job;
  (void)args;
  (void)env;
  (void)infd;
  (void)outfd;
  (void)allow_networking;

  return (0);
}

//
// 'papplSystemSetExtUserGroup()' - Stub
//
void
papplSystemSetExtUserGroup(
    pappl_system_t *system,
    const char     *username,
    const char     *groupname)
{
  (void)system;
  (void)username;
  (void)groupname;
}

//
// 'papplSystemStopExtCommand()' - Stub
//
void
papplSystemStopExtCommand(
    pappl_system_t *system,
    int            number)
{
  (void)system;
  (void)number;
}

//
// '_papplSystemStopAllExtCommands()' - Stub
//
void
_papplSystemStopAllExtCommands(
    pappl_system_t *system)
{
  (void)system;
}
