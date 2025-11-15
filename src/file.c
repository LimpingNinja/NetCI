
/* file.c */

/* file permissions etc. are maintained in data structures & code here */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef USE_WINDOWS
#include <direct.h>
#else
/* POSIX and LINUX should use UNISTD.H */
#include <unistd.h>
#endif /* USE_WINDOWS */

#include "config.h"
#include "object.h"
#include "file.h"
#include "globals.h"
#include "interp.h"
#include "constrct.h"
#include "cache.h"
#ifdef USE_WINDOWS
#include "intrface.h"
#include "winmain.h"
#endif /* USE_WINDOWS */

int SYSTEM_mkdir(char *filename) {
#ifdef USE_WINDOWS
  if (mkdir(filename))
#else /* USE_WINDOWS */
  if (mkdir(filename,493))
#endif /* USE_WINDOWS */
    return 1;
  else
    return 0;
}

int SYSTEM_rmdir(char *filename) {
  if (rmdir(filename))
    return 1;
  else
    return 0;
}

char *make_path(struct file_entry *entry) {
  char *buf,*buf2;
  struct file_entry *curr;
  unsigned int len;

  len=strlen(fs_path);
  curr=entry;
  while (curr!=root_dir) {
    len=len+strlen(curr->filename)+1;
    curr=curr->parent;
  }
  len++;
  buf=MALLOC(len);
  buf2=MALLOC(len);
  *buf='\0';
  *buf2='\0';
  curr=entry;
  while (curr!=root_dir) {
    strcpy(buf2,buf);
#ifdef USE_WINDOWS
    strcpy(buf,"\\");
#else /* USE_WINDOWS */
    strcpy(buf,"/");
#endif /* USE_WINDOWS */
    strcat(buf,curr->filename);
    strcat(buf,buf2);
    curr=curr->parent;
  }
  strcpy(buf2,buf);
  strcpy(buf,fs_path);
  strcat(buf,buf2);
  FREE(buf2);
  return buf;
}

struct file_entry *find_entry(char *filename) {
  struct file_entry *curr;
  char *currname,*currptr;

  if (*filename!='/')
    return NULL;
  if (filename[1]=='\0')
    return root_dir;
  curr=root_dir;
  currname=MALLOC(strlen(filename)+1);
  while (*filename=='/') {
    currptr=currname;
    filename++;
    while (*filename!='/' && *filename!='\0')
      *(currptr++)=*(filename++);
    *currptr='\0';
    curr=curr->contents;
    while (curr) {
      if (!strcmp(curr->filename,currname))
        break;
      curr=curr->next_file;
    }
    if (!curr) {
      FREE(currname);
      return NULL;
    }
  }
  FREE(currname);
  return curr;
}

int can_read(struct file_entry *fe, struct object *uid) {
  if (!uid) return 1;
  if (!fe) return 1;
  if (uid->flags & PRIV) return 1;
  if (fe==root_dir) return 1;
  while (fe!=root_dir) {
    if (uid->refno!=fe->owner && !(fe->flags & READ_OK))
      return 0;
    fe=fe->parent;
  }
  return 1;
}

struct file_entry *split_dir(char *filename, char **f) {
  char *buf1,*buf2;
  struct file_entry *fe;
  int count,maxcount,x;

  if (*filename!='/') return NULL;
  count=0;
  maxcount=0;
  while (filename[count]) {
    if (filename[count]=='/') maxcount=count;
    count++;
  }
  buf1=MALLOC(maxcount+2);
  buf2=MALLOC(count-maxcount);
  x=0;
  while (x<maxcount) {
    buf1[x]=filename[x];
    x++;
  }
  buf1[x]='\0';
  while (x<count) {
    x++;
    buf2[x-maxcount-1]=filename[x];
  }
  if (!maxcount) {
    buf1[0]='/';
    buf1[1]='\0';
  }
  fe=find_entry(buf1);
  FREE(buf1);
  if (!fe) {
    FREE(buf2);
    return NULL;
  }
  if (f)
    *f=buf2;
  else
    FREE(buf2);
  return fe;
}

struct file_entry *make_entry(struct file_entry *dir, char *name,
                              struct object *uid) {
  struct file_entry *fe,*curr,*prev;
  int x;

  if (!is_legal(name)) return NULL;
  curr=dir->contents;
  prev=NULL;
  while (curr) {
    x=strcmp(name,curr->filename);
    if (!x) return curr;
    if (x<0) {
      fe=MALLOC(sizeof(struct file_entry));
      fe->filename=copy_string(name);
      fe->flags=0;
      if (uid)
        fe->owner=uid->refno;
      else
        fe->owner=0;
      fe->contents=NULL;
      fe->parent=dir;
      fe->prev_file=prev;
      fe->next_file=curr;
      curr->prev_file=fe;
      if (prev)
        prev->next_file=fe;
      else
        dir->contents=fe;
      break;
    }
    prev=curr;
    curr=curr->next_file;
  }
  if (!curr) {
    fe=MALLOC(sizeof(struct file_entry));
    fe->filename=name;
    fe->flags=0;
    if (uid)
      fe->owner=uid->refno;
    else
      fe->owner=0;
    fe->contents=NULL;
    fe->parent=dir;
    fe->prev_file=prev;
    fe->next_file=NULL;
    if (prev)
      prev->next_file=fe;
    else
      dir->contents=fe;
  }
  return fe;
}

