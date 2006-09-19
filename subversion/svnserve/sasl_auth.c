/*
 * sasl_auth.c :  Functions for SASL-based authentication
 *
 * ====================================================================
 * Copyright (c) 2006 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 */

#include "svn_private_config.h"
#ifdef SVN_HAVE_SASL

#define APR_WANT_STRFUNC
#include <apr_want.h>
#include <apr_general.h>
#include <apr_strings.h>

#include "svn_types.h"
#include "svn_string.h"
#include "svn_pools.h"
#include "svn_error.h"
#include "svn_ra_svn.h"
#include "svn_base64.h"

#include "private/svn_atomic.h"

#include "server.h"

#include "../libsvn_ra_svn/ra_svn_sasl.h"

/* SASL calls this function before doing anything with a username, which gives
   us an opportunity to do some sanity-checking.  If the username contains
   an '@', SASL interprets the part following the '@' as the name of the 
   authentication realm, and worst of all, this realm overrides the one that
   we pass to sasl_server_new().  If we didn't check this, a user that could 
   successfully authenticate in one realm would be able to authenticate 
   in any other realm, simply by appending '@realm' to his username. */
static int canonicalize_username(sasl_conn_t *conn,
                                 void *context, /* not used */
                                 const char *in, /* the username */
                                 unsigned inlen, /* its length */
                                 unsigned flags, /* not used */
                                 const char *user_realm,
                                 char *out, /* the output buffer */
                                 unsigned out_max, unsigned *out_len)
{
  int realm_len = strlen(user_realm);
  char *pos;

  *out_len = inlen;

  /* If the username contains an '@', the part after the '@' is the realm
     that the user wants to authenticate in. */
  pos = memchr(in, '@', inlen);
  if (pos)
    {
      /* The only valid realm is user_realm (i.e. the repository's realm).
         If the user gave us another realm, complain. */
      if (strncmp(pos+1, user_realm, inlen-(pos-in+1)) != 0)
        return SASL_BADPROT;
    }
  else
    *out_len += realm_len + 1;

  /* First, check that the output buffer is large enough. */
  if (*out_len > out_max)
    return SASL_BADPROT;

  /* Copy the username part. */
  strncpy(out, in, inlen);

  /* If necessary, copy the realm part. */
  if (!pos)
    {
      out[inlen] = '@';
      strncpy(&out[inlen+1], user_realm, realm_len);
    }

  return SASL_OK;
}

static sasl_callback_t callbacks[] =
{
  { SASL_CB_CANON_USER, canonicalize_username, NULL },
  { SASL_CB_LIST_END, NULL, NULL }
};

static svn_error_t *initialize(void)
{
  int result;
  apr_status_t status;

  status = svn_ra_svn__sasl_common_init();
  if (status)
    return svn_error_wrap_apr(status,
                              _("Could not initialize the SASL library"));

  /* The second parameter tells SASL to look for a configuration file
     named subversion.conf. */
  result = sasl_server_init(callbacks, "subversion");
  if (result != SASL_OK)
    {
      svn_error_t *err = svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                                          sasl_errstring(result, NULL, NULL));
      return svn_error_quick_wrap(err, 
                                  _("Could not initialize the SASL library"));
    }
  return SVN_NO_ERROR;
}

svn_error_t *sasl_init(void)
{
  SVN_ERR(svn_atomic_init_once(&svn_ra_svn__sasl_status, initialize));
  return SVN_NO_ERROR;
}

/* Tell the client the authentication failed. This is only used during
   the authentication exchange (i.e. inside try_auth()). */
static svn_error_t *
fail_auth(svn_ra_svn_conn_t *conn, apr_pool_t *pool, sasl_conn_t *sasl_ctx)
{
  const char *msg = sasl_errdetail(sasl_ctx);
  SVN_ERR(svn_ra_svn_write_tuple(conn, pool, "w(c)", "failure", msg));
  return svn_ra_svn_flush(conn, pool);
}

