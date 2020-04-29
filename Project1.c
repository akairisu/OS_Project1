#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <string.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/mman.h>

typedef struct task{
	char taskname[512];
	int readytime;
	int exectime;
	pid_t pid;
}task;

typedef struct cirlink{
	int taskid;
	struct cirlink *next;
	struct cirlink *prev;
}cirlink;

typedef struct queue{
	int taskid;
	int time;
	struct queue *next;
}queue;

void set_priority(pid_t pid, int priority){
    struct sched_param param;
    param.sched_priority = priority;
    if (sched_setscheduler(pid, SCHED_FIFO, &param) == -1){
	    perror("sched_setscheduler() failed");
	    exit(1);
    }
}

void print_queue(queue *list){
	queue *tmp = list;
	while(tmp != NULL){
		printf("%d ", tmp->taskid);
		tmp = tmp->next;
	}
	printf("\n");
	return;
}

queue *init_queue(int id, int time){
	queue *processlist = (queue*)malloc(sizeof(queue));
	processlist->taskid = id;
	processlist->time = time;
	processlist->next = NULL;
	return processlist;
}

queue *insert_queue(queue *list, queue *insertproc){
	if(list == NULL){
		list = insertproc;
	}
	else{
		queue *temp = list;
		while(list->next != NULL){
			list = list->next;
		}
		list->next = insertproc;
		list = temp;
	}
	return list;
}

queue *delete_queue(queue *list, int id){
	queue *tmp = list;
	queue *head = list;
	queue *prev = NULL;
	while(tmp->taskid != id){
		prev = tmp;
		tmp = tmp->next;
	}
	if(prev != NULL){	
		while(list->next != tmp){
			list = list->next;
		}
		list->next = list->next->next;
		list = head;
	}
	else{
		list = list->next;
	}
	return list;
}

cirlink *init_cirlink(int id){
	cirlink *processlist = (cirlink*)malloc(sizeof(cirlink));
	processlist->taskid = id;
	processlist->next = processlist;
	processlist->prev = processlist;
	return processlist;
}

cirlink *insert_cirlink(cirlink *list, cirlink *insertproc){
	if(list == NULL){
		return insertproc;	
	}	
	list->prev->next = insertproc;
	insertproc->prev = list->prev;
	list->prev = insertproc;
	insertproc->next = list;
	return list;
}

cirlink *delete_cirlink(int id, cirlink *processlist){
	cirlink *temp = processlist;
	while(temp->taskid != id){
		temp = temp->next;
	}
	if(temp->next == temp){
		return NULL;
	}
	temp->prev->next = temp->next;
	temp->next->prev = temp->prev;
	if(temp == processlist){
		return processlist->next;
	}
	else return processlist;
}

void print_cirlink(cirlink *list){
	if(list == NULL){
		printf("Error");
		return;	
	}
	cirlink *head = list;
	while(list->next != head){
		printf("%d ", list->taskid);
		list = list->next;	
	}	
	printf("%d\n", list->taskid);
	list = head;
	return;
}

void do_idle(){
	volatile unsigned long i; 
	for(i=0;i<1000000UL;i++);
	return;
}

void do_task_FIFO(task tasks[], int id, int *timer, int *finish){
	pid_t pid = fork();
	if(pid == 0){
		long long start_time = syscall(333);
		for(int j = 0 ; j < tasks[id].exectime ; j++){
			volatile unsigned long i; 
			for(i=0;i<1000000UL;i++);
			*timer+=1;
		}
		long long end_time = syscall(333);
		syscall(334, getpid() , start_time, end_time);
		printf("%s %d\n", tasks[id].taskname , getpid());
		*finish = id;
		exit(0);
	}
	else{
		tasks[id].pid = pid;
		set_priority(pid, 1);	
	}
	return;
}

void do_task_RR(task tasks[], int id, int *timer, int *finish){
	pid_t pid = fork();
	if(pid == 0){
		long long start_time = syscall(333);
		for(int j = 0 ; j < tasks[id].exectime ; j++){
			volatile unsigned long i; 
			for(i=0;i<1000000UL;i++);
			*timer+=1;
			if(j % 500 == 0 && j != tasks[id].exectime - 1 && j != 0){				
				set_priority(getpid(), 1);	
			}
		}
		long long end_time = syscall(333);
		syscall(334, getpid() , start_time, end_time);
		printf("%s %d\n", tasks[id].taskname , getpid());
		*finish = id;
		exit(0);
	}
	else{
		tasks[id].pid = pid;
		set_priority(pid, 1);	
	}
	return;
}

