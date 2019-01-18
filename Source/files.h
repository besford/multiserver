#ifndef FILES_H
#define FILES_H

off_t get_fsize(int fd);
uid_t get_fuid(int fd);
gid_t get_fgid(int fd);
time_t get_faccessed(int fd);
time_t get_fmodified(int fd);

#endif
