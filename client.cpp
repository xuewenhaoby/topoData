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
#include <string.h>

#include "opspf.h"
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
OpspfInfData selfInf[6]; 
int satelliteId =2;

void network_init()
{
	puts("sudo echo 1 > /proc/sys/net/ipv4/ip_forward");
	system("sudo echo 1 > /proc/sys/net/ipv4/ip_forward");
}

// unsigned short csum(unsigned short *buf, int nwords)
// { 
// 	unsigned long sum;
// 	for (sum = 0; nwords > 0; nwords--)
// 	{
// 		sum += *buf++;
// 	}
// 	sum = (sum >> 16) + (sum & 0xffff);
// 	sum += (sum >> 16);
// 	return (unsigned short)(~sum);
// }
 
int main(int argc,char* argv[])
{
	network_init();
	int satelliteId = atoi(argv[1]);
	cout<<satelliteId<<endl;
	initializeAllInterface(satelliteId);
	// pthread_t recv;
	// pthread_create(&recv,NULL,recv_opspf, NULL);
	// pthread_join(recv,NULL);
				// 	pthread_t send;
				// pthread_create(&send,NULL,encapsulate_and_send_opspf, NULL);
				// pthread_join(send,NULL);
	OpspfSendProtocolPacket(selfInf[0],OPSPF_HELLO_PACKET);

	

	return 0;
}

void initializeAllInterface(int satelliteId)
{
	for(int i=0;i<6;i++)
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
	

	//sd=socket(AF_PACKET,SOCK_DGRAM,htons(ETHERTYPE_IP));
	//sd = socket(AF_INET, SOCK_RAW, IPPROTO_OPSPF);
	 sd = socket(AF_INET, SOCK_DGRAM, 0);   
	if (sd < 0)
	{
		perror("socket() error");
		// If something wrong just exit
		exit(-1);
	}
	strncpy(interface.ifr_name,dev,sizeof(dev));

	struct sockaddr_in addr,netmask;

	ret = ioctl(sd,SIOCGIFFLAGS,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not do ioctl"<<endl;
	}

	ret = ioctl(sd,SIOCGIFHWADDR,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not get HWADDR"<<endl;
	}

	interface.ifr_flags |= IFF_PROMISC;
	ret = ioctl(sd,SIOCSIFFLAGS,(char*)&interface);
	if(ret<0)
	{
		close(sd);
		cout<<"Can not set to promisc mode"<<endl;
	}

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

// void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType)
// {
// 	char buffer[PCKT_LEN] ;
// 	int count = 0;

// 	struct iphdr *ip = (struct iphdr *) buffer;
// 	OPSPF_Header* opspf_header =(OPSPF_Header*)(buffer + sizeof(struct iphdr));

// 	memset(buffer, 0, PCKT_LEN);

// 	switch(packetType)
// 	{
// 		case OPSPF_HELLO_PACKET:
// 		{		
// 			OpspfHelloInfo* helloInfo = (OpspfHelloInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			helloInfo->satelliteId = inf.satelliteId;
// 			helloInfo->portId =inf.portId;
// 			opspf_header->packetType = OPSPF_HELLO_PACKET;
// 			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfHelloInfo);
// 			memcpy(buffer + sizeof(iphdr) ,opspf_header , sizeof(opspf_header));
// 			memcpy(buffer + sizeof(iphdr)+sizeof(opspf_header),helloInfo, sizeof(helloInfo));
// 			break;
// 		}
// 		case OPSPF_LSU_PACKET:
// 		{
// 			OpspfLsuInfo* lsuInfo = (OpspfLsuInfo*)(buffer+sizeof(struct iphdr)+sizeof(OPSPF_Header));
// 			//*lsuInfo = opspf->lsuData;
// 			opspf_header->packetType = OPSPF_LSU_PACKET;
// 			opspf_header->pktlen = sizeof(OPSPF_Header)+sizeof(OpspfLsuInfo);
// 			memcpy(buffer + sizeof(iphdr) ,opspf_header , sizeof(opspf_header));
// 			memcpy(buffer + sizeof(iphdr)+sizeof(opspf_header),lsuInfo , sizeof(lsuInfo));
// 			break;
// 		}
// 		default:
// 		{
// 			cout<<"Unknown protocol packet type to send!"<<endl;
// 			break;
// 		}
// 	}

// 	struct sockaddr_in sin, din;
// 	sin.sin_family = AF_INET;
// 	din.sin_family = AF_INET;
// 	sin.sin_addr.s_addr = inf.ip;
// 	din.sin_addr.s_addr = inf.ip;
 
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
 
// 	setuid(getpid());//如果不是root用户，需要获取权限	
// 	bind(inf.sock, (struct sockaddr *)&din, sizeof(din));
 
// 	// Send loop, send for every 2 second for 2000000 count
// 	std::cout << "Ip length:" << ip->tot_len << std::endl;
// 	char buf[1024] = {0};
// 	for (count = 1; count <= 2000000; count++)
// 	{
		
// 		if (sendto(inf.sock, buf, sizeof(buf), 0, (struct sockaddr *)&din, sizeof(din)) < 0)
// 			// Verify
// 		{
// 			perror("sendto() error");
// 			exit(-1);
// 		}
// 		else
// 		{
// 			printf("Count #%u - sendto() is OK.\n", count);
// 					string cmd = "ip route add 190.0.13.0/24 via 190.0.5.2 dev sat2p1";
// 	//string cmd = " ./route_test";
// 	system(cmd.c_str());
// 			sleep(2);
// 		}
// 	}
// 	//close(sd);

	
// }
void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType)
{
	int sockfd = inf.sock;
	struct sockaddr_in servAddr;    
    servAddr.sin_family = AF_INET;    
    servAddr.sin_addr.s_addr = inet_addr("190.0.1.2");    
    servAddr.sin_port = htons(8002);    
    if (bind(sockfd, (const struct sockaddr *)&servAddr, sizeof(servAddr)) == -1)    
        {cout<<"bind error"<<endl;}  
     char buf[500] = {0};
     fgets(buf, sizeof(buf), stdin);  
     for(int i=0;i<10;i++)
     {
		if (sendto(sockfd, buf, strlen(buf), 0,    
		       (const struct sockaddr *)&servAddr, sizeof(servAddr)) == -1)    
		{
			cout<<"sendto error"<<endl;
		}
		else
		{
			cout<<"send success"<<to_string(i)<<endl;
				string cmd = "bash route.sh";
		//string cmd = " ./route_test";
		system(cmd.c_str());
		}
     }

}
void *Send_Hello_Packet(void *ptr)
{
	while(1)
	{
		for (int i = 0; i < 6; i++)
		{
			OpspfSendProtocolPacket(selfInf[i],OPSPF_HELLO_PACKET);
		}
		sleep(10);
	}
	pthread_exit(NULL);

}