/* Used if we run into a SASL error outside try_auth(). */
static svn_error_t *
fail_cmd(svn_ra_svn_conn_t *conn, apr_pool_t *pool, sasl_conn_t *sasl_ctx)
{
  svn_error_t *err = svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                                      sasl_errdetail(sasl_ctx));
  SVN_ERR(svn_ra_svn_write_cmd_failure(conn, pool, err));
  return svn_ra_svn_flush(conn, pool);
}

static svn_error_t *try_auth(svn_ra_svn_conn_t *conn, 
                             sasl_conn_t *sasl_ctx,
                             apr_pool_t *pool,
                             server_baton_t *b,
                             svn_boolean_t *success)
{
  const char *out, *mech;
  const svn_string_t *arg = NULL, *in;
  unsigned int outlen;
  int result;
  svn_boolean_t use_base64;

  *success = FALSE;

  /* Read the client's chosen mech and the initial token. */
  SVN_ERR(svn_ra_svn_read_tuple(conn, pool, "w(?s)", &mech, &in));

  if (strcmp(mech, "EXTERNAL") == 0 && !in)
    in = svn_string_create(b->tunnel_user, pool);
  else if (in)
    in = svn_base64_decode_string(in, pool);

  /* For CRAM-MD5, we don't base64-encode stuff. */
  use_base64 = (strcmp(mech, "CRAM-MD5") != 0);

  result = sasl_server_start(sasl_ctx, mech, 
                             in ? in->data : NULL, 
                             in ? in->len : 0, &out, &outlen);

  if (result != SASL_OK && result != SASL_CONTINUE) 
    return fail_auth(conn, pool, sasl_ctx);

  while (result == SASL_CONTINUE)
    {
      svn_ra_svn_item_t *item;

      arg = svn_string_ncreate(out, outlen, pool);
      /* Encode what we send to the client. */
      if (use_base64)
        arg = svn_base64_encode_string(arg, pool);

      SVN_ERR(svn_ra_svn_write_tuple(conn, pool, "w(s)", "step", arg));

      /* Read and decode the client response. */
      SVN_ERR(svn_ra_svn_read_item(conn, pool, &item));
      if (item->kind != SVN_RA_SVN_STRING)
        return SVN_NO_ERROR;

      in = item->u.string;
      if (use_base64)
        in = svn_base64_decode_string(in, pool);
      result = sasl_server_step(sasl_ctx, in->data, in->len, &out, &outlen);
    }

  if (result != SASL_OK) 
    return fail_auth(conn, pool, sasl_ctx);

  /* Send our last response, if necessary. */
  if (outlen)
    arg = svn_base64_encode_string(svn_string_ncreate(out, outlen, pool),
                                   pool);
  else
    arg = NULL;

  *success = TRUE;
  SVN_ERR(svn_ra_svn_write_tuple(conn, pool, "w(?s)", "success", arg));

  return SVN_NO_ERROR; 
}

static svn_error_t *get_local_hostname(char **hostname, 
                                       apr_socket_t *sock)
{
  apr_status_t apr_err;
  apr_sockaddr_t *sa;

  apr_err = apr_socket_addr_get(&sa, APR_LOCAL, sock);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, NULL);

  apr_err = apr_getnameinfo(hostname, sa, 0);
  if (apr_err)
    return svn_error_wrap_apr(apr_err, NULL);

  return SVN_NO_ERROR;
}

static apr_status_t sasl_dispose_cb(void *data)
{
  sasl_conn_t *sasl_ctx = (sasl_conn_t*) data;
  sasl_dispose(&sasl_ctx);
  return APR_SUCCESS;
}

