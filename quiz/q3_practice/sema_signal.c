#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h> // for EAGAIN
#include <stdlib.h> // 為了 exit() 和 perror()

sem_t data_ready_signal;
char *shared_data = NULL; // 共享的資料

void *consumer_thread(void *arg) {
    printf("Consumer: 啟動，開始檢查資料...\n");
    
    // 1. 嘗試取得信號
    while (sem_trywait(&data_ready_signal) != 0) {
        // 2. 檢查錯誤
        if (errno == EAGAIN) {
            // EAGAIN 代表信號量為 0 (資源不可用)
            printf("Consumer: 資料還沒好，我先做點別的事 (1 秒)...\n");
            sleep(1);
        } else {
            // 發生其他錯誤
            perror("sem_trywait error");
            break;
        }
    }

    // 3. 成功取得信號 (sem_trywait 返回 0)
    printf("Consumer: 收到信號！開始處理資料: '%s'\n", shared_data);
    return NULL;
}

void *producer_thread(void *arg) {
    printf("Producer: 啟動，正在準備資料 (需要 3 秒)...\n");
    sleep(3);
    
    shared_data = "這是一筆重要的資料";
    printf("Producer: 資料準備完成，發出信號！\n");
    
    // 4. 釋放信號 (將計數器從 0 變 1)
    sem_post(&data_ready_signal); 
    return NULL;
}

int main() {
    pthread_t producer, consumer;

    // 5. 初始化信號量，初始值為 0 (代表資料還沒準備好)
    if (sem_init(&data_ready_signal, 0, 0) != 0) {
        perror("Semaphore 初始化失敗");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&consumer, NULL, consumer_thread, NULL) != 0) {
        perror("Consumer thread 建立失敗");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&producer, NULL, producer_thread, NULL) != 0) {
        perror("Producer thread 建立失敗");
        exit(EXIT_FAILURE);
    }

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    sem_destroy(&data_ready_signal);
    return 0;
}