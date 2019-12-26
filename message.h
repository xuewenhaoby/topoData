#ifndef __MESSAGA_H__
#define __MESSAGE_H__
#include "opspf.h"
#define SET(a,_a) a = _a
#define GET(a) return a
#define MESSAGE_FREE(m) delete m; m = NULL
enum EventType{
	MSG_NODE_SendHelloPacket,

	MSG_NODE_SendLsuPacket,

	MSG_NODE_FloodLsuPacket,

	MSG_SOCKET_Initialize,

	MSG_ROUTE_Update,
	
	// MSG_SAT_UpdatePos,
	
	MSG_TIME_Timer,

};

class Message{
public:
	Message(){}
	Message(EventType _type){
		SET(type,_type);
	}
	~Message(){}

	//void * getNode(){GET(node);}
	EventType getEventType(){GET(type);}
	void setEventType(EventType _type){SET(type,_type);}
private:
	//void *node;
	EventType type;
};
#endif