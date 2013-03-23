#ifndef SUBCLIENT
#define SUBCLIENT

#include <stdint.h>
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>

class SubClient
{
    private:
    struct sockaddr_in client;
    int sock_desc;
    
	public:
	bool connect(std::string ip, int port);
	bool recv(LVData * data_out);
	bool send(uint32_t size_of_data, int8_t * data);
	void disconnect();
};

#endif /* SUBCLIENT */
