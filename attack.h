#ifndef __ATTACK_H__
#define __ATTACK_H__

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
#define OPSPF_LSU_INTERVAL 1
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

enum AttackMode
{
    SEND_FAKE_HELLO_PACKET =1,
    SEND_FAKE_LSU_PACKET   =2,
    MODIFY_LSU_PACKET =3,
    MODIFY_LSACK_PACKET =4,
    SEND_FAKE_LSACK_PACKET = 5

};

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


void initialize(int satelliteId);

OpspfInfData interface_init(int satelliteId,int portId);

void OpspfSendProtocolPacket(OpspfInfData inf,OpspfPacketType packetType);

void *recv_opspf(void *ptr);

void analyseIP(struct iphdr *ip);

void Opspf_Handle_HelloPacket(const OpspfHelloInfo* helloInfo);

void Opspf_Modify_LsuPacket(const OpspfLsuInfo* lsuInfo);

void Opspf_Modify_LsuackPacket(const OpspfLsuackInfo* lsuackInfo);

void Send_FakeHello_Packet();

void Send_FakeLsu_Packet();

void Send_FakeLsack_Packet();



#endif

