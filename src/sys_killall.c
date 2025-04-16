/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

 #include "common.h"
 #include "syscall.h"
 #include "stdio.h"
 #include "libmem.h"
 
 #include "queue.h"  // added for queue_t
 #include <string.h> // added for strcmp()
 
 int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
 {
     char proc_name[100];
     uint32_t data;
 
     //hardcode for demo only
     uint32_t memrg = regs->a1;
     
     /* TODO: Get name of the target proc */
     //proc_name = libread..
     int i = 0;
     data = 0;
     while(data != -1){
         libread(caller, memrg, i, &data);
         proc_name[i]= data;
         if(data == -1) proc_name[i]='\0';
         i++;
     }
     printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);
 
     /* TODO: Traverse proclist to terminate the proc
      *       stcmp to check the process match proc_name
      */
     //caller->running_list
     //caller->mlq_ready_queu
     struct queue_t *running_list = caller->running_list;
 #ifdef MLQ_SCHED
     struct queue_t *mlq_ready_queue = caller->mlq_ready_queue;
 #endif
 
     /* TODO Maching and terminating 
      *       all processes with given
      *        name in var proc_name
      */
     
     // Xử lý các tiến trình trong running_list
    struct queue_t tmp = {0};
    tmp.size = 0;
    int reg_idx = 0;
    while (!empty(running_list)) {
        struct pcb_t *proc = dequeue(running_list);
        if (strcmp(proc->path, proc_name) == 0) {
            libfree(proc, reg_idx); // Giải phóng bộ nhớ
        } else {
            enqueue(&tmp, proc); // Tiến trình không trùng tên thì đưa vào tmp
        }
        reg_idx++;
    }
    while (!empty(&tmp)) {
        struct pcb_t *proc = dequeue(&tmp);
        enqueue(running_list, proc); // Đưa lại các tiến trình vào running_list
    }
 
 #ifdef MLQ_SCHED
     struct queue_t tmp2 = {0};
     tmp2.size = 0;
     reg_idx = 0;
     while(!empty(mlq_ready_queue)){
         struct pcb_t *proc = dequeue(mlq_ready_queue);
         if(strcmp(proc->path, proc_name) == 0){
             libfree(proc, reg_idx);
         }else{
             enqueue(&tmp2, proc);
         }
         reg_idx++;
     }
     while(!empty(&tmp2)){
         struct pcb_t *proc = dequeue(&tmp2);
         enqueue(mlq_ready_queue, proc);
     }
 #endif
     return 0; 
 }