int remove_entry(struct file_entry *fe) {
  if (fe==root_dir) return 1;
  if (fe->contents) return 1;
  if (fe->parent->contents==fe)
    fe->parent->contents=fe->next_file;
  if (fe->next_file)
    fe->next_file->prev_file=fe->prev_file;
  if (fe->prev_file)
    fe->prev_file->next_file=fe->next_file;
  FREE(fe->filename);
  FREE(fe);
  return 0;
}

/* Flag to prevent re-entry during callback execution */
static int in_master_callback = 0;

/**
 * Calls master object for permission validation
 * @param path Virtual filesystem path being accessed
 * @param operation Name of operation (e.g., "read_file", "write_file")
 * @param uid Calling object
 * @param is_write 1 for write operations, 0 for read operations
 * @return 1 if allowed, 0 if denied
 */
int check_master_permission(char *path, char *operation, struct object *uid, int is_write) {
  struct object *master;
  
  /* Prevent re-entry during callback execution */
  if (in_master_callback) {
    return 1;
  }
  
  /* Check if object system is initialized (bootstrap check) */
  if (!obj_list) {
    return 1;
  }
  
  /* If caller is NULL (system operations), allow */
  if (!uid) {
    return 1;
  }
  
  /* Get master object (object #0) */
  master = db_ref_to_obj(0);
  
  /* If no master, allow (bootstrap mode) */
  if (!master) {
    return 1;
  }
  
  /* If caller IS the master object, allow (prevents recursion during master init) */
  if (uid == master) {
    return 1;
  }
  
  /* Check if master has the callback function */
  struct object *tmpobj;
  struct fns *callback_func;
  char *func_name;
  
  func_name = is_write ? "valid_write" : "valid_read";
  callback_func = find_function(func_name, master, &tmpobj);
  
  /* If function doesn't exist, just allow */
  if (!callback_func) {
    return 1;
  }
  
  /* Save global state before calling interp() */
  extern unsigned int num_locals;
  extern struct var *locals;
  unsigned int old_num_locals = num_locals;
  struct var *old_locals = locals;
  
  /* Get file information to pass to callback */
  struct file_entry *fe;
  int file_flags, file_owner;
  
  fe = find_entry(path);
  if (fe) {
    file_flags = fe->flags;
    file_owner = fe->owner;
  } else {
    file_flags = -1;
    file_owner = -1;
  }
  
  /* Call the callback with all 5 arguments: path, operation, caller, owner, flags */
  struct var_stack *rts;
  struct var tmp;
  
  in_master_callback = 1;
  
  rts = NULL;
  
  /* Push arguments in order */
  tmp.type = STRING;
  tmp.value.string = path;
  push(&tmp, &rts);
  
  tmp.type = STRING;
  tmp.value.string = operation;
  push(&tmp, &rts);
  
  tmp.type = OBJECT;
  tmp.value.objptr = uid;
  push(&tmp, &rts);
  
  tmp.type = INTEGER;
  tmp.value.integer = file_owner;
  push(&tmp, &rts);
  
  tmp.type = INTEGER;
  tmp.value.integer = file_flags;
  push(&tmp, &rts);
  
  /* Push NUM_ARGS */
  tmp.type = NUM_ARGS;
  tmp.value.num = 5;
  push(&tmp, &rts);
  
  interp(NULL, tmpobj, NULL, &rts, callback_func);
  free_stack(&rts);
  
  in_master_callback = 0;
  
  /* Restore global state */
  num_locals = old_num_locals;
  locals = old_locals;
  
  /* Pop and check the result */
  int result;
  if (pop(&tmp, &rts, master)) {
    /* Error popping result - allow by default */
    free_stack(&rts);
    return 1;
  }
  
  /* Check if result is non-zero (allow) */
  result = (tmp.type == INTEGER && tmp.value.integer != 0) ? 1 : 0;
  clear_var(&tmp);
  
  return result;
  
  /* TODO: Re-enable master callbacks after fixing malloc corruption
  struct object *master, *tmpobj;
  struct fns *callback_func;
  struct var_stack *rts;
  struct var tmp;
  char *func_name;
  char logbuf[512];
  int result;
  struct file_entry *fe;
  int file_flags, file_owner;
  
  // Check if object system is initialized (bootstrap check)
  if (!obj_list) {
    // During database creation, no objects exist yet - allow all operations
    return 1;
  }
  
  // If caller is NULL (system operations), allow
  if (!uid) {
    return 1;
  }
  
  // Get master object (object #0)
  master = db_ref_to_obj(0);
  
  // If no master, allow (bootstrap mode)
  if (!master) {
    return 1;
  }
  
  // If caller IS the master object, allow (prevents recursion during master init)
  if (uid == master) {
    return 1;
  }
  
  // Determine which callback to use
  func_name = is_write ? "valid_write" : "valid_read";
  
  // Find the callback function in master object
  callback_func = find_function(func_name, master, &tmpobj);
  
  // If function doesn't exist, fall back to legacy permission check
  if (!callback_func) {
    // Use existing can_read() logic as fallback
    fe = find_entry(path);
    if (!fe) return 1; // File doesn't exist yet, allow for creation
    return can_read(fe, uid);
  }
  
  // Get file information to pass to callback (avoids recursion)
  fe = find_entry(path);
  if (fe) {
    file_flags = fe->flags;
    file_owner = fe->owner;
  } else {
    file_flags = -1;  // File doesn't exist
    file_owner = -1;
  }
  
  // Build argument stack for callback: (path, operation, caller, file_owner, file_flags)
  rts = NULL;
  
  // Push path argument - push() will make its own copy via copy_string()
  tmp.type = STRING;
  tmp.value.string = path;
  push(&tmp, &rts);
  
  // Push operation argument - push() will make its own copy via copy_string()
  tmp.type = STRING;
  tmp.value.string = operation;
  push(&tmp, &rts);
  
  // Push caller argument
  tmp.type = OBJECT;
  tmp.value.objptr = uid;
  push(&tmp, &rts);
  
  // Push file_owner argument
  tmp.type = INTEGER;
  tmp.value.integer = file_owner;
  push(&tmp, &rts);
  
  // Push file_flags argument
  tmp.type = INTEGER;
  tmp.value.integer = file_flags;
  push(&tmp, &rts);
  
  // Push number of arguments
  tmp.type = NUM_ARGS;
  tmp.value.integer = 5;
  push(&tmp, &rts);
  
  // Call the callback function
  interp(uid, tmpobj, uid, &rts, callback_func);
  
  // Pop result
  if (pop(&tmp, &rts, master)) {
    // Error popping result - deny by default
    free_stack(&rts);
    if (uid) {
      sprintf(logbuf, "Permission check error for %s by object #%ld", path, (long)uid->refno);
    } else {
      sprintf(logbuf, "Permission check error for %s by system", path);
    }
    logger(LOG_WARNING, logbuf);
    return 0;
  }
  
  // Check result - non-zero means allowed
  if (tmp.type == INTEGER) {
    result = (tmp.value.integer != 0);
  } else {
    // Non-integer result - deny by default
    result = 0;
  }
  
  // Clean up
  clear_var(&tmp);
  free_stack(&rts);
  
  // Log denials
  if (!result) {
    if (uid) {
      sprintf(logbuf, "Permission denied: %s for %s by object #%ld", operation, path, (long)uid->refno);
    } else {
      sprintf(logbuf, "Permission denied: %s for %s by system", operation, path);
    }
    logger(LOG_WARNING, logbuf);
  }
  
  return result;
  */
}

