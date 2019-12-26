#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sys/wait.h>
#define runCmd_noReply(m) run_cmd(m,"1> /dev/null 2> /dev/null") 

using namespace std;

void run_cmd(char *_cmd,const char *attr)
{
	char cmd[100];
	sprintf(cmd,"%s %s",_cmd,attr);
	if(system(cmd) < 0){
		printf("%s error:%s\n",cmd,strerror(errno));
	}
}
int main()
{
	int i,j;
	for(i=0;i<49;i++)
{
	 for(j=0;j<6;j++)
	 {
	 	char cmd[100];
		sprintf(cmd,"ip link del sat%dp%d",i,j);
	 	runCmd_noReply(cmd);
	 }
	char cmd[100];
		sprintf(cmd,"ip netns del sat%d",i);
		runCmd_noReply(cmd);
}
}

