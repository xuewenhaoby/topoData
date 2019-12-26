#include <unistd.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include<memory.h>
#include<stdlib.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h> // sockaddr_ll
#include<arpa/inet.h>
#include<netinet/if_ether.h>
#include <net/if.h>
#include<iomanip>

#include <sys/ioctl.h>
#include<iostream>

#include <pthread.h>

#include "opspf.h"
// The packet length


//#define dev "sat1p3"
//UDP的伪头部
//计算校验和

using namespace std;
struct arg_route
{
    int src_id;
    int dst_id;
    int gw_addr;
    int dst_addr; 
    int src_portid;
    int linkid;
};
struct arg_route args;
OpspfInfData selfInf[4]; 
int satelliteId =3;
//OpspfInfData inf;

void IpStr(char *buf,int addr,int maskSize)
{
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	sprintf(buf,"%d.%d.%d.%d/%d",a1,a2,a3,a4,maskSize);
}

void IpStr(char *buf,int addr)
{
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	sprintf(buf,"%d.%d.%d.%d",a1,a2,a3,a4);
}


void BIpStr(char *buf,int addr,int maskSize)
{
	int tmp = 0;
	for(int i = 0; i < 32-maskSize;i++){
		tmp <<= 1;
		tmp |= 0x00000001;
	}
	addr |= tmp;
	int a1 = (addr & 0xff000000) >> 24;
	int a2 = (addr & 0x00ff0000) >> 16;
	int a3 = (addr & 0x0000ff00) >> 8;
	int a4 = (addr & 0x000000ff);
	sprintf(buf,"%d.%d.%d.%d",a1,a2,a3,a4);
}

void network_init()
{
	puts("sudo echo 1 > /proc/sys/net/ipv4/ip_forward");
	system("sudo echo 1 > /proc/sys/net/ipv4/ip_forward");
}

unsigned short csum(unsigned short *buf, int nwords)
{ 
	unsigned long sum;
	for (sum = 0; nwords > 0; nwords--)
	{
		sum += *buf++;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (unsigned short)(~sum);
}

void analyseIP(struct iphdr *ip)
{
    unsigned char* p = (unsigned char*)&ip->saddr;
    printf("Source IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
    p = (unsigned char*)&ip->daddr;
    printf("Destination IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
    args.src_id=satelliteId;
    args.dst_addr=ip->daddr; 
    args.src_portid=0;

}
void analyseOPSPF(OPSPF_Header* opspfhdr)
{
    printf("OPSPF -----\n");
    // printf("type: %u\n", icmp->icmp_type);
    // printf("sub code: %u\n", icmp->icmp_code);
}
 
int main()
{
	network_init();        
    initializeAllInterface();
	recv_opspf();
	// pthread_t recv,send;
	
	// pthread_create(&recv,NULL,recv_opspf, NULL);
	// //pthread_create(&send,NULL,encapsulate_and_send_opspf, NULL);
	// pthread_join(recv,NULL);
	//pthread_join(send,NULL);
	return 0;
}

void initializeAllInterface()
{
	for(int i=0;i<MaxInterfaceNum;i++)
    {
    	selfInf[i] =interface_init(satelliteId,i);        	
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
	
	sd = socket(PF_PACKET, SOCK_RAW, htons(ETHERTYPE_IP));
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
		cout<<"Can not get index"<<endl;
	}

	ret = ioctl(sd,SIOCGIFHWADDR,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not get HWADDR"<<endl;
	}

	ret = ioctl(sd,SIOCGIFFLAGS,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not do ioctl"<<endl;
	}

	interface.ifr_flags |= IFF_PROMISC;
	ret = ioctl(sd,SIOCSIFFLAGS,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not set to promisc mode"<<endl;
	}

	memcpy(if_mac,interface.ifr_hwaddr.sa_data,sizeof(if_mac));
	memcpy(inf.if_mac ,if_mac,sizeof(if_mac));

	ret = ioctl(sd,SIOCGIFINDEX,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not get index"<<endl;
	}
	if_index = interface.ifr_ifindex;

	struct sockaddr_in addr,netmask;
	ret=ioctl(sd,SIOCGIFADDR,(char*)&interface);
	if(ret<0)
	{
		cout<<"Can not get IP"<<endl;
	}
	inf.ip = ((struct sockaddr_in *)&interface.ifr_addr)->sin_addr.s_addr;
	cout<<inf.ip<<endl;

	ret=ioctl(sd,SIOCGIFNETMASK,(char*)&interface);
	if(ret<0)
	{
		cout<<"Can not get netmask"<<endl;
	}
	 inf.netmask = ((struct sockaddr_in *)&interface.ifr_netmask)->sin_addr.s_addr;

	 inf.sock = sd;
	 inf.if_index = if_index;
	if(setsockopt(sd,SOL_SOCKET,SO_BINDTODEVICE,(char*)&interface,sizeof(struct ifreq))<0)
	{
		perror("SO_BINDTODEVICE failed");
	}


	// if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(int)))
	// {
	// 	perror("setsockopt() error");
	// 	exit(-1);
	// }
	else
		printf("setsockopt() is OK.\n");

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
			OpspfHelloInfo* helloInfo = (OpspfHelloInfo*)(buffer+sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			helloInfo->satelliteId = inf.satelliteId;
			helloInfo->portId =inf.portId;
			opspf_header->packetType = OPSPF_HELLO_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfHelloInfo);
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(opspf_header));
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(opspf_header),helloInfo, sizeof(helloInfo));
			break;
		}
		case OPSPF_LSU_PACKET:
		{
			OpspfLsuInfo* lsuInfo = (OpspfLsuInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			//*lsuInfo = opspf->lsuData;
			opspf_header->packetType = OPSPF_LSU_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuInfo);
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(opspf_header));
			memcpy(buffer +sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(opspf_header),lsuInfo , sizeof(lsuInfo));
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
	din.sin_addr.s_addr = args.dst_addr;
 
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
//	client.sll_protocol = htons(VSTRONG_PROTOCOL);
	dstmac.sll_ifindex = inf.if_index;
	dstmac.sll_halen = htons(ETH_HLEN);
	memcpy(dstmac.sll_addr,inf.if_mac,dstmac.sll_halen);
	memcpy(eth_header->h_dest , inf.if_mac,ETH_ALEN);
 
	setuid(getpid());//如果不是root用户，需要获取权限	
 
	// Send loop, send for every 2 second for 2000000 count
	std::cout << "Ip length:" << ip->tot_len << std::endl;
		
		if (sendto(inf.sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dstmac, sizeof(dstmac)) < 0)
			// Verify
		{
			perror("sendto() error");
			exit(-1);
		}	
		//close(inf.sock);
}