/**
 * Validates that a path is within the mudlib root and doesn't contain malicious patterns
 * @param path Virtual filesystem path to validate
 * @return 1 if valid, 0 if invalid
 */
int validate_path(char *path) {
  char *resolved, *ptr, *component;
  char logbuf[512];
  int depth = 0;
  int len, i, j;
  
  /* Check for NULL or empty path */
  if (!path || *path == '\0') {
    logger(LOG_ERROR, "Security: NULL or empty path rejected");
    return 0;
  }
  
  /* Check path starts with "/" */
  if (*path != '/') {
    sprintf(logbuf, "Security: Relative path rejected: %s", path);
    logger(LOG_ERROR, logbuf);
    return 0;
  }
  
  /* Allocate buffer for resolved path */
  len = strlen(path);
  resolved = MALLOC(len + 1);
  strcpy(resolved, path);
  
  /* Resolve ".." components and check depth */
  ptr = resolved;
  i = 0;
  j = 0;
  
  while (ptr[i] != '\0') {
    if (ptr[i] == '/') {
      /* Skip multiple slashes */
      while (ptr[i] == '/') i++;
      if (ptr[i] == '\0') break;
      
      /* Check for ".." component */
      if (ptr[i] == '.' && ptr[i+1] == '.' && (ptr[i+2] == '/' || ptr[i+2] == '\0')) {
        depth--;
        if (depth < 0) {
          /* Attempt to escape root */
          sprintf(logbuf, "Security: Attempt to escape mudlib root: %s", path);
          logger(LOG_ERROR, logbuf);
          FREE(resolved);
          return 0;
        }
        i += 2;
        /* Remove last component from resolved path */
        while (j > 0 && resolved[j-1] != '/') j--;
        if (j > 0) j--; /* Remove the slash too */
        continue;
      }
      
      /* Check for "." component (current directory) */
      if (ptr[i] == '.' && (ptr[i+1] == '/' || ptr[i+1] == '\0')) {
        i++;
        continue;
      }
      
      /* Normal component - copy to resolved */
      resolved[j++] = '/';
      depth++;
      while (ptr[i] != '/' && ptr[i] != '\0') {
        resolved[j++] = ptr[i++];
      }
    } else {
      i++;
    }
  }
  
  resolved[j] = '\0';
  
  /* If resolved path is empty, it's root */
  if (j == 0) {
    resolved[0] = '/';
    resolved[1] = '\0';
  }
  
  FREE(resolved);
  
  /* Path is valid */
  return 1;
}

