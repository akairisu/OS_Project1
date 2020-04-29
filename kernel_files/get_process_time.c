#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/ktime.h>

asmlinkage long long sys_get_process_time(void){
	struct timespec t;
	getnstimeofday(&t);
	return (long long)t.tv_sec * 1000000000 + (long long)t.tv_nsec;
}
