/*
 * lock.c: mod_dav_svn locking provider functions
 *
 * ====================================================================
 * Copyright (c) 2000-2004 CollabNet.  All rights reserved.
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



#include <httpd.h>
#include <http_log.h>
#include <mod_dav.h>
#include <apr_uuid.h>

#include "svn_fs.h"
#include "svn_repos.h"
#include "svn_dav.h"
#include "svn_time.h"
#include "svn_pools.h"

#include "dav_svn.h"


/* Every provider needs to define an opaque locktoken type. */
struct dav_locktoken
{
  const char *uuid;

};



/* Return the supportedlock property for a resource */
static const char *
dav_svn_get_supportedlock(const dav_resource *resource)
{
  /* ### we only support locks of scope "exclusive" and of type
     "write".  but what sort of string does mod_dav expect from us?
     the docs don't say.  something like

     <D:lockscope><D:exclusive/></D:lockscope>
     <D:locktype><D:write/></D:locktype>

     ??
  */

  return "";  /* temporary: just to suppress compile warnings */
}



/* Parse a lock token URI, returning a lock token object allocated
 * in the given pool.
 */
static dav_error *
dav_svn_parse_locktoken(apr_pool_t *pool,
                        const char *char_token,
                        dav_locktoken **locktoken_p)
{
  /* ### okay, so we need to be able convert a locktoken URI into just
     a lock token.  this means just pulling the UUID out of the URI. */

  return 0;  /* temporary: just to suppress compile warnings */
}



/* Format a lock token object into a URI string, allocated in
 * the given pool.
 *
 * Always returns non-NULL.
 */
static const char *
dav_svn_format_locktoken(apr_pool_t *p,
                         const dav_locktoken *locktoken)
{
  /* ### and do the reverse:  take a token UUID and embed it into a URI. */

  return "";  /* temporary: just to suppress compile warnings */
}



/* Compare two lock tokens.
 *
 * Result < 0  => lt1 < lt2
 * Result == 0 => lt1 == lt2
 * Result > 0  => lt1 > lt2
 */
static int
dav_svn_compare_locktoken(const dav_locktoken *lt1,
                          const dav_locktoken *lt2)
{
  /* ### what on earth does it mean for a locktoken to be "greater"
     than another?   ...maybe this is just for nice sorted output?? */

  return 0;  /* temporary: just to suppress compile warnings */
}



/* Open the provider's lock database.
 *
 * The provider may or may not use a "real" database for locks
 * (a lock could be an attribute on a resource, for example).
 *
 * The provider may choose to use the value of the DAVLockDB directive
 * (as returned by dav_get_lockdb_path()) to decide where to place
 * any storage it may need.
 *
 * The request storage pool should be associated with the lockdb,
 * so it can be used in subsequent operations.
 *
 * If ro != 0, only readonly operations will be performed.
 * If force == 0, the open can be "lazy"; no subsequent locking operations
 * may occur.
 * If force != 0, locking operations will definitely occur.
 */
static dav_error *
dav_svn_open_lockdb(request_rec *r,
                    int ro,
                    int force,
                    dav_lockdb **lockdb)
{
  /* ### mod_dav.h recommends this be a 'lazy' db open, that this call
     should be a cheap no-op if possible.  we don't really have a
     separate database to open anyway, other than the original opening
     of the fs way back in get_resource().  */

  /* ### and no, we don't need to pay attention to the httpd.conf
     DAVLockDB directive.  our locks are stored in the repository. */

  /* ### I think this function is going to do nothing but create a
     lockdb structure as 'context' for other funcs in this vtable.
     There's no dav_resource to verify here.  If you look at
     deadprops.c, the only thing we might want to do is get the
     authz_read callback 'ready to go' in the lockdb. */

  return 0;  /* temporary: just to suppress compile warnings */
}



/* Indicates completion of locking operations */
static void
dav_svn_close_lockdb(dav_lockdb *lockdb)
{
  /* ### free the lockdb struct, that's it. */

  return;
}



