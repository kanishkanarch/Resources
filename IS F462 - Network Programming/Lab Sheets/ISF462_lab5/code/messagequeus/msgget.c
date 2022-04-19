#include <stdio.h>     
#include <sys/types.h> 
#include <sys/ipc.h>   
#include <sys/msg.h>   

main(){
int queue_id = msgget(IPC_PRIVATE, 0600); 
if (queue_id == -1) {
    perror("msgget");
    exit(1);
}
}
