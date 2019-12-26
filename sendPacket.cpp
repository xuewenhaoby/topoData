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
#include <cstring>
#include <fstream>
#include <iostream>
#include <pthread.h>

#include "timer.h"
//#include "message.h"
#include "opspf.h"

using namespace std;

StaticTopo G;
OpspfInfData selfInf[6];
ISDB Int_DB[SAT_NUM];
int satelliteId;

int main(int argc,char* argv[])
{


	satelliteId = atoi(argv[1]);
	cout<<satelliteId<<endl;
	initializeAllInterface(satelliteId);
	InitStaticTopo();
	initializeLinkDB(satelliteId);
	pthread_t recv;
	pthread_create(&recv,NULL,recv_opspf, NULL);
	pthread_join(recv,NULL);
}




void initializeAllInterface(int satelliteId)
{
	for(int i=0;i<SINF_NUM;i++)
    {
    	selfInf[i] =interface_init(satelliteId,i);        	
    }
    for (int i = 0; i < SAT_NUM; ++i)
    {
    	for (int j = 0; j < SINF_NUM; ++j)
    	{
			Int_DB[i].linkId[j]=-1;
			Int_DB[i].port_stat[j]=false;
			Int_DB[i].dst_satid[j]=-1;
			Int_DB[i].dst_portid[j]=-1;
    	}
    }

}
void initializeLinkDB(int satelliteId)
{
	for(int i = 0; i < SINF_NUM; i++)
	{
		int linkId = selfInf[i].linkId;
		Int_DB.linkId[i]=selfInf[i].linkId;
		IslNode *nodes = G.isl[linkId-1].endpoint;
		if(linkId != -1 && satelliteId == nodes[0].nodeId){
			int nIds[2] = {satelliteId, nodes[1].nodeId};
			int pIds[2] = {i, nodes[1].inf};
			int addrs[2] = {nodes[0].ip,nodes[1].ip};

		}
		//Int_DB initial 
	    if(linkId!=-1)
	    {
		    if(G.isl[infData[i].linkId-1].endpoint[0].nodeId==satelliteId)
		    {  
		        Int_DB.dst_satid[i]=G.isl[selfInf[i].linkId-1].endpoint[1].nodeId;  
	            Int_DB.dst_portid[i]=G.isl[selfInf[i].linkId-1].endpoint[1].inf;
		    }
		    else
		    {
			Int_DB.dst_satid[i]=G.isl[selfInf[i].linkId-1].endpoint[0].nodeId;
			Int_DB.dst_portid[i]=G.isl[selfInf[i].linkId-1].endpoint[0].inf;
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
	//cout<<inf.ip<<endl;

	ret=ioctl(sd,SIOCGIFNETMASK,(char*)&interface);
	if(ret<0)
	{
		cout<<"Can not get netmask"<<endl;
	}
	 inf.netmask = ((struct sockaddr_in *)&interface.ifr_netmask)->sin_addr.s_addr;

	 inf.sock = sd;
	 inf.if_index = if_index;
	 inf.satelliteId = satelliteId;
	 inf.portId = portId;
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

void *Send_Hello_Packet(void *ptr)
{
	for (int i = 0; i < 6; i++)
	{
		OpspfSendProtocolPacket(selfInf[i],OPSPF_HELLO_PACKET);
	}
	sleep(1);
	pthread_exit(NULL);

}

// void *encapsulate_and_send_opspf(void *ptr)
// {

// 	int index=-1;
// 	for(int i=0;i<6;i++)
// 	{
// 		if(args.src_portid==i)
// 		{
// 			index = i;
// 			cout<<index<<endl;
// 		}
// 	}
// 	if(index!=-1)
// 	{
// 		OpspfSendProtocolPacket(selfInf[index],OPSPF_HELLO_PACKET);
// 	}
	
	
// 	//pthread_exit(NULL);
// }


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
			OpspfLsuInfo* lsuInfo = (OpspfLsuInfo*)(buffer+sizeof(struct ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
			lsuInfo->srcSatelliteId = srcSatelliteId;
		    lsuInfo->srcPortId=srcPortId;
		    lsuInfo->dstSatelliteId=dstSatelliteId;
		    lsuInfo->dstPortId=dstPortId;
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
	// int linkid=G.arcs[satelliteId-1][satelliteId].linkId;
	// if(G.isl[linkid-1].endpoint[0].nodeId==inf.satelliteId)
 //    {
 //         inf.gw_addr = G.isl[linkid-1].endpoint[1].ip;
 //    }
 //    else
 //    {
 //         inf.gw_addr = G.isl[linkid-1].endpoint[0].ip;
 //    }
	inf.gw_addr = inf.ip;
	struct sockaddr_in sin, din;
	sin.sin_family = AF_INET;
	din.sin_family = AF_INET;
	sin.sin_addr.s_addr = inf.ip;
	din.sin_addr.s_addr = inf.gw_addr;
 
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
	//std::cout << "Ip length:" << ip->tot_len << std::endl;
		
	if (sendto(inf.sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&dstmac, sizeof(dstmac)) < 0)
		// Verify
	{
		//perror("sendto() error");
		//cout<<"The port is down"<<endl;
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
	// if ((sockfd = socket(PF_PACKET,  SOCK_DGRAM, htons(ETH_P_IP)))== -1)
	if ((sockfd = socket(PF_PACKET,  SOCK_RAW, htons(ETH_P_ALL)))== -1)
	{    
	    printf("socket error!\n");
	}
	while (1)
	{
		pthread_t sendHello;
		pthread_create(&sendHello,NULL,Send_Hello_Packet, NULL);
		pthread_join(sendHello,NULL);
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
	    if(ip->protocol != IPPROTO_OPSPF )
	    {
	    	continue;
	    }
	    if(ip->daddr ==selfInf[0].ip || ip->daddr ==selfInf[1].ip || ip->daddr ==selfInf[2].ip 
	    	|| ip->daddr ==selfInf[3].ip|| ip->daddr ==selfInf[4].ip|| ip->daddr ==selfInf[5].ip)
	    {
	    	//cout<<"Packet arriving at the target port !"<<endl;
	    	for (int i = 0; i < SINF_NUM; ++i)
	    	{
	    		if(ip->daddr == selfInf[i].ip)
	    		{
	    			interfaceIndex = i;
	    		}
	    	}
	    }
	    else
	    {
	        if(ip->saddr != selfInf[0].ip && ip->saddr != selfInf[1].ip && ip->saddr != selfInf[2].ip && ip->saddr != selfInf[3].ip 
	        	&& ip->saddr != selfInf[4].ip && ip->saddr != selfInf[5].ip)
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
		                    Opspf_Handle_HelloPacket(helloInfo,interfaceIndex);
		                    break;
		                  }
		                case OPSPF_LSU_PACKET:
		                {
		                    OpspfLsuInfo *lsuInfo =(OpspfLsuInfo*)(buf +sizeof(ethhdr)+sizeof(struct iphdr)+sizeof(OPSPF_Header));
		                    Opspf_Handle_LsuPacket(lsuInfo,interfaceIndex);
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

	       
	    	}

	    }
	    

	  
	}
	close(sockfd);
    
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
}

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo,int interfaceIndex)
{
    cout<<" HelloPacket"<<endl;
    printf("%d",helloInfo->satelliteId);
    printf("\n");
    printf("%d",helloInfo->portId);
    selfInf[interfaceIndex].ttl = OPSPF_PORT_TTL;

    if(selfInf[interfaceIndex].stat == false)
    {

    	    //  UpdateOpspfRoutingTable();
    	for (int i = 0; i < SINF_NUM; ++i)
	    {

	    	OpspfSendProtocolPacket(selfInf[i],OPSPF_LSU_PACKET);

	    }

    }



    //  OpspfInfData *inf = ;

    //  inf->ttl = OPSPF_PORT_TTL;

    // if(inf->status==OPSPF_PORT_DOWN)
    // {
    //  UpdateOpspfRoutingTable();

    //  inf->changeStat = OPSPF_PORT_DOWN;

    // }
    for (int i = 0; i < SINF_NUM; ++i)
    {
    	if(i!=interfaceIndex)
    	{
    		OpspfSendProtocolPacket(selfInf[i],OPSPF_LSU_PACKET);
    	}

    }
 

   
}

void Opspf_Handle_LsuPacket(const OpspfLsuInfo* lsuInfo,int interfaceIndex)
{
    cout<<" LsuPacket"<<endl;
    int srcSatelliteId = lsuInfo->srcSatelliteId;
    int srcPortId = lsuInfo->srcPortId;
    int dstSatelliteId = lsuInfo->dstSatelliteId;
    int dstPortId = lsuInfo->dstPortId;

    int linkid = G.arcs[srcSatelliteId-1][dstSatelliteId-1].linkId;

    bool flood = false;

    if(lsuInfo->changeStat == OPSPF_PORT_LINKDOWN)
    {
    	if(Int_DB.port_stat[interfaceIndex] == true)
    	{
    		flood = true;
			Int_DB.port_stat[interfaceIndex] == false;
    	}
    }
    else if(lsuInfo->changeStat == OPSPF_PORT_RELINK)
    {
    	if(Int_DB.port_stat[interfaceIndex] == false)
    	{
    		flood = true;
			Int_DB.port_stat[interfaceIndex] == true;
    	}
    }
    if(flood)
    {
    	//UpdateOpspfRoutingTable();
	    for (int i = 0; i < SINF_NUM; ++i)
	    {
	    	if(i!=interfaceIndex && selfInf[i].stat == true )
	    	{
	    		OpspfSendProtocolPacket(selfInf[i],OPSPF_LSU_PACKET);
	    	}

	    }
    	
    }

	// pthread_t sendLsu;
	// pthread_create(&send,NULL,encapsulate_and_send_opspf, NULL);
	// pthread_join(send,NULL);

}

void InitStaticTopo()
{
	// Set G.isl
	ReadIslFile(ISL_FILE);
	// Initilaize default value
	for(int i = 0; i < SAT_NUM; i++){
		// Initialize G.arcs
		for(int j = 0; j < SAT_NUM; j++){
			if(i == j){
				G.arcs[i][j].weight = 0;
				G.arcs[i][j].linkId = LINKID_SELFLOOP;
			}else{
				G.arcs[i][j].weight = MAX_ARC_WEIGHT;
				G.arcs[i][j].linkId = LINKID_NOLINK;
			}
		}
		
		// Initialize infData
		// for(int j = 0; j < SINF_NUM; j++){
		// 	InfData tmp = {-1,false};
		// 	sats[i].setInfData(tmp,j);
		// }
	}
	// Set value
	for(int i = 0; i < SLINK_NUM; i++){
		Isl* isl = & G.isl[i];
		int v[2] = {isl->endpoint[0].nodeId-1, isl->endpoint[1].nodeId-1};
		// Set G.arcs
		for(int j = 0; j < 2; j++){
			G.arcs[ v[j] ][ v[(j+1)%2] ].linkId = isl->linkId;
			G.arcs[ v[j] ][ v[(j+1)%2] ].weight = isl->weight;
		}
		// Set infData
		// for(int j = 0; j < 2; j++){
		// 	int inf = isl->endpoint[j].inf;
		// 	(sats[v[j]].acqInfData(inf))->linkId = isl->linkId;
		// }
	}
}

void ReadIslFile(string file_name)
{
	memset(&G,0,sizeof(StaticTopo));
	ifstream in;
	in.open(file_name.c_str());
	string s;
	while (getline(in, s)){
		int p0 = s.find(",", 0);
		string sub = s.substr(1, p0);
		int id = atoi(sub.c_str());
		G.isl[id - 1].linkId = id;

		int * value = (int*) G.isl[id - 1].endpoint;
		for (int i = 0; i < 2; i++){
			int p1 = s.find(":",p0);
			sub = s.substr(p0 + 1, p1 - p0 - 1);
			*value = atoi(sub.c_str());

			int p2 = s.find("|", p1);
			sub = s.substr(p1 + 1, p2 - p1 - 1);
			*(value+1) = atoi(sub.c_str());
			
			int p3 = s.find(",", p2);
			sub = s.substr(p2 + 1, p3 - p2 - 1);
			*(value+2) = atoi(sub.c_str());

			p0 = p3;
			value += 3;
		}
		value = NULL;

		int pos2 = s.find("]");
		sub = s.substr(p0 + 1, pos2 - p0 - 1);
		G.isl[id - 1].weight = atoi(sub.c_str());

		G.isl[id - 1].subnetIp = G.isl[id - 1].endpoint[0].ip & 0xffffff00;
	}
	in.close();
}

NodePos ReadLocFile(int id,int time)
{
	ifstream fileIn(MOBILITY_FILE, ios_base::binary);

	int t[2];
	int startTime[2];
	Coord location[2];
	t[0] = time;
	t[1] = t[0] + 1;
	int v = id-1;
	for(int i = 0; i < 2; i++)
	{
		startTime[i] = v * DATA_SIZE + t[i] % DATA_SIZE;
		fileIn.seekg(sizeof(Coord) * startTime[i], ios_base::beg);
		fileIn.read((char *)(location + i), sizeof(Coord));
	}
	NodePos pos;
	memcpy(& pos.loc, location, sizeof(Coord));
	pos.isNorth = (location[0].lat < location[1].lat);

	fileIn.close();
	return pos;
}


NodePos getPos(int satelliteId)
{
	ReadLocFile(satelliteId,GetTime());
}

bool getInfStat(int interfaceIndex)
{
	int srcNodeId = satelliteId;
	int dstNodeId = -1;
	Isl isl = G.isl[ selfInf[interfaceIndex].linkId-1 ];
	for(int i = 0; i < 2; i++)
	{
		if(srcNodeId == isl.endpoint[i].nodeId)
		{
			dstNodeId = isl.endpoint[(i+1)%2].nodeId;
			break;
		}
	}
	NodePos pos = getPos(srcNodeId);

	if(dstNodeId == getForeSatelliteId(srcNodeId) ||
		dstNodeId == getRearSatelliteId(srcNodeId))
	{
		return true;
	}
	else if(dstNodeId == getSideSatelliteId(srcNodeId, true, pos.isNorth) ||
		dstNodeId == getSideSatelliteId(srcNodeId, false, pos.isNorth)) 
	{
		return (abs(getPos(srcNodeId).loc.lat) < TOPO_BETA) && 
				(abs(getPos(dstNodeId).loc.lat) < TOPO_BETA);
	}
	else
	{
		return false;
	}
}

int getOrbitId(int id)
{
	return (id-1)/8 + 1;
}


int getOrbitIndex(int id)
{
	return (id-1)%8 + 1;
}

int getForeSatelliteId(int id)
{
	int orbitId = getOrbitId(id);
	int index = getOrbitIndex(id);
	return (orbitId - 1)*8 + index % 8 + 1;
}

int getRearSatelliteId(int id)
{
	int orbitId = getOrbitId(id);
	int index = getOrbitIndex(id);
	
	return (orbitId - 1)*8 + (index + 6)%8 + 1;
}

int getSideSatelliteId(int id,bool isEast,bool isNorth)
{
	int orbitId = getOrbitId(id);
	int index = getOrbitIndex(id);
	int v = -1;

	if(orbitId == 1){
		if(isEast){
			if(isNorth)
				v = orbitId*8 + (index + 6)% 8;
			else
				v = orbitId*8 + index-1;
		}
	}else if(orbitId == 6){
		if(!isEast){
			if(isNorth)
				v = (orbitId - 2)*8 + index % 8;
			else
				v = (orbitId - 2)*8 + index-1;			
		}
	}else{
		if(isEast){
			if(isNorth)
				v = orbitId*8 + (index + 6)% 8;
			else
				v = orbitId*8 + index-1;
		}else{
			if(isNorth)
				v = (orbitId - 2)*8 + index % 8;
			else
				v = (orbitId - 2)*8 + index-1;	
		}
	}

	return v + 1;
}





