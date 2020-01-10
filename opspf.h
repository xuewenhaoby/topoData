#ifndef __OPSPF_H__
#define __OPSPF_H__

#include <cstring>
#include <fstream>
#include <iostream>
#include <arpa/inet.h>

using namespace std;
#define COMMON_IPHDR_LEN 5
#define IP_VERSION 4
#define DEFAULT_OPSPF_TOS 0xc0
#define DEFAULT_OPSPF_TTL 1

#ifndef  IPPROTO_OPSPF
#define  IPPROTO_OPSPF 200
#endif
#define  OPSPF_VERSION 2

#define OPSPF_HELLO_INTERVAL 1
#define OPSPF_PORT_TTL 4
#define OPSPF_LSU_TTL 3
#define OPSPF_LSU_TIME 3

#define SLINK_NUM 128
#define SINF_NUM 6
#define TOPO_M 6 // number of orbit
#define TOPO_N 8 // number of satellite on one orbit
#define SAT_NUM TOPO_M * TOPO_N
#define TOPO_BETA 70
#define LINKID_SELFLOOP 0
#define LINKID_NOLINK -1
#define MAX_ARC_WEIGHT 1000

#define MOBILITY_FILE "topofile/orbit.bin"
//The size of mobility data : 24*60*60+1 24 hours data per second
#define DATA_SIZE 86401
#define ISL_FILE "topofile/linktable.txt"

#define BUF_STR_SIZE 100
#define PCKT_LEN 1000


typedef int NodeAddress;

enum OpspfPacketType
{
    OPSPF_HELLO_PACKET =1,
    OPSPF_LSU_PACKET   =2,
    OPSPF_LSACK_PACKET =3

};


enum OpspfPortChangeStatType
{
    OPSPF_PORT_LINKDOWN =0,
    OPSPF_PORT_RELINK =1,
    OPSPF_PORT_NOCHANGE =2
};

struct IslNode
{
    int nodeId;
    int inf;
    NodeAddress ip;
};

struct Isl
{
    int linkId;
    NodeAddress subnetIp;
    unsigned int weight;
    IslNode endpoint[2];
};

struct ArcNode
{
    int linkId;
    unsigned int weight;
};

struct StaticTopo
{
    Isl isl[SLINK_NUM];
    ArcNode arcs[SAT_NUM][SAT_NUM]; // weight;
};

struct Coord{
    double lat;
    double lon;
};

typedef struct Position{
    Coord loc;
    bool isNorth;
}NodePos;


typedef struct OpspfInterfaceData
{
    int type;
    in_addr_t ip;
    in_addr_t netmask;
    in_addr_t gw_addr;
    int sock;
    int ttl;
    int lsutimestamp;
    bool lsuAck;
    int satelliteId;
    int portId;
    int if_index;
    unsigned char if_mac[6];
    int linkId;
    bool stat;
    OpspfPortChangeStatType changeStat;
}OpspfInfData;

typedef struct InterfaceDatabase{
    int linkId[SINF_NUM];
    bool port_stat[SINF_NUM];
    int dst_portid[SINF_NUM];
    int dst_satid[SINF_NUM];
}ISDB;

// typedef struct InterfaceDatabase{
//     int linkId[SINF_NUM];
//     bool port_stat[SINF_NUM];
//     int dst_portid[SINF_NUM];
//     int dst_satid[SINF_NUM];
// }ISDB;

// typedef struct _iphdr //定义IP首部 
// { 
//     unsigned char h_verlen; //4位首部长度+4位IP版本号 
//     unsigned char tos; //8位服务类型TOS 
//     unsigned short total_len; //16位总长度（字节） 
//     unsigned short ident; //16位标识 
//     unsigned short frag_and_flags; //3位标志位 
//     unsigned char ttl; //8位生存时间 TTL 
//     unsigned char proto; //8位协议 (TCP, UDP 或其他) 
//     unsigned short checksum; //16位IP首部校验和 
//     unsigned int sourceIP; //32位源IP地址 
//     unsigned int destIP; //32位目的IP地址 
// }IP_HEADER; 


typedef struct _opspfhdr
{
    OpspfPacketType packetType;
    u_int16_t pktlen;
    int dst_addr;

}OPSPF_Header;

typedef struct opspf_hello_info
{
    int satelliteId;  
    int portId;
}OpspfHelloInfo;

typedef struct opspf_lsu_info
{
    int satelliteId;
    int portId;
    int srcSatelliteId;
    int srcPortId;
    int dstSatelliteId;
    int dstPortId;
    OpspfPortChangeStatType changeStat;
}OpspfLsuInfo;

typedef struct opspf_lsack_info
{
    int receive_satelliteId;
    int receive_portId;
}OpspfLsuackInfo;

typedef struct struct_network_opspf
{
    OpspfLsuInfo lsuData;
    int selfsatelliteId;
    StaticTopo G;
    OpspfInfData selfInf[6];
    ISDB Int_DB[SAT_NUM];


}OpspfData;

struct Tmp_Route_Table 
{
    int src_id;
    NodeAddress gw_addr;
    int dst_id;
    int src_portid;
    int gw_portid;
    int linkid;
    int cost;
};

unsigned short csum(unsigned short *buf, int nwords);

void network_init();

void initializeAllInterface(int satelliteId);

OpspfInfData interface_init(int satelliteId,int portId);

void initializeAllInt_DB();

void Int_DB_init(int satelliteId);

void initialize(int satelliteId);

void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType);

//void recv_opspf();

void *recv_opspf(void *ptr);

void *route_test1(void *ptr);

void *encapsulate_and_send_opspf(void *ptr);

void analyseIP(struct iphdr *ip);

void analyseOPSPF(OPSPF_Header* opspfhdr);

void Opspf_Handle_LsuPacket(const OpspfLsuInfo* lsuInfo,int interfaceIndex);

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo,int interfaceIndex);

void Opspf_Handle_LsuackPacket(const OpspfLsuackInfo* lsuackInfo,int interfaceIndex);

void *Send_Hello_Packet(void *ptr);

void Send_Hello_Packet();

void *Send_Lsu_Packet(void *ptr);

void Send_Lsu_Packet();

void Flood_Lsu_Packet(int interfaceIndex);



void Routing_table(int src_id);

int FindMin(unsigned int distance[],bool mark[]);

void Dijkstra(
    unsigned int sup_array[][SAT_NUM],
    int src_id,
    unsigned int distance[],
    int path[]);

int FindPreNode(
    int path[],
    int src_id,
    int dst_id);

void link_route_write();

void UpdateOpspfRoutingTable();

string uint2strIP(uint addr);

void route_test();

void *route_test(void *ptr);

#endif

