#include <string.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

int main()
{
	//string cmd = "./route_test";
	string cmd = "ip route add 190.0.13.0/24 via 190.0.5.2 dev sat2p1";
	system(cmd.c_str());
	return 0;
}
