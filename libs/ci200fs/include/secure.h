/* secure.h */

/*
 *  To use secure.h properly, include the function call
 *  check_secure(mode,path) as the first line of code in the
 *  function init(). init() should be a static function, for security
 *  reasons.
 *
 *  Then, when the object is cloned, have the cloning object call
 *  the function set_secure() in the secured object.  The secured
 *  object will destruct() itself after the current execution thread
 *  is completed if the cloning object fails the security check.
 *
 *  The security features are SECURE_BOOTOBJ, which allows only
 *  the object /boot#0 to create objects of the type,
 *  SECURE_PRIV, which allows only objects with the PRIV flag set
 *  to create objects of the type, and SECURE_PATH, which allows
 *  only objects of the pathname specified when check_secure() was
 *  called to create objects.
 *
 */

var _secure;
var _securetype;
var _securepath;

#define SECURE_BOOTOBJ 1
#define SECURE_PRIV    2
#define SECURE_PATH     4

static _check_secure() {
  if (!_secure) {
    destruct(this_object());
  }
  _secure=0;
}

static check_secure(int mode, string path) {
  if (prototype(this_object())) return;
  _securetype=mode;
  _securepath=path;
  alarm(0,"_check_secure");
}

set_secure() {
  if (_secure) return;
  _secure=caller_object();
  if (!_secure) return;
  if (_securetype & SECURE_BOOTOBJ)
    if (_secure!=atoo("/boot")) {
      _secure=0;
      return;
    }
  if (_securetype & SECURE_PRIV)
    if (!priv(_secure)) {
      _secure=0;
      return;
    }
  if (_securetype & SECURE_PATH)
    if (leftstr(otoa(_secure),strlen(_securepath)+1)!=_securepath+"#") {
      _secure=0;
      return;
    }
  return;
}
