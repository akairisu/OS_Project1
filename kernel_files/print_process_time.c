#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage void sys_print_process_time(pid_t pid, long long start, long long end){
	start %= 1000000000000000000;
	end %= 1000000000000000000;	
	long long start_time = start / 1000000000;
	long long end_time = end / 1000000000;
	long long start_time_nano = start % 1000000000;
	long long end_time_nano = end % 1000000000;	
	printk("[Project1] %d %09lld.%09lld %09lld.%09lld\n", pid, start_time, start_time_nano, end_time, end_time_nano);
	return;
}
