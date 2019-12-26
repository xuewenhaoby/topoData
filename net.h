#ifndef __NET_H__
#define __NET_H__

#include <unistd.h>

void IpStr(char *buf,int addr,int maskSize);
void IpStr(char *buf,int addr);
void BIpStr(char *buf,int addr,int maskSize);

#endif