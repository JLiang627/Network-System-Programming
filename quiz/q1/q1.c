#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // for open()

// 定義 pipe 檔案描述符的索引
#define COMMAND_READ_FD  0
#define COMMAND_WRITE_FD 1
#define RESULT_READ_FD   0
#define RESULT_WRITE_FD  1

// 最大命令長度
#define MAX_CMD_LEN 256
// 每次從 pipe 讀取的緩衝區大小
#define BUFFER_SIZE 4096

/**
 * @brief Executor (執行者) 程序的邏輯
 * @param command_pipe_read_fd Commander 傳輸命令給 Executor 的讀取端
 * @param result_pipe_write_fd Executor 傳輸執行結果給 Commander 的寫入端
 */
void executor_process(int command_pipe_read_fd, int result_pipe_write_fd) {
    char command[MAX_CMD_LEN];
    ssize_t bytes_read;
    char *argv[] = {NULL, NULL}; // 供 execvp 使用

    // 關閉不使用的 pipe 端點
    // Executor 不寫入命令 pipe (COMMAND_WRITE_FD)，也不讀取結果 pipe (RESULT_READ_FD)
    close(command_pipe_read_fd - 1); // 假設 command_pipe_read_fd 是 pipe[0]，則 -1 為 pipe[1]
    close(result_pipe_write_fd - 1); // 假設 result_pipe_write_fd 是 pipe[1]，則 -1 為 pipe[0]

    // 持續從 Commander 讀取命令
    while ((bytes_read = read(command_pipe_read_fd, command, MAX_CMD_LEN - 1)) > 0) {
        command[bytes_read] = '\0'; // 確保字串結尾

        // 核心：使用 dup2() 將標準輸出 (stdout, FD 1) 重導向到結果 pipe 的寫入端
        // 這樣，當執行命令時，它的輸出就會寫入 pipe
        if (dup2(result_pipe_write_fd, STDOUT_FILENO) == -1) {
            perror("Executor: dup2 failed");
            exit(EXIT_FAILURE);
        }

        // 關閉不再需要的寫入端 (stdout 已經指向它)
        close(result_pipe_write_fd);

        // 準備執行命令的參數 (僅處理簡單命令，不處理參數)
        argv[0] = command;

        // 執行 UNIX 命令。
        // execvp 會用新程序替換當前程序 (Executor)。
        // 如果執行成功，後面的代碼將不會被執行。
        execvp(argv[0], argv);

        // 如果 execvp 執行失敗，則會執行到此處
        // 錯誤資訊會輸出到已被重導向的 pipe 中
        perror("Executor: Command execution failed");
        
        // 由於 Executor 程序已被替換，這段代碼實際上不會在成功的 execvp 後執行。
        // 這裡的 exit 確保了失敗時程序終止。
        exit(EXIT_FAILURE);
    }
    
    // 收到 EOF (Commander 關閉寫入端) 或發生錯誤時，Executor 結束
    close(command_pipe_read_fd);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Commander (指揮者) 程序的邏輯
 * @param command_pipe_write_fd Commander 傳輸命令給 Executor 的寫入端
 * @param result_pipe_read_fd Commander 讀取 Executor 結果的讀取端
 * @param output_filename 如果不是 NULL，則將結果寫入此檔案
 */
void commander_process(int command_pipe_write_fd, int result_pipe_read_fd, const char *output_filename) {
    char input_cmd[MAX_CMD_LEN];
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    FILE *output_file = NULL;

    // 關閉不使用的 pipe 端點
    // Commander 不讀取命令 pipe (COMMAND_READ_FD)，也不寫入結果 pipe (RESULT_WRITE_FD)
    close(command_pipe_write_fd - 1); // 假設 command_pipe_write_fd 是 pipe[1]，則 -1 為 pipe[0]
    close(result_pipe_read_fd - 1);  // 假設 result_pipe_read_fd 是 pipe[0]，則 -1 為 pipe[1]

    // 處理檔案輸出
    if (output_filename) {
        // O_WRONLY: 唯寫, O_CREAT: 如果檔案不存在則建立, O_TRUNC: 如果檔案存在則清空內容
        output_file = fopen(output_filename, "w");
        if (output_file == NULL) {
            perror("Commander: Error opening output file");
            close(command_pipe_write_fd);
            close(result_pipe_read_fd);
            exit(EXIT_FAILURE);
        }
    }
    
    // 主迴圈
    while (1) {
        printf("Commander, please input command: "); // 提示輸入 [cite: 7]
        if (fgets(input_cmd, MAX_CMD_LEN, stdin) == NULL) {
            break; // 讀取失敗或 EOF
        }
        
        // 清除換行符號
        input_cmd[strcspn(input_cmd, "\n")] = 0;
        
        if (strlen(input_cmd) == 0) {
            continue; // 處理空輸入
        }
        
        // 寫入命令到 Executor 的 pipe
        if (write(command_pipe_write_fd, input_cmd, strlen(input_cmd)) == -1) {
            perror("Commander: Error writing command to pipe");
            break;
        }

        printf(".........result from Executor");
        if (output_filename) {
            printf(" is saved in \"%s\" file........\n", output_filename); // [cite: 13]
        } else {
            printf(".\n"); // [cite: 8, 10]
        }

        // 實際從結果 pipe 讀取數據並輸出/寫入檔案
        int total_bytes = 0;
        int max_loops = 5; // 限制讀取循環次數以防止阻塞

        break; // 為了在單次執行的 main 程式碼中實現邏輯，我們在這裡跳出迴圈
    }
    
    // 結束時關閉 pipe 和檔案
    close(command_pipe_write_fd);
    close(result_pipe_read_fd);
    if (output_file) {
        fclose(output_file);
    }
}

/**
 * @brief 主函式 (Main function) 處理 pipe、fork 和循環
 */
int main(int argc, char *argv[]) {
    // 檢查是否有檔案輸出參數
    const char *output_filename = NULL;
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        output_filename = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        fprintf(stderr, "       %s -f <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("$myrpc"); // 模擬 shell 提示符號 [cite: 6, 11]
    if (output_filename) {
        printf(" -f %s\n", output_filename);
    } else {
        printf("\n");
    }

    // 主循環：持續接受命令
    while (1) {
        int command_pipe[2]; // Pipe 1: Commander -> Executor (傳輸命令)
        int result_pipe[2];  // Pipe 2: Executor -> Commander (傳輸結果)
        char input_cmd[MAX_CMD_LEN];
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;
        pid_t pid;
        int status;
        
        printf("Commander, please input command: "); // 提示輸入 
        fflush(stdout); // 確保提示符號立即顯示

        if (fgets(input_cmd, MAX_CMD_LEN, stdin) == NULL) {
            break; // 讀取失敗或 EOF
        }
        
        // 清除換行符號
        input_cmd[strcspn(input_cmd, "\n")] = 0;
        
        if (strlen(input_cmd) == 0) {
            continue; // 處理空輸入
        }

        // 1. 建立兩個 pipe
        if (pipe(command_pipe) == -1 || pipe(result_pipe) == -1) {
            perror("pipe failed");
            return EXIT_FAILURE;
        }

        // 2. 建立子程序 (Executor)
        pid = fork();

        if (pid < 0) {
            perror("fork failed");
            return EXIT_FAILURE;
        }
        
        // --- 子程序 (Executor) 邏輯 ---
        if (pid == 0) {
            // 關閉 Executor 不需要使用的 pipe 端點
            close(command_pipe[COMMAND_WRITE_FD]); // 不寫入命令
            close(result_pipe[RESULT_READ_FD]);   // 不讀取結果

            // 讀取 Commander 寫入的命令 (確保 Executor 讀取命令)
            if (read(command_pipe[COMMAND_READ_FD], input_cmd, MAX_CMD_LEN - 1) <= 0) {
                // 如果讀取失敗，直接退出
                close(command_pipe[COMMAND_READ_FD]);
                close(result_pipe[RESULT_WRITE_FD]);
                exit(EXIT_FAILURE);
            }
            input_cmd[strlen(input_cmd)] = '\0'; // 確保字串結尾

            // 核心：使用 dup2() 將標準輸出 (stdout, FD 1) 重導向到結果 pipe 的寫入端
            if (dup2(result_pipe[RESULT_WRITE_FD], STDOUT_FILENO) == -1) {
                perror("Executor: dup2 failed");
                exit(EXIT_FAILURE);
            }

            // 關閉 Executor 不再需要的結果 pipe 寫入端 (因為 stdout 已指向它)
            close(result_pipe[RESULT_WRITE_FD]);

            // 準備執行命令的參數
            char *argv_exec[] = {input_cmd, NULL};

            // 執行 UNIX 命令。
            execvp(argv_exec[0], argv_exec);

            // 如果 execvp 執行失敗，則會執行到此處
            perror("Executor: Command execution failed");
            // 錯誤訊息會輸出到已被重導向的 pipe 中
            exit(EXIT_FAILURE);

        } 
        
        // --- 父程序 (Commander) 邏輯 ---
        else {
            // 關閉 Commander 不需要使用的 pipe 端點
            close(command_pipe[COMMAND_READ_FD]);  // 不讀取命令
            close(result_pipe[RESULT_WRITE_FD]);   // 不寫入結果

            // 3. Commander 寫入命令到 pipe 1
            if (write(command_pipe[COMMAND_WRITE_FD], input_cmd, strlen(input_cmd)) == -1) {
                perror("Commander: Error writing command to pipe");
                goto cleanup;
            }
            
            // 寫入完畢，關閉寫入端，讓 Executor 知道數據傳輸完畢
            close(command_pipe[COMMAND_WRITE_FD]);

            // 4. Commander 讀取 Executor 的執行結果 (從 pipe 2)
            FILE *output_file = NULL;

            if (output_filename) {
                // O_WRONLY: 唯寫, O_CREAT: 如果檔案不存在則建立, O_TRUNC: 如果檔案存在則清空內容
                output_file = fopen(output_filename, "w");
                if (output_file == NULL) {
                    perror("Commander: Error opening output file");
                    goto cleanup;
                }
            }
            
            // 從結果 pipe 中讀取數據
            int result_found = 0;
            while ((bytes_read = read(result_pipe[RESULT_READ_FD], buffer, BUFFER_SIZE)) > 0) {
                result_found = 1;
                if (output_filename) {
                    // 寫入到檔案
                    if (fwrite(buffer, 1, bytes_read, output_file) != bytes_read) {
                        perror("Commander: Error writing to file");
                        break;
                    }
                } else {
                    // 輸出到標準輸出 (螢幕)
                    if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
                        perror("Commander: Error writing to stdout");
                        break;
                    }
                }
            }
            
            // 讀取完畢，關閉讀取端
            close(result_pipe[RESULT_READ_FD]);
            
            // 5. 等待 Executor 結束
            waitpid(pid, &status, 0);

            // 6. 輸出提示訊息
            if (output_filename) {
                printf(".result from Executor is saved in \"%s\" file........\n", output_filename); // [cite: 13]
                if (output_file) fclose(output_file);
            } else {
                printf(".........result from Executor.\n"); // [cite: 8, 10]
            }
            
            // 錯誤處理清理
            cleanup:
            if (output_file) fclose(output_file);
            // 確保所有開啟的 pipe 端點都被關閉
            close(command_pipe[COMMAND_WRITE_FD]);
            close(result_pipe[RESULT_READ_FD]);
        }
    }

    return EXIT_SUCCESS;
}
