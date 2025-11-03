#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

/* 1. 宣告一個全域的互斥鎖 */
pthread_mutex_t stack_lock = PTHREAD_MUTEX_INITIALIZER;

/* 全域變數 (共享資源) */
int count = 0;
int stack[10];

/* 修正後的 push 函式 */
void push(int n) {
    pthread_mutex_lock(&stack_lock);

    if (count < 10) {  // 檢查 Stack 是否滿
        stack[count] = n;
        count++;
        printf("Push: %d, Stack size: %d\n", n, count);
    } else {
        printf("Stack is full, cannot push %d\n", n);
    }

    pthread_mutex_unlock(&stack_lock);
}

/* 修正後的 pop 函式 */
int pop() {
    pthread_mutex_lock(&stack_lock);

    int value = -1; // 若stack為空則回傳-1以示錯誤
    if (count > 0) {
        count--;
        value = stack[count];
        printf("Pop: %d, Stack size: %d\n", value, count);
    } else {
        printf("Stack is empty, cannot pop\n");
    }

    pthread_mutex_unlock(&stack_lock);

    return value;
}

/* 執行緒專用的 push 測試函式 */
void* thread_push(void* arg) {
    int i;
    for (i = 0; i < 6; i++) {
        push(i + 1);
        usleep(100000); // 模擬delay
    }
    pthread_exit(NULL);
}

/* 執行緒專用的 pop 測試函式 */
void* thread_pop(void* arg) {
    int i;
    for (i = 0; i < 6; i++) {
        pop();
        usleep(150000); // 模擬delay
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t tid1, tid2;

    // 建立一個 push 執行緒、一個 pop 執行緒
    pthread_create(&tid1, NULL, thread_push, NULL);
    pthread_create(&tid2, NULL, thread_pop, NULL);

    // 等待兩個執行緒結束
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    printf("All threads done.\n");

    // 釋放互斥鎖資源
    pthread_mutex_destroy(&stack_lock);

    return 0;
}
