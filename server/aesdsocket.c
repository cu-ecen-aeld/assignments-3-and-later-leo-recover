#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define MYPORT "9000"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 200 // max number of bytes we can get at once 

#define FILE_PATH "/var/tmp/aesdsocketdata"

int sockfd, newfd;
FILE *f = NULL;
char *buf = NULL;

static void signal_handler(int sig_number)
{
    int errno_saved = errno;
    syslog(LOG_INFO, "Caught signal, exiting");
    if (buf) {
        free(buf);
    }
    if (f) {
        fclose(f);
    }
    remove(FILE_PATH);
    // shutdown(sockfd);
    errno = errno_saved;
    exit(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int status, numbytes, total_len=0; 
    int init_buf=1; // flag to initialize the buffer
    pid_t pid;

    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct addrinfo hints;
    struct addrinfo *servinfo;  // will point to the results
    char s[INET_ADDRSTRLEN];
    char buf_recv[MAXDATASIZE];
    char *ptr = NULL;
    char *line = NULL;

    struct sigaction exit_action;

    // initialize the buffers
    memset(buf_recv, 0, sizeof buf_recv);
    memset(s, 0, sizeof s);
    
    openlog(NULL, 0, LOG_USER);
    syslog(LOG_DEBUG, "aesdsocket starting");

    // register the action
    memset(&exit_action, 0, sizeof exit_action);
    exit_action.sa_handler = signal_handler;

    if( sigaction(SIGTERM, &exit_action, NULL) != 0 ) {
        perror("Error registering for SIGTERM");
    }
    if( sigaction(SIGINT, &exit_action, NULL) ) {
        perror("Error registering for SIGINT");
    }

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        syslog(LOG_ERR, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(-1);
    }

    // get the socket
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    int yes=1;

    // lose the pesky "Address already in use" error message
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    } 

    // bind it to the port we passed in to getaddrinfo():
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
        syslog(LOG_ERR, "Bind error!");
        perror("bind");
        freeaddrinfo(servinfo);
        exit(-1);
    }
    freeaddrinfo(servinfo);

    // Parse -d argument
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        syslog(LOG_INFO, "Daemon mode requested... forking");
        pid = fork();
    } else {
        pid = 0;
    }

    switch(pid) {
        case -1:
        break;

        case 0:
            // start listening
            if (listen(sockfd, BACKLOG) != 0) {
                syslog(LOG_ERR, "Listen error!");
                exit(-1);
            }

            // accept loop
            while (1) {
                syslog(LOG_DEBUG, "Waiting for the connection...");
                sin_size = sizeof their_addr;
                newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
                if (newfd == -1) {
                    perror("accept");
                    exit(-1);
                }

                inet_ntop(their_addr.ss_family,
                    get_in_addr((struct sockaddr *)&their_addr),
                    s, sizeof s);
                syslog(LOG_INFO, "Accepted connection from %s\n", s);

                f = fopen(FILE_PATH, "a+");
                if (f == NULL) {
                    perror("file open");
                    exit(-1);
                }

                // recv inner loop
                while ((numbytes = recv(newfd, buf_recv, MAXDATASIZE-1, 0)) != 0) {
                    if (numbytes == -1) {
                        perror("recv");
                        exit(-1);
                    }
                    syslog(LOG_INFO, "Packet received: %s\n", buf_recv);
                    
                    // buffer it until we get the newline
                    total_len += (numbytes+1);
                    syslog(LOG_DEBUG, "sizeof buf = %d\n", total_len);
                    if (init_buf) {
                        buf = malloc(total_len);
                        memset(buf, 0, total_len);
                        init_buf = 0;    
                    } else {
                        buf = realloc(buf, total_len);
                        memset(buf + total_len - numbytes, 0, numbytes);
                    }
                    if (buf == NULL) {
                        perror("allocation");
                        exit(-1);
                    }
                    
                    strncat(buf, buf_recv, numbytes);
                    syslog(LOG_DEBUG, "Buffer content: %s\n", buf);
                    
                    // flush to file if \n is found
                    if ((ptr = memchr(buf, '\n', total_len))) {
                        syslog(LOG_DEBUG, "Flushing to file %ld bytes", ptr-buf+1);
                        fwrite(buf, sizeof(char), ptr-buf+1, f);
                        free(buf);
                        buf = NULL;
                        init_buf = 1;
                        total_len = 0;

                        // return the full content of the file to the client
                        int nread; 
                        size_t len;
                        fseek(f, 0L, SEEK_SET);
                        while ((nread = getline(&line, &len, f)) != -1) {
                            syslog(LOG_DEBUG, "Sending line: %s", line);
                            if (send(newfd, line, nread, 0) == -1) {
                                perror("Send");
                                exit(-1);
                            }
                        }
                        free(line);
                        line = NULL;
                    }
                }
                syslog(LOG_INFO, "Closed connection from %s\n", s);
                free(buf);
                buf = NULL;
                fclose(f);
                f = NULL;
            }
        break;

        default:
        break;
    }
    exit(EXIT_SUCCESS);
}