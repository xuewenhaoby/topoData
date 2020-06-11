#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string.h>

#define MOBILITY_FILE "topofile/orbit.bin"
#define DOUBLE_MOBILITY_FILE "topofile/topoData.txt"
#define TOPO_M 6 // number of orbit
#define TOPO_N 8 // number of satellite on one orbit
#define SAT_NUM TOPO_M * TOPO_N
#define DATA_SIZE 86401

using namespace std;
struct Coord{
    double lat;
    double lon;
};

typedef struct Position{
    Coord loc;
    bool isNorth;
}NodePos;

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

int main()
{
	ofstream fin;
	fin.open(DOUBLE_MOBILITY_FILE,ios::app);

	for (int i = 0; i < SAT_NUM; ++i)
	{
		for (int j = 0; j < 200 ; ++j)
		{
			// char* str = new char[40];
			// memset(str,0,40);
			 NodePos position = ReadLocFile(i,j);
			// sprintf(str,"%.d %.d",pos.loc.lat,pos.loc.lon);
			// fin.write(str,strlen(str));
			// delete str;

			string cmd = to_string(position.loc.lat)+" "+to_string(position.loc.lon)+" "+to_string(position.isNorth)+" ";
			fin<<cmd<<endl;
			cout<<cmd<<endl;
			

		}
	}
	fin.close();		
	return 0;
}