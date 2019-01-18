#ifndef MAIN_H
#define MAIN_H

#define PORT          80
#define PATH          getenv("PWD")
#define WEBROOT       "/Web"
#define BUFFER        1024
#define MAX_THREADS   64
#define MAX_TASKS     65536

typedef struct server_t server_t;

int           init(server_t*);
int           setup_env(server_t*);
int           drop_privileges(server_t*);
void          connection(void* arg);
void          destroy(server_t*);
uid_t         getuid_byName(const char* name);
gid_t         getgid_byName(const char* name);
int           response(int client, char* res);
int           fresponse(int fd, int res);
int           request(int fd, char* req);


#endif
