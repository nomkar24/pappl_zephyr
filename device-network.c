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
#include <zephyr/net/net_if.h>

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


// Helper to skip a DNS name in a packet
static const uint8_t *
skip_name(const uint8_t *reader, const uint8_t *packet_end)
{
  while (reader < packet_end)
  {
    uint8_t len = *reader;
    if (len == 0)
    {
      return (reader + 1);
    }
    if ((len & 0xC0) == 0xC0)
    {
      if (reader + 2 > packet_end)
        return (NULL);
      return (reader + 2);
    }
    reader += 1 + len;
  }
  return (NULL);
}

// Helper to get the first label of a DNS name (for instance name)
static const uint8_t *
get_first_label_rec(
    const uint8_t *reader,
    const uint8_t *packet_start,
    const uint8_t *packet_end,
    char          *buf,
    size_t        bufsize,
    int           depth)
{
  if (depth > 5)
    return (NULL);
  if (reader >= packet_end)
    return (NULL);

  uint8_t len = *reader;
  if ((len & 0xC0) == 0xC0)
  {
    if (reader + 2 > packet_end)
      return (NULL);
    uint16_t offset = ((len & 0x3F) << 8) | reader[1];
    const uint8_t *ptr = packet_start + offset;
    if (ptr >= packet_end)
      return (NULL);
    get_first_label_rec(ptr, packet_start, packet_end, buf, bufsize, depth + 1);
    return (reader + 2);
  }
  else if (len > 0)
  {
    if (reader + 1 + len > packet_end)
      return (NULL);
    size_t copy_len = len < (bufsize - 1) ? len : (bufsize - 1);
    memcpy(buf, reader + 1, copy_len);
    buf[copy_len] = '\0';
    return (skip_name(reader, packet_end));
  }
  return (NULL);
}

static const uint8_t *
get_first_label(
    const uint8_t *reader,
    const uint8_t *packet_start,
    const uint8_t *packet_end,
    char          *buf,
    size_t        bufsize)
{
  return get_first_label_rec(reader, packet_start, packet_end, buf, bufsize, 0);
}

// Helper to parse incoming mDNS response packet. Returns true if callback says to stop.
static bool
parse_dns_response(
    const uint8_t            *packet,
    size_t                   packet_len,
    const struct sockaddr_in *sender_addr,
    pappl_device_cb_t        cb,
    void                     *data)
{
  if (packet_len < 12)
    return (false);

  uint16_t flags = (packet[2] << 8) | packet[3];
  if (!(flags & 0x8000))
    return (false); // Not a response

  uint16_t q_count = (packet[4] << 8) | packet[5];
  uint16_t ans_count = (packet[6] << 8) | packet[7];
  uint16_t aut_count = (packet[8] << 8) | packet[9];
  uint16_t add_count = (packet[10] << 8) | packet[11];

  printk("parse_dns_response: flags=0x%04x, Q=%d, ANS=%d, AUTH=%d, ADD=%d\n", flags, q_count, ans_count, aut_count, add_count);

  const uint8_t *reader = packet + 12;
  const uint8_t *packet_end = packet + packet_len;

  // Skip Question section
  for (int i = 0; i < q_count; i++)
  {
    reader = skip_name(reader, packet_end);
    if (!reader || reader + 4 > packet_end)
      return (false);
    reader += 4; // Skip QTYPE and QCLASS
  }

  // Parse Answer, Authority, and Additional sections
  int total_records = ans_count + aut_count + add_count;
  char printer_name[64] = "";
  char printer_uri[128] = "";
  bool found_ptr = false;
  uint16_t port = 9100;
  bool is_ipp = false;

  for (int i = 0; i < total_records; i++)
  {
    const uint8_t *record_name_ptr = reader;
    reader = skip_name(reader, packet_end);
    if (!reader || reader + 10 > packet_end)
      break;

    uint16_t type = (reader[0] << 8) | reader[1];
    uint16_t rdlength = (reader[8] << 8) | reader[9];
    const uint8_t *rdata = reader + 10;

    if (rdata + rdlength > packet_end)
      break;

    reader = rdata + rdlength;

    if (type == 12) // PTR
    {
      get_first_label(rdata, packet, packet_end, printer_name, sizeof(printer_name));

      // Match service type from record name
      const uint8_t *temp = record_name_ptr;
      int hops = 0;
      while (temp < packet_end && hops < 10)
      {
        uint8_t len = *temp;
        if (len == 0)
          break;
        if ((len & 0xC0) == 0xC0)
        {
          if (temp + 2 > packet_end)
            break;
          uint16_t offset = ((len & 0x3F) << 8) | temp[1];
          temp = packet + offset;
          hops++;
          continue;
        }
        if (len == 15 && memcmp(temp + 1, "_pdl-datastream", 15) == 0)
        {
          printk("parse_dns_response: found _pdl-datastream record, name='%s'\n", printer_name);
          is_ipp = false;
          found_ptr = true;
          break;
        }
        if (len == 4 && memcmp(temp + 1, "_ipp", 4) == 0)
        {
          printk("parse_dns_response: found _ipp record, name='%s'\n", printer_name);
          is_ipp = true;
          port = 631;
          found_ptr = true;
          break;
        }
        temp += 1 + len;
      }
    }
    else if (type == 33) // SRV
    {
      if (rdlength >= 6)
      {
        port = (rdata[4] << 8) | rdata[5];
        printk("parse_dns_response: found SRV record, target port=%d\n", port);
      }
    }
  }

  if (found_ptr && printer_name[0] != '\0')
  {
    char ip_str[32];
    inet_ntop(AF_INET, &sender_addr->sin_addr, ip_str, sizeof(ip_str));
    if (is_ipp)
      snprintf(printer_uri, sizeof(printer_uri), "ipp://%s:%d/ipp/print", ip_str, port);
    else
      snprintf(printer_uri, sizeof(printer_uri), "socket://%s:%d", ip_str, port);

    printk("parse_dns_response: reporting discovered printer '%s' at URI '%s'\n", printer_name, printer_uri);

    // cb returns false to continue, true to stop (as per std callback rules)
    if (!(*cb)(printer_name, printer_uri, NULL, data))
    {
      printk("parse_dns_response: callback returned false (requesting stop)\n");
      return (true); // Stop requested
    }
  }

  return (false);
}

