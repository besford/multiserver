#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#include "main.h"
#include "threadpool.h"
#include "files.h"
#include "response.h"
#include "mime.h"
#include "errors.h"

struct server_t {
    int sockfd, newsockfd;
    socklen_t client_len;
    struct sockaddr_in server_addr, client_addr;
    threadpool_t *workers;
    gid_t gid;
    uid_t uid;
};

//global array for supported file mimetypes of server
mime_t files[] = {
    {"txt", "text/plain"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"jpg", "image/jpg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"ico", "image/ico"},
    {"gif", "image/gif"},
    {"pdf", "application/pdf"},
    {"js", "application/javascript"},
    {0, 0}};

char path[BUFFER];

static int volatile running = 1;

void quit_handler(int derp) {
    //quit signal received, set running flag to 0
    printf("\nStopping server\n");
    running = 0;
}

int main() {
    //allocate memory for server
    server_t *server = (server_t *)malloc(sizeof(server_t));
    if (server == NULL)
        error("Failed to allocate memory for server");

    //setup server environment
    printf("Setuping up server environment\n");
    if (setup_env(server) < 0)
        error("Failed to setup server environment, terminating");

    //initialize server
    printf("Initalizing server\n");
    if (init(server) < 0)
        error("Failed to init server, terminating");

    //drop privileges down to user level
    if (drop_privileges(server) < 0)
        error("Failed to drop privileges, terminating");

    printf("Server start successful!\n");
    printf("Server running on PATH: \"%s\" PORT: %i (CTRL+C to close)\n", path, PORT);

    //listen for clients
    printf("Listening for clients\n");
    listen(server->sockfd, 5);
    server->client_len = sizeof(struct sockaddr_in);

    //initialize quit handler
    struct sigaction quit;
    quit.sa_handler = quit_handler;
    quit.sa_flags = 0;
    sigemptyset(&quit.sa_mask);
    sigaction(SIGINT, &quit, NULL);

    //main server loop
    while (running) {
        //schedule connection request to be handled
        server->newsockfd = accept(server->sockfd, (struct sockaddr *)&server->client_addr, &server->client_len);
        threadpool_schedule(server->workers, &connection, &server->newsockfd);
    }

    destroy(server);

    return 0;
}

int init(server_t *server) {
    //open streamsocket over INET
    //note that steamsockets use TCP protocol
    printf("--opening socket\n");
    server->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->sockfd < 0) {
        exception("Error opening socket");
        destroy(server);
        return -1;
    }

    //configure server socket for immediate reuse after server termination
    printf("--configuring socket\n");
    int enable = 1;
    if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        exception("Failed to setup socket");
        destroy(server);
        return -2;
    }

    //initialize server addr_in
    memset((char *)&server->server_addr, '\0', sizeof(struct sockaddr_in));
    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port = htons(PORT);

    //attempt to bind to PORT
    printf("--attempting to bind to port: %i\n", PORT);
    if (bind(server->sockfd, (struct sockaddr *)&server->server_addr, sizeof(struct sockaddr_in)) < 0) {
        exception("Failed to bind port");
        destroy(server);
        return -3;
    }

    //create threadpool
    printf("--creating worker threads\n");
    server->workers = threadpool_create(8, 32);
    if (server->workers == NULL) {
        exception("Failed to create threadpool");
        destroy(server);
        return -4;
    }

    return 0;
}

//setups necessary OS conditions for server to execute
//returns 0 if successful, else returns a negative value to indicate an error
int setup_env(server_t *server) {
    //check if root user
    printf("--configuring user and group settings\n");
    if (getuid() != 0) {
        exception("Server must be run as root");
        return -1;
    }

    //check if user is already made, if not, make user
    uid_t usrid = getuid_byName("multiserver");
    if ((int)usrid < 0) {
        system("useradd multiserver");
        usrid = getuid_byName("multiserver");
        server->uid = usrid;
    }
    else
        server->uid = usrid;

    //Test if group is already made, if not, system call to make group
    gid_t grpid = getgid_byName("multiserver");
    if ((int)grpid < 0) {
        system("groupadd multiserver");
        grpid = getgid_byName("multiserver");
        server->gid = grpid;
    }
    else
        server->gid = grpid;

    //get server working directory
    if (getcwd(path, sizeof(path)) == NULL) {
        exception("Unable to get server directory");
        return -2;
    }

    //append webroot to path
    printf("--initializing path\n");
    strcat(path, WEBROOT);

    //Change ownership
    if (chown(path, server->uid, server->gid) == -1) {
        exception("Could not change owner");
        return -2;
    }

    //Used for setting permissions
    printf("--configuring server permissions\n");
    char permission[] = "0777";
    int temp = strtoul(permission, 0, 8);

    //Change permission bits
    system("chmod -R 777 Web");
    if (chmod(path, temp) < 0) {
        exception("Could not change permission bits");
        return -3;
    }

    //set ownership
    char own[BUFFER];
    memset(own, '\0', sizeof(own));
    sprintf(own, "chown -R %i.%i Web", (int)server->uid, (int)server->gid);
    system(own);
    if (chown(path, server->uid, server->gid) < 0) {
        exception("Could not get ownership of webroot");
        return -4;
    }

    //change server root to webroot
    if (chdir(path) < 0) {
        exception("Could not change dir");
        return -5;
    }

    //set webroot as root directory for server process
    if (chroot(path) < 0) {
        exception("Could not change server root");
        return -6;
    }

    return 0;
}

