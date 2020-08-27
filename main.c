#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <errno.h>
#include <libcgroup.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

int child_main(){
    char* const args[]={"/bin/bash", NULL};
    chroot("./root");
    chdir("/");
    execv("/bin/bash", args);
    return 0;
}

int main(){
    int ret = 0;
    if((ret = cgroup_init()) != 0){
        printf("Error : cgroup init error, %s\n", cgroup_strerror(ret));
        return -1;
    }

    struct cgroup *group = NULL;
    group = cgroup_new_cgroup("mygroup");
    
    struct cgroup_controller *memory = NULL;
    memory = cgroup_add_controller(group, "memory");
    
    if(memory == NULL){
        printf("Error : cgroup add controller");
        return 0;
    }

    if((ret = cgroup_set_value_uint64(memory, "memory.limit_in_bytes", 1024*1024)) != 0){
        printf("Error : cgroup mem set error, %s\n", cgroup_strerror(ret));
        cgroup_free(&group);
        return -1;
    }
    if((ret = cgroup_create_cgroup(group, 0)) != 0){
        printf("Error : create cgroup error, %s\n", cgroup_strerror(ret));
        cgroup_free(&group);
        return -1;
    }
    
    void* child_stack = malloc(1024 * 1024);
    int child_pid = clone(child_main, child_stack+(1024 * 1024), CLONE_NEWNET | CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWUTS | SIGCHLD, 0);
    
    if((ret = cgroup_attach_task_pid(group, child_pid) != 0)){
        printf("Error : attach task, %s\n", cgroup_strerror(ret));
        free(child_stack);
        cgroup_delete_cgroup(group, 1);
        cgroup_free(&group);
        return -1;
    }
    
    printf("pid : %d\n", child_pid);
    
    int status = 0;

    waitpid(child_pid, &status, 0); //wait child
    printf("Error code : %d\n", status);
    free(child_stack);

    cgroup_delete_cgroup(group, 1);
    cgroup_free(&group); 
    return 0;
}
