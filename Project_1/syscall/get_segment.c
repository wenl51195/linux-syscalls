#include <linux/kernel.h>      // printk()
#include <linux/syscalls.h>    // SYSCALL_DEFINE1
#include <linux/sched.h>       // current, task_struct  
#include <linux/uaccess.h>     // copy_from_user, copy_to_user
#include <linux/pid.h>         // find_task_by_vpid

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

SYSCALL_DEFINE1(get_segment,
                struct ProcessSegments __user *, user_thread_seg)
{
    struct ProcessSegments process_segments;
    struct task_struct* task;
    int ret = 0;

    // 從使用者空間複製 Process ID
    ret = copy_from_user(&process_segments, user_thread_seg, sizeof(struct ProcessSegments));
    if (ret != 0) {
        printk(KERN_ERR "copy_from_user failed\n");
        return -EFAULT;
    }

    // 查找對應的 Process
    task = find_task_by_vpid(process_segments.pid);
    if (!task) {
        printk(KERN_ERR "Process with PID %d not found\n", process_segments.pid);
        return -ESRCH;
    }

    process_segments.code_seg.start_addr = task->mm->start_code;
    process_segments.code_seg.end_addr = task->mm->end_code;
    process_segments.code_seg.size = task->mm->end_code - task->mm->start_code;

    process_segments.data_seg.start_addr = task->mm->start_data;
    process_segments.data_seg.end_addr = task->mm->end_data;
    process_segments.data_seg.size  = task->mm->end_data - task->mm->start_data;

    process_segments.heap_seg.start_addr = task->mm->start_brk;
    process_segments.heap_seg.end_addr = task->mm->brk;
    process_segments.heap_seg.size = task->mm->brk - task->mm->start_brk;

    process_segments.stack_seg.start_addr = task->mm->start_stack + task->mm->stack_vm;
    process_segments.stack_seg.end_addr = task->mm->start_stack;
    process_segments.stack_seg.size = process_segments.stack_seg.start_addr - process_segments.stack_seg.end_addr;


    // 複製回 user space
    ret = copy_to_user(user_thread_seg, &process_segments, sizeof(struct ProcessSegments));
    if (ret != 0) {
        printk(KERN_ERR "get_segment: copy_to_user failed\n");
        return -EFAULT;
    }

    return 0;
}