void do_task_PSJF(task tasks[], int id, int *timer, int *finish, int *next, int *timecounter){
	pid_t pid = fork();
	if(pid == 0){
		long long start_time = syscall(333);
		for(int j = 0 ; j < tasks[id].exectime ; j++){
			volatile unsigned long i; 
			for(i=0;i<1000000UL;i++);
			*timer+=1;
			*timecounter+=1;
			if(*timer == *next && j != tasks[id].exectime - 1){
				set_priority(getpid(), 1);	
			}
		}
		long long end_time = syscall(333);
		syscall(334, getpid() , start_time, end_time);
		printf("%s %d\n", tasks[id].taskname , getpid());
		*finish = id;
		exit(0);
	}
	else{
		tasks[id].pid = pid;
		set_priority(pid, 1);	
	}
	return;
}

void swap(task tasks[], int i, int j){
	task temp;
	temp = tasks[i];
	tasks[i] = tasks[j];
	tasks[j] = temp;
	return;
}

void bubblesort(task tasks[], int tasknum){
	for(int i = 0; i < tasknum; i++){
		for(int j = 0 ; j < tasknum - i - 1; j++){
			if(tasks[j].readytime > tasks[j + 1].readytime){
				swap(tasks, j, j + 1);
			}
		}
	}
	return;
}

int *create_share_var(char *filename){
	int shfd = shm_open(filename , O_RDWR | O_CREAT, 0777);	
	if(shfd<0){ 
		perror("shm_open"); 
		exit(EXIT_FAILURE); 
	}
	ftruncate(shfd , sizeof(int));
	int *ad = (int*)mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, shfd, (off_t)0);
	if (ad == MAP_FAILED){ 
		perror("mmap"); exit(EXIT_FAILURE); 
	}
	return ad;
}

void schedule_FIFO(task tasks[], int tasknum){
	int *timer = create_share_var("timer");
	*timer = 0;
	int *finish = create_share_var("finish");
	*finish = -2;
	int count = 0;
	int endcount = -1;
	queue *processlist = NULL;
	while(endcount != tasknum - 1){
		while(*timer < tasks[count].readytime && processlist == NULL){
			set_priority(0, 1);
			do_idle();
			*timer+=1;	
		}
		set_priority(getpid(), 10);
		while(*timer >= tasks[count].readytime && count < tasknum){
			queue *process = init_queue(count, tasks[count].exectime);
			processlist = insert_queue(processlist, process);
			do_task_FIFO(tasks, count, timer, finish);
			count+=1;
		}
		set_priority(tasks[processlist->taskid].pid, 99);
		sched_yield();
		if(*finish != endcount){
			endcount = *finish;
			processlist = processlist->next;
		}
	}
	return;
}

void schedule_RR(task tasks[], int tasknum){
	int *timer = create_share_var("timer");
	*timer = 0;
	int *finish = create_share_var("finish");
	*finish = -1;
	int count = 0;
	int endcount = 0;
	int end = -1;
	int NULLflag = 0;
	cirlink *processlist = NULL;
	while(*timer < tasks[count].readytime && processlist == NULL){
		set_priority(getpid(), 1);
		do_idle();
		*timer+=1;	
	}
	set_priority(getpid(), 10);
	while(*timer >= tasks[count].readytime && count < tasknum){						
		cirlink *process = init_cirlink(count);
		processlist = insert_cirlink(processlist, process);
		do_task_RR(tasks, count, timer, finish);
		count+=1;
	}
	while(endcount != tasknum){
		set_priority(tasks[processlist->taskid].pid, 99);
		sched_yield();
		if(endcount == tasknum){
			break;		
		}
		while(*timer < tasks[count].readytime && processlist == NULL){
			set_priority(getpid(), 1);
			do_idle();
			*timer+=1;	
		}
		set_priority(getpid(), 10);
		if(processlist == NULL){
			NULLflag = 1;	
		}
		while(*timer >= tasks[count].readytime && count < tasknum){						
			cirlink *process = init_cirlink(count);
			processlist = insert_cirlink(processlist, process);
			do_task_RR(tasks, count, timer, finish);
			count+=1;
		}
		if(NULLflag != 1){
			processlist = processlist->next;
		}
		if(*finish != end){
			end = *finish;
			processlist = delete_cirlink(end , processlist);
			endcount+=1;
		}
	}
	return;
}

