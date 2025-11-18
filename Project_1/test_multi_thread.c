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

// 全域變數定義
int bss_value;                         // BSS 段（未初始化）
int data_value = 123;                  // DATA 段（已初始化）
static __thread int tls_value = 246;   // 執行緒本地儲存

// 函數定義（在 TEXT 段中）
int code_function() {
    return 0;
}

unsigned long get_phys_addr(unsigned long virtual_address) {
    unsigned long physical_address = 0;
	syscall(get_vir_to_phy, &virtual_address, &physical_address);
    return physical_address;
}

void thread_memory_info(const char* name) {
    int stack_value = 100;  // 區域變數（在各自的 stack 中）
    
    unsigned long TLS = (unsigned long)&tls_value;
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
}

void *thread1(void *arg) {
    thread_memory_info("thread1");
    pthread_exit(NULL);
}

void *thread2(void *arg) {
    thread_memory_info("thread2");
    pthread_exit(NULL);
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_join(t1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t2, NULL);
    thread_memory_info("main");

    return 0;
}
