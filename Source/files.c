#include <sys/stat.h>
#include <stdlib.h>

#include "files.h"

//Returns the size of a file in Bytes,
//else returns -1
off_t get_fsize(int fd) {
  struct stat* stats;
  
  stats = malloc(sizeof(struct stat));
  fstat(fd, stats);
  
  off_t size = stats->st_size;
  if (size < 0)
    return -1;
  
  free(stats);
    
  return size;
}

//returns the uid of a file
uid_t get_fuid(int fd) {
  struct stat* stats = malloc(sizeof(struct stat));
  fstat(fd, stats);
  
  uid_t user = stats->st_uid;
  
  free(stats);
  return user;
}

//returns the gid of a file
gid_t get_fgid(int fd) {
  struct stat* stats = malloc(sizeof(struct stat));
  
  fstat(fd, stats);
  gid_t group = stats->st_gid;
  
  free(stats);
  return group;
}

//returns last accessed time of a file
time_t get_faccessed(int fd) {
  struct stat* stats = malloc(sizeof(struct stat));
  
  fstat(fd, stats);
  time_t last_accessed = stats->st_atime;
  
  free(stats);
  return last_accessed;
}

//returns last modified time of a file
time_t get_fmodified(int fd) {
  struct stat* stats = malloc(sizeof(struct stat));
  
  fstat(fd, stats);
  time_t last_modified = stats->st_mtime;
  
  free(stats);
  return last_modified;
}
