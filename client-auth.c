#include "client-private.h"
#include "system-private.h"

// Check Basic credentials against PAPPL system password config
static int pappl_authenticate_user(pappl_client_t *client, const char *username, const char *password)
{
  char hash[256];

  // PAPPL has a built-in password hash tool
  papplSystemHashPassword(client->system, client->system->password, password, hash, sizeof(hash));

  // Compare computed hash with stored hash
  return (strcmp(hash, client->system->password) == 0);
}

http_status_t papplClientIsAuthorized(pappl_client_t *client)
{
  // Simply call group auth with dummy parameters
  return (_papplClientIsAuthorizedForGroup(client, false, NULL, 0));
}

http_status_t _papplClientIsAuthorizedForGroup(
    pappl_client_t *client,
    bool           allow_remote,
    const char     *group,
    gid_t          groupid)
{
  const char *authorization;

  // 1. If no system password/auth callback is set, allow access
  if (!client->system->password[0] && !client->system->auth_cb)
    return (HTTP_STATUS_CONTINUE);

  // 2. If custom callback is registered, delegate
  if (client->system->auth_cb)
    return ((client->system->auth_cb)(client, group, groupid, client->system->auth_cbdata));

  // 3. Read and decode Basic authorization header
  if ((authorization = httpGetField(client->http, HTTP_FIELD_AUTHORIZATION)) != NULL && *authorization)
  {
    if (!strncmp(authorization, "Basic ", 6))
    {
      char username[512], *password;
      size_t userlen = sizeof(username);

      authorization += 6;
      httpDecode64(username, &userlen, authorization, NULL);

      if ((password = strchr(username, ':')) != NULL)
      {
        *password++ = '\0';

        if (pappl_authenticate_user(client, username, password))
        {
          cupsCopyString(client->username, username, sizeof(client->username));
          return (HTTP_STATUS_CONTINUE); // Authorized!
        }
      }
    }
  }

  // Not authorized
  return (HTTP_STATUS_UNAUTHORIZED);
}