// Callback for socket scheme dynamic listing via mDNS
static bool
pappl_socket_list(
    pappl_devtype_t     types,
    pappl_device_cb_t   cb,
    void                *data,
    pappl_deverror_cb_t err_cb,
    void                *err_data)
{
  (void)types;
  (void)err_cb;
  (void)err_data;

  printk("pappl_socket_list: starting mDNS discovery scan...\n");

  uint8_t *buffer = malloc(1024);
  if (!buffer)
  {
    printk("pappl_socket_list: failed to allocate 1024 bytes buffer\n");
    return (false);
  }

  // Prepare standard mDNS unicast-response (QU bit set) query packets
  static const uint8_t socket_query[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    15, '_', 'p', 'd', 'l', '-', 'd', 'a', 't', 'a', 's', 't', 'r', 'e', 'a', 'm',
    4, '_', 't', 'c', 'p',
    5, 'l', 'o', 'c', 'a', 'l',
    0,
    0x00, 0x0c, 0x80, 0x01
  };

  static const uint8_t ipp_query[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    4, '_', 'i', 'p', 'p',
    4, '_', 't', 'c', 'p',
    5, 'l', 'o', 'c', 'a', 'l',
    0,
    0x00, 0x0c, 0x80, 0x01
  };

  int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0)
  {
    printk("pappl_socket_list: failed to create UDP socket (errno=%d)\n", errno);
    free(buffer);
    return (false);
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    printk("pappl_socket_list: setsockopt(SO_REUSEADDR) failed (errno=%d)\n", errno);
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    printk("pappl_socket_list: setsockopt(SO_REUSEPORT) failed (errno=%d)\n", errno);

  // Bind to a random port to avoid port-sharing conflicts with native mDNS responder
  struct sockaddr_in local_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(0);
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
  {
    printk("pappl_socket_list: bind to random port failed (errno=%d)\n", errno);
    close(fd);
    free(buffer);
    return (false);
  }

  struct net_if *iface = net_if_get_default();
  struct in_addr local_ip;
  memset(&local_ip, 0, sizeof(local_ip));
  bool has_ip = false;

  if (iface && iface->config.ip.ipv4)
  {
    for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++)
    {
      if (iface->config.ip.ipv4->unicast[i].ipv4.is_used)
      {
        local_ip = iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr;
        has_ip = true;
        break;
      }
    }
  }

  if (has_ip)
  {
    char ip_str[32];
    inet_ntop(AF_INET, &local_ip, ip_str, sizeof(ip_str));
    printk("pappl_socket_list: sending queries on active IP %s...\n", ip_str);

    // Route outgoing multicast packets on the same active interface
    struct ip_mreq mreq_if;
    memset(&mreq_if, 0, sizeof(mreq_if));
    mreq_if.imr_interface = local_ip;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &mreq_if, sizeof(mreq_if)) < 0)
    {
      printk("pappl_socket_list: setsockopt(IP_MULTICAST_IF) failed (errno=%d)\n", errno);
    }
  }

  // Target mDNS multicast IPv4 endpoint
  struct sockaddr_in mcast_addr;
  memset(&mcast_addr, 0, sizeof(mcast_addr));
  mcast_addr.sin_family = AF_INET;
  mcast_addr.sin_port = htons(5353);
  mcast_addr.sin_addr.s_addr = inet_addr("224.0.0.251");

  // Send queries
  ssize_t s1 = sendto(fd, socket_query, sizeof(socket_query), 0, (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
  ssize_t s2 = sendto(fd, ipp_query, sizeof(ipp_query), 0, (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
  printk("pappl_socket_list: sent queries (socket_sent=%d, ipp_sent=%d)\n", (int)s1, (int)s2);

  // Poll for responses with a 1500ms timeout
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;

  int64_t start_time = k_uptime_get();
  bool callback_stopped = false;

#define MAX_DISCOVERED_IPS 32
  uint32_t discovered_ips[MAX_DISCOVERED_IPS];
  int num_discovered_ips = 0;

  printk("pappl_socket_list: entering poll loop...\n");

  while (k_uptime_get() - start_time < 1500 && !callback_stopped)
  {
    int64_t elapsed = k_uptime_get() - start_time;
    int remaining = 1500 - (int)elapsed;
    if (remaining <= 0)
      break;

    int r = poll(&pfd, 1, remaining);
    if (r < 0)
    {
      printk("pappl_socket_list: poll failed (errno=%d)\n", errno);
      break;
    }
    else if (r == 0)
    {
      // Timeout
    }
    else if (pfd.revents & POLLIN)
    {
      struct sockaddr_in sender_addr;
      socklen_t sender_len = sizeof(sender_addr);
      ssize_t len = recvfrom(fd, buffer, 1024, 0, (struct sockaddr *)&sender_addr, &sender_len);
      if (len > 0)
      {
        char ip_str[32];
        inet_ntop(AF_INET, &sender_addr.sin_addr, ip_str, sizeof(ip_str));
        printk("pappl_socket_list: received %d bytes from %s:%d\n", (int)len, ip_str, ntohs(sender_addr.sin_port));

        if (len > 12)
        {
          uint32_t ip_val = sender_addr.sin_addr.s_addr;
          bool duplicate = false;
          for (int k = 0; k < num_discovered_ips; k++)
          {
            if (discovered_ips[k] == ip_val)
            {
              duplicate = true;
              break;
            }
          }

          if (!duplicate)
          {
            if (parse_dns_response(buffer, len, &sender_addr, cb, data))
            {
              callback_stopped = true;
              break;
            }
            if (num_discovered_ips < MAX_DISCOVERED_IPS)
            {
              discovered_ips[num_discovered_ips++] = ip_val;
            }
          }
        }
      }
    }
  }

  printk("pappl_socket_list: scan finished, discovered %d unique hosts. callback_stopped=%d\n", num_discovered_ips, callback_stopped);

  close(fd);
  free(buffer);

  return (callback_stopped);
}


//
// '_papplDeviceAddNetworkSchemesNoLock()' - Add all of the supported network schemes.
//

void
_papplDeviceAddNetworkSchemesNoLock(void)
{
  _papplDeviceAddSchemeNoLock("socket", PAPPL_DEVTYPE_SOCKET, pappl_socket_list, pappl_socket_open, pappl_socket_close, pappl_socket_read, pappl_socket_write, pappl_socket_status, pappl_socket_supplies, pappl_socket_getid);
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
