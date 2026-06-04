//
// Network device support code for the Printer Application Framework (Zephyr Port)
//
// Copyright © 2019-2026 by Michael R Sweet.
// Copyright © 2007-2019 by Apple Inc.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "device-private.h"
#include "printer-private.h"
#include <cups/transcode.h>

#define _PAPPL_MAX_SNMP_SUPPLY	32

typedef struct _pappl_socket_s		// Socket device data
{
  int			fd;			// File descriptor connection to device
  char			*host;			// Hostname
  int			port;			// Port number
  http_addrlist_t	*list,			// Address list
			*addr;			// Connected address
  int			snmp_fd,		// SNMP socket
			charset;		// Character set
  size_t		num_supplies;		// Number of supplies
  pappl_supply_t	supplies[_PAPPL_MAX_SNMP_SUPPLY];
						// Supplies
  int			colorants[_PAPPL_MAX_SNMP_SUPPLY],
						// Colorant indices
			levels[_PAPPL_MAX_SNMP_SUPPLY],
						// Current level
			max_capacities[_PAPPL_MAX_SNMP_SUPPLY],
						// Max capacity
			units[_PAPPL_MAX_SNMP_SUPPLY];
						// Supply units
} _pappl_socket_t;

//
// Local functions...
//

static void		pappl_socket_close(pappl_device_t *device);
static char		*pappl_socket_getid(pappl_device_t *device, char *buffer, size_t bufsize);
static bool		pappl_socket_open(pappl_device_t *device, const char *device_uri, pappl_job_t *job);
static ssize_t		pappl_socket_read(pappl_device_t *device, void *buffer, size_t bytes);
static pappl_preason_t	pappl_socket_status(pappl_device_t *device);
static size_t		pappl_socket_supplies(pappl_device_t *device, size_t max_supplies, pappl_supply_t *supplies);
static ssize_t		pappl_socket_write(pappl_device_t *device, const void *buffer, size_t bytes);


//
// '_papplDeviceAddNetworkSchemesNoLock()' - Add all of the supported network schemes.
//

void
_papplDeviceAddNetworkSchemesNoLock(void)
{
  _papplDeviceAddSchemeNoLock("socket", PAPPL_DEVTYPE_SOCKET, NULL, pappl_socket_open, pappl_socket_close, pappl_socket_read, pappl_socket_write, pappl_socket_status, pappl_socket_supplies, pappl_socket_getid);
}


//
// 'pappl_socket_close()' - Close a network socket.
//

static void
pappl_socket_close(pappl_device_t *device)	// I - Device
{
  _pappl_socket_t	*sock = (_pappl_socket_t *)papplDeviceGetData(device);
					// Socket device


  if (sock)
  {
    if (sock->fd >= 0)
      close(sock->fd);

    free(sock->host);
    httpAddrFreeList(sock->list);
    free(sock);

    papplDeviceSetData(device, NULL);
  }
}


//
// 'pappl_socket_getid()' - Get the current IEEE-1284 device ID.
//

static char *				// O - Device ID or `NULL`
pappl_socket_getid(
    pappl_device_t *device,		// I - Device
    char           *buffer,		// I - Buffer
    size_t         bufsize)		// I - Size of buffer
{
  (void)device;

  if (buffer && bufsize > 0)
    buffer[0] = '\0';

  return (NULL);
}


//
// 'pappl_socket_open()' - Open a network socket.
//

