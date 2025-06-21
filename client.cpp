#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
//create client socket
//create server socket address
//connect
//send
int main(){
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    connect(clientSocket, (sockaddr *)&serverAddress, sizeof(serverAddress));
    const char *msg = "koko server labass";
    send(clientSocket, msg, strlen(msg), 0);
    close(clientSocket);
}