#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#define NUM_THREADS 5
#define INCREMENTS 1000000

int global_counter = 0;
sem_t sem_lock; // 宣告信號量

void *increment_counter(void *arg) {
    for (int i = 0; i < INCREMENTS; i++) {
        
        // 1. 取得信號量 (如果為 0 則等待)
        sem_wait(&sem_lock);
        
        /* === 臨界區間 === */
        global_counter++;
        /* === 臨界區間結束 === */
        
        // 2. 釋放信號量
        sem_post(&sem_lock);
    }
    return NULL;
}

int main() {
    pthread_t tid[NUM_THREADS];

    // 3. 初始化信號量，初始值為 1
    // (第 2 個參數 0 表示此信號量由同一行程中的執行緒共享)
    sem_init(&sem_lock, 0, 1);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&tid[i], NULL, increment_counter, NULL);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

    // 4. 銷毀信號量
    sem_destroy(&sem_lock);

    printf("最終計數器結果: %d\n", global_counter);
    return 0;
}