static bool				// O - `true` on success, `false` on failure
pappl_socket_open(
    pappl_device_t *device,		// I - Device
    const char     *device_uri,		// I - Device URI
    pappl_job_t    *job)		// I - Job or `NULL` if none
{
  _pappl_socket_t	*sock;		// Socket device
  char			scheme[32],	// URI scheme
			userpass[32],	// Username/password (not used)
			host[256],	// Host name or make
			resource[256],	// Resource path, if any
			*options;	// Pointer to options, if any
  int			port;		// Port number
  char			port_str[32];	// String for port number


  (void)job;

  // Allocate memory for the socket...
  if ((sock = (_pappl_socket_t *)calloc(1, sizeof(_pappl_socket_t))) == NULL)
  {
    papplDeviceError(device, "Unable to allocate memory for socket device: %s", strerror(errno));
    return (false);
  }

  sock->fd = -1;
  sock->snmp_fd = -1;
  sock->charset = -1;

  // Split apart the URI...
  httpSeparateURI(HTTP_URI_CODING_ALL, device_uri, scheme, sizeof(scheme), userpass, sizeof(userpass), host, sizeof(host), &port, resource, sizeof(resource));

  if ((options = strchr(resource, '?')) != NULL)
    *options++ = '\0';

  if (strcmp(scheme, "socket") != 0)
  {
    papplDeviceError(device, "Unsupported scheme '%s' for socket backend on Zephyr.", scheme);
    free(sock);
    return (false);
  }

  sock->host = strdup(host);
  sock->port = port;

  // Lookup the address of the printer...
  snprintf(port_str, sizeof(port_str), "%d", sock->port);
  if ((sock->list = httpAddrGetList(sock->host, AF_UNSPEC, port_str)) == NULL)
  {
    papplDeviceError(device, "Unable to lookup '%s:%d': %s", sock->host, sock->port, cupsGetErrorString());
    free(sock->host);
    free(sock);
    return (false);
  }

  sock->fd   = -1;
  sock->addr = httpAddrConnect(sock->list, &sock->fd, 30000, NULL);

  if (sock->fd < 0)
  {
    papplDeviceError(device, "Unable to connect to '%s:%d': %s", sock->host, sock->port, cupsGetErrorString());
    free(sock->host);
    httpAddrFreeList(sock->list);
    free(sock);
    return (false);
  }

  sock->snmp_fd = -1; // SNMP is bypassed/disabled on Zephyr socket connections

  papplDeviceSetData(device, sock);

  return (true);
}


//
// 'pappl_socket_read()' - Read from a network socket.
//

static ssize_t				// O - Bytes read
pappl_socket_read(
    pappl_device_t *device,		// I - Device
    void           *buffer,		// I - Read buffer
    size_t         bytes)		// I - Bytes to read
{
  _pappl_socket_t	*sock = (_pappl_socket_t *)papplDeviceGetData(device);
  ssize_t		bytes_read;


  if (!sock || sock->fd < 0)
    return (-1);

  while ((bytes_read = recv(sock->fd, buffer, bytes, 0)) < 0)
  {
    if (errno != EINTR && errno != EAGAIN)
      return (-1);
  }

  return (bytes_read);
}


//
// 'pappl_socket_status()' - Get the current network device status.
//

static pappl_preason_t			// O - New "printer-state-reasons" values
pappl_socket_status(
    pappl_device_t *device)		// I - Device
{
  (void)device;
  return (PAPPL_PREASON_NONE);
}


//
// 'pappl_socket_supplies()' - Query supply levels.
//

static size_t				// O - Number of supplies
pappl_socket_supplies(
    pappl_device_t *device,		// I - Device
    size_t         max_supplies,	// I - Maximum number of supplies to return
    pappl_supply_t *supplies)		// O - Supplies
{
  (void)device;
  (void)max_supplies;
  (void)supplies;
  return (0);
}


//
// 'pappl_socket_write()' - Write to a network socket.
//

static ssize_t				// O - Bytes written
pappl_socket_write(
    pappl_device_t *device,		// I - Device
    const void     *buffer,		// I - Write buffer
    size_t         bytes)		// I - Bytes to write
{
  _pappl_socket_t	*sock = (_pappl_socket_t *)papplDeviceGetData(device);
  const char	*ptr;			// Pointer into buffer
  ssize_t	count,			// Total bytes written
		written;		// Bytes written this time


  if (!sock || sock->fd < 0)
    return (-1);

  for (count = 0, ptr = (const char *)buffer; count < (ssize_t)bytes; count += written, ptr += written)
  {
    if ((written = send(sock->fd, ptr, bytes - (size_t)count, 0)) < 0)
    {
      if (errno == EINTR || errno == EAGAIN)
      {
        written = 0;
	continue;
      }

      return (-1);
    }
  }

  return (count);
}