/* Take a resource out of the lock-null state. */
static dav_error *
dav_svn_remove_locknull_state(dav_lockdb *lockdb,
                              const dav_resource *resource)
{
  /* ### need to read up in RFC 2518:  what are lock-null resources,
     how do they work?  I forgot.  */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Create a (direct) lock structure for the given resource. A locktoken
** will be created.
**
** The lock provider may store private information into lock->info.
*/
static dav_error *
dav_svn_create_lock(dav_lockdb *lockdb,
                    const dav_resource *resource,
                    dav_lock **lock)
{
  /* ### call svn_repos_fs_lock(), and build a dav_lock struct to return. */

  /* ### my only concern is:  does mod_dav return "enough" fields in
     its LOCK response for the client to recreate an svn_lock_t
     structure?  I'm worried that there's no lock-creationdate field
     in the returned dav_lock struct.  Need to check the RFC.  */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Get the locks associated with the specified resource.
**
** If resolve_locks is true (non-zero), then any indirect locks are
** resolved to their actual, direct lock (i.e. the reference to followed
** to the original lock).
**
** The locks, if any, are returned as a linked list in no particular
** order. If no locks are present, then *locks will be NULL.
**
** #define DAV_GETLOCKS_RESOLVED   0    -- resolve indirects to directs 
** #define DAV_GETLOCKS_PARTIAL    1    -- leave indirects partially filled 
** #define DAV_GETLOCKS_COMPLETE   2    -- fill out indirect locks
*/
static dav_error *
dav_svn_get_locks(dav_lockdb *lockdb,
                  const dav_resource *resource,
                  int calltype,
                  dav_lock **locks)
{
  /* ### verify that resource isn't a collection, then call
         svn_fs_get_lock() and returned a linked list of exactly one
         dav_lock (since we only support 1 exclusive lock per
         resource).  return NULL if not locked.  we can pretty much
         ignore the 'calltype' arg, since we don't have lockable
         collections, and thus don't have indirect locks.  */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Find a particular lock on a resource (specified by its locktoken).
**
** *lock will be set to NULL if the lock is not found.
**
** Note that the provider can optimize the unmarshalling -- only one
** lock (or none) must be constructed and returned.
**
** If partial_ok is true (non-zero), then an indirect lock can be
** partially filled in. Otherwise, another lookup is done and the
** lock structure will be filled out as a DAV_LOCKREC_INDIRECT.
*/
static dav_error *
dav_svn_find_lock(dav_lockdb *lockdb,
                  const dav_resource *resource,
                  const dav_locktoken *locktoken,
                  int partial_ok,
                  dav_lock **lock)
{
  /* ### since we don't support shared locks, this function can share
     the same factorized code as dav_svn_get_locks, right? */
  
  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Quick test to see if the resource has *any* locks on it.
**
** This is typically used to determine if a non-existent resource
** has a lock and is (therefore) a locknull resource.
**
** WARNING: this function may return TRUE even when timed-out locks
**          exist (i.e. it may not perform timeout checks).
*/
static dav_error *
dav_svn_has_locks(dav_lockdb *lockdb,
                  const dav_resource *resource,
                  int *locks_present)
{
  /* ### again, this function can share the same factorized code as
     the previous two.  even if a resource doesn't exist,
     svn_fs_get_lock() will return a lock for a reserved name. */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Append the specified lock(s) to the set of locks on this resource.
**
** If "make_indirect" is true (non-zero), then the specified lock(s)
** should be converted to an indirect lock (if it is a direct lock)
** before appending. Note that the conversion to an indirect lock does
** not alter the passed-in lock -- the change is internal the
** append_locks function.
**
** Multiple locks are specified using the lock->next links.
*/
static dav_error *
dav_svn_append_locks(dav_lockdb *lockdb,
                     const dav_resource *resource,
                     int make_indirect,
                     const dav_lock *lock)
{
  /* ### need to return error if the resource is already locked.  we
     don't support multiple shared locks on a resource. */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Remove any lock that has the specified locktoken.
**
** If locktoken == NULL, then ALL locks are removed.
*/
static dav_error *
dav_svn_remove_lock(dav_lockdb *lockdb,
                    const dav_resource *resource,
                    const dav_locktoken *locktoken)
{
  /* ### call svn_repos_fs_unlock() on resource, using incoming
         locktoken. */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Refresh all locks, found on the specified resource, which has a
** locktoken in the provided list.
**
** If the lock is indirect, then the direct lock is referenced and
** refreshed.
**
** Each lock that is updated is returned in the <locks> argument.
** Note that the locks will be fully resolved.
*/
static dav_error *
dav_svn_refresh_locks(dav_lockdb *lockdb,
                      const dav_resource *resource,
                      const dav_locktoken_list *ltl,
                      time_t new_time,
                      dav_lock **locks)
{
  /* ### call svn_repos_fs_lock() using the incoming locktokens, with
     an expiration of 'new_time'.   return a single new lock.  WORRY: 
     does it matter that the returned lock will have a *new* token??
     libsvn_fs never truly 'refreshes' a lock:  it just destroys and
     creates a new one.  */

  return 0;  /* temporary: just to suppress compile warnings */
}



/*
** Look up the resource associated with a particular locktoken.
**
** The search begins at the specified <start_resource> and the lock
** specified by <locktoken>.
**
** If the resource/token specifies an indirect lock, then the direct
** lock will be looked up, and THAT resource will be returned. In other
** words, this function always returns the resource where a particular
** lock (token) was asserted.
**
** NOTE: this function pointer is allowed to be NULL, indicating that
**       the provider does not support this type of functionality. The
**       caller should then traverse up the repository hierarchy looking
**       for the resource defining a lock with this locktoken.
*/
static dav_error *
dav_svn_lookup_resource(dav_lockdb *lockdb,
                        const dav_locktoken *locktoken,
                        const dav_resource *start_resource,
                        const dav_resource **resource)
{
  /* ### call svn_fs_get_lock_from_token(), then
     dav_svn_get_resource() on lock->path.   It looks like we can
     pretty much ignore 'start_resource', since we don't have indirect
     locks.  */

  return 0;  /* temporary: just to suppress compile warnings */
}





/* The main locking vtable, provided to mod_dav */

const dav_hooks_locks dav_svn_hooks_locks = {
  dav_svn_get_supportedlock,
  dav_svn_parse_locktoken,
  dav_svn_format_locktoken,
  dav_svn_compare_locktoken,
  dav_svn_open_lockdb,
  dav_svn_close_lockdb,
  dav_svn_remove_locknull_state,
  dav_svn_create_lock,
  dav_svn_get_locks,
  dav_svn_find_lock,
  dav_svn_has_locks,
  dav_svn_append_locks,
  dav_svn_remove_lock,
  dav_svn_refresh_locks,
  dav_svn_lookup_resource,
  NULL                          /* hook structure context */
};
