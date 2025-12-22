#include "stack.h"

void push(long pid, char ch, struct shmbuf *shmp) {
    // 進入臨界區間 (Critical Section)
    if (sem_wait(&shmp->sem) == -1)
        errorHandler("sem_wait");

    if (shmp->top == -1) {
        printf("[Process %ld] Stack is full, cannot push '%c'\n", pid, ch);
    } else {
        int top = shmp->top;
        shmp->stack[top] = ch;
        shmp->top--;
        printf("[Process %ld] Pushed '%c' (top now at index %d)\n", pid, ch, shmp->top);
    }

    // 離開臨界區間
    if (sem_post(&shmp->sem) == -1)
        errorHandler("sem_post");
}

char pop(long pid, struct shmbuf *shmp) {
    // 進入臨界區間
    if (sem_wait(&shmp->sem) == -1)
        errorHandler("sem_wait");

    char ch = '\0';
    if (shmp->top == STACK_SIZE - 1) {
        printf("[Process %ld] Stack is empty, cannot pop\n", pid);
    } else {
        int top = shmp->top;
        ch = shmp->stack[top + 1];
        shmp->top++;
        printf("[Process %ld] Popped '%c' (top now at index %d)\n", pid, ch, shmp->top);
    }

    // 離開臨界區間
    if (sem_post(&shmp->sem) == -1)
        errorHandler("sem_post");

    return ch;
}

void print_stack(struct shmbuf *shmp) {
    // 注意：這裡其實也應該受 Semaphore 保護，
    // 但為了觀察並發時的交錯情況，有時會故意不加鎖，
    // 既然題目邏輯是為了Debug，這裡我們假設讀取是安全的或僅作示意。
    
    printf("--- Current Stack Content ---\n");
    // 從堆疊底部 (index STACK_SIZE-1) 還是頂部印出取決於習慣，
    // 這裡依照原始邏輯，印出有效資料
    if (shmp->top == STACK_SIZE - 1) {
        printf("(Empty)\n");
    } else {
        for (int i = shmp->top + 1; i < STACK_SIZE; i++) {
            printf("[%d]: %c\n", i, shmp->stack[i]);
        }
    }
    printf("-----------------------------\n");
}

int main(int argc, char *argv[]) {
    int fd;
    struct shmbuf *shmp;

    // 1. 確保環境乾淨：先嘗試刪除舊的共享記憶體物件
    shm_unlink(SHM_PATH);

    // 2. 建立共享記憶體
    fd = shm_open(SHM_PATH, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) errorHandler("shm_open");

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
        errorHandler("ftruncate");

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errorHandler("mmap");

    shmp->top = STACK_SIZE - 1; 

    // 初始化 Semaphore，設為 0 代表一開始鎖住，由 Parent 開啟
    // 第二個參數 1 表示 process-shared
    if (sem_init(&shmp->sem, 1, 0) == -1)
        errorHandler("sem_init");

    // 4. 產生第一個子行程
    pid_t pid = fork();
    if (pid == -1) errorHandler("fork 1");

    if (pid == 0) { // Child 1
        long child1_pid = (long)getpid();
        
        push(child1_pid, 'A', shmp);
        print_stack(shmp);

        push(child1_pid, 'B', shmp);
        print_stack(shmp);

        printf("Process %ld sleep for 2 seconds...\n", child1_pid);
        sleep(2);

        pop(child1_pid, shmp);
        print_stack(shmp);

        pop(child1_pid, shmp);
        print_stack(shmp);

        push(child1_pid, 'C', shmp);
        print_stack(shmp);

        printf("Process %ld done.\n", child1_pid);
        exit(EXIT_SUCCESS);
    }

    // 5. 產生第二個子行程
    pid = fork();
    if (pid == -1) errorHandler("fork 2");

    if (pid == 0) { // Child 2
        long child2_pid = (long)getpid();

        push(child2_pid, 'a', shmp);
        print_stack(shmp);

        pop(child2_pid, shmp);
        print_stack(shmp);

        push(child2_pid, 'b', shmp);
        print_stack(shmp);

        push(child2_pid, 'c', shmp);
        print_stack(shmp);

        printf("Process %ld done.\n", child2_pid);
        exit(EXIT_SUCCESS);
    }

    // 6. 父行程啟動機制
    // 信號量初始為 0，子行程現在都在 sem_wait 等待。
    // 父行程執行 sem_post 加 1，釋放其中一個子行程進入臨界區間。
    printf("Parent process releasing the lock...\n");
    if (sem_post(&shmp->sem) == -1)
        errorHandler("sem_post");

    // 等待子行程結束
    wait(NULL);
    wait(NULL);

    printf("Final Stack State:\n");
    print_stack(shmp);

    // 7. 清理資源
    // 銷毀 semaphore (雖然記憶體要被釋放了，但這是好習慣)
    sem_destroy(&shmp->sem);
    // 解除映射
    munmap(shmp, sizeof(*shmp));
    // 刪除共享記憶體物件
    shm_unlink(SHM_PATH);

    exit(EXIT_SUCCESS);
}