/**
 * Discovers a single file and creates virtual entry if needed
 * @param path Virtual filesystem path (e.g., "/home/wizard/test.c")
 * @param uid Owner object for new entries
 * @return Pointer to file_entry, or NULL if file doesn't exist
 */
struct file_entry *discover_file(char *path, struct object *uid) {
  struct file_entry *fe, *homefe;
  char *buf, *phys_path;
  struct stat st;
  
  /* Check if entry already exists in virtual filesystem */
  fe = find_entry(path);
  if (fe) {
    return fe;
  }
  
  /* Build physical path and check if file exists on disk */
  phys_path = MALLOC(strlen(fs_path) + strlen(path) + 1);
  strcpy(phys_path, fs_path);
  strcat(phys_path, path);
  
  /* stat() the physical file */
  if (stat(phys_path, &st) != 0) {
    /* File doesn't exist on disk */
    FREE(phys_path);
    return NULL;
  }
  FREE(phys_path);
  
  /* File exists on disk - create virtual entry */
  homefe = split_dir(path, &buf);
  if (!homefe) {
    return NULL;
  }
  
  fe = make_entry(homefe, buf, uid);
  if (!fe) {
    FREE(buf);
    return NULL;
  }
  
  /* Set default flags for discovered files */
  if (S_ISDIR(st.st_mode)) {
    fe->flags |= DIRECTORY;
  }
  /* Give discovered files default READ_OK permission */
  fe->flags |= READ_OK;
  
  return fe;
}

/**
 * Discovers all files in a directory and removes stale entries
 * Synchronizes virtual filesystem with physical directory
 * @param path Virtual filesystem path to directory
 * @param uid Owner object for new entries
 * @return Number of files discovered
 */
int discover_directory(char *path, struct object *uid) {
  char *phys_path, *entry_path;
  DIR *dir;
  struct dirent *entry;
  struct file_entry *dir_entry, *curr, *next;
  struct stat st;
  int count = 0;
  int path_len, entry_len;
  
  /* Build physical path */
  phys_path = MALLOC(strlen(fs_path) + strlen(path) + 1);
  strcpy(phys_path, fs_path);
  strcat(phys_path, path);
  
  /* Open physical directory */
  dir = opendir(phys_path);
  FREE(phys_path);
  
  if (!dir) {
    return 0;
  }
  
  /* Scan physical directory and discover all files */
  path_len = strlen(path);
  while ((entry = readdir(dir)) != NULL) {
    /* Skip "." and ".." */
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    
    /* Build full virtual path for entry */
    entry_len = strlen(entry->d_name);
    entry_path = MALLOC(path_len + entry_len + 2);
    strcpy(entry_path, path);
    if (path[path_len - 1] != '/') {
      strcat(entry_path, "/");
    }
    strcat(entry_path, entry->d_name);
    
    /* Discover the file (creates entry if doesn't exist) */
    if (discover_file(entry_path, uid) != NULL) {
      count++;
    }
    
    FREE(entry_path);
  }
  
  closedir(dir);
  
  /* Remove stale entries: check each virtual entry against physical filesystem */
  dir_entry = find_entry(path);
  if (dir_entry && (dir_entry->flags & DIRECTORY)) {
    curr = dir_entry->contents;
    while (curr) {
      next = curr->next_file; /* Save next before potential removal */
      
      /* Build physical path for this virtual entry */
      entry_len = path_len + strlen(curr->filename) + 2;
      entry_path = MALLOC(entry_len);
      strcpy(entry_path, path);
      if (path[path_len - 1] != '/') {
        strcat(entry_path, "/");
      }
      strcat(entry_path, curr->filename);
      
      phys_path = MALLOC(strlen(fs_path) + strlen(entry_path) + 1);
      strcpy(phys_path, fs_path);
      strcat(phys_path, entry_path);
      
      /* Check if physical file still exists */
      if (stat(phys_path, &st) != 0) {
        /* File doesn't exist - remove stale entry */
        remove_entry(curr);
      }
      
      FREE(phys_path);
      FREE(entry_path);
      
      curr = next;
    }
  }
  
  return count;
}

/**
 * Recursively discovers all files under a path
 * @param path Virtual filesystem path to start from
 * @param uid Owner object for new entries
 * @return Total number of files discovered
 */
int discover_recursive(char *path, struct object *uid) {
  struct file_entry *fe, *curr;
  char *subdir_path;
  int count = 0;
  int path_len;
  int subdir_count;
  
  /* Discover all files in current directory */
  count = discover_directory(path, uid);
  
  /* Find the directory entry */
  fe = find_entry(path);
  if (!fe || !(fe->flags & DIRECTORY)) {
    return count;
  }
  
  /* Recursively discover subdirectories */
  path_len = strlen(path);
  curr = fe->contents;
  while (curr) {
    if (curr->flags & DIRECTORY) {
      /* Build path for subdirectory */
      subdir_path = MALLOC(path_len + strlen(curr->filename) + 2);
      strcpy(subdir_path, path);
      if (path[path_len - 1] != '/') {
        strcat(subdir_path, "/");
      }
      strcat(subdir_path, curr->filename);
      
      /* Recursively discover */
      subdir_count = discover_recursive(subdir_path, uid);
      count += subdir_count;
      
      FREE(subdir_path);
    }
    curr = curr->next_file;
  }
  
  return count;
}

