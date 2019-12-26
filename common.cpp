#include "common.h"
ThreadPool pool;
ThreadPool link_route_pool; 
StaticTopo G;
OpspfInfData selfInf[6];
ISDB Int_DB[SAT_NUM];
OpspfLsuInfo lsuData;
Tmp_Route_Table route_table;
unsigned int sup_array[SAT_NUM][SAT_NUM];
int receive_satelliteId = -1;
int receive_portId = -1;
int satelliteId;
bool recv_send_LSU =false;
int recv_send_LSU_index =-1;