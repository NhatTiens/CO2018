#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if(q->size >= MAX_QUEUE_SIZE) return;
        q->proc[q->size] = proc;              // Them process vao queue
        q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if(empty(q)) return NULL;
        struct pcb_t * target_proc = NULL;   // pcb can deqeueu
        target_proc = q->proc[0];    // pcb tra ve
        // Xoa pcb khoi queue
        int i;
        for( i= 0; i< q->size -1; i++){
                q->proc[i] = q->proc[i+1];
        }
        q->size --;
        return target_proc;
}

