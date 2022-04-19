#include <stdio.h>     
#include <sys/types.h> 
#include <sys/ipc.h>   
#include <sys/msg.h>   

main(){
int queue_id = msgget(0567, IPC_CREAT|IPC_EXCL|0600); 
if (queue_id == -1) {
    perror("msgget");
    exit(1);
}
}
