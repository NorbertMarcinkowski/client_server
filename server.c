#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

int QUIT = 0;
int work = 0;
int STAT = 0;
struct shm_mem_t {
    int size;
    int data[100];
    int res_sum;
};
pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;

void *stat_and_quit() {
    while (1) {
        char tab[20];
        printf("what do you want? \n");
        scanf("%19s", tab);
        scanf("%*[^\n]s");
        if (strcmp("quit", tab) == 0) {
            pthread_mutex_lock(&MUTEX);
            QUIT = 1;
            pthread_mutex_unlock(&MUTEX);
            break;
        } else if (strcmp("stat", tab) == 0) {
            pthread_mutex_lock(&MUTEX);
            long stat = STAT;
            pthread_mutex_unlock(&MUTEX);
            printf("\nstats: %ld\n\n", stat);
        } else {
            printf("error\n");
        }
    }
    return NULL;
}

void *sum_fun() {

    sem_t *sem_server = sem_open("sem_server", O_CREAT | O_EXCL, 0777, 0);
    if (sem_server == NULL) {
        return NULL;
    }
    sem_t *sem_client = sem_open("sem_client", O_CREAT | O_EXCL, 0777, 0);
    if (sem_server == NULL) {
        sem_close(sem_server);
        sem_unlink("sem_server");
        return NULL;
    }

    while (1) {
        pthread_mutex_lock(&MUTEX);
        int quit = QUIT;
        pthread_mutex_unlock(&MUTEX);
        if (quit == 1) {
            break;
        }

        pthread_mutex_lock(&MUTEX);
        work = 0;
        pthread_mutex_unlock(&MUTEX);

        sem_wait(sem_server);

        pthread_mutex_lock(&MUTEX);
        work = 1;
        pthread_mutex_unlock(&MUTEX);

        int mem = shm_open("shm_server", O_RDWR | O_RDONLY, 0777);
        if (ftruncate(mem, sizeof(struct shm_mem_t))) {
            sem_close(sem_server);
            sem_unlink("sem_server");
            sem_close(sem_client);
            sem_unlink("sem_client");
            break;
        }
        struct shm_mem_t *mem_t = mmap(NULL, sizeof(struct shm_mem_t),
                PROT_READ | PROT_WRITE, MAP_SHARED, mem, 0);
        if (mem_t == NULL) {
            sem_close(sem_server);
            sem_unlink("sem_server");
            sem_close(sem_client);
            sem_unlink("sem_client");
            return NULL;
        }

        pthread_mutex_lock(&MUTEX);
        STAT += 1;
        pthread_mutex_unlock(&MUTEX);
        
        sem_post(sem_client);
        sem_wait(sem_server);

        int sum = 0;
        for (int i = 0; i < mem_t->size; ++i) {
            sum += mem_t->data[i];
        }
        mem_t->res_sum = sum;
        sem_post(sem_client);
        sem_wait(sem_server);

    }
    sem_unlink("sem_server");
    sem_unlink("sem_client");
    sem_close(sem_server);
    sem_close(sem_client);

    return NULL;
}

int main() {
    pthread_t server;
    pthread_t client;
    pthread_create(&server, NULL, stat_and_quit, NULL);
    pthread_create(&client, NULL, sum_fun, NULL);
    pthread_join(server, NULL);

    pthread_mutex_lock(&MUTEX);
    int w = work;
    pthread_mutex_unlock(&MUTEX);

    sem_t *sem_server = sem_open("sem_server", O_CREAT, 0777, 0);
    if (w == 0 && sem_server != NULL) {
        sem_post(sem_server);
        sem_close(sem_server);
        sem_unlink("sem_server");
    }
    pthread_join(client, NULL);
    return 0;
}
