# C 編譯器 (Compiler required by HW8)
CC = gcc

# 編譯旗標：
CFLAGS = -Wall -g -I. -Wno-deprecated-declarations

# 執行檔目標：
# TARGET_SUSPEND: Exercise 22-3 (基準程式)
TARGET_SUSPEND = sig_speed_sigsuspend
# TARGET_WAITINFO: Exercise 22-3 (修改版程式)
TARGET_WAITINFO = sig_speed_sigwaitinfo
# TARGET_SV_IMPL:
TARGET_SV_IMPL = sv_signal_impl

# 共同目標檔 (Common Object Files)
COMMON_OBJS = error_functions.o get_num.o

# --- 主要編譯目標 ---

# 預設 'all' 會編譯所有三個目標
all: $(TARGET_SUSPEND) $(TARGET_WAITINFO) $(TARGET_SV_IMPL)

# 規則 1: 編譯 sigwaitinfo 版本 (Exercise 22-3)
$(TARGET_WAITINFO): sig_speed_sigwaitinfo.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ 

# 規則 2: 編譯 sigsuspend 基準版本 (Exercise 22-3)
$(TARGET_SUSPEND): sig_speed_sigsuspend.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# 規則 3: 編譯 System V API 實作 (Exercise 22-4)
$(TARGET_SV_IMPL): sv_signal_impl.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# --- 共同目標檔規則 (Implicit Rules) ---
error_functions.o: error_functions.c
	$(CC) $(CFLAGS) -c $< -o $@
	
get_num.o: get_num.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- 清理規則 ---
.PHONY: clean

clean:
	rm -f $(TARGET_SUSPEND) $(TARGET_WAITINFO) $(TARGET_SV_IMPL) *.o