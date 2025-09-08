#include <linux/kernel.h>      // printk()
#include <linux/uaccess.h>     // copy_from_user(), copy_to_user()
#include <linux/syscalls.h>    // SYSCALL_DEFINE2()
#include <linux/sched.h>       // current, task_struct
#include <linux/mm.h>          // mm_struct, PAGE_MASK 
#include <asm/pgtable.h>       // page table 操作函數和類型

SYSCALL_DEFINE2(get_vir_to_phy,
                unsigned long __user *, virtual_addr,
                unsigned long __user *, physical_addr)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    unsigned long vaddr = 0;
    unsigned long paddr = 0;
    unsigned long page_addr = 0;
    unsigned long page_offset = 0;
    
    copy_from_user(&vaddr, virtual_addr, sizeof(unsigned long));
     
    pgd = pgd_offset(current->mm, vaddr);
    printk("pgd_val = 0x%lx, pgd_index = %lu\n", pgd_val(*pgd), pgd_index(vaddr));
    if(pgd_none(*pgd)) {
        printk("not mapped in pgd\n");
        return -1;
    }

    p4d = p4d_offset(pgd, vaddr);
    printk("p4d_val = 0x%lx, p4d_index = %lu\n", p4d_val(*p4d), p4d_index(vaddr));
    if(p4d_none(*p4d)) {
        printk("not mapped in p4d\n");
        return -1;
    }

    pud = pud_offset(p4d, vaddr);
    printk("pud_val = 0x%lx, pud_index = %lu\n", pud_val(*pud), pud_index(vaddr));
    if(pud_none(*pud)) {
        printk("not mapped in pud\n");
        return -1;
    }

    pmd = pmd_offset(pud, vaddr);
    printk("pmd_val = 0x%lx, pmd_index = %lu\n", pmd_val(*pmd), pmd_index(vaddr));
    if(pmd_none(*pmd)) {
        printk("not mapped in pmd\n");
        return -1;
    }

    pte = pte_offset_kernel(pmd, vaddr);
    printk("pte_val = 0x%lx, ptd_index = %lu\n", pte_val(*pte), pte_index(vaddr));
    if(pte_none(*pte)) {
        printk("not mapped in pte\n");
        return -1;
    }

    page_addr = pte_val(*pte) & PAGE_MASK;
	page_offset = vaddr & ~PAGE_MASK;
	paddr = page_addr | page_offset;
    
    printk("page_addr = %lx, page_offset = %lx\n", page_addr, page_offset);
    printk("vir_addr = %lx, phy_addr = %lx\n", vaddr, paddr);
    
    copy_to_user(physical_addr, &paddr, sizeof(unsigned long));
    
    return 1;
}