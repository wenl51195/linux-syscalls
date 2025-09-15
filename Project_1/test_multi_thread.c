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

unsigned long get_phys_addr(unsigned long virtual_address) {
    unsigned long physical_address = 0;
	int ret = syscall(get_vir_to_phy, &virtual_address, &physical_address);
    if (ret != 0 || physical_address == 0)
        return (unsigned long)-1; // 查不到
    return physical_address;
}

void thread_memory_info(const char* name) {
    int stack_value = 100;  // 區域變數（在各自的 stack 中）
    
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
    
    if (heap) free((void*)heap);
}

void *thread1(void *arg) {
    sleep(2);
    thread_memory_info("thread1");
    pthread_exit(NULL);
}

void *thread2(void *arg) {
    sleep(4);
    thread_memory_info("thread2");
    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    
    sleep(10);
    
    thread_memory_info("main");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
