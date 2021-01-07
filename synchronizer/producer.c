#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h> 
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/file.h>

#define SHM_SIZE_KEY 3000
#define SHM_BUFF_KEY 4000
#define FULL_SEM_KEY 300
#define EMPTY_SEM_KEY 400
#define MUTEX_SEM_KEY 500


int *memsizeAddr, *buff;
int full, empty, mutex, buffid, memsizeid;

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void Down(int sem);
void Up(int sem);
void Init(int buffsize);
int* InitBuff(int memsize);
int CreateSem(int semkey);
void DestroySem(int sem);
void FreeResources();
void SignalHandler(int signum);

int main(){
    int buffsize;
    int memsize;

    printf("\n\nProducer: please enter the buffer size: ");
    scanf("%d", &buffsize);
    if(buffsize <= 0){
        perror("\n\nProducer: invalid buffer size\n");
        exit(-1);
    }
    buffsize--;
    memsize = buffsize*sizeof(int);

    Init(buffsize);

    buff = InitBuff(memsize);

    full = CreateSem(FULL_SEM_KEY);
    empty = CreateSem(FULL_SEM_KEY);
    mutex = CreateSem(FULL_SEM_KEY);

    signal(SIGINT, SignalHandler);

    FreeResources();

    return 0;
}


void Down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("\n\nProducer: Error in down()\n");
        exit(-1);
    }
}

void Up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("\n\nProducer: Error in up()\n");
        exit(-1);
    }
}

void Init(int buffsize)
{
    memsizeid = shmget(SHM_SIZE_KEY, sizeof(int), IPC_CREAT|0644);
    if (memsizeid == -1)
    {
        perror("\n\nProducer: Error in create the shared memory for buffer size\n");
        exit(-1);
    }
    
    memsizeAddr = (int *)shmat(memsizeid, (void *)0, 0);
    if (memsizeAddr == -1)
    {
        perror("\n\nProducer: Error in attach the for buffer size shared memory\n");
        exit(-1);
    }
    
    *memsizeAddr = buffsize;
}

int* InitBuff(int memsize)
{
    buffid = shmget(SHM_BUFF_KEY, memsize, IPC_CREAT|0644);
    if (buffid == -1)
    {
        perror("\n\nProducer: Error in create the shared memory for buffer size\n");
        exit(-1);
    }
    else printf("\n\nProducer: Buffer id: %d\n", buffid);
    
    int* buff;
    buff = (int *)shmat(buffid, (void *)0, 0);
    if (buff == -1)
    {
        perror("\n\nProducer: Error in attach the for buffer\n");
        exit(-1);
    }
    else printf("\n\nProducer: Buffer attached successfuly\n");

    return buff;
    
}

int CreateSem(int semkey)
{
    int sem = semget(semkey, 1, 0666 | IPC_CREAT);
    if(sem == -1){
        perror("\n\nProducer: Error in create sem\n");
        exit(-1);
    }
    return sem;
}

void DestroySem(int sem)
{
    if(semctl(sem, 0, IPC_RMID) == -1){
        perror("\n\nProducer: Error in destroy sem\n");
        exit(-1);
    }
}

void FreeResources()
{
    shmdt((void*)buff);
    shmdt((void*)memsizeAddr);

    DestroySem(full);
    DestroySem(empty);
    DestroySem(mutex);

    shmctl(memsizeid, IPC_RMID, NULL);
    shmctl(buffid, IPC_RMID, NULL);
}

void SignalHandler(int signum)
{
    printf("\n\nProducer: terminating...\n");
    FreeResources();
    exit(0);
}