#include "net.h"
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
// string uint2strIP(uint addr)
// {
// 	string res = "";
// 	uint x = 0xff << 24 ;
// 	for (int i = 3; i >=0 ; i--)
// 	{
// 		res = res + to_string((addr&x)>>(8*i)) + ".";
// 		x >>= 8;
// 	}
// 	res.pop_back();

// 	return res;
// }