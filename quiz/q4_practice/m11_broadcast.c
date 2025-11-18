#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_WORKERS 5

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ready = 0; // 共享狀態：0=未準備好, 1=準備好

void *worker(void *arg) {
    int id = *(int *)arg;
    
    pthread_mutex_lock(&lock);
    printf("Worker %d: 就位，等待信號...\n", id);
    
    // 等待 ready 變成 1
    while (ready == 0) {
        pthread_cond_wait(&cond, &lock);
    }
    // 醒來後繼續執行
    printf("Worker %d: 收到信號！開始工作！\n", id);
    pthread_mutex_unlock(&lock);
    
    return NULL;
}

int main() {
    pthread_t threads[NUM_WORKERS];
    int ids[NUM_WORKERS];

    // 1. 建立所有工作執行緒
    for (int i = 0; i < NUM_WORKERS; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    // 模擬主執行緒正在準備資料
    printf("[Main]: 正在準備資料 (2秒)...\n");
    sleep(2);

    // 2. 修改狀態並廣播
    pthread_mutex_lock(&lock);
    ready = 1; // 修改狀態
    printf("[Main]: 資料準備完成，廣播通知所有執行緒！\n");
    pthread_cond_broadcast(&cond); // 喚醒所有睡在 cond 的執行緒
    pthread_mutex_unlock(&lock);

    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}