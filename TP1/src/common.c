#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>

#include <arpa/inet.h>

int addrparse(const char *addrstr, const char *portstr,
                struct sockaddr_storage *storage)
{
    if(addrstr == NULL || portstr == NULL){
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if(port == 0){
        return -1;
    }  
    port  = htons(port); //host to network short

    struct in_addr inaddr4; // 32-bit IP address
    if(inet_pton(AF_INET, addrstr, &inaddr4)){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IP address
    if(inet_pton(AF_INET6, addrstr, &inaddr6)){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6;
        memcpy(&(addr6->sin6_addr), inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;

}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize){
   int version;
   char addrstr[INET6_ADDRSTRLEN + 1] = "";
   uint16_t port;



   if(addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
             INET6_ADDRSTRLEN + 1)){
                 logexit("ntop");
        }
        port = ntohs(addr4->sin_port); //network to host short
   }else if( addr->sa_family == AF_INET6){
       version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
             INET6_ADDRSTRLEN + 1)){
                 logexit("ntop");
        }
        port = ntohs(addr6->sin6_port); //network to host short
   }else {
       logexit('unknow protocol family');
   }
    if(str){
        snprintf(str, strsize, "IPV%d %s %hu", version, addrstr, port);
    }
}