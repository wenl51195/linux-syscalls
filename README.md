# linux-syscalls

## 環境設置
* Linux version: Ubuntu 20.04
    * Download: [ubuntu-20.04.6-desktop-amd64.iso](https://releases.ubuntu.com/20.04/)
* Linux kernel version: linux-5.8.1
    * 查詢版本 `uname-r`
    * Download: [linux-5.8.1.tar.gz](https://ftp.ntu.edu.tw/linux/kernel/v5.x/?fbclid=IwAR0fq2e0T60YB54O2xZGouyQ33z4o_kxkmElhdn-y9CqIZnq2bc2lwVIdwk) 
    * 解壓縮 `tar zxvf linux-5.8.1.tar.gz`
* VirtualBox
    * Processors: 6 cores
    * Basic memory: 6 GB
    * Hard Disk:30 GB 
        * 實際用量 27.88 GB

## 編譯 Kernel 步驟
[參考網頁](https://dev.to/jasper/adding-a-system-call-to-the-linux-kernel-5-8-1-in-ubuntu-20-04-lts-2ga8)

1. 在同層目錄 (proj1) 新增 Makefile
    ```
    $ nano Makefile
    obj-y := get_vir_to_phy.o
    ```
2. 回到上層目錄 (linux-5.8.1) 修改 Makefile
    ```
    $ cd ..
    $ nano Makefile
    ```
    - 找到下列這行，在最後面添加 proj1/
    ```
    core-y += kernel/ cert/ mm/ fs/ ipc/ security/ crypto/ block/
    ↓ 添加 proj1
    core-y += kernel/ cert/ mm/ fs/ ipc/ security/ crypto/ block/ proj1/
    ```
    * 若在其他資料夾也有 system call，則往後加
    * 例如同層目錄中有另外一個資料夾 proj2 有 system call
    ```
    core-y += kernel/ cert/ mm/ fs/ ipc/ security/ crypto/ block/ proj1/ proj2
    ```
3. 在 system call header 加入你的 system call 對應的函數原型
    ```
    $ nano include/linux/syscalls.h
    ```
    * 在文件底下 `#endif` 前添加
    ```
    # 在檔案中添加函數宣告
    asmlinkage long sys_get_vir_to_phy(unsigned long __user *virtual_addr, unsigned long __user *physical_addr);
    struct ProcessSegments; // 前置宣告但要確保在實作函數的 .c 檔案有完整定義
                            // 若未前置宣告會報錯, 需在全域 header 定義 struct ProcessSegments  
    asmlinkage long sys_get_segment(struct ProcessSegments __user *user_thread_seg);
    ```
4. 把新增的 system call 加到 system call table 裡
    * 在檔案末尾添加新的系統呼叫
    ```
    $ nano arch/x86/entry/syscalls/syscall_64.tbl
    
    # 格式: <number> <abi> <name> <entry point>
    545	64	get_vir_to_phy  sys_get_vir_to_phy
    546	64	get_segment		sys_get_segment
    ```
5. 安裝編譯 kernel 前需要的套件
    ```
    # Ubuntu/Debian
    $ sudo apt update
    $ sudo apt install build-essential libncurses-dev libssl-dev libelf-dev bison flex -y
    ```
6. 生成設定檔 
    ```
    $ sudo make menuconfig
    -> Save
    -> Exit
    ```
7. 查核心數量
    ```
    $ nproc
    ```
8. 開始 Compile Kernel
    ```
    $ sudo make -j6
    ```
9. 安裝模組
    ```
    $ sudo make modules_install install -j6
    ```
10. 更新 OS 的 bootloader 使用 new kernel
    ```
    $ nano /etc/default/grub

    GRUB_TIMEOUT_STYLE=menu
    GRUB_TIMEOUT=15

    $ sudo update-grub
    ```
11. 重開機
    ```
    $ sudo reboot
    ```

## 編譯 Kernel 時遇到的問題
-   ```
    No rule to make target ‘debian/canonical-certs.pem‘, needed by ‘certs/  x509_certificate_list‘
    ```
    > 解法：編輯 .config 文件，修改 `CONFIG_SYSTEM_TRUSTED_KEYS="debian/canonical-certs.pem"` 為 `CONFIG_SYSTEM_TRUSTED_KEYS=""` [參考網頁](https://blog.csdn.net/qq_36393978/article/details/118157426)
-   ```
    Failed to generate BTF for vmlinux
    ```
    > 解法：編輯 .config 文件，修改 `CONFIG_DEBUG_INFO=y` 為 `CONFIG_DEBUG_INFO=n`