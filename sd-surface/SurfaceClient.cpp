//The headers
#include <iostream>
#include <stdint.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <stdio.h>  
#include <string.h>  
#include <cstring>
#include <stdlib.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include "SurfaceClient.hpp"
#include "../libs/libptp++.hpp"

/*
SurfaceClient::SurfaceClient()
{
} */

bool SurfaceClient::connect(std::string ip, int port) 
{
    sock_desc = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock_desc == -1)
    {
        std::cout << "cannot create socket!\n" << std::endl;
        return false;
    }

    memset(&client, 0, sizeof(client));  
    client.sin_family = AF_INET;  
    client.sin_addr.s_addr = inet_addr(ip.c_str());  
    client.sin_port = htons(port);
    
    std::cout << "IP: " << ip << " PORT: " << port << std::endl;
    if(::connect(sock_desc, (struct sockaddr*)&client, sizeof(client)) != 0 )
    {
        close(sock_desc);
        return false;
    }
    return true;
}


bool SurfaceClient::send(uint32_t size_of_data, int8_t * data)
{

    int sent = 0;
    sent = ::send(sock_desc, &size_of_data, 4, 0);
    while(sent < size_of_data) 
    {
        int bytes_sent = 0;
        bytes_sent = ::send(sock_desc, &data+sent, size_of_data-sent, 0);
        if(bytes_sent == -1) {
            std::cout << "Cannot write to server!" << std::endl;
            return false;
        }
        sent += bytes_sent;
    }
    return true;
}

bool SurfaceClient::recv(LVData * out) 
{
    int recvd = 0;
    uint32_t size;
    ::recv(sock_desc, &size, 4, 0);
    while(recvd < size) 
    {
        int bytes_recvd;
        bytes_recvd = ::recv(sock_desc, out+recvd, size-recvd, 0);
        if(bytes_recvd == -1) {
            std::cout << "Cannot receive data!" << std::endl;
            return false;
        }
        recvd += bytes_recvd;
    }
    return true;
}

void SurfaceClient::disconnect()
{
    close(sock_desc);
}
