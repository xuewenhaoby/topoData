#ifndef __COMMON_H__
#define __COMMOM_H__

#include "thread_pool.h"
#include "opspf.h"
extern ThreadPool pool; 
extern ThreadPool link_route_pool;
extern StaticTopo G;
extern OpspfInfData selfInf[6];
extern ISDB Int_DB[SAT_NUM];
extern OpspfLsuInfo lsuData;
extern Tmp_Route_Table route_table;
extern unsigned int sup_array[SAT_NUM][SAT_NUM];
extern int receive_satelliteId ;
extern int receive_portId ;

extern int satelliteId;
extern bool recv_send_LSU;
extern int recv_send_LSU_index;
#endif