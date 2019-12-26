#include "topo.h"
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
	NodePos dstpos = getPos(dstNodeId);

	if(dstNodeId == getForeSatelliteId(srcNodeId) ||
		dstNodeId == getRearSatelliteId(srcNodeId))
	{
		return true;
	}
	else if(dstNodeId == getSideSatelliteId(srcNodeId, true, pos.isNorth) ||
		dstNodeId == getSideSatelliteId(srcNodeId, false, pos.isNorth)) 
	{
		return (fabs(pos.loc.lat) < TOPO_BETA) && 
				(fabs(dstpos.loc.lat) < TOPO_BETA);
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
// void TopoInitialize()
// {
// 	InitStaticTopo();

// 	for(int i = 0; i < SAT_NUM; i++)
// 	{
// 		 Sats[i].setId(i+1);
// 		 Sats[i].initialize();
// 	// Pool.addTask(new NodeTask(new Message(
// 	// 	&Sats[i],MSG_NODE_Initialize)));
// 	}
// }
// void TopoFinalize()
// {
// 	for(int i = 0; i < SAT_NUM; i++)
// 	{
// 		// NodeTask *tk = new NodeTask(
// 		// 	(void*)new Message(&Sats[i],MSG_NODE_Finalize));
// 		// Pool.addTask(tk);
// 		Sats[i].finalize();
// 	}
// }
// void TopoUpdate()
// {
	
// }
// void InitStaticTopo()
// {
// 	ReadIslFile(ISL_FILE);
// 	for(int i=0;i<SAT_NUM;i++)
// 	{
// 		for(int j = 0; j < SAT_NUM; j++){
// 			if(i == j){
// 				G.arcs[i][j].weight = 0;
// 				G.arcs[i][j].linkId = LINKID_SELFLOOP;
// 			}else{
// 				G.arcs[i][j].weight = MAX_ARC_WEIGHT;
// 				G.arcs[i][j].linkId = LINKID_NOLINK;
// 			}
// 		}

// 		for(int j = 0; j < SINF_NUM; j++){
// 			InfData tmp = {-1,false};
// 			Sats[i].setInfData(tmp,j);
// 		}
// 	}
// }

// int GetForeSatIndex(int idx,int num)
// {
// 	int obtIdx = GetOrbitIndex(idx);
// 	int sIdx = GetSatIndexInOrbit(idx);
// 	return obtIdx*TOPO_N + (sIdx+num+TOPO_N)%TOPO_N;
// }

// int GetRearSatIndex(int idx, int num)
// {
// 	int obtIdx = GetOrbitIndex(idx);
// 	int sIdx = GetSatIndexInOrbit(idx);
// 	return obtIdx*TOPO_N + (sIdx-num+TOPO_N)%TOPO_N;
// }

// int GetSideSatIndex(int idx,bool isEast,bool isNorth)
// {
// 	int obtIdx = GetOrbitIndex(idx);
// 	int sIdx = GetSatIndexInOrbit(idx);
// 	int v = -1;

// 	if(obtIdx == 0){
// 		if(isEast){
// 			if(isNorth)
// 				v = (obtIdx+1)*TOPO_N + (sIdx-1+TOPO_N)%TOPO_N;
// 			else
// 				v = (obtIdx+1)*TOPO_N + sIdx;
// 		}
// 	}else if(obtIdx == 5){
// 		if(!isEast){
// 			if(isNorth)
// 				v = (obtIdx-1)*TOPO_N + (sIdx+1)%TOPO_N;
// 			else
// 				v = (obtIdx-1)*TOPO_N + sIdx;			
// 		}
// 	}else{
// 		if(isEast){
// 			if(isNorth)
// 				v = (obtIdx+1)*TOPO_N + (sIdx+7)%TOPO_N;
// 			else
// 				v = (obtIdx+1)*TOPO_N + sIdx;
// 		}else{
// 			if(isNorth)
// 				v = (obtIdx-1)*TOPO_N + (sIdx+1)%TOPO_N;
// 			else
// 				v = (obtIdx-1)*TOPO_N + sIdx;	
// 		}
// 	}

// 	return v;
// }

// NodePos RandomPos()
// {
// 	srand((unsigned)time(NULL));
// 	Coord loc = {RANDOM(-90,90),RANDOM(-180,180)};
// 	NodePos pos = {loc,false};
// 	return pos;	
// }

// double CalculateDistance(Coord x, Coord y)
// {
// 	double wx = RADIAN(x.lat);
// 	double wy = RADIAN(y.lat);
// 	double a = wx - wy;
// 	double b = RADIAN(x.lon) - RADIAN(y.lon);
// 	double distance = pow(sin(a/2),2) + cos(wx)*cos(wy)*pow(sin(b/2),2);
// 	return 2*EARTH_RADIUS*asin(sqrt(distance));
// }
