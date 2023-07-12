#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

struct shm_mem_t{
    int size;
    int data[100];
    int res_sum;
};

int main(int argc, char **argv){

    if(argc < 2){
        printf("wrong number of arguments\n");
        return 9;
    }
    FILE *f = fopen(*(argv + 1),"rt");
    if(f == NULL){
        printf("can't open file\n");
        return 2;
    }

    int size = 0;
    int tab[100];
    while (!feof(f) && size < 100){
        int res = fscanf(f,"%d\n",&tab[size]);
        if(res != 1){
            printf("file error\n");
            return 2;
        }
        size++;
    }
    fclose(f);

    sem_t *sem_server = sem_open("sem_server",O_CREAT | O_EXCL, 0777, 0);
    if(sem_server == NULL){
        sem_server = sem_open("sem_server",O_CREAT, 0777, 0);
        if(sem_server == NULL){
            return 3;
        }
    } else{
        printf("server doesn't exist\n");
        sem_unlink("sem_server");
        sem_close(sem_server);
        return 3;
    }

    sem_t *sem_client = sem_open("sem_client",O_CREAT, 0777, 0);
    if(sem_server == NULL){
        return 3;
    }

    int mem = shm_open("shm_server", O_CREAT | O_EXCL | O_RDWR, 0777);
    if(ftruncate(mem, sizeof(struct shm_mem_t))){
        printf("server is full\n");
        return 3;
    }
    struct shm_mem_t *mem_t = mmap(NULL, sizeof(struct shm_mem_t), PROT_READ | PROT_WRITE,
            MAP_SHARED, mem, 0);
    if(mem_t == NULL){
        printf("mmap error\n");
        return 3;
    }

    sem_post(sem_server);
    sem_wait(sem_client);

    mem_t->size = size;
    for (int i = 0; i < size; ++i) {
        mem_t->data[i] = tab[i];
    }

    sem_post(sem_server);
    sem_wait(sem_client);

    int suma = mem_t->res_sum;
    printf("\n\nsuma of elements %d\n\n",suma);

    munmap(mem_t, sizeof(struct shm_mem_t));
    shm_unlink("shm_server");

    sem_post(sem_server);

    return 0;
}