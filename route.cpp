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

using namespace std;

#include "opspf.h"
// The packet length

struct arg_route
{
    int src_id;
    int dst_id;
    int gw_addr;
    int dst_addr; 
    int src_portid;
    int linkid;
};
arg_route args[3];

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

	//sd=socket(AF_PACKET,SOCK_DGRAM,htons(ETHERTYPE_IP));
	sd = socket(PF_PACKET, SOCK_RAW, IPPROTO_OPSPF);
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

	inf.satelliteId = 47;
	inf.portId = 3;

	 return inf;
}

void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType,in_addr_t dstAddr)
{
	char buffer[PCKT_LEN] ;
	unsigned
	int count = 0;
	struct ethhdr *eth_header = (struct ethhdr *)buffer;
	struct iphdr *ip = (struct iphdr *) (buffer+sizeof(struct ethhdr));
	OPSPF_Header* opspf_header =(OPSPF_Header*)(buffer + sizeof(struct ethhdr)+sizeof(struct iphdr));

	//缓存清零
	memset(buffer, 0, PCKT_LEN);

	switch(packetType)
	{
		case OPSPF_HELLO_PACKET:
		{		
			OpspfHelloInfo* helloInfo = (OpspfHelloInfo*)(buffer+ sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			helloInfo->satelliteId = inf.satelliteId;
			helloInfo->portId =inf.portId;
			opspf_header->packetType = OPSPF_HELLO_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfHelloInfo);
			opspf_header->dst_addr=args[0].dst_addr;
			memcpy(buffer + sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(opspf_header));
			memcpy(buffer + sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(opspf_header),helloInfo, sizeof(helloInfo));
			break;
		}
		case OPSPF_LSU_PACKET:
		{
			OpspfLsuInfo* lsuInfo = (OpspfLsuInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			//*lsuInfo = opspf->lsuData;
			opspf_header->packetType = OPSPF_LSU_PACKET;
			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuInfo);
			memcpy(buffer + sizeof(struct ethhdr)+ sizeof(iphdr) ,opspf_header , sizeof(opspf_header));
			memcpy(buffer + sizeof(struct ethhdr)+ sizeof(iphdr)+sizeof(opspf_header),lsuInfo , sizeof(lsuInfo));
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
	din.sin_addr.s_addr = dstAddr;
 
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
 
	setuid(getpid());//如果不是root用户，需要获取权限	
 
	// Send loop, send for every 2 second for 2000000 count
	std::cout << "Ip length:" << ip->tot_len << std::endl;

	// Fabricate the MAC header or we can use the
	// standard header structures but assign our own values.
	struct sockaddr_ll dstmac;
	memset(&dstmac,0,sizeof(dstmac));
	dstmac.sll_family = AF_PACKET;
	//client.sll_protocol = htons(VSTRONG_PROTOCOL);
	dstmac.sll_ifindex = inf.if_index;
	dstmac.sll_halen = htons(ETH_HLEN);
	memcpy(dstmac.sll_addr,inf.if_mac,dstmac.sll_halen);
	memcpy(eth_header->h_dest , inf.if_mac,ETH_ALEN);
	for (count = 1; count <= 2000000; count++)
	{
		if (sendto(inf.sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dstmac, sizeof(dstmac)) < 0)
			// Verify
		{
			perror("sendto() error");
			exit(-1);
		}
		else
		{
			printf("Count #%u - sendto() is OK.\n", count);
			sleep(10);
		}

	}
	//close(sd);

	
}

int main()
{
	
	args[0].src_id=1;
    args[0].dst_id=3;
    args[0].dst_addr=inet_addr("1.0.4.2"); 
    args[0].src_portid=0;
    OpspfInfData inf =interface_init(args[0].src_id,args[0].src_portid);
	OpspfSendProtocolPacket(inf,OPSPF_LSU_PACKET,args[0].dst_addr);
	return 0;
}

