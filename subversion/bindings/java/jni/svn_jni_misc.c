/*
 * svn_jni_misc.c miscelleneous help functions
 *
 * ====================================================================
 * Copyright (c) 2000-2001 CollabNet.  All rights reserved.
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

#include <jni.h>
#include <svn_client.h>

/*
 * utility function to throw a java exception
 */
void
svn_jni_misc__throw_exception_by_name(JNIEnv *env,
				 const char *name,
				 const char *msg)
{
  jclass cls = NULL;

#ifdef SVN_JNI__VERBOSE
  fprintf(stderr, 
	  "svn_jni__throw_exception_by_name(name=\"%s\", msg=\"%s\"\n",
	  name, msg);
#endif
  /* ensure there is enough memory and stuff
   * for one local reference
   */
  if( (*env)->PushLocalFrame(env, 1) >= 0 )
    {
      jclass cls = (*env)->FindClass(env, name);

      /* if class is null, an exception already has occured */
      if( cls != NULL )
	{
	  (*env)->ThrowNew(env, cls, msg);
	}

      /* pop stack frame */
      (*env)->PopLocalFrame(env, NULL);
    }

  return;
}
   

svn_client_auth_baton_t *
svn_jni_misc__make_auth_baton(JNIEnv *env, jobject jobj)
{
  /* the code here will build the auth_baton structure
   * right now, this doesnt work. now only NULL
   * is being returned 
   */
#ifdef SVN_JNI__VERBOSE
    fprintf(stderr, "svn_jni__make_auth_baton\n");
#endif

  return NULL;
} 


/* local variables:
 * eval: (load-file "../../../svn-dev.el")
 * end: */












