// Group : I
// Author: Wyatt Probst
// Date: 04/01/2025
// Description: Implements ACQUIRE/RELEASE and GRANT/WAIT/DENY message handling

#include <iostream>
#include <cstring>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

struct TrainMessage {
    long type; //1=request, 2=response
    int train_id;         
    char command[7]; //acquire or release
};

#define MSGKEY 1234  

void server() {
    int msgid = msgget(MSGKEY, IPC_CREAT | 0666); //Permissions may need to be adjusted

    TrainMessage message;

    while (true) {
        //Replace test output with calls to logging function
        msgrcv(msgid, &message, sizeof(message) - sizeof(long), 1, 0);
        cout << "Server received request from Train " << message.train_id << ": " << message.command << endl;

        message.type = 2; 
        strcpy(message.command, "granted");
        
        msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0);
        cout << "Server granted access to Train " << message.train_id << endl;
    }
}

void train(int train_id) {
    int msgid = msgget(MSGKEY, 0666);

    TrainMessage message;
    message.train_id = train_id;
    message.type = 1; 
    strcpy(message.command, "acquire"); 

    msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0);
    cout << "Train " << train_id << " sent request: " << message.command << endl;

    msgrcv(msgid, &message, sizeof(message) - sizeof(long), 2, 0);
    cout << "Train " << train_id << " received response: " << message.command << endl;
}

int main() {
    //Replace this part with forking logic
    pid_t pid = fork();

    if (pid == 0) {
        server();
    } 
    //Pass in parsing output
    else {
        sleep(3);  //Adjust sleep times as necessary
        train(1);
        sleep(2);
        train(2);
        sleep(2);
        train(3);
    }

    return 0;
}
