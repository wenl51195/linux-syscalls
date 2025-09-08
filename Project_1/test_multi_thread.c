/*
 * 功能：測試多執行緒間記憶體段的共享情況
 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <sys/syscall.h>
#include <sys/types.h>

// 系統呼叫號碼定義
#define get_vir_to_phy 545
#define get_segment 546

// 全域變數定義
int bss_value;                                          // BSS 段（未初始化）
int data_value = 123;                                   // DATA 段（已初始化）
static __thread int thread_local_storage_value = 246;   // 執行緒本地儲存

// 函數定義（在 TEXT 段中）
int code_function() {
    return 0;
}

// 記憶體段結構定義
struct Segment {
    unsigned long start_addr;
    unsigned long end_addr;
    unsigned long size;
};

struct ProcessSegments {
    pid_t pid;
    struct Segment code_seg;
    struct Segment data_seg;
    struct Segment heap_seg;
    struct Segment stack_seg;
};

// 獲取實體地址
unsigned long get_phys_addr(unsigned long virtual_address) {
    unsigned long physical_address = 0;
	int ret = syscall(get_vir_to_phy, &virtual_address, &physical_address);
    if (ret != 0 || physical_address == 0)
        return (unsigned long)-1; // 查不到
    return physical_address;
}

// 執行緒記憶體資訊
void thread_memory_info(const char* name) {
    // 區域變數（在各自的 stack 中）
    int stack_value = (strcmp(name, "main") == 0) ? 10 : 
                     (strcmp(name, "thread1") == 0) ? 100 : 200;
    
    // 取得各種記憶體位址
    unsigned long TLS = (unsigned long)&thread_local_storage_value;
    unsigned long stack = (unsigned long)&stack_value;
    unsigned long lib = (unsigned long)getpid;               // 函式庫
    unsigned long heap = (unsigned long)malloc(sizeof(int)); // 動態分配
    unsigned long bss = (unsigned long)&bss_value;
    unsigned long data = (unsigned long)&data_value;
    unsigned long code = (unsigned long)code_function;
    
    printf("============= %s =============\n", name);
    printf("pid = %d\ntid = %d\n", (int)getpid(), (int)gettid());
    printf("Segment\tVirtual Addr\tPhysical Addr\n");
    printf("-------\t------------\t-------------\n");
    printf("TLS\t0x%08lx\t0x%08lx\n", TLS, get_phys_addr(TLS));
    printf("Stack\t0x%08lx\t0x%08lx\n", stack, get_phys_addr(stack));
    printf("Lib\t0x%08lx\t0x%08lx\n", lib, get_phys_addr(lib));
    printf("Heap\t0x%08lx\t0x%08lx\n", heap, get_phys_addr(heap));
    printf("BSS\t0x%08lx\t0x%08lx\n", bss, get_phys_addr(bss));
    printf("Data\t0x%08lx\t0x%08lx\n", data, get_phys_addr(data));
    printf("Code\t0x%08lx\t0x%08lx\n", code, get_phys_addr(code));
    
    // 獲取記憶體段詳細資訊
    printf("\n=== Memory Segment Details ===\n");
    struct ProcessSegments thread_segs;
    memset(&thread_segs, 0, sizeof(thread_segs));
    thread_segs.pid = (int)gettid();
    
    int ret = syscall(get_segment, (void *)&thread_segs);
    if (ret != 0) {
        printf("Error: Failed to get segment information\n");
        if (heap) free((void*)heap);
        return;
    }
    
    printf("Segment\tVirtual Range\t\t\tSize\tPhysical Range\n");
    printf("-------\t-------------\t\t\t---------\t--------------\n");
    
    // Code 段
    unsigned long code_start_phy = get_phys_addr(thread_segs.code_seg.start_addr);
    unsigned long code_end_phy = get_phys_addr(thread_segs.code_seg.end_addr);
    printf("Code\t0x%08lx-0x%08lx\t%lu\t0x%08lx-0x%08lx\n",
           thread_segs.code_seg.start_addr,
           thread_segs.code_seg.end_addr,
		   thread_segs.code_seg.size,
           code_start_phy, code_end_phy);
    
    // Data 段
    unsigned long data_start_phy = get_phys_addr(thread_segs.data_seg.start_addr);
    unsigned long data_end_phy = get_phys_addr(thread_segs.data_seg.end_addr);
    printf("Data\t0x%08lx-0x%08lx\t%lu\t0x%08lx-0x%08lx\n",
           thread_segs.data_seg.start_addr,
           thread_segs.data_seg.end_addr,
		   thread_segs.data_seg.size,
           data_start_phy, data_end_phy);
    
    // Heap 段
    unsigned long heap_start_phy = get_phys_addr(thread_segs.heap_seg.start_addr);
    unsigned long heap_end_phy = get_phys_addr(thread_segs.heap_seg.end_addr);
    printf("Heap\t0x%08lx-0x%08lx\t%lu\t0x%08lx-0x%08lx\n",
            thread_segs.heap_seg.start_addr,
            thread_segs.heap_seg.end_addr,
			thread_segs.heap_seg.size,
            heap_start_phy, heap_end_phy);

    // Stack 段
    unsigned long stack_start_phy = get_phys_addr(thread_segs.stack_seg.start_addr);
    unsigned long stack_end_phy = get_phys_addr(thread_segs.stack_seg.end_addr);
    printf("Stack\t0x%08lx-0x%08lx\t%lu\t0x%08lx-0x%08lx\n",
           thread_segs.stack_seg.start_addr,
           thread_segs.stack_seg.end_addr,
		   thread_segs.stack_seg.size,
           stack_start_phy, stack_end_phy);
    
    printf("\n");
    
    // 清理動態分配的記憶體
    if (heap) free((void*)heap);
}

// 執行緒 1 函數
void *thread1(void *arg) {
    sleep(2);  // 確保執行順序
    thread_memory_info("thread1");
    pthread_exit(NULL);
}

// 執行緒 2 函數
void *thread2(void *arg) {
    sleep(4);  // 確保執行順序
    thread_memory_info("thread2");
    pthread_exit(NULL);
}

// 主程式
int main() {
    pthread_t t1, t2;
    // 創建執行緒
    if (pthread_create(&t1, NULL, thread1, NULL) != 0) {
        perror("Failed to create thread1");
        return 1;
    }
    
    if (pthread_create(&t2, NULL, thread2, NULL) != 0) {
        perror("Failed to create thread2");
        return 1;
    }
    
    // 等待足夠時間讓子執行緒完成
    sleep(10);
    
    // 主執行緒資訊
    thread_memory_info("main");
    
    // 顯示執行緒控制塊地址
    printf("========== Thread Control Information ==========\n");
    printf("Thread 1 handle address: %p\n", (void*)&t1);
    printf("Thread 2 handle address: %p\n", (void*)&t2);
    printf("Main thread TID: %d\n", (int)gettid());
    printf("\n");
    
    // 等待子執行緒結束
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("Completed.\n");
    return 0;
}