int find_shortest(queue *processlist){
	int min = 2147483647;
	int minid;
	queue *tmp = processlist;
	while(tmp != NULL){
		if(tmp->time < min){
			min = tmp->time;
			minid = tmp->taskid;
		}
		tmp = tmp->next;
	}
	return minid;
}

void schedule_SJF(task tasks[], int tasknum){	
	int *timer = create_share_var("timer");
	*timer = 0;
	int *finish = create_share_var("finish");
	*finish = -2;
	int count = 0;
	int endcount = 0;
	int end = -1;
	queue *processlist = NULL;
	while(endcount != tasknum){
		while(*timer < tasks[count].readytime && processlist == NULL){
			set_priority(getpid(), 1);
			do_idle();
			*timer+=1;	
		}
		set_priority(getpid(), 10);
		while(*timer >= tasks[count].readytime && count < tasknum){
			queue *process = init_queue(count, tasks[count].exectime);
			processlist = insert_queue(processlist, process);
			do_task_FIFO(tasks, count, timer, finish);
			count+=1;
		}
		int prior = -1;
		prior = find_shortest(processlist);
		set_priority(tasks[prior].pid, 99);
		sched_yield();
		if(*finish != end){
			end = *finish;
			processlist = delete_queue(processlist, end);
			endcount+=1;
		}
	}
	return;
}

queue *subtract_time(int id, queue *list, int *timecounter){
	queue *head = list;
	while(list->taskid != id){
		list = list->next;
	}
	list->time -= *timecounter;
	return head;
}

void schedule_PSJF(task tasks[], int tasknum){	
	int *timer = create_share_var("timer");
	*timer = 0;
	int *finish = create_share_var("finish");
	*finish = -1;
	int *next = create_share_var("next");
	*next = 0;
	int *timecounter = create_share_var("timecounter");
	*timecounter = 0;
	int count = 0;
	int endcount = 0;
	int end = -1;
	queue *processlist = NULL;
	while(endcount != tasknum){
		while(*timer < tasks[count].readytime && processlist == NULL){
			set_priority(getpid(), 1);
			do_idle();
			*timer+=1;	
		}
		set_priority(getpid(), 10);
		while(*timer >= tasks[count].readytime && count < tasknum){
			queue *process = init_queue(count, tasks[count].exectime);
			processlist = insert_queue(processlist, process);
			if(count != tasknum - 1){
				*next = tasks[count + 1].readytime;
			}
			do_task_PSJF(tasks, count, timer, finish, next, timecounter);
			count+=1;
		}
		int prior = find_shortest(processlist);
		*timecounter = 0;
		set_priority(tasks[prior].pid, 99);
		sched_yield();
		processlist = subtract_time(prior, processlist, timecounter);
		if(*finish != end){
			end = *finish;
			processlist = delete_queue(processlist, end);
			endcount+=1;
		}
	}
	return;
}

int main(int argc, char** argv){
	char policy[10];
	int tasknum;
	scanf("%s", policy);
	scanf("%d", &tasknum);
	task tasks[tasknum];
	for(int i = 0 ; i < tasknum; i++){
		scanf("%s%d%d", tasks[i].taskname, &tasks[i].readytime, &tasks[i].exectime); 
	}
	set_priority(getpid(), 10);
	bubblesort(tasks, tasknum);
	switch(policy[0]){
		case 'F':
		        schedule_FIFO(tasks, tasknum);	
			break;
		case 'R':
			schedule_RR(tasks, tasknum);
			break;
		case 'S':
			schedule_SJF(tasks, tasknum);
			break;
		case 'P':
			schedule_PSJF(tasks, tasknum);
			break;
	}
	for(int i = 0 ; i < tasknum ; i++){
		wait(NULL);
	}
	return 0;
}