int drop_privileges(server_t *server) {
    //drop group from root to user level
    if (setgid(server->gid) < 0) {
        exception("Could not change groupId");
        return -1;
    }

    //drop user from root to user
    if (setuid(server->uid) < 0) {
        exception("Could not change userID");
        return -2;
    }

    return 0;
}

//handles incoming connections
void connection(void *arg) {
    char req[BUFFER], res[BUFFER], tmp[BUFFER];
    char *ptr;
    int file, len;

    //Initialize file descriptor for client
    int client = *((int *)arg);

    //check if connection was successful
    if (client < 0) {
        exception("Failed to establish connection with client");
        return;
    }
    else
        printf("Connection established with client\n");

    //initialize buffers
    memset(req, '\0', sizeof(req));
    memset(res, '\0', sizeof(res));
    memset(tmp, '\0', sizeof(tmp));

    //read request from client
    len = read(client, req, sizeof(req) - 1);
    if (len < 0) {
        exception("Failed  to read from socket");
        return;
    }
    else {
        printf("Request from client: ");
        printf("%s\n", req);
    }

    //handle request
    ptr = strstr(req, " HTTP/");
    if (ptr == NULL)
        printf("Not an HTTP request\n");
    else {
        *ptr = 0;
        ptr = NULL;

        //get the request type, currently only handling HTTP GET
        if (strncmp(req, "GET ", 4) == 0)
            ptr = req + 4;

        if (ptr == NULL)
            response(client, bad_req);
        else {
            if (ptr[strlen(ptr) - 1] == '/')
                strcat(ptr, "index.html");

            //setup path for opening files
            strcat(res, ptr);
            char *s = strchr(ptr, '.');

            //check all supported mimtypes to determine if
            //requested file is supported
            int cur = 0;
            while (files[cur].extension != 0) {
                //if mimetype supported
                if (strcmp(s + 1, files[cur].extension) == 0) {
                    //open file
                    file = open(res, O_RDONLY, 0);
                    printf("Opening \"%s\"\n", res);

                    //handle errors while accessing file
                    if (file < 0) {
                        //file does not exist
                        if (errno == ENOENT) {
                            printf("404 File not found\n");
                            response(client, not_found);
                        }

                        //access denied due to lack of read permissions
                        if (errno == EACCES) {
                            printf("403 Forbidden Access\n");
                            response(client, forbidden);
                        }
                    }
                    else {
                        //check if owner of file, nandle if not
                        if (getuid() != get_fuid(file)) {
                            printf("403 Forbidden Access\n");
                            response(client, forbidden);
                            return;
                        }

                        //send response header to client
                        memset(tmp, '\0', sizeof(tmp));
                        printf("200 OK, Content-Type: %s\n", files[cur].type);
                        response(client, ok);
                        sprintf(tmp, "Content-Type: %s\n", files[cur].type);
                        response(client, tmp);
                        sprintf(tmp, "Content-Length: %i\n\n", (int)get_fsize(file));
                        response(client, tmp);

                        //send response data to client
                        if (ptr == req + 4) {
                            //handle error during send
                            if (fresponse(client, file) < 0) {
                                exception("Failed to send file to client");
                                return;
                            }
                        }
                    }

                    // done handling request 
                    break;
                }
                //handled Unsupported mimetype
                if (cur == (sizeof(files) / sizeof(files[0]) - 2)) {
                    printf("415 Unsupported Media Type\n");
                    response(client, unsupported_media);
                }

                //increment current file.
                cur++;
            }
        }
    }

    //cleanup connection with client
    close(client);
    shutdown(client, SHUT_RDWR);

    return;
}

//Returns the gid associated with a input username if they exist on system
//else return -1
gid_t getgid_byName(const char *name) {
    struct group *group = getgrnam(name);
    if (group == NULL) return -1;
    return group->gr_gid;
}

//Returns the uid associated with an input username if they exist on system
//else return -1
uid_t getuid_byName(const char *name) {
    struct passwd *user = getpwnam(name);
    if (user == NULL) return -1;
    return user->pw_uid;
}

//Sends response to client, returns the number of Bytes sent
//else returns -1
int response(int client, char *res) {
    int len = strlen(res);
    //If write failed, return -1
    if (write(client, res, len) < 0) return -1;
    //otherwise, return number of bytes sent
    return (len + 1) * sizeof(char);
}

//Sends a file to client, returning the number of bytes sent.
//else returns -1
int fresponse(int client, int res) {
    //check if file has valid len
    int len = 0;
    if ((len = (int)get_fsize(res)) < 0) return -1;

    //keep sending until buffer is empty
    int total_sent = 0;
    int sent = 0;
    while (total_sent < len) {
        sent = sendfile(client, res, 0, len - total_sent);
        //check if failed to send
        if (sent <= 0) return -2;
        total_sent += sent;
    }

    return get_fsize(res);
}

//Receives a request from a client, returns the number of
//Bytes received. Not used
int request(int client, char *req) { return 0; }

//Responsible for cleaning up the server
void destroy(server_t *server) {
    printf("Destroying server\n");

    //signal threadpool workers to stop accepting connections, then destroy threads
    if (server->workers != NULL) threadpool_destroy(server->workers);

    //close server sockets
    close(server->sockfd);
    shutdown(server->sockfd, SHUT_RDWR);
    shutdown(server->newsockfd, SHUT_RDWR);

    //destruct server
    free(server);
}
