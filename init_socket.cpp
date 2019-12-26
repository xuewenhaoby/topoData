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


#include "opspf.h"

using namespace std;
OpspfInfData selfInf[6];
int main(int argc,char* argv[])
{
	network_init();
	int satelliteId = atoi(argv[1]);
	cout<<satelliteId<<endl;
	initializeAllInterface(satelliteId);
}
void network_init()
{
	puts("sudo echo 1 > /proc/sys/net/ipv4/ip_forward");
	system("sudo echo 1 > /proc/sys/net/ipv4/ip_forward");
}

void initializeAllInterface(int satelliteId)
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
	//cout<<inf.ip<<endl;

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
