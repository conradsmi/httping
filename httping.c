#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

#define MAX_URL_LEN 2048
#define MAX_PORT_LEN 6
#define PORT "80"
#define PAGE "/"

struct ping {
    int id;
    double resptime;
    int respsize;
    int errcode;
};

char helpmsg[] =
"\
HTTPing - Developed by Conrad Smith\n\
----------\n\
Sends -(-p)rofile number of GET requests to -(-u)rl\n\
and returns each response (along with some statistics)\n\
-----\n\
Options\n\
-----\n\
-h, --help: prints this help message and ends execution.\n\
-u, --url: the URL to send GET requests to. Can be of the following forms:\n\
\twww.google.com, www.google.com:80/index.html, www.google.com/, www.google.com:80\n\
\tIf the port and page is not specified, port 80 will be used and page \'/\' will\n\
\tbe used by default.\n\
\t(Note that the preceding http:// or https:// should not be included)\n\
-p, --profile: the number of GET requests to send. Each request is preceded\n\
\tby a one second break.\n\
";

// get sockaddr, IPv4 or IPv6:
void *getaddr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// return a socket to the specified ip on the specified port
int getsock(char *ip, char *port) {
    struct addrinfo hints, *res, *s;
    int errcode, sock, bytes_sent, len;
    char name[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // listen for any address
    if ((errcode = getaddrinfo(ip, port, &hints, &res)) != 0) {
        fprintf(stderr, "Error getting address info: %s\n", gai_strerror(errcode));
        return -1;
    }

    // iterate through received addresses and bind to first available
    for (s = res; s != NULL; s = s->ai_next) {
        if ((sock = socket(s->ai_family, s->ai_socktype, s->ai_protocol)) == -1) {
            continue;
        }

        if (connect(sock, s->ai_addr, s->ai_addrlen) == -1) {
            close(sock);
            continue;
        }

        break;
    }

    if (s == NULL) {
        perror("Could not connect to host");
        return -1;
    }

    // get name of client
    if (getpeername(sock, s->ai_addr, &(s->ai_addrlen)) == 0) {
        inet_ntop(s->ai_family, getaddr(s->ai_addr), name, sizeof(name));
        printf("Bound to host (%s); sending data...\n", name);
    }
    else {
        perror("Could not resolve client name");
        return -1;
    }

    freeaddrinfo(res);

    return sock;
}

// sort by time
int comptime(const void *p1, const void *p2) {
    return ((struct ping *)p1)->resptime > ((struct ping *)p2)->resptime;
}

// sort by id
int compid(const void *p1, const void *p2) {
    return ((struct ping *)p1)->id > ((struct ping *)p2)->id;
}

int main(int argc, char *argv[]) {
    int option, opt_index = 0, i;
    char *optname;
    int sockfd, profile = -1, urlsize, reqsize, domsize = 0, portsize = 0, pagesize = 0, status, cpysiz, buflen = 0;
    char *url = NULL, *request, port[MAX_PORT_LEN], domain[MAX_URL_LEN], page[MAX_URL_LEN], buf[BUFSIZ];
    clock_t before, after;
    double fastresp = (double)INT_MAX, slowresp = (double)INT_MIN, meanresp = 0, medresp = 0;
    double posrate = 0, errrate = 0, elapsed = 0;
    int maxresp = INT_MIN, minresp = INT_MAX;

    struct option long_options[] = {
        {"help", no_argument, 0, 0},
        {"url", required_argument, 0, 0},
        {"profile", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while ((option = getopt_long(argc, argv, ":hu:p:", long_options, &opt_index)) != -1) {
        switch (option) {
            case 0:
                optname = long_options[opt_index].name;
                switch (optname[0]) {
                    case 'h':
                        printf("%s", helpmsg);
                        exit(EXIT_SUCCESS);
                        break;
                    case 'u':
                        url = optarg;
                        break;
                    case 'p':
                        profile = atoi(optarg);
                        break;
                }
                break;
            case 'h':
                printf("%s", helpmsg);
                exit(EXIT_SUCCESS);
                break;
            case 'u':
                url = optarg;
                break;
            case 'p':
                profile = atoi(optarg);
                break;
            case ':':
                printf("Option needs a value\n");
                exit(EXIT_FAILURE);
                break;
            case '?':
                printf("Unknown option: %c\n", optopt);
                exit(EXIT_FAILURE);
                break;
        }
    }
    if (url == NULL || profile == -1) {
        printf("%s", helpmsg);
        exit(EXIT_SUCCESS);
    }

    // argument checking
    urlsize = strlen(url);
    if (urlsize > MAX_URL_LEN) {
        printf("URL is too long. Please enter a URL no longer than 2048 characters.\n");
        exit(EXIT_FAILURE);
    }
    if (profile == 0) {
        printf("The profile argument is either 0 or not a number. Please enter a valid profile.\n");
        exit(EXIT_FAILURE);
    }

    // clear garbage values
    memset(port, 0, MAX_PORT_LEN);
    memset(domain, 0, MAX_URL_LEN);
    memset(page, 0, MAX_URL_LEN);

    // parse the URL. (sorry, this is ugly)
    for (int i = 0; i < urlsize; i++) {
        // handle the domain
        if (url[i] == ':' && strcmp(domain, "") == 0) {
            snprintf(domain, (i + 1) * sizeof(char), "%s", url);
            // make user remove http/https
            if (strcmp(domain, "http") == 0 || strcmp(domain, "https") == 0) {
                printf("Please remove the protocol from the url (http:// or https://) and try again.\n");
                exit(EXIT_FAILURE);
            }
            domsize = strlen(domain);
        }
        // handle the port
        else if (url[i] == '/' && strcmp(port, "") == 0) {
            // if port not specified, initialize domain
            if (strcmp(domain, "") == 0) {
                cpysiz = (i == urlsize - 1) ? i + 1 : i;
                cpysiz *= sizeof(char);
                snprintf(domain, cpysiz, "%s", url);
                domsize = strlen(domain);
            }
            else {
                snprintf(port, i * sizeof(char) - domsize, "%s", url + domsize + 1);
                portsize = strlen(port);
            }
        }
        // handle the page
        else if (i == urlsize - 1 && strcmp(page, "") == 0) {
            if (strcmp(domain, "") == 0) {
                snprintf(domain, i * sizeof(char) + 2, "%s", url);
                domsize = strlen(domain);
            }
            else {
                snprintf(page, i - domsize - portsize, "%s", url + domsize + portsize + 2);
                pagesize = strlen(page);
            }
            break;
        }
    }
    // set defaults
    if (portsize == 0)
        strcpy(port, PORT);
    if (pagesize == 0)
        strcpy(page, PAGE);

    // form GET request
    request = calloc(urlsize + 128, sizeof(char));
    sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", page, domain);
    reqsize = strlen(request);
    // ignore broken pipe signal (execution will end otherwise)
    signal(SIGPIPE, SIG_IGN);

    // getsock handles most errors, but just in case...
    if((sockfd = getsock(domain, port)) == -1) {
        perror("Socket error");
        free(request);
        exit(EXIT_FAILURE);
    }
    int set = 1;

    struct ping pings[profile];
    // reset errno just in case
    errno = 0;
    // ping a profile# of times
    for (i = 0; i < profile; i++) {
        // space out the requests
        sleep(1);
        printf("Ping #%d...\n", i + 1);
        // track write/read time
        before = clock();
        send(sockfd, request, reqsize, MSG_NOSIGNAL);
        buflen = read(sockfd, buf, BUFSIZ);
        after = clock();
        // print the response
        printf("--------------------\n");
        printf("%s\n", buf);
        printf("--------------------\n");
        // convert time to ms
        elapsed = ((double)(after - before) / CLOCKS_PER_SEC) * 1000;
        // keep statistical data
        meanresp += elapsed;
        slowresp = (elapsed > slowresp) ? elapsed : slowresp;
        fastresp = (elapsed < fastresp) ? elapsed : fastresp;
        maxresp = (buflen > maxresp) ? buflen : maxresp;
        minresp = (buflen < minresp) ? buflen : minresp;
        errrate += (errno == 0) ? 0 : 1;

        // keep data on each ping
        pings[i].resptime = elapsed;
        pings[i].respsize = buflen;
        pings[i].errcode = errno;
        pings[i].id = i + 1;
        // reset the errno in case it was set
        errno = 0;
    }
    // finalize mean and success rate
    meanresp /= profile;
    posrate = 1 - (errrate / profile);

    // sort pings by time taken
    qsort(pings, profile, sizeof(struct ping), comptime);
    // calculate median
    if (profile % 2 == 0)
        medresp = (pings[profile / 2 - 1].resptime + pings[profile / 2].resptime) / (double)2;
    else
        medresp = pings[profile / 2].resptime;

    // print results
    printf("Results for %d HTTP GET requests to host %s\n", profile, domain);
    printf("--------------------\n");
    printf("Fastest response time: %.3lfms\n", fastresp);
    printf("Slowest response time: %.3lfms\n", slowresp);
    printf("Mean response time: %.3lfms\n", meanresp);
    printf("Median response time: %.3lfms\n", medresp);
    printf("Size (in bytes) of largest response: %d\n", maxresp);
    printf("Size (in bytes) of smallest response: %d\n", minresp);
    printf("Success rate: %%%.2lf\n", posrate * 100);
    printf("All error codes are listed below\n");
    printf("----------\n");

    // sort by id and print each code in order
    qsort(pings, profile, sizeof(struct ping), compid);
    for (int i = 0; i < profile; i++) {
        if (pings[i].errcode != 0)
            printf("Ping #%d: %s\n", pings[i].id, strerror(pings[i].errcode));
    }

    // cleanup
    free(request);

    return 0;
}