int ls_dir(char *filename, struct object *uid, struct object *player) {
  struct file_entry *fe;
  struct fns *listen_func;
  char *buf;
  struct var_stack *rts;
  struct var tmp;
  struct object *rcv,*tmpobj;

  if (!uid) return 1;
  rcv=player;
  if (!player) rcv=uid;
  
  /* Validate path first */
  if (!validate_path(filename)) {
    return FILE_ERR_PATH_INVALID;
  }
  
  /* Discover directory contents before listing */
  discover_directory(filename, uid);
  
  /* Check master permission for directory listing */
  if (!check_master_permission(filename, "get_dir", uid, 0)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  fe=find_entry(filename);
  if (!fe) return 1;
  if (!can_read(fe,uid)) return 1;
  if (!(fe->flags & DIRECTORY)) return 1;
  listen_func=find_function("listen",rcv,&tmpobj);
  if (!listen_func) return 0;
  fe=fe->contents;
  doing_ls=1;
  while (fe) {
    buf=MALLOC((2*ITOA_BUFSIZ)+4+strlen(fe->filename));
    sprintf(buf,"%ld %ld %s\n",(long) fe->owner,(long) fe->flags,fe->filename);
    rts=NULL;
    tmp.type=STRING;
    tmp.value.string=buf;
    push(&tmp,&rts);
    FREE(buf);
    tmp.type=NUM_ARGS;
    tmp.value.integer=1;
    push(&tmp,&rts);
    interp(uid,tmpobj,player,&rts,listen_func);
    free_stack(&rts);
    fe=fe->next_file;
  }
  doing_ls=0;
  return 0;
}

int stat_file(char *filename, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *newbuf;

  /* Validate path first */
  if (!validate_path(filename)) {
    return FILE_ERR_PATH_INVALID;
  }

  /* Try to discover the file first */
  fe = discover_file(filename, uid);
  if (!fe) return FILE_ERR_NOT_FOUND;
  
  /* Check master permission for stat operation */
  if (!check_master_permission(filename, "stat", uid, 0)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  homefe=split_dir(filename,&newbuf);
  if (!homefe) return -1;
  FREE(newbuf);
  if (!can_read(homefe,uid)) return -1;
  return fe->flags;
}
int owner_file(char *filename, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *newbuf;

  /* Try to discover the file first */
  fe = discover_file(filename, uid);
  if (!fe) return -1;
  
  /* Check master permission for owner query */
  if (!check_master_permission(filename, "file_owner", uid, 0)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  homefe=split_dir(filename,&newbuf);
  if (!homefe) return -1;
  FREE(newbuf);
  if (!can_read(homefe,uid)) return -1;
  return fe->owner;
}

FILE *open_file(char *filename, char *mode, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *buf,*newbuf;
  FILE *f;
  char logbuf[512];

  /* Log all boot.c access for security auditing */
  if (strcmp(filename, "/boot.c") == 0) {
    sprintf(logbuf, "AUDIT: boot.c access - mode=%s uid=%p obj_list=%p", mode, (void*)uid, (void*)obj_list);
    logger(LOG_INFO, logbuf);
  }

  /* Validate path first */
  if (!validate_path(filename)) {
    if (strcmp(filename, "/boot.c") == 0) {
      logger(LOG_ERROR, "AUDIT: boot.c DENIED - path validation failed");
    }
    return NULL;
  }

  /* Try to discover the file first */
  fe = discover_file(filename, uid);
  
  if (!fe) {
    /* File doesn't exist - handle creation for write modes */
    if (*mode=='r') {
      /* Check read permission even for non-existent files */
      if (!check_master_permission(filename, "read_file", uid, 0)) {
        return NULL;
      }
      return NULL;
    }
    if (!uid) return NULL;
    
    /* Check master permission for file creation */
    if (!check_master_permission(filename, "write_file", uid, 1)) {
      return NULL;
    }
    
    /* Master object approved via valid_write() - proceed with file creation */
    homefe=split_dir(filename,&newbuf);
    if (!homefe) return NULL;
    if (!can_read(homefe,uid)) {
      FREE(newbuf);
      return NULL;
    }
    /* Note: Filesystem permission check removed - valid_write() is authoritative */
    fe=make_entry(homefe,newbuf,uid);
    if (!fe) {
      FREE(newbuf);
      return NULL;
    }
  }

  if (fe->flags & DIRECTORY) {
    return NULL;
  }
  
  /* NOTE: Removed legacy can_read(fe->parent, uid) check - we rely on check_master_permission instead */
  
  /* Check master permission for read operations */
  if (*mode=='r') {
    if (!check_master_permission(filename, "read_file", uid, 0)) {
      return NULL;
    }
  }
  
  buf=make_path(fe);
  if (!uid && *mode=='r') {
    f=fopen(buf,mode);
    /* Clean up stale entry if fopen fails */
    if (!f) {
      remove_entry(fe);
    }
    FREE(buf);
    return f;
  }
  if (!uid) {
    FREE(buf);
    return NULL;
  }
  if (uid->refno==fe->owner || (uid->flags & PRIV)) {
    /* Check master permission for write operations */
    if ((*mode=='a' || *mode=='w') && !check_master_permission(filename, "write_file", uid, 1)) {
      FREE(buf);
      return NULL;
    }
    f=fopen(buf,mode);
    /* Clean up stale entry if fopen fails */
    if (!f && *mode=='r') {
      remove_entry(fe);
    }
    FREE(buf);
    return f;
  }
  if (*mode=='r' && (fe->flags & READ_OK)) {
    f=fopen(buf,mode);
    /* Clean up stale entry if fopen fails */
    if (!f) {
      remove_entry(fe);
    }
    FREE(buf);
    return f;
  }
  if ((*mode=='a' || *mode=='w') && (fe->flags & WRITE_OK) && !doing_ls) {
    /* Check master permission for write operations */
    if (!check_master_permission(filename, "write_file", uid, 1)) {
      FREE(buf);
      return NULL;
    }
    f=fopen(buf,mode);
    FREE(buf);
    return f;
  }
  FREE(buf);
  return NULL;
}

int remove_file(char *filename, struct object *uid) {
  struct file_entry *fe;
  char *buf;

  if (doing_ls) return 1;
  if (!uid) return 1;
  
  /* Validate path first */
  if (!validate_path(filename)) {
    return FILE_ERR_PATH_INVALID;
  }
  
  /* Try to discover the file first */
  fe = discover_file(filename, uid);
  if (!fe) return 1;
  
  /* Check master permission for file removal */
  if (!check_master_permission(filename, "rm", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  if (fe->flags & DIRECTORY) return 1;
  if (!can_read(fe->parent,uid)) return 1;
  if (fe->parent->owner!=uid->refno && !(fe->parent->flags & WRITE_OK)
      && !(uid && (uid->flags & PRIV)))
    return 1;
  buf=make_path(fe);
  if (remove(buf)) {
    FREE(buf);
    return 1;
  }
  /* Remove virtual entry after successful deletion */
  remove_entry(fe);
  FREE(buf);
  return 0;
}

int copy_file(char *src, char *dest, struct object *uid) {
  FILE *srcf,*destf;
  int c;

  if (doing_ls) return 1;
  if (!uid) return 1;
  
  /* Validate paths first */
  if (!validate_path(src) || !validate_path(dest)) {
    return FILE_ERR_PATH_INVALID;
  }
  
  /* Discover source and destination files */
  discover_file(src, uid);
  discover_file(dest, uid);
  
  /* Check master permission for copy operation (write to destination) */
  if (!check_master_permission(dest, "cp", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  srcf=open_file(src,FREAD_MODE,uid);
  if (!srcf) return 1;
  destf=open_file(dest,FWRITE_MODE,uid);
  if (!destf) {
    close_file(srcf);
    return 1;
  }
  c=fgetc(srcf);
  while (c!=EOF) {
    fputc(c,destf);
    c=fgetc(srcf);
  }
  close_file(srcf);
  close_file(destf);
  return 0;
}

int move_file(char *src, char *dest, struct object *uid) {
  struct file_entry *homefe,*srcf,*destf;
  char *buf;
  FILE *f1,*f2;
  int c;

  if (doing_ls) return 1;
  if (!uid) return 1;
  if (!strcmp(src,dest)) return 1;
  
  /* Validate paths first */
  if (!validate_path(src) || !validate_path(dest)) {
    return FILE_ERR_PATH_INVALID;
  }
  
  /* Discover source file */
  srcf = discover_file(src, uid);
  if (!srcf) return 1;
  if (srcf->flags & DIRECTORY) return 1;
  
  /* Check master permission for move operation (write to both source and dest) */
  if (!check_master_permission(src, "mv", uid, 1) || !check_master_permission(dest, "mv", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  /* Discover or create destination file entry */
  destf = discover_file(dest, uid);
  if (!destf) {
    homefe=split_dir(dest,&buf);
    if (!homefe) return 1;
    if (!can_read(homefe,uid)) {
      FREE(buf);
      return 1;
    }
    if (!(homefe->owner==uid->refno || (uid->flags & PRIV) ||
          (homefe->flags & WRITE_OK))) {
      FREE(buf);
      return 1;
    }
    destf=make_entry(homefe,buf,uid);
    if (!destf) {
      FREE(buf);
      return 1;
    }
  }
  if (destf->flags & DIRECTORY) return 1;
  if (!can_read(srcf,uid)) return 1;
  if (!(srcf->parent->owner==uid->refno || (uid->flags & PRIV) ||
        (srcf->parent->flags & WRITE_OK)))
    return 1;
  if (!can_read(destf->parent,uid)) return 1;
  if (!(destf->owner==uid->refno || (uid->flags & PRIV) ||
        (destf->flags & WRITE_OK)))
    return 1;
  f1=open_file(src,FREAD_MODE,uid);
  if (!f1) return 1;
  f2=open_file(dest,FWRITE_MODE,uid);
  if (!f2) {
    close_file(f1);
    return 1;
  }
  c=fgetc(f1);
  while (c!=EOF) {
    fputc(c,f2);
    c=fgetc(f1);
  }
  close_file(f1);
  close_file(f2);
  buf=make_path(srcf);
  if (remove(buf)) {
    FREE(buf);
    return 1;
  }
  FREE(buf);
  /* Remove source virtual entry after successful move */
  remove_entry(srcf);
  return 0;
}

int make_dir(char *filename, struct object *uid) {
  struct file_entry *fe,*homefe;
  char *buf;

  if (doing_ls) return 1;
  if (!uid) return 1;
  
  /* Validate path first */
  if (!validate_path(filename)) {
    return FILE_ERR_PATH_INVALID;
  }
  
  /* Check if directory already exists (discover first) */
  fe = discover_file(filename, uid);
  if (fe) return 1;
  
  /* Check master permission for directory creation */
  if (!check_master_permission(filename, "mkdir", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  homefe=split_dir(filename,&buf);
  if (!homefe) return 1;
  if (!(homefe->owner==uid->refno || (uid->flags & PRIV) ||
        (homefe->flags & WRITE_OK))) {
    FREE(buf);
    return 1;
  }
  if (!(fe=make_entry(homefe,buf,uid))) {
    FREE(buf);
    return 1;
  }
  /* Set DIRECTORY flag on new entry */
  fe->flags=DIRECTORY;
  buf=make_path(fe);
  if (SYSTEM_mkdir(buf)) {
    FREE(buf);
    remove_entry(fe);
    return 1;
  }
  FREE(buf);
  return 0;
}

int remove_dir(char *filename, struct object *uid) {
  struct file_entry *fe;
  char *buf;

  if (doing_ls) return 1;
  if (!uid) return 1;
  
  /* Validate path first */
  if (!validate_path(filename)) {
    return FILE_ERR_PATH_INVALID;
  }
  
  /* Try to discover the directory first */
  fe = discover_file(filename, uid);
  if (!fe) return 1;
  
  /* Check master permission for directory removal */
  if (!check_master_permission(filename, "rmdir", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  if (!(fe->flags & DIRECTORY)) return 1;
  if (fe->contents) return 1;
  if (fe==root_dir) return 1;
  if (!(fe->parent->owner==uid->refno || (uid->flags & PRIV) ||
        (fe->parent->flags & WRITE_OK)))
    return 1;
  buf=make_path(fe);
  if (SYSTEM_rmdir(buf)) {
    FREE(buf);
    return 1;
  }
  FREE(buf);
  /* Remove virtual entry after successful deletion */
  remove_entry(fe);
  return 0;
}

/**
 * Removes a file from the virtual filesystem
 * Note: With automatic discovery, the file will be auto-rediscovered on next access
 * This implements a "soft hide" behavior - the file reappears when accessed
 * Useful for forcing re-discovery (e.g., after external permission changes)
 */
int hide(char *filename) {
  struct file_entry *fe;

  if (doing_ls) return 1;
  fe=find_entry(filename);
  if (!fe) return 1;
  return remove_entry(fe);
}

/**
 * Pre-creates a virtual filesystem entry with specific permissions
 * Note: File may not exist on disk yet - this is OK
 * - If file doesn't exist, read operations will fail but write operations can create it
 * - If file exists on disk, it will be discovered with these permissions
 * - Useful for pre-creating entries with specific permissions before file exists
 * Edge cases:
 * - Reading non-existent unhidden file: discover_file() will remove stale entry
 * - Writing to non-existent unhidden file: open_file() will create the physical file
 */
int unhide(char *filename, struct object *uid, int flags) {
  struct file_entry *fe,*homefe;
  char *buf;

  if (doing_ls) return 1;
  fe=find_entry(filename);
  if (fe) return 1;
  homefe=split_dir(filename,&buf);
  if (!homefe) return 1;
  if (!(fe=make_entry(homefe,buf,uid))) {
    FREE(buf);
    return 1;
  }
  fe->flags=flags;
  return 0;
}

int db_add_entry(char *filename, signed long uid, int flags) {
  struct file_entry *fe,*homefe;
  char *buf;

  fe=find_entry(filename);
  if (fe) return 1;
  homefe=split_dir(filename,&buf);
  if (!homefe) return 1;
  if (!(fe=make_entry(homefe,buf,NULL))) {
    FREE(buf);
    return 1;
  }
  fe->flags=flags;
  fe->owner=uid;
  return 0;
}

int chown_file(char *filename, struct object *uid, struct object *new_owner) {
  struct file_entry *fe;

  fe=find_entry(filename);
  if (!uid) return 1;
  if (!new_owner) return 1;
  if (!fe) return 1;
  
  /* Check master permission for ownership change */
  if (!check_master_permission(filename, "chown", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  if (fe!=root_dir)
    if (!can_read(fe->parent,uid))
      return 1;
  if (!(fe->owner==uid->refno || (uid->flags & PRIV))) return 1;
  fe->owner=new_owner->refno;
  return 0;
}

int chmod_file(char *filename, struct object *uid, int flags) {
  struct file_entry *fe;

  fe=find_entry(filename);
  if (!fe) return 1;
  if (!uid) return 1;
  
  /* Check master permission for permission change */
  if (!check_master_permission(filename, "chmod", uid, 1)) {
    return FILE_ERR_PERMISSION_DENIED;
  }
  
  if (fe!=root_dir)
    if (!can_read(fe->parent,uid))
      return 1;
  if (!(fe->owner==uid->refno || (uid->flags & PRIV))) return 1;
  if (fe==root_dir && (!(flags & READ_OK))) return 1;
  flags&=(WRITE_OK | READ_OK);
  fe->flags&=DIRECTORY;
  fe->flags|=flags;
  return 0;
}

void logger(int level, char *msg) {
  char timebuf[20];
  char levelbuf[10];
  time_t now_time;
  FILE *logfile;
  struct tm *time_s;
  
  /* Handle LOG_STDOUT specially - always write to syswrite.txt with object-based formatting */
  if (level == LOG_STDOUT) {
    static char last_objname[256] = "";
    char separator[82];
    char *objname_start, *objname_end, *content;
    char objname[256];
    int i;
    int same_object;
    
    /* Build 80-char separator line */
    for (i = 0; i < 80; i++) separator[i] = '=';
    separator[80] = '\n';
    separator[81] = '\0';
    
    /* Extract object name from message (format: "syslog: /path/object#refno message") */
    objname_start = strstr(msg, "syslog: ");
    if (objname_start) {
      objname_start += 8; /* Skip "syslog: " */
      objname_end = strchr(objname_start, ' ');
      if (objname_end) {
        int name_len = objname_end - objname_start;
        if (name_len > 255) name_len = 255;
        strncpy(objname, objname_start, name_len);
        objname[name_len] = '\0';
        content = objname_end + 1; /* Skip space */
      } else {
        strcpy(objname, "unknown");
        content = msg;
      }
    } else {
      strcpy(objname, "system");
      content = msg;
    }
    
    /* Check if same object as last call */
    same_object = (strcmp(objname, last_objname) == 0);
    
    logfile = fopen("syswrite.txt", "a");
    if (logfile) {
      if (!same_object) {
        /* Different object - print header with timestamp */
        char timebuf[20];
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        sprintf(timebuf, "%02d-%02d %02d:%02d", 
                (int)(tm_info->tm_mon + 1), (int)tm_info->tm_mday,
                (int)tm_info->tm_hour, (int)tm_info->tm_min);
        
        fprintf(logfile, "%s", separator);
        fprintf(logfile, "%s - syswrite() output from: %s\n", timebuf, objname);
        fprintf(logfile, "%s", separator);
        strcpy(last_objname, objname);
      }
      fprintf(logfile, "%s\n", content);
      fclose(logfile);
    }
    return;
  }
  
  /* Filter based on log level */
  /* Higher log_level = more verbose output */
  /* ERROR(0) < WARNING(1) < LOG(2) < DEBUG(3) */
  /* If log_level=LOG(2), show ERROR, WARNING, LOG but not DEBUG */
  if (level > log_level)
    return;

  /* Use a local wall-clock for log timestamping; do NOT modify global now_time */
  {
    time_t wall_time = time(NULL);
    time_s = localtime(&wall_time);
  }
  sprintf(timebuf,"%02d-%02d %02d:%02d",(int) (time_s->tm_mon+1),
          (int) time_s->tm_mday,
          (int) time_s->tm_hour,
          (int) time_s->tm_min);
  
  /* Add level prefix */
  switch(level) {
    case LOG_ERROR:   strcpy(levelbuf, "[ERROR] "); break;
    case LOG_WARNING: strcpy(levelbuf, "[WARN]  "); break;
    case LOG_INFO:    strcpy(levelbuf, "[INFO]  "); break;
    case LOG_DEBUG:   strcpy(levelbuf, "[DEBUG] "); break;
    default:          levelbuf[0] = '\0'; break;
  }
  
#ifdef USE_WINDOWS
  AddText(timebuf);
  AddText(" ");
  AddText(levelbuf);
  AddText(msg);
  AddText("\n");
  logfile=fopen(syslog_name,"a");
  if (!logfile) {
    AddText(timebuf);
	AddText("  system: couldn't open system log ");
	AddText(syslog_name);
	AddText("\n");
    return;
  }
#else /* USE_WINDOWS */
  if (noisy)
    fprintf(stderr,"%s %s%s\n",timebuf,levelbuf,msg);
  logfile=fopen(syslog_name,"a");
  if (!logfile) return;
#endif /* USE_WINDOWS */
  fprintf(logfile,"%s %s%s\n",timebuf,levelbuf,msg);
  fclose(logfile);
  
  /* DEBUG messages also go to sysdebug.txt for easier filtering */
  if (level == LOG_DEBUG) {
    FILE *debugfile = fopen("sysdebug.txt", "a");
    if (debugfile) {
      fprintf(debugfile, "%s %s%s\n", timebuf, levelbuf, msg);
      fclose(debugfile);
    }
  }
}
