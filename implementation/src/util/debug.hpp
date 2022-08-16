#ifndef DEBUG_HPP
#define DEBUG_HPP

/*
	attach gdb in realtime to current process and print stack trace
	extremely useful for multi-process environments like mpi
	
	**complete code in this file is from**
	https://stackoverflow.com/questions/4636456/how-to-get-a-stack-trace-for-c-using-gcc-with-line-number-information
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/prctl.h>

/** \brief print_trace attaches gdb and prints a stack trace
 * 
 * details attaching gdb in realtime to the current process
 * and printing a stack trace is extremely useful for
 * multi-process environments like mpi. This function is from:
 * https://stackoverflow.com/questions/4636456/how-to-get-a-stack-trace-for-c-using-gcc-with-line-number-information 
 * 
 * return bool is always false
 */
bool print_trace() {
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)]=0;
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
    int child_pid = fork();
    if (!child_pid) {
        dup2(2,1); // redirect output to stderr - edit: unnecessary?
        execl("/usr/bin/gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
        abort(); /* If gdb failed to start */
    } else {
        waitpid(child_pid,NULL,0);
    }
    
    return false;
}

#endif
