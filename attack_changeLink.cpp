#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "attack_changeLink.h"
#include "shell.h"

using namespace std;

int satelliteId,satelliteId_opposite,attackMode;

int main(int argc,char* argv[])
{
	satelliteId = atoi(argv[1]);
	satelliteId_opposite = atoi(argv[2]);
	string br = cmd(SAT_BR_NAME, satelliteId);
	attackMode = atoi(argv[3]);
	attackLinkChange(br);
	switch(attackMode)
	{
		case SEND_FAKE_HELLO_PACKET:
		{
			changeOVS_flow_v1(br);
			break;
		}
		case SEND_FAKE_LSU_PACKET :
		{
			changeOVS_flow_v1(br);
			break;
		}
		case MODIFY_LSU_PACKET:
		{
			changeOVS_flow_v2(br);
			break;

		}
		case MODIFY_LSACK_PACKET :
		{
			changeOVS_flow_v2(br);
			break;

		}
		case SEND_FAKE_LSACK_PACKET :
		{
			changeOVS_flow_v3(br);
			break;
		}
		default:
		{
			break;
		}
		
	}
}

void attackLinkChange(string br)
{
	//delete veth-peer
	string ns = cmd(SAT_NAME,satelliteId);
	string ns_opposite = cmd(SAT_NAME,satelliteId_opposite);
	run_q(ns_do(ns,veth_del(cmd(SAT_NAME,satelliteId,"p0"))));
	run_q(ns_do(ns_opposite,veth_del(cmd(SAT_NAME,satelliteId_opposite,"p0"))));
	//add attack netns 
	string ns_attack = cmd(ATTACK_NAME,satelliteId);
	run_q(ns_add(ns_attack));
	run_q(ns_do(ns_attack,dev_set_stat("lo",true)));
	//add OVS
	// string br = cmd(SAT_BR_NAME, satelliteId);
	run(ovs_add_br(br));
	run(ovs_set_mode(br));

	string opposite_tap1 = ns_opposite + "p0", opposite_tap2 = br+"p1";
	run_q(link_add(opposite_tap1,opposite_tap2));
	run_q(ns_add_port(ns_opposite,opposite_tap1));
	run_q(ns_do(ns_opposite,dev_set_stat(opposite_tap1,true)));
	run(dev_set_stat(opposite_tap2,true));

	string ns_tap1 = ns + "p0", ns_tap2 = br+"p2";
	run_q(link_add(ns_tap1,ns_tap2));
	run_q(ns_add_port(ns,ns_tap1));
	run_q(ns_do(ns,dev_set_stat(ns_tap1,true)));
	run(dev_set_stat(ns_tap2,true));

	string attack_tap1 = ns_attack + "p100", attack_tap2 = br+"p3";
	run_q(link_add(attack_tap1,attack_tap2));
	run_q(ns_add_port(ns_attack,attack_tap1));
	run_q(ns_do(ns_attack,dev_set_stat(attack_tap1,true)));
	run(dev_set_stat(attack_tap2,true));
	run_q(ns_do(ns_attack,dev_addr_add(attack_tap1,"190.0.1.100/24","190.0.1.255")));
	
	string port1 = cmd(br,"p1");
	run(ovs_add_port(br,port1,1));
	
	string port2 = cmd(br,"p2");
	run(ovs_add_port(br,port2,2));

	string port3 = cmd(br,"p3");
	run(ovs_add_port(br,port3,3));
}

void changeOVS_flow_v1(string br)
{
	run(ovs_add_flow(br,"in_port=2,priority=10,actions=output:1"));
	run(ovs_add_flow(br,"in_port=1,priority=10,actions=output:2"));
	run(ovs_add_flow(br,"in_port=3,priority=10,actions=output:1"));
}

void changeOVS_flow_v2(string br)
{
	run(ovs_add_flow(br,"in_port=2,priority=10,actions=output:3"));
	run(ovs_add_flow(br,"in_port=1,priority=10,actions=output:2"));
	run(ovs_add_flow(br,"in_port=3,priority=10,actions=output:1"));
}

void changeOVS_flow_v3(string br)
{
	run(ovs_add_flow(br,"in_port=2,priority=10,actions=output:1"));
	run(ovs_add_flow(br,"in_port=1,priority=10,actions=output:3"));
	run(ovs_add_flow(br,"in_port=3,priority=10,actions=output:1"));
}
