// Group: I
// Author: Wyatt Probst
// Date: 04/20/2025
// Description: Implements ACQUIRE/RELEASE message passing to and from server and train. Has placeholders in place to test that
// should be replaced with calls to the other files.

#include <iostream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string>

using namespace std;

struct TrainData {
    long type; //1 for request and 2 for response
    int id; //train number
    string message; //Acquire or release message
}; 

key_t key = 1; //update key if needed
int messageQueue = msgget(key, 0666 | IPC_CREAT);  //creates message queue 

void server() {
    TrainData data;
    
    while (true) {
        msgrcv(messageQueue, &data, sizeof(data), 1, 0); //waits to receive messages from trains with while(true)
        cout << "Request received " << data.id << " received request" << endl; //test messages to be replaced with logging function
        
        data.type = 2; 
        data.message = "release";
        
        msgsnd(messageQueue, &data, sizeof(data), 0); // sends messages back to trains to grant or deny
        cout << "Request received " << data.id << " granted or denied access" << endl; //need to implement synchronization
    }
}

void train(int id) {
    TrainData data;
    data.id = id;
    data.type = 1;
    data.message = "acquire";
    
    msgsnd(messageQueue, &data, sizeof(data), 0); //sends messages to server
    cout << "Train " << id << " sent request" << endl;
    
    msgrcv(messageQueue, &data, sizeof(data), 2, 0); //receives message from server
    cout << "Train " << id << " received message" << endl;
}

int main() {
    //Replace this with forking logic
    pid_t pid = fork();

    if (pid == 0) {
        server();
    } 
    //Pass in parsing output
    else {
        //sleep(3);  //Adjust sleep times as necessary
        train(1);
        //sleep(2);  //Without the sleep the server may not print on csx server
        train(2);
        //sleep(2);
        train(99999);
    }
    return 0;
}
