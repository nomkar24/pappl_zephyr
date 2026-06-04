//
// SNMP stub functions for the Zephyr port of Printer Application Framework.
//
// Copyright © 2020-2022 by Michael R Sweet
// Copyright © 2007-2014 by Apple Inc.
// Copyright © 2006-2007 by Easy Software Products, all rights reserved.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "snmp-private.h"


//
// '_papplSNMPClose()' - Close an SNMP socket (stub).
//

void
_papplSNMPClose(int fd)
{
  (void)fd;
}


//
// '_papplSNMPCopyOID()' - Copy an OID.
//

int *
_papplSNMPCopyOID(
    int       *dst,
    const int *src,
    int       dstsize)
{
  int	i;

  for (i = 0, dstsize --; i < dstsize && src[i] >= 0; i ++)
    dst[i] = src[i];

  dst[i] = -1;

  return (dst);
}


//
// '_papplSNMPIsOID()' - Test whether a SNMP response contains the specified OID.
//

int
_papplSNMPIsOID(
    _pappl_snmp_t *packet,
    const int     *oid)
{
  int	i;

  if (!packet || !oid)
    return (0);

  for (i = 0; i < _PAPPL_SNMP_MAX_OID && oid[i] >= 0 && packet->object_name[i] >= 0; i ++)
  {
    if (oid[i] != packet->object_name[i])
      return (0);
  }

  return (i < _PAPPL_SNMP_MAX_OID && oid[i] == packet->object_name[i]);
}


//
// '_papplSNMPIsOIDPrefixed()' - Test whether a SNMP response uses the specified OID prefix.
//

int
_papplSNMPIsOIDPrefixed(
    _pappl_snmp_t *packet,
    const int     *prefix)
{
  int	i;

  if (!packet || !prefix)
    return (0);

  for (i = 0; i < _PAPPL_SNMP_MAX_OID && prefix[i] >= 0 && packet->object_name[i] >= 0; i ++)
  {
    if (prefix[i] != packet->object_name[i])
      return (0);
  }

  return (i < _PAPPL_SNMP_MAX_OID);
}


//
// '_papplSNMPOIDToString()' - Convert an OID to a string.
//

char *
_papplSNMPOIDToString(
    const int *src,
    char      *dst,
    size_t    dstsize)
{
  char	*dstptr,
	*dstend;

  if (!src || !dst || dstsize < 4)
    return (NULL);

  for (dstptr = dst, dstend = dstptr + dstsize - 1; *src >= 0 && dstptr < dstend; src ++, dstptr += strlen(dstptr))
    snprintf(dstptr, (size_t)(dstend - dstptr + 1), ".%d", *src);

  if (*src >= 0)
    return (NULL);
  else
    return (dst);
}


//
// '_papplSNMPOpen()' - Open a SNMP socket (stub).
//

int
_papplSNMPOpen(int family)
{
  (void)family;
  return (-1);
}


//
// '_papplSNMPRead()' - Read and parse a SNMP response (stub).
//

_pappl_snmp_t *
_papplSNMPRead(
    int           fd,
    _pappl_snmp_t *packet,
    double        timeout)
{
  (void)fd;
  (void)packet;
  (void)timeout;
  return (NULL);
}


//
// '_papplSNMPWalk()' - Enumerate a group of OIDs (stub).
//

int
_papplSNMPWalk(
    int              fd,
    http_addr_t      *address,
    int              version,
    const char       *community,
    const int        *prefix,
    double           timeout,
    _pappl_snmp_cb_t cb,
    void             *data)
{
  (void)fd;
  (void)address;
  (void)version;
  (void)community;
  (void)prefix;
  (void)timeout;
  (void)cb;
  (void)data;
  return (-1);
}


//
// '_papplSNMPWrite()' - Send an SNMP query packet (stub).
//

int
_papplSNMPWrite(
    int           fd,
    http_addr_t   *address,
    int           version,
    const char    *community,
    _pappl_asn1_t request_type,
    const unsigned request_id,
    const int     *oid)
{
  (void)fd;
  (void)address;
  (void)version;
  (void)community;
  (void)request_type;
  (void)request_id;
  (void)oid;
  return (0);
}