void *encapsulate_and_send_opspf(void *ptr)
{
	 //int sock;
	 //sock = interface_init(1,3);
	//OpspfInfData inf =interface_init(args.src_id,args.src_portid);
	int index=-1;
	for(int i=0;i<4;i++)
	{
		if(args.src_portid==i)
		{
			index = i;
			cout<<index<<endl;
		}
	}
	if(index!=-1)
	{
		OpspfSendProtocolPacket(selfInf[index],OPSPF_HELLO_PACKET);
	}
	
	
	//pthread_exit(NULL);
}

void *recv_opspf(void *ptr)
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
        if (ip->protocol == IPPROTO_OPSPF)
        {
            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(ethhdr)+sizeof(struct iphdr));
            analyseOPSPF(opspfhdr);
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
                    //Opspf_Handle_LsuPacket(lsuInfo);
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
        printf("\n\n");
        pthread_t send;
        //inf =interface_init(args.src_id,args.src_portid);
        OpspfInfData inf =interface_init(args.src_id,args.src_portid);
        pthread_create(&send,NULL,encapsulate_and_send_opspf, NULL);
        //pthread_exit(NULL);
        // pthread_join(send,NULL);
    }
    close(sockfd);
    
    
}

void recv_opspf()
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
	    if(ip->daddr ==selfInf[0].ip || ip->daddr ==selfInf[0].ip)
        {
        	cout<<"Packet arriving at the target port !"<<endl;
        }
        else
        {
	        if(ip->saddr != selfInf[0].ip && ip->saddr != selfInf[1].ip)
	        {

		        analyseIP(ip);
		        // size_t iplen =  (ip->h_verlen&0x0f)*4;
		        if (ip->protocol == IPPROTO_OPSPF)
		        {
		            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(ethhdr)+sizeof(struct iphdr));
		            analyseOPSPF(opspfhdr);
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
		                    //Opspf_Handle_LsuPacket(lsuInfo);
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
		        printf("\n\n");
		        pthread_t send;
	        	pthread_create(&send,NULL,encapsulate_and_send_opspf, NULL);
	        	pthread_join(send,NULL);
	        	//pthread_exit(NULL);
	       
	    	}

        }
        

      
    }
    close(sockfd);
    
}



void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo)
{
    cout<<" HelloPacket"<<endl;
        printf("%d",helloInfo->satelliteId);
        printf("\n");
        printf("%d",helloInfo->portId);
    //  OpspfInfData *inf = ;

    //  inf->ttl = OPSPF_PORT_TTL;

    // if(inf->status==OPSPF_PORT_DOWN)
    // {
    //  UpdateOpspfRoutingTable();

    //  inf->changeStat = OPSPF_PORT_DOWN;

    // }

   
}

// void Opspf_Handle_LsuPacket(const OpspfLsuInfo* lsuInfo)
// {
//     cout<<" LsuPacket"<<endl;
//     int srcSatelliteId = lsuInfo->srcSatelliteId;
//     int srcPortId = lsuInfo->srcPortId;
//     int dstSatelliteId = lsuInfo->dstSatelliteId;
//     int dstPortId = lsuInfo->dstPortId;

//     bool flood = false;
//     if(lsuInfo->changeStat == OPSPF_PORT_LINKDOWN)
//     {

//     }
//     else if(lsuInfo->changeStat == OPSPF_PORT_RELINK)
//     {

//     }
//     if(flood)
//     {
//     	UpdateOpspfRoutingTable();
//     	for(int i=0;i<MaxInterfaceNum;i++)
//     	{
//     		if(i!=portId )
//     		{
//     			OpspfSendProtocolPacket(inf,OPSPF_LSU_PACKET);
//     		}
//     	}
    	
//     }

// }