void *encapsulate_and_send_opspf(void *ptr)
{
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
}

void *recv_opspf(void *ptr)
{
    int sockfd;
    struct iphdr *ip;
    char buf[PCKT_LEN];
    ssize_t n;
    /* capture ip datagram without ethernet header */
    if ((sockfd = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP)))== -1)
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
        //接收数据不包括数据链路帧头
        ip = (struct iphdr *)(buf);
        if(ip->daddr ==selfInf[0].ip || ip->daddr ==selfInf[0].ip)
        {
        	cout<<"Packet arriving at the target port !"<<endl;
        }
        else
        {
        	if(ip->saddr != selfInf[0].ip && ip->saddr != selfInf[1].ip)
	        {
	        	analyseIP(ip);
		        if (ip->protocol == IPPROTO_OPSPF)
		        {
		            OPSPF_Header *opspfhdr = (OPSPF_Header *)(buf +sizeof(iphdr));

		            switch(opspfhdr->packetType)
		            {
		                case OPSPF_HELLO_PACKET:
		                  {
		                    OpspfHelloInfo *helloInfo =(OpspfHelloInfo*)(buf +sizeof(struct iphdr)+sizeof(OPSPF_Header));
		                    //Opspf_Handle_HelloPacket(helloInfo);
		                    break;
		                  }
		                case OPSPF_LSU_PACKET:
		                {
		                    OpspfLsuInfo *lsuInfo =(OpspfLsuInfo*)(buf +sizeof(struct iphdr)+sizeof(OPSPF_Header));
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
	        }
	        
        }


    }
    close(sockfd);
    
}


void analyseIP(struct iphdr *ip)
{
    unsigned char* p = (unsigned char*)&ip->saddr;
   // printf("Source IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
    p = (unsigned char*)&ip->daddr;
    //printf("Destination IP\t: %u.%u.%u.%u\n",p[0],p[1],p[2],p[3]);
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

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo)
{
    cout<<" HelloPacket"<<endl;

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

}

// int FindMin(unsigned int distance[],
// 	bool mark[])
// {
// 	int minNodeId;
// 	unsigned int min = MAX_WEIGHT;
// 	for(int i=0;i<SAT_NUM;i++)
// 	{
// 		if(!mark[i]&&distance[i]<min)
// 		{
// 			min = distance[i];
// 			minNodeId = i;
// 		}
// 	}
// 	if(min==MAX_WEIGHT)
// 	{
// 		return -1;
// 	}
// 	return minNodeId;
// }

// int FindPreNode(int path[],
// 	int srcid,
// 	int dstid)
// {
// 	int temp = dstid -1;
// 	if(path[temp]==srcid)
// 	{
// 		return temp;
// 	}
// 	else if(path[temp]==-1)
// 	{
// 		return -1
// 	}
// 	else 
// 		return FindPreNode(path,srcid,path[temp]+1);
// }

// void Dijkstra(unsigned int sup_array[][SAT_NUM],
// 	int srcid,
// 	unsigned int distance[],
// 	int path[])
// {
// 	bool mark[SAT_NUM];
// 	int temp = srcid-1;
// 	for(int i=0;i<SAT_NUM;i++)
// 	{
// 		mark[i]=false;
// 		distance[i] = sup_array[temp][i];
// 		if(distance[i]== MAX_WEIGHT)
// 		{
// 			path[i] = -1;
// 		}
// 		else
// 		{
// 			path[i] = temp;
// 		}
// 	}

// 	mark[temp] = true;
// 	distance[temp] = 0;
// 	for(int i=0; i<SAT_NUM; i++)
// 	{
// 		temp = FindMin(distance,mark);
// 		if(temp==-1)
// 		{
// 			return;
// 		}

// 		mark[temp]=true;
// 		for(int j=0;j<SAT_NUM;j++)
// 		{
// 			if(!mark[j]&&sup_array[temp][j]+distance[temp]<distance[j])
// 			{
// 				distance[j] = sup_array[temp][j]+distance[temp];
// 				path[j] = temp;
// 			}
// 		}
// 	}
// }

// void RoutingTable(int src_id, StaticTopo topo, ISDB inf_DB[])
// {
// 	int path[SAT_NUM],next_hop[SAT_NUM];
// 	unsigned int sup_array[SAT_NUM][SAT_NUM];
// 	unsigned int distance[SAT_NUM];
// 	int linkId;
// 	int src_portid,gw_id,dstid,gw_portid;
// 	for(int i=0;i<SAT_NUM;i++)
// 	{
// 		for(int j=0;j<SAT_NUM;j++)
// 		{
// 			int temp_link = topo.arcs[i][j].linkId;
// 			int portId;
// 			if(temp_link!=-1&&temp_link!=0)
// 			{
// 				if(i+1==topo.isl[temp_link-1].endpoint[0].nodeId)
// 				{
// 					portId = topo.isl[temp_link-1].endpoint[0].inf;
// 				}
// 				else
// 				{
// 					portId = topo.isl[temp_link-1].endpoint[1].inf;
// 				}
// 				if(inf_DB[i].port_stat[portId]==1)
// 				{
// 					sup_array[i][j] = topo.arcs[i][j].weight;
// 				}
// 				else
// 				{
// 					sup_array[i][j] = MAX_WEIGHT;
// 				}

// 			}
// 			else
// 			{
// 				sup_array[i][j] = MAX_WEIGHT;
// 			}
// 		}
// 	}
// 	Dijkstra(sup_array,srcid,distance,path);

// 	for (int i = 0; i < SAT_NUM; ++i)
// 	{
// 		next_hop[i] = FindPreNode(path,srcid,i+1);

// 		if(next_hop[i]!=-1 &&srcid!=i)//find other node existing next hop
// 		{
// 			 linkId = topo.arcs[srcid-1][next_hop[i]].linkId;
// 			if(linkId!=-1)
// 			{
// 				if(topo.isl[linkId-1].endpoint[0].nodeId == srcid)
// 				{
// 					src_portid = topo.isl[linkId-1].endpoint[0].inf;
// 					gw_portid = topo.isl[linkId-1].endpoint[1].inf;
// 				}
// 				else
// 				{
// 					src_portid = topo.isl[linkId-1].endpoint[1].inf;
// 					gw_portid = topo.isl[linkId-1].endpoint[0].inf;
// 				}
// 			}
// 		}
// 	}


// }


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

// #include <unistd.h>
// #include <stdio.h>
// #include <sys/socket.h>
// #include <netinet/ip.h>
// #include <netinet/udp.h>
// #include<memory.h>
// #include<stdlib.h>
// #include <linux/if_ether.h>
// #include <linux/if_packet.h> // sockaddr_ll
// #include<arpa/inet.h>
// #include<netinet/if_ether.h>
// #include<iomanip>
// #include<iostream>
// #include <string.h>

// using namespace std;
 
// // The packet length
// #define PCKT_LEN 100
 
// //UDP的伪头部
// struct UDP_PSD_Header
// {
// 	u_int32_t src;
// 	u_int32_t des;
// 	u_int8_t  mbz;
// 	u_int8_t ptcl;
// 	u_int16_t len;
// };
// //计算校验和
// unsigned short csum(unsigned short *buf, int nwords)
// { 
// 	unsigned long sum;
// 	for (sum = 0; nwords > 0; nwords--)
// 	{
// 		sum += *buf++;
// 	}
// 	sum = (sum >> 16) + (sum & 0xffff);
// 	sum += (sum >> 16);
// 	return (unsigned short)(~sum);
// }
 
 
// // Source IP, source port, target IP, target port from the command line arguments
// int main(int argc, char *argv[])
// {
// 	int sd;
// 	char buffer[PCKT_LEN] ;
// 	//查询www.chongfer.cn的DNS报文
// 	unsigned char DNS[] = { 0xd8, 0xcb , 0x01, 0x00, 0x00, 0x01, 0x00 ,0x00,
// 		0x00, 0x00, 0x00, 0x00, 0x03, 0x77, 0x77, 0x77,
// 		0x08, 0x63, 0x68, 0x6f, 0x6e, 0x67, 0x66, 0x65,
// 		0x72, 0x02, 0x63, 0x6e, 0x00, 0x00, 0x01, 0x00,
// 		0x01 };
// 	struct iphdr *ip = (struct iphdr *) buffer;
// 	struct udphdr *udp = (struct udphdr *) (buffer + sizeof(struct iphdr));
// 	// Source and destination addresses: IP and port
// 	struct sockaddr_in sin, din;
// 	int  one = 1;
// 	const int *val = &one;
// 	//缓存清零
// 	memset(buffer, 0, PCKT_LEN);
 
 
// 	// Create a raw socket with UDP protocol
// 	//sd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
// 	if (sd < 0)
// 	{
// 		perror("socket() error");
// 		// If something wrong just exit
// 		exit(-1);
// 	}
// 	else
// 		printf("socket() - Using SOCK_RAW socket and UDP protocol is OK.\n");
// 	//IPPROTO_TP说明用户自己填写IP报文
// 	//IP_HDRINCL表示由内核来计算IP报文的头部校验和，和填充那个IP的id 
// 	if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(int)))
// 	{
// 		perror("setsockopt() error");
// 		exit(-1);
// 	}
// 	else
// 		printf("setsockopt() is OK.\n");
 
// 	// The source is redundant, may be used later if needed
// 	// The address family
// 	sin.sin_family = AF_INET;
// 	din.sin_family = AF_INET;
// 	// Port numbers
// 	sin.sin_port = htons(20);
// 	din.sin_port = htons(20);
// 	// IP addresses
// 	sin.sin_addr.s_addr = inet_addr("190.0.1.2");
// 	din.sin_addr.s_addr = inet_addr("190.0.1.3");
 
// 	// Fabricate the IP header or we can use the
// 	// standard header structures but assign our own values.
// 	ip->ihl = 5;
// 	ip->version = 4;//报头长度，4*32=128bit=16B
// 	ip->tos = 0; // 服务类型
// 	ip->tot_len = ((sizeof(struct iphdr) + sizeof(struct udphdr)+sizeof(DNS)));
// 	//ip->id = htons(54321);//可以不写
// 	ip->ttl = 64; // hops生存周期
// 	ip->protocol = 17; // UDP
// 	ip->check = 0;
// 	// Source IP address, can use spoofed address here!!!
// 	ip->saddr = inet_addr("190.0.1.2");
// 	// The destination IP address
// 	ip->daddr = inet_addr("190.0.1.3");
 
// 	// Fabricate the UDP header. Source port number, redundant
// 	udp->source = htons(20);//源端口
// 	// Destination port number
// 	udp->dest = htons(20);//目的端口
// 	udp->len = htons(sizeof(struct udphdr)+sizeof(DNS));//长度
// 	//forUDPCheckSum用来计算UDP报文的校验和用
// 	//UDP校验和需要计算 伪头部、UDP头部和数据部分
// 	char * forUDPCheckSum = new char[sizeof(UDP_PSD_Header) + sizeof(udphdr)+sizeof(DNS)+1];
// 	memset(forUDPCheckSum, 0, sizeof(UDP_PSD_Header) + sizeof(udphdr) + sizeof(DNS) + 1);
// 	UDP_PSD_Header * udp_psd_Header = (UDP_PSD_Header *)forUDPCheckSum;
// 	udp_psd_Header->src = inet_addr("190.0.1.2");
// 	udp_psd_Header->des = inet_addr("190.0.1.3");
// 	udp_psd_Header->mbz = 0;
// 	udp_psd_Header->ptcl = 17;
// 	udp_psd_Header->len = htons(sizeof(udphdr)+sizeof(DNS));
// 	memcpy(forUDPCheckSum + sizeof(UDP_PSD_Header), udp, sizeof(udphdr));
// 	memcpy(forUDPCheckSum + sizeof(UDP_PSD_Header) + sizeof(udphdr), DNS, sizeof(DNS));
 
// 	//ip->check = csum((unsigned short *)ip, sizeof(iphdr)/2);//可以不用算
// 	//计算UDP的校验和，因为报文长度可能为单数，所以计算的时候要补0
// 	udp->check = csum((unsigned short *)forUDPCheckSum,(sizeof(udphdr)+sizeof(UDP_PSD_Header)+sizeof(DNS)+1)/2);
 
// 	setuid(getpid());//如果不是root用户，需要获取权限	
 
// 	// Send loop, send for every 2 second for 2000000 count
// 	printf("Trying...\n");
// 	printf("Using raw socket and UDP protocol\n");
// 	printf("Using Source IP: %s port: %u, Target IP: %s port: %u.\n", argv[1], atoi(argv[2]), argv[3], atoi(argv[4]));
// 	std::cout << "Ip length:" << ip->tot_len << std::endl;
// 	int count;
// 	//将DNS报文拷贝进缓存区
// 	memcpy(buffer + sizeof(iphdr) + sizeof(udphdr), DNS, sizeof(DNS));
	
// 	for (count = 1; count <= 2000000; count++)
// 	{
		
// 		if (sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr *)&din, sizeof(din)) < 0)
// 			// Verify
// 		{
// 			perror("sendto() error");
// 			exit(-1);
// 		}
// 		else
// 		{
// 			printf("Count #%u - sendto() is OK.\n", count);
// 				string cmd = "bash route.sh";
// 	//string cmd = " ./route_test";
// 	system(cmd.c_str());
// 			sleep(2);
// 		}
// 	}
// 	close(sd);
// 	return 0;
// }

// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <errno.h>
// #include <string.h>
 
// #define MYPORT 8887
// char* SERVERIP = "190.0.1.3";
 
// #define ERR_EXIT(m) \
//     do \
// { \
//     perror(m); \
//     exit(EXIT_FAILURE); \
//     } while(0)
 
// void echo_cli(int sock)
// {
//     struct sockaddr_in servaddr;
//     memset(&servaddr, 0, sizeof(servaddr));
//     servaddr.sin_family = AF_INET;
//     servaddr.sin_port = htons(MYPORT);
//     servaddr.sin_addr.s_addr = inet_addr(SERVERIP);
    
//     int ret;
//     char sendbuf[1024] = {0};
//     char recvbuf[1024] = {0};
//     while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
//     {
        
//         printf("向服务器发送：%s\n",sendbuf);
//         sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
        
//         ret = recvfrom(sock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
//         if (ret == -1)
//         {
//             if (errno == EINTR)
//                 continue;
//             ERR_EXIT("recvfrom");
//         }
//         printf("从服务器接收：%s\n",recvbuf);
        
//         memset(sendbuf, 0, sizeof(sendbuf));
//         memset(recvbuf, 0, sizeof(recvbuf));
//     }
    
//     close(sock);
    
    
// }
 
// int main(void)
// {
//     int sock;
//     if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
//         ERR_EXIT("socket");
    
//     echo_cli(sock);
    
//     return 0;
// }