svn_error_t *sasl_auth_request(svn_ra_svn_conn_t *conn, 
                               apr_pool_t *pool,
                               server_baton_t *b, 
                               enum access_type required,
                               svn_boolean_t needs_username)
{
  sasl_conn_t *sasl_ctx;
  apr_pool_t *subpool;
  const char *localaddrport = NULL, *remoteaddrport = NULL;
  const char *mechlist;
  char *hostname = NULL;
  sasl_security_properties_t secprops = SVN_RA_SVN__DEFAULT_SECPROPS;
  svn_boolean_t success, no_anonymous;
  int mech_count, result = SASL_OK;

  if (conn->sock)
    {
      SVN_ERR(svn_ra_svn__get_addresses(&localaddrport, &remoteaddrport,
                                        conn->sock, pool));
      SVN_ERR(get_local_hostname(&hostname, conn->sock));
    }

  /* Create a SASL context. SASL_SUCCESS_DATA tells SASL that the protocol 
     supports sending data along with the final "success" message. */
  result = sasl_server_new("svn",
                           hostname, b->realm, 
                           localaddrport, remoteaddrport,
                           NULL, SASL_SUCCESS_DATA,
                           &sasl_ctx);
  if (result != SASL_OK)
    {
      svn_error_t *err = svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                                          sasl_errstring(result, NULL, NULL));
      SVN_ERR(svn_ra_svn_write_cmd_failure(conn, pool, err));
      return svn_ra_svn_flush(conn, pool);
    }

  /* Make sure the context is always destroyed. */
  apr_pool_cleanup_register(pool, sasl_ctx, sasl_dispose_cb,
                            apr_pool_cleanup_null);

  /* Don't allow PLAIN or LOGIN, since we don't support TLS yet. */
  secprops.security_flags = SASL_SEC_NOPLAINTEXT;

  /* Don't allow ANONYMOUS if a username is required. */
  no_anonymous = needs_username || get_access(b, UNAUTHENTICATED) < required;
  if (no_anonymous)
    secprops.security_flags |= SASL_SEC_NOANONYMOUS;

  /* Set security properties. */
  result = sasl_setprop(sasl_ctx, SASL_SEC_PROPS, &secprops);
  if (result != SASL_OK)
    return fail_cmd(conn, pool, sasl_ctx);

  /* SASL needs to know if we are externally authenticated. */
  if (b->tunnel_user)
    result = sasl_setprop(sasl_ctx, SASL_AUTH_EXTERNAL, b->tunnel_user);
  if (result != SASL_OK)
    return fail_cmd(conn, pool, sasl_ctx);

  /* Get the list of mechanisms. */
  result = sasl_listmech(sasl_ctx, NULL, NULL, " ", NULL, 
                         &mechlist, NULL, &mech_count);

  if (result != SASL_OK)
    return fail_cmd(conn, pool, sasl_ctx);

  if (mech_count == 0)
    {
      svn_error_t *err = svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                                          _("Could not obtain the list"
                                          " of SASL mechanisms"));
      SVN_ERR(svn_ra_svn_write_cmd_failure(conn, pool, err));
      return svn_ra_svn_flush(conn, pool);
    }

  /* Send the list of mechanisms and the realm to the client. */
  SVN_ERR(svn_ra_svn_write_cmd_response(conn, pool, "(w)c", 
                                        mechlist, b->realm));

  /* The main authentication loop. */
  subpool = svn_pool_create(pool);
  do
    {
      svn_pool_clear(subpool);
      SVN_ERR(try_auth(conn, sasl_ctx, subpool, b, &success));
    }
  while (!success);
  svn_pool_destroy(subpool);

  if (no_anonymous)
    {
      char *p;
      const void *user;

      /* Get the authenticated username. */
      result = sasl_getprop(sasl_ctx, SASL_USERNAME, &user);

      if (result != SASL_OK)
        return fail_cmd(conn, pool, sasl_ctx);

      if ((p = strchr(user, '@')) != NULL)
        /* Drop the realm part. */
        b->user = apr_pstrndup(pool, user, p - (char *)user);
      else
        {
          svn_error_t *err;
          err = svn_error_create(SVN_ERR_RA_NOT_AUTHORIZED, NULL,
                                 _("Couldn't obtain the authenticated"
                                 " username"));
          SVN_ERR(svn_ra_svn_write_cmd_failure(conn, pool, err));
          return svn_ra_svn_flush(conn, pool);
        }
    }

  return SVN_NO_ERROR;
}

#endif /* SVN_HAVE_SASL */
