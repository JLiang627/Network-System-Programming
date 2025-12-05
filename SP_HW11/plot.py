import subprocess
import re
import matplotlib.pyplot as plt
import os

# =================設定區域=================
# 執行檔名稱
PROGRAM_NAME = "./program"
# 確保 C 程式已經編譯 (如果沒有，腳本會嘗試編譯)
SOURCE_FILE = "program.c"

# =================輔助函式=================
def compile_program():
    if not os.path.exists(PROGRAM_NAME) or os.path.getmtime(SOURCE_FILE) > os.path.getmtime(PROGRAM_NAME):
        print("正在編譯 C 程式...")
        # 使用與 Makefile 相同的參數
        cmd = ["gcc", "program.c", "-o", "program", "-std=c11", "-Wall", "-Wextra", "-lrt", "-Wno-unused-parameter", "-Wno-unused-variable"]
        subprocess.check_call(cmd)
        print("編譯完成！")

def run_test(D, R, C, B):
    """執行 ./program 並回傳 Loss Rate"""
    cmd = [PROGRAM_NAME, str(D), str(R), str(C), str(B)]
    try:
        # 執行 C 程式並擷取輸出
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=4000)
        output = result.stdout
        
        # 使用正規表示法抓取 Loss rate
        # 格式: Loss rate: 1 - (X/Y) = 0.123456
        match = re.search(r"Loss rate:.*=\s*(-?[\d\.]+)", output)
        if match:
            loss_rate = float(match.group(1))
            # 修正可能的負值 (雖然您的程式現在應該修正了)
            return max(0.0, loss_rate) 
        else:
            print(f"警告: 無法解析輸出 (D={D}, R={R}, C={C}, B={B})")
            return 0.0
    except subprocess.TimeoutExpired:
        print(f"執行逾時: D={D}, R={R}, C={C}, B={B}")
        return 1.0 # 逾時當作全掉
    except Exception as e:
        print(f"執行錯誤: {e}")
        return 0.0

# =================主程式=================
def main():
    compile_program()
    
    # --- 任務 1: 固定 D=1000, B=3，改變 R (X軸) ---
    print("\n開始執行任務 1 (改變傳送速率 R)...")
    D = 1000
    B = 3
    # 根據作業圖表，X軸的 R 值 (ms)
    R_values = [1000, 800, 500, 300] 
    # 不同的 Consumer 數量曲線
    C_list = [10, 100, 1000]
    
    plt.figure(figsize=(10, 6))
    
    for C in C_list:
        loss_rates = []
        print(f"  正在測試 Consumer={C}...")
        for R in R_values:
            lr = run_test(D, R, C, B)
            loss_rates.append(lr)
            print(f"    R={R}ms -> Loss Rate={lr:.4f}")
        
        # 畫線
        plt.plot(R_values, loss_rates, marker='o', label=f'consumer={C}')

    plt.title(f'Effect of Transmission Rate (R) on Loss Rate (B={B}, D={D})')
    plt.xlabel('Transmission Rate R (ms)')
    plt.ylabel('Loss Rate')
    plt.legend()
    plt.grid(True)
    plt.gca().invert_xaxis() # 讓 X 軸從大到小 (1000 -> 300)，類似作業範例
    plt.savefig('task1_result.png')
    print("任務 1 完成，圖表已存為 task1_result.png")

    # --- 任務 2: 固定 D=1000, R=500，改變 B (X軸) ---
    print("\n開始執行任務 2 (改變 Buffer Size B)...")
    D = 1000
    R = 500
    # X軸: Buffer Size 1 到 10
    B_values = range(1, 11) 
    C_list = [10, 100, 1000]

    plt.figure(figsize=(10, 6))

    for C in C_list:
        loss_rates = []
        print(f"  正在測試 Consumer={C}...")
        for B in B_values:
            lr = run_test(D, R, C, B)
            loss_rates.append(lr)
            print(f"    B={B} -> Loss Rate={lr:.4f}")
            
        plt.plot(B_values, loss_rates, marker='o', label=f'consumer={C}')

    plt.title(f'Effect of Buffer Size (B) on Loss Rate (R={R}, D={D})')
    plt.xlabel('Buffer Size (B)')
    plt.ylabel('Loss Rate')
    plt.legend()
    plt.grid(True)
    plt.xticks(B_values) # 強制顯示所有整數刻度
    plt.savefig('task2_result.png')
    print("任務 2 完成，圖表已存為 task2_result.png")

if __name__ == "__main__":
    main()