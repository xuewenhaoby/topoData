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
#include "thread_pool.h"
#include "message.h"
#include "common.h"
#include "task.h"
#include "topo.h"
#include "opspf.h"


using namespace std;




int main(int argc,char* argv[])
{
	satelliteId = atoi(argv[1]);
	initialize(satelliteId);


	pthread_t recv;
	pthread_create(&recv,NULL,recv_opspf, NULL);
	pthread_join(recv,NULL);
	route_test();
}

void initialize(int satelliteId)
{
	initializeAllInterface(satelliteId);
	InitStaticTopo();
	initializeAllInt_DB();
}


void initializeAllInterface(int satelliteId)
{
	for(int i=0;i<SINF_NUM;i++)
    {
    	selfInf[i] =interface_init(satelliteId,i); 
    	selfInf[i].changeStat = OPSPF_PORT_NOCHANGE; 
    	selfInf[i].stat = true;   
    	selfInf[i].ttl = OPSPF_PORT_TTL;   
    	selfInf[i].lsuAck = false;
    }
    for (int i = 0; i < SAT_NUM; ++i)
    {
    	for (int j = 0; j < SINF_NUM; ++j)
    	{
			Int_DB[i].linkId[j]=-1;
			Int_DB[i].port_stat[j]=true;
			Int_DB[i].dst_satid[j]=-1;
			Int_DB[i].dst_portid[j]=-1;
    	}
    }
}

void initializeAllInt_DB()
{
	for (int i = 1; i <= SAT_NUM; ++i)
	{
		Int_DB_init(i);
	}
}

void Int_DB_init(int satelliteId)
{
	for(int i = 0; i < SINF_NUM; i++)
	{
		int linkId = selfInf[i].linkId;
		Int_DB[satelliteId-1].linkId[i]=selfInf[i].linkId;
		//Int_DB initial 
	    if(linkId!=-1)
	    {
		    if(G.isl[selfInf[i].linkId-1].endpoint[0].nodeId==satelliteId)
		    {  
		        Int_DB[satelliteId-1].dst_satid[i]=G.isl[selfInf[i].linkId-1].endpoint[1].nodeId;  
	            Int_DB[satelliteId-1].dst_portid[i]=G.isl[selfInf[i].linkId-1].endpoint[1].inf;
		    }
		    else
		    {
			Int_DB[satelliteId-1].dst_satid[i]=G.isl[selfInf[i].linkId-1].endpoint[0].nodeId;
			Int_DB[satelliteId-1].dst_portid[i]=G.isl[selfInf[i].linkId-1].endpoint[0].inf;
		    }
	    }
	}
}

OpspfInfData interface_init(int satelliteId,int portId)
{
	int sd; 
	int  one = 1;
	const int *val = &one;
	int ret;
	char dev[BUF_STR_SIZE];
	sprintf(dev,"sat%dp%d",satelliteId,portId);
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

void *Send_Hello_Packet(void *ptr)
{
	for (int i = 0; i < SINF_NUM; i++)
	{
		if(selfInf[i].stat)
		{
			if(0 == selfInf[i].ttl--)//Time exceeds limit,judge the link port is down
			{
				selfInf[i].changeStat = OPSPF_PORT_LINKDOWN;
				//selfInf[i].stat = false;
				Int_DB[satelliteId-1].port_stat[i] = false;
				//UpdateOpspfRoutingTable();
				// pthread_t sendLsu;
		  //       pthread_create(&sendLsu,NULL,Send_Lsu_Packet, NULL);
		  //       pthread_join(sendLsu,NULL);

			}
			else
			{
				OpspfSendProtocolPacket(selfInf[i],OPSPF_HELLO_PACKET);
			}
		}
		else
		{
			cout<<"All ports down !"<<endl;
		}
	
	}
	sleep(10);
	pthread_exit(NULL);
}

void Send_Hello_Packet()
{
	for (int i = 0; i < SINF_NUM; i++)
	{
		if(selfInf[i].stat)
		{
			if(0 == selfInf[i].ttl--)//Time exceeds limit,judge the link port is down
			{
				cout<<to_string(i)<<"port is down"<<endl;
				selfInf[i].changeStat = OPSPF_PORT_LINKDOWN;
				Int_DB[satelliteId-1].port_stat[i] = false;
				//UpdateOpspfRoutingTable();
			  			pool.addTask(new NodeTask(new Message(
			 		MSG_NODE_SendLsuPacket)));

			}
			else
			{
				OpspfSendProtocolPacket(selfInf[i],OPSPF_HELLO_PACKET);
			}
		}
	
	}
}

void *Send_Lsu_Packet(void* ptr)
{
	for (int i = 0; i < SINF_NUM; ++i)
	{
		if(selfInf[i].changeStat != OPSPF_PORT_NOCHANGE)
		{
			lsuData.changeStat = selfInf[i].changeStat ;
			lsuData.satelliteId = satelliteId;
			lsuData.portId = i;
			lsuData.srcSatelliteId = satelliteId;
			lsuData.srcPortId = i;

			Isl isl = G.isl[ selfInf[i].linkId-1 ];
			for(int k = 0; k < 2; k++)
			{
				if(satelliteId == isl.endpoint[i].nodeId)
				{
					lsuData.dstSatelliteId = isl.endpoint[(k+1)%2].nodeId;
					lsuData.dstPortId      = isl.endpoint[(k+1)%2].inf;
				}
			}
			for (int j = 0; j < SINF_NUM; ++j)
			{
				if(selfInf[j].stat == true && selfInf[j].lsuAck == false)
				{
					selfInf[j].lsutimestamp = GetTime();
					OpspfSendProtocolPacket(selfInf[j],OPSPF_LSU_PACKET);
				}
			selfInf[i].changeStat = OPSPF_PORT_NOCHANGE;
			}

		}
	}

	sleep(10);
	//pthread_exit(NULL);
}

void Send_Lsu_Packet()
{
	for (int i = 0; i < SINF_NUM; ++i)
	{
		if(selfInf[i].changeStat != OPSPF_PORT_NOCHANGE && selfInf[i].stat ==true)
		{
			for (int j = 0; j < SINF_NUM; ++j)
			{
				selfInf[j].lsuAck = false;
			}
			lsuData.changeStat = selfInf[i].changeStat ;
			lsuData.srcSatelliteId = satelliteId;
			lsuData.srcPortId = i;

			Isl isl = G.isl[ selfInf[i].linkId-1 ];
			for(int k = 0; k < 2; k++)
			{
				if(satelliteId == isl.endpoint[i].nodeId)
				{
					lsuData.dstSatelliteId = isl.endpoint[(k+1)%2].nodeId;
					lsuData.dstPortId      = isl.endpoint[(k+1)%2].inf;
				}
			}
	
		}
		
	}

	for (int j = 0; j < SINF_NUM; ++j)//flood from each port 
	{
		if(selfInf[j].stat == true && selfInf[j].lsuAck == false && Int_DB[satelliteId-1].port_stat[j] == true)
		{
			lsuData.satelliteId = satelliteId;
			lsuData.portId = j;
			selfInf[j].lsutimestamp = GetTime();
			cout<<to_string(lsuData.srcSatelliteId)+to_string(lsuData.srcPortId)+to_string(j)<<endl;
			OpspfSendProtocolPacket(selfInf[j],OPSPF_LSU_PACKET);
		}
	}



	for (int j = 0; j < SINF_NUM; ++j)
	{
		if(selfInf[j].stat == true && selfInf[j].lsuAck == false)
		{
			selfInf[j].stat ==false;
		

		}		
	}
}

void Flood_Lsu_Packet(int interfaceIndex)
{
	

	if(recv_send_LSU ==true)
	{
		for (int j = 0; j < SINF_NUM; ++j)
		{
			selfInf[j].lsuAck = false;
		}

		for (int j = 0; j < SINF_NUM; ++j)
		{
			if(selfInf[j].stat == true && selfInf[j].lsuAck == false && Int_DB[satelliteId-1].port_stat[j] == true && j != interfaceIndex)
			{
				selfInf[j].lsutimestamp = GetTime();
				cout<<to_string(lsuData.srcSatelliteId)+to_string(lsuData.srcPortId)+to_string(j)<<endl;
				OpspfSendProtocolPacket(selfInf[j],OPSPF_LSU_PACKET);
			}

		}
		sleep(OPSPF_LSU_TTL);
	}
	

	for (int j = 0; j < SINF_NUM; ++j)
	{
		if(selfInf[j].stat == true && selfInf[j].lsuAck == false)
		{
			selfInf[j].stat ==false;
			selfInf[j].changeStat = OPSPF_PORT_NOCHANGE;

		}		
	}
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
			
			//cout<<"Send_Lsu_Packet satelliteId"+ to_string(lsuInfo->srcSatelliteId) +" portId" +to_string(lsuInfo->srcPortId)<<endl;
			lsuackInfo->receive_satelliteId = receive_satelliteId;
			lsuackInfo->receive_portId = receive_portId;
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(OPSPF_Header),lsuackInfo , sizeof(lsuackInfo));
			break;
		}
		default:
		{
			cout<<"Unknown protocol packet type to send!"<<endl;
			break;
		}
	}

	// for (int i = 0; i < SAT_NUM; i++)
	// {
	// 	for (int j = 0; j < SAT_NUM; j++)
	// 	{
	// 		int linkid=G.arcs[i][j].linkId;
	// 		if(linkid!=0 && linkid !=-1)
	// 		{
	// 			if(G.isl[linkid-1].endpoint[0].nodeId==inf.satelliteId )
	// 		    {
	// 		         inf.gw_addr = G.isl[linkid-1].endpoint[1].ip;
	// 		    }
	// 		    else if(G.isl[linkid-1].endpoint[1].nodeId==inf.satelliteId )
	// 		    {
	// 		         inf.gw_addr = G.isl[linkid-1].endpoint[0].ip;
	// 		    }
	// 		}

	// 	}


	// }

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
		//perror("sendto() error");
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
	/* capture ip datagram without ethernet header */
	// struct sockaddr_in sin;
	// sin.sin_family = AF_INET;

	// sin.sin_addr.s_addr = inet_addr("190.0.5.1");


	// sockfd = socket(AF_INET,SOCK_DGRAM,0);
	// if(bind(sockfd,(struct sockaddr*)&sin,sizeof(sin))<0)
	// {
	// 	printf("bind error!\n");
	// }
	//if ((sockfd = socket(PF_PACKET,  SOCK_DGRAM, 0))== -1)
	if ((sockfd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_ALL)))== -1)
	{    
	    printf("socket error!\n");
	}
	StartTimer();
	//route_test();

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
	    // route_test();
	    if(ip->protocol != IPPROTO_OPSPF )
	    {
	    	continue;
	    }
	    if(ip->saddr == selfInf[0].ip || ip->saddr == selfInf[1].ip || ip->saddr == selfInf[2].ip || ip->saddr == selfInf[3].ip 
	        	|| ip->saddr == selfInf[4].ip || ip->saddr == selfInf[5].ip)//self packet
        {
        	continue;

 		}
 		else
 		{
 			for (int i = 0; i < SINF_NUM; ++i)
		    	{
		    		if(ip->daddr == selfInf[i].ip)
		    		{
		    			interfaceIndex = i;
		    		}
		    	}
		    		unsigned char* tmp1 = (unsigned char*)&ip->daddr;
		        	unsigned char* tmp2;
		        	for (int i = 0; i < SINF_NUM; ++i)
			    	{
			    		tmp2 = (unsigned char*)&selfInf[i].ip;
			    		if(tmp1[0]==tmp2[0]&&tmp1[1]==tmp2[1]&&tmp1[2]==tmp2[2])// the same subnet ip 
			    		{
			    			interfaceIndex = i;
			    			//cout<<"Port"+to_string(interfaceIndex)+" receive pkt"<<endl;
			    		}
			    	}
			        //analyseIP(ip);
			        if (ip->protocol == IPPROTO_OPSPF)
			        {
			            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(ethhdr)+sizeof(struct iphdr));
			            //analyseOPSPF(opspfhdr);
			            switch(opspfhdr->packetType)
			            {
			                case OPSPF_HELLO_PACKET:
			                  {
			                    OpspfHelloInfo *helloInfo =(OpspfHelloInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			                    Opspf_Handle_HelloPacket(helloInfo,interfaceIndex);
			                    break;
			                  }
			                case OPSPF_LSU_PACKET:
			                {
			                    OpspfLsuInfo *lsuInfo =(OpspfLsuInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			                    Opspf_Handle_LsuPacket(lsuInfo,interfaceIndex);
			                    break; 
			                }
			                case OPSPF_LSACK_PACKET:
			                {
			                	OpspfLsuackInfo *lsuackInfo =(OpspfLsuackInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			                    Opspf_Handle_LsuackPacket(lsuackInfo,interfaceIndex);
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
 		}	  
	}
	EndTimer();
	close(sockfd);
    
}

// void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType)
// {
// 	char buffer[PCKT_LEN] ;
// 	int count = 0;
// 	//struct ethhdr *eth_header = (struct ethhdr *)buffer;
// 	struct iphdr *ip = (struct iphdr *)buffer;
// 	OPSPF_Header* opspf_header =(OPSPF_Header*)(buffer + sizeof(struct iphdr));

// 	//缓存清零
// 	memset(buffer, 0, PCKT_LEN);

// 	switch(packetType)
// 	{
// 		case OPSPF_HELLO_PACKET:
// 		{		
// 			opspf_header->packetType = OPSPF_HELLO_PACKET;
// 			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfHelloInfo);
// 			memcpy(buffer + sizeof(iphdr) ,opspf_header , sizeof(OPSPF_Header));

// 			OpspfHelloInfo* helloInfo = (OpspfHelloInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			helloInfo->satelliteId = inf.satelliteId;
// 			helloInfo->portId =inf.portId;
// 			//cout<<"Send_Hello_Packet satelliteId"+ to_string(helloInfo->satelliteId) +" portId" +to_string(helloInfo->portId)<<endl;

			

// 			memcpy(buffer + sizeof(iphdr)+sizeof(OPSPF_Header),helloInfo, sizeof(OpspfHelloInfo));
// 			break;
// 		}
// 		case OPSPF_LSU_PACKET:
// 		{
// 			opspf_header->packetType = OPSPF_LSU_PACKET;
// 			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuInfo);
// 			memcpy(buffer + sizeof(iphdr) ,opspf_header , sizeof(OPSPF_Header));

// 			OpspfLsuInfo* lsuInfo = (OpspfLsuInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			*lsuInfo  = lsuData;
// 			//cout<<"Send_Lsu_Packet satelliteId"+ to_string(lsuInfo->srcSatelliteId) +" portId" +to_string(lsuInfo->srcPortId)<<endl;

// 			memcpy(buffer + sizeof(iphdr)+sizeof(OPSPF_Header),lsuInfo , sizeof(lsuInfo));
// 			break;
// 		}
// 		case OPSPF_LSACK_PACKET:
// 		{
// 			opspf_header->packetType = OPSPF_LSACK_PACKET;
// 			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuackInfo);
// 			memcpy(buffer + sizeof(iphdr) ,opspf_header , sizeof(OPSPF_Header));

// 			OpspfLsuackInfo* lsuackInfo = (OpspfLsuackInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			
// 			//cout<<"Send_Lsu_Packet satelliteId"+ to_string(lsuInfo->srcSatelliteId) +" portId" +to_string(lsuInfo->srcPortId)<<endl;
// 			lsuackInfo->receive_satelliteId = receive_satelliteId;
// 			lsuackInfo->receive_portId = receive_portId;
// 			memcpy(buffer + sizeof(iphdr)+sizeof(OPSPF_Header),lsuackInfo , sizeof(lsuackInfo));
// 			break;
// 		}
// 		default:
// 		{
// 			cout<<"Unknown protocol packet type to send!"<<endl;
// 			break;
// 		}
// 	}

// 	// for (int i = 0; i < SAT_NUM; i++)
// 	// {
// 	// 	for (int j = 0; j < SAT_NUM; j++)
// 	// 	{
// 	// 		int linkid=G.arcs[i][j].linkId;
// 	// 		if(linkid!=0 && linkid !=-1)
// 	// 		{
// 	// 			if(G.isl[linkid-1].endpoint[0].nodeId==inf.satelliteId )
// 	// 		    {
// 	// 		         inf.gw_addr = G.isl[linkid-1].endpoint[1].ip;
// 	// 		    }
// 	// 		    else if(G.isl[linkid-1].endpoint[1].nodeId==inf.satelliteId )
// 	// 		    {
// 	// 		         inf.gw_addr = G.isl[linkid-1].endpoint[0].ip;
// 	// 		    }
// 	// 		}

// 	// 	}


// 	// }

// 	struct sockaddr_in sin, din;
// 	sin.sin_family = AF_INET;
// 	din.sin_family = AF_INET;
// 	sin.sin_addr.s_addr = inf.ip;
// 	din.sin_addr.s_addr = INADDR_ANY;

 
// 	// Fabricate the IP header or we can use the
// 	// standard header structures but assign our own values.
// 	ip->ihl = COMMON_IPHDR_LEN;
// 	ip->version = IP_VERSION;//报头长度，4*32=128bit=16B
// 	ip->tos = DEFAULT_OPSPF_TOS; // 服务类型
// 	ip->tot_len = (sizeof(struct iphdr) + opspf_header->pktlen);
// 	//ip->id = htons(54321);//可以不写
// 	ip->ttl = DEFAULT_OPSPF_TTL; // hops生存周期
// 	ip->protocol = IPPROTO_OPSPF; // OPSPF
// 	ip->check = 0;

// 	ip->saddr = sin.sin_addr.s_addr;
// 	ip->daddr = din.sin_addr.s_addr;

// 	// struct sockaddr_ll dstmac;
// 	// memset(&dstmac,0,sizeof(dstmac));
// 	// dstmac.sll_family = AF_PACKET;
// 	// dstmac.sll_ifindex = inf.if_index;
// 	// dstmac.sll_halen = htons(ETH_HLEN);
// 	// memcpy(dstmac.sll_addr,inf.if_mac,dstmac.sll_halen);
// 	//memcpy(eth_header->h_dest , inf.if_mac,ETH_ALEN);
 
// 	setuid(getpid());//如果不是root用户，需要获取权限	
// 	if (sendto(inf.sock, buffer, ip->tot_len, 0, (struct sockaddr *)&din, sizeof(din)) < 0)	
// 	//if (sendto(inf.sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dstmac, sizeof(dstmac)) < 0)
// 		// Verify
// 	{
// 		perror("sendto() error");
// 	}	
// 	else 
// 	{
// 		cout<<"send is ok"<<endl;
// 	}
// 		//close(inf.sock);
	
// }

// void *recv_opspf(void *ptr)
// {
// 	int sockfd;
// 	struct iphdr *ip;
// 	struct ethhdr *eth_header;
// 	char buf[2048];
// 	ssize_t n;
// 	int interfaceIndex;
// 	/* capture ip datagram without ethernet header */
// 	//if ((sockfd = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP)))== -1)
// 	if ((sockfd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_ALL)))== -1)
// 	{    
// 	    printf("socket error!\n");
// 	}
// 	StartTimer();
// 	while (1)
// 	{

// 	    n = recv(sockfd, buf, sizeof(buf), 0);
// 	    if (n == -1)
// 	    {
// 	        printf("recv error!\n");
// 	        break;
// 	    }
// 	    else if (n==0)
// 	        continue;
// 	    //eth_header = (struct ethhdr*)(buf);
// 	    ip = ( struct iphdr *)(buf);
// 	    //route_test();
// 	    if(ip->protocol != IPPROTO_OPSPF )
// 	    {
// 	    	continue;
// 	    }
// 	    if(ip->saddr == selfInf[0].ip || ip->saddr == selfInf[1].ip || ip->saddr == selfInf[2].ip || ip->saddr == selfInf[3].ip 
// 	        	|| ip->saddr == selfInf[4].ip || ip->saddr == selfInf[5].ip)//self packet
//         {
//         	continue;

//  		}
//  		else
//  		{
//  			for (int i = 0; i < SINF_NUM; ++i)
// 		    	{
// 		    		if(ip->daddr == selfInf[i].ip)
// 		    		{
// 		    			interfaceIndex = i;
// 		    		}
// 		    	}
// 		    		unsigned char* tmp1 = (unsigned char*)&ip->daddr;
// 		        	unsigned char* tmp2;
// 		        	for (int i = 0; i < SINF_NUM; ++i)
// 			    	{
// 			    		tmp2 = (unsigned char*)&selfInf[i].ip;
// 			    		if(tmp1[0]==tmp2[0]&&tmp1[1]==tmp2[1]&&tmp1[2]==tmp2[2])// the same subnet ip 
// 			    		{
// 			    			interfaceIndex = i;
// 			    			//cout<<"Port"+to_string(interfaceIndex)+" receive pkt"<<endl;
// 			    		}
// 			    	}
// 			        //analyseIP(ip);
// 			        if (ip->protocol == IPPROTO_OPSPF)
// 			        {
// 			            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(struct iphdr));
// 			            //analyseOPSPF(opspfhdr);
// 			            switch(opspfhdr->packetType)
// 			            {
// 			                case OPSPF_HELLO_PACKET:
// 			                  {
// 			                    OpspfHelloInfo *helloInfo =(OpspfHelloInfo*)(buf +sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			                    Opspf_Handle_HelloPacket(helloInfo,interfaceIndex);
// 			                    break;
// 			                  }
// 			                case OPSPF_LSU_PACKET:
// 			                {
// 			                    OpspfLsuInfo *lsuInfo =(OpspfLsuInfo*)(buf +sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			                    Opspf_Handle_LsuPacket(lsuInfo,interfaceIndex);
// 			                    break; 
// 			                }
// 			                case OPSPF_LSACK_PACKET:
// 			                {
// 			                	OpspfLsuackInfo *lsuackInfo =(OpspfLsuackInfo*)(buf +sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			                    Opspf_Handle_LsuackPacket(lsuackInfo,interfaceIndex);
// 			                    break; 
// 			                }

// 			                default:
// 			                    break;
// 			            }
// 			        }
// 			        else
// 			        {
// 			            printf("other protocol!\n");
// 			        }  
//  		}	  
// 	}
// 	EndTimer();
// 	close(sockfd);
    
// }

void analyseIP(struct iphdr *ip)
{
    unsigned char* p = (unsigned char*)&ip->saddr;
    //printf("Source IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
    p = (unsigned char*)&ip->daddr;
    //printf("Destination IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
}

void analyseOPSPF(OPSPF_Header* opspfhdr)
{
    //printf("OPSPF -----\n");
}

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo,int interfaceIndex)
{

   cout<<"HelloPacket"+to_string(helloInfo->satelliteId)+ to_string(helloInfo->portId)<<endl;
   //cout<<"satelliteId : " +to_string(helloInfo->satelliteId)<<endl;
   //cout<<"portId : " + to_string(helloInfo->portId)<<endl;
    selfInf[interfaceIndex].ttl = OPSPF_PORT_TTL;

    if(Int_DB[satelliteId-1].port_stat[interfaceIndex] == false) 
    {
    	selfInf[interfaceIndex].changeStat = OPSPF_PORT_RELINK;
    	Int_DB[satelliteId-1].port_stat[interfaceIndex] = true;
    	UpdateOpspfRoutingTable();
    	pthread_exit(NULL);
    	route_test();
    				

   //  	pool.addTask(new NodeTask(new Message(
 		// MSG_ROUTE_Update)));

    	// for (int i = 0; i < SINF_NUM; ++i)
	    // {
	    // 	OpspfSendProtocolPacket(selfInf[i],OPSPF_LSU_PACKET);
	    // }
		// pthread_t sendLsu;
  //       pthread_create(&sendLsu,NULL,Send_Lsu_Packet, NULL);
  //       pthread_join(sendLsu,NULL);
		pool.addTask(new NodeTask(new Message(
		    MSG_NODE_SendLsuPacket)));
  
    }  
}

void Opspf_Handle_LsuPacket(const OpspfLsuInfo* lsuInfo,int interfaceIndex)
{
    
    recv_send_LSU =true;
    recv_send_LSU_index =interfaceIndex;
    int srcSatelliteId = lsuInfo->srcSatelliteId;
    int srcPortId = lsuInfo->srcPortId;
    int dstSatelliteId = lsuInfo->dstSatelliteId;
    int dstPortId = lsuInfo->dstPortId;
    int stat =lsuInfo->changeStat;
	cout<<" LsuPacket"+to_string(srcSatelliteId)+to_string(srcPortId)<<endl;

   // cout<<"srcSatelliteId : "+to_string(srcSatelliteId)<<endl;
   // cout<<"srcPortId : "+to_string(srcPortId)<<endl;
    // cout<<"dstSatelliteId : "+to_string(dstSatelliteId)<<endl;
    // cout<<"dstPortId : "+to_string(dstPortId)<<endl;
    // cout<<"stat : "+to_string(stat)<<endl;

    receive_satelliteId = lsuInfo->satelliteId;
    receive_portId = lsuInfo->portId;
    
    OpspfSendProtocolPacket(selfInf[interfaceIndex],OPSPF_LSACK_PACKET);//sending back ACK packet

    int linkid = G.arcs[srcSatelliteId-1][dstSatelliteId-1].linkId;

    bool flood = false;

    if(lsuInfo->changeStat == OPSPF_PORT_LINKDOWN )
    {
    	// if(*lsuInfo == lsuData)
    	// {
    	// 	flood = false;
    	// }

    		if(Int_DB[srcSatelliteId-1].port_stat[srcPortId] == true ||Int_DB[dstSatelliteId-1].port_stat[dstPortId] == true)
	    	{	
	    		flood = true;
	    		cout<<"flood_1"<<endl;
				Int_DB[srcSatelliteId-1].port_stat[srcPortId] == false;
				Int_DB[dstSatelliteId-1].port_stat[dstPortId] == false;

	    	}
 

    }
    else if(lsuInfo->changeStat == OPSPF_PORT_RELINK)
    {
    	// if(*lsuInfo == lsuData)
    	// {
    	// 	flood = false;
    	// }

    		if(Int_DB[srcSatelliteId-1].port_stat[srcPortId] == false ||Int_DB[dstSatelliteId-1].port_stat[dstPortId] == false)
	    	{
	    		flood = true;
	    		cout<<"flood_2"<<endl;
				Int_DB[srcSatelliteId-1].port_stat[srcPortId] == true;
				Int_DB[dstSatelliteId-1].port_stat[dstPortId] == true;
	    	}
    	

    }
    if(flood)
    {
    	UpdateOpspfRoutingTable();
		// pool.addTask(new NodeTask(new Message(
 	// 	MSG_ROUTE_Update)));
    	route_test();
    	lsuData = *lsuInfo;
	    // for (int i = 0; i < SINF_NUM; ++i)
	    // {
	    // 	if(i!=interfaceIndex && selfInf[i].stat == true )
	    // 	{
	    // 		OpspfSendProtocolPacket(selfInf[i],OPSPF_LSU_PACKET);
	    // 	}

	    // }
	   pool.addTask(new NodeTask(new Message(
			 		MSG_NODE_FloodLsuPacket)));
    	
    }
}

void Opspf_Handle_LsuackPacket(const OpspfLsuackInfo* lsuackInfo,int interfaceIndex)
{
    cout<<"ACKPacket"+to_string(lsuackInfo->receive_satelliteId)+to_string(lsuackInfo->receive_portId)<<endl;
    //cout<<"satelliteId : " +to_string(lsuackInfo->receive_satelliteId)<<endl;
    //cout<<"portId : " + to_string(lsuackInfo->receive_portId)<<endl;
   if(GetTime() - selfInf[interfaceIndex].lsutimestamp <= 1000 )// not exceed the lsuack interval, opposite point receive the LSU packet timely
   {
   		//cout<<to_string(GetTime() - selfInf[interfaceIndex].lsutimestamp)<<"not exceed the lsuack interval"<<endl;
    	if(lsuackInfo->receive_satelliteId == lsuData.satelliteId && lsuackInfo->receive_portId == interfaceIndex)
    	{
    		selfInf[interfaceIndex].lsuAck = true;
    		selfInf[interfaceIndex].changeStat = OPSPF_PORT_NOCHANGE;

    	}
    	
    }
    else
    {

    }
   
}

void UpdateOpspfRoutingTable()
{
	int path[SAT_NUM],next_hop[SAT_NUM];
	unsigned int distance[SAT_NUM];
	int src_portid,dst_id,linkId,gw_id;
	int dst_linkid;
	int temp_link;
    NodeAddress gw_addr;

    for(int i=0; i<SAT_NUM; i++)
    {
    	for(int j=0;j<SAT_NUM;j++)
    	{
    		temp_link = G.arcs[i][j].linkId;
    		int k =0;
    		if(temp_link != -1 && temp_link!=0) //exists link between satellite nodes
    		{
    			if(G.isl[temp_link-1].endpoint[0].nodeId==i+1)
    			{
    				k=G.isl[temp_link-1].endpoint[0].inf;
    			}
    			else
    				k=G.isl[temp_link-1].endpoint[1].inf;
    			if(Int_DB[i].port_stat[k]== true)
    				sup_array[i][j]=G.arcs[i][j].weight;
    			else
    				sup_array[i][j]=MAX_ARC_WEIGHT;

    		}
    		else 
    			sup_array[i][j]=MAX_ARC_WEIGHT;
    	}
     }

    Dijkstra(sup_array,satelliteId,distance,path);
    cout<<"dijkstra"<<endl;

    for (int i = 0; i < SAT_NUM; ++i)
    {
    	next_hop[i]=FindPreNode(path,satelliteId,i+1);
    	if(i != satelliteId && next_hop[i] != -1)
    	{
    		gw_id = next_hop[i]+1;
    		dst_id = i+1;
    		linkId=G.arcs[satelliteId-1][next_hop[i]].linkId;

    		if(linkId != -1)
    		{
    			if(G.isl[linkId-1].endpoint[0].nodeId==satelliteId)
                {
                     src_portid=G.isl[linkId-1].endpoint[0].inf;
                     //gw_portid=topo.isl[linkid-1].endpoint[1].inf;
                     gw_addr = G.isl[linkId-1].endpoint[1].ip;
                }
                else
                {
                     src_portid=G.isl[linkId-1].endpoint[1].inf;
                     //gw_portid=topo.isl[linkid-1].endpoint[0].inf;
                     gw_addr = G.isl[linkId-1].endpoint[0].ip;
                }

                route_table.src_id = satelliteId;
                route_table.gw_addr=gw_addr;
                route_table.src_portid = src_portid;
                route_table.linkid = linkId;

    //            	LinkRouteTask *lrt=new LinkRouteTask((void*)&route_table);
				// link_route_pool.addTask(lrt);

    		}
    	}
    }
}

void Routing_table(int src_id,StaticTopo topo,ISDB Int_DB[])
{
		// int path[SAT_NUM],next_hop[SAT_NUM];
	//     unsigned int distance[SAT_NUM];
	//     //variable for route_write
	//     int src_portid,dst_id,linkid,gw_id;
	//     int dst_linkid;
	//     NodeAddress gw_addr;
	//     //support array_weight_update
	//     for(int i=0;i<SAT_NUM;i++)
	//     {
	//         for(int j=0;j<SAT_NUM;j++)
	//         {
	// 			int temp_link = topo.arcs[i][j].linkId;
	// 			int k =0;
	// 			if(temp_link!=-1&&temp_link!=0)
	// 			{
	// 			if(topo.isl[temp_link-1].endpoint[0].nodeId==i+1)
	// 			    k=topo.isl[temp_link-1].endpoint[0].inf;

	// 			else
	// 			    k=topo.isl[temp_link-1].endpoint[1].inf;
	//             if(Int_DB[i].port_stat[k]==1)
	//             sup_array[i][j]=topo.arcs[i][j].weight;
	// 			else 
	// 			sup_array[i][j]=MAX_ARC_WEIGHT;
	// 			}
	// 			else
	// 			sup_array[i][j]=MAX_ARC_WEIGHT;       
	//         }
	//     }

	//     Dijkstra(sup_array,src_id,distance,path);

	//     for(int i=0;i<SAT_NUM;i++)
	//     {
	// 		//find next_hop
	// 		next_hop[i]=FindPreNode(path,src_id,i+1);
	//         //thread
	// 		struct arg_route args[SAT_NUM];

	//         if(i!=src_id-1 && next_hop[i]!=-1)
	//         {
	//             int temp=next_hop[i];
	//             gw_id=temp+1;
	//             dst_id=i+1;
	//             linkid=topo.arcs[src_id-1][temp].linkId;
	            
	//             if(linkid != -1)
	//             {
	//                 if(topo.isl[linkid-1].endpoint[0].nodeId==src_id)
	//                 {
	//                      src_portid=topo.isl[linkid-1].endpoint[0].inf;
	//                      //gw_portid=topo.isl[linkid-1].endpoint[1].inf;
	//                      gw_addr = topo.isl[linkid-1].endpoint[1].ip;
	//                 }
	//                 else
	//                 {
	//                      src_portid=topo.isl[linkid-1].endpoint[1].inf;
	//                      //gw_portid=topo.isl[linkid-1].endpoint[0].inf;
	//                      gw_addr = topo.isl[linkid-1].endpoint[0].ip;
	//                 }
	// 				//args[i].src_id=src_id;
	// 				//args[i].gw_addr=gw_addr;
	// 				//args[i].dst_addr=dst_addr;
	// 				//args[i].dst_id=dst_id;
	// 				//args[i].src_portid=src_portid;
	// 				//args[i].linkid= linkid;
	// 				// RouteTask *rt=new RouteTask((void*)&args[i]);
	// 				// route_pool.addTask(rt);
	// 				//LinkRouteTask *lrt=new LinkRouteTask((void*)&args[i]);
	// 				//link_route_pool.addTask(lrt);
	//             }
	//         }
	//     }
}


int FindMin(
    unsigned int distance[],
    bool mark[]
)
{
    int n=0;
    unsigned int min=MAX_ARC_WEIGHT;
    for(int k=0;k<SAT_NUM;k++)
    {
        if(!mark[k] && distance[k]<min)
        {
            min = distance[k];
            n = k;
        }
    }
    if(min == MAX_ARC_WEIGHT)
        return -1;

    return n;

}

void Dijkstra(
    unsigned int sup_array[][SAT_NUM],
    int src_id,
    unsigned int distance[],
    int path[])
{
    int temp =src_id-1;
    bool mark[SAT_NUM];
    //initialize mark , add it to the set path[]
    for(int i=0;i < SAT_NUM;i++)
    {
        mark[i]=false;
        distance[i]=sup_array[temp][i];
        if(distance[i]!=MAX_ARC_WEIGHT)
            path[i]=temp;
        else
            path[i]=-1;
    }
    //mark the choosed point temp and update its distance
    mark[temp]=true;
    distance[temp]=0;

    for(int i=0;i < SAT_NUM;i++)
    {
        //find the closed point and mark it
        if((temp = FindMin(distance, mark)) == -1)
			return;
        mark[temp]=true;

        for(int j=0;j<SAT_NUM;j++)
        {
            //not marked and temp is the closed point, addit to the set and update the shortest path
            if(!mark[j] && (sup_array[temp][j]+distance[temp]< distance[j]))
            {
                distance[j]= sup_array[temp][j]+distance[temp];
                path[j]=temp;
            }
        }
    }
}

int FindPreNode(
    int path[],
    int src_id,
    int dst_id)
{
    int temp =dst_id-1;
    if(path[temp]==src_id-1)
       return temp;
    else if(path[temp] == -1)
       return -1;
    else 
       return FindPreNode(path,src_id,path[temp]+1);
}

void link_route_write()
{
	int temp_link;
	int dst_id = route_table.dst_id;
	int src_portid = route_table.src_portid;
	NodeAddress gw_addr = route_table.gw_addr;

	for(int p=0;p<SINF_NUM;p++)
	{
		if( Int_DB[dst_id-1].dst_satid[p]!=satelliteId)
		{
			temp_link=Int_DB[dst_id-1].linkId[p];
			NodeAddress temp_link_subnetIp =  G.isl[temp_link-1].subnetIp;
			// char dstlink_netStr[BUF_STR_SIZE],gw_netStr[BUF_STR_SIZE];

			// IpStr(dstlink_netStr,temp_link_subnetIp,24);
			// IpStr(gw_netStr,gw_addr);
	        
	  //       char cmd[BUF_STR_SIZE],cmd_del[BUF_STR_SIZE];
			// sprintf(cmd_del,"ip route del %s",dstlink_netStr);
			// system(cmd_del.c_str());
	  //       sprintf(cmd,"ip route add %s via %s dev sat%dp%d",
		 //    dstlink_netStr,gw_netStr,satelliteId,src_portid);
			// system(cmd.c_str());

			string dstlink_netStr = uint2strIP(temp_link_subnetIp);
			string cmd_del = "ip netns exec sat" +to_string(satelliteId) +" ip route del " +dstlink_netStr;
			system(cmd_del.c_str());
			cout<<cmd_del<<endl;
			string gw_netStr = uint2strIP(gw_addr);
			string cmd = "ip netns exec sat" +to_string(satelliteId) +" ip route add "+dstlink_netStr+ "/24 via "+ gw_netStr + " dev sat"+to_string(satelliteId)+"p"+to_string(src_portid);
			system(cmd.c_str());
			cout<<cmd<<endl;
		}
	}

}

void route_test()
{
	string cmd = "bash route.sh";
	//string cmd = " ./route_test";
	//system(cmd.c_str());
}

// uint str2uintIP(string ipStr,int hostId)
// {

// 	uint ip =0;
// 	 ipStr += ".";
// 	 int p1=0, p2=0,p3=0;
// 	 for (int i = 3; i >=0; i--)
// 	 {
// 	 	p2= ipStr.find('.',p1);
// 	 	string tmp = ipStr.substr(p1,p2-p1);
// 	 	ip+=stoi(tmp)<<(8*i);
// 	 	p1=p2+1;
// 	 }
// 	 uint mask = 0;
// 	 p3 = ipStr.find('/',0);
// 	 mask = stoi(ipStr.substr(p3+1,1));
// 	 Users[hostId-1].setMask(mask);

// 	 return ip;

// }
string uint2strIP(uint addr)
{
	string res = "";
	uint x = 0xff << 24 ;
	for (int i = 3; i >=0 ; i--)
	{
		res = res + to_string((addr&x)>>(8*i)) + ".";
		x >>= 8;
	}
	res.pop_back();

	return res;
}





