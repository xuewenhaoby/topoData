#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include<memory.h>
#include<stdlib.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h> // sockaddr_ll
#include<arpa/inet.h>
#include<netinet/if_ether.h>
#include<iomanip>
#include<iostream>


#include "opspf.h"

using namespace std;

void analyseIP(struct iphdr *ip);
void analyseOPSPF(OPSPF_Header *opspfhdr);

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo)
{
        cout<<" HelloPacket"<<endl;
        printf("%d",helloInfo->satelliteId);
        printf("%d",helloInfo->portId);
    // OpspfInfData *inf = &opspf->infData[InterfaceIndex];

    // inf->ttl = opspf->portTTL;

    // if(inf->status==OPSPF_PORT_DOWN)
    // {
    //  UpdateOpspfRoutingTable();
    // }

}

void Opspf_Handle_LsuPacket(const OpspfLsuInfo* lsuInfo)
{
    cout<<" LsuPacket"<<endl;
    printf("%d",lsuInfo->srcSatelliteId);
    printf("%d",lsuInfo->srcPortId);
    printf("%d",lsuInfo->dstSatelliteId);
    printf("%d",lsuInfo->dstPortId);
}

int main(void)
{
    int sockfd;
     struct iphdr *ip;
     struct ethhdr *eth_header;
    char buf[2048];
    ssize_t n;
    /* capture ip datagram without ethernet header */
    if ((sockfd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_ALL)))== -1)
    {    
        printf("socket error!\n");
        return 1;
    }
    while (1)
    {
        n = recv(sockfd, buf, sizeof(buf), 0);
        if (n == -1)
        {
            printf("recv error!\n");
            break;
        }
        else if (n==0)
            continue;
        eth_header = (struct ethhdr*)(buf);
        ip = ( struct iphdr *)(buf+sizeof(ethhdr));
        analyseIP(ip);
        // size_t iplen =  (ip->h_verlen&0x0f)*4;
        // // TCP_HEADER *tcp = (TCP_HEADER *)(buf +iplen);
        if (ip->protocol == IPPROTO_OPSPF)
        {
            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(ethhdr)+sizeof(struct iphdr));
           // analyseOPSPF(opspfhdr);
            switch(opspfhdr->packetType)
            {
                case OPSPF_HELLO_PACKET:
                  {
                    OpspfHelloInfo *helloInfo =(OpspfHelloInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
                    Opspf_Handle_HelloPacket(helloInfo);
                    break;
                  }
                case OPSPF_LSU_PACKET:
                {
                    OpspfLsuInfo *lsuInfo =(OpspfLsuInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
                    Opspf_Handle_LsuPacket(lsuInfo);
                    break; 
                }

                default:
                    break;
            }
        }
        else
        {
            printf("other protocol!\n");
        }        
       // printf("\n\n");
    }
    close(sockfd);
    return 0;
}

void analyseIP(struct iphdr *ip)
{
    unsigned char* p = (unsigned char*)&ip->saddr;
    printf("Source IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
    p = (unsigned char*)&ip->daddr;
    printf("Destination IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);

}
void analyseOPSPF(OPSPF_Header* opspfhdr)
{
    printf("OPSPF -----\n");
    // printf("type: %u\n", icmp->icmp_type);
    // printf("sub code: %u\n", icmp->icmp_code);
}