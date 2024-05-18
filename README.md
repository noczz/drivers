# Linux高级字符设备驱动程序
## main.c
  实现了scull的基本方法（open,release,read,write,ioctl...）  

## seq_file.c
  设计了seq_file接口,该接口能将内核空间的设备属性输出到/proc/scullseq下，提供用户空间访问  

## access.c
实现了四种访问控制：
1. 同一时间段仅限一个进程访问（自旋锁）  
2. 多进程互斥访问（竞争条件）
3. 多进程互斥访问，并未获得资源的进程进入阻塞
4. 多进程访问tty时，独享私有字符设备  

## pipe.c
管道，可以通过poll同时查询多个管道的状态，实现进程间的通信  
