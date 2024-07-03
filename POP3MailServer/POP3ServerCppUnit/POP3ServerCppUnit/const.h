#ifndef _CONST_H_
#define _CONST_H_

#include "./kernelTypes.h"

const uint8 SR_AUTOMATE_TYPE_ID = 0x00;
const uint8 TEST_AUTOMATE_TYPE_ID = 0x01;

const uint8 SR_AUTOMATE_MBX_ID = 0x00;
const uint8 TEST_MBX_ID = 0x01;

//	Server states
enum SrStates {
	FSM_SERVER_IDLE = 0,
	FSM_SERVER_USER_AUTORISATION,
	FSM_SERVER_PASS_AUTORISATION,
	FSM_SERVER_ACTIVE,
	FSM_SERVER_STATE_COUNT
};

const uint16 MSG_Conn_req	=	0x0001;
const uint16 MSG_user		=	0x0002;
const uint16 MSG_pass		=	0x0003;
const uint16 MSG_state		=	0x0004;
const uint16 MSG_retr		=	0x0005;
const uint16 MSG_quit		=	0x0006;
const uint16 MSG_other		=	0x0007;




#define ADRESS "127.0.0.0"	//localhost variable in client
#define PORT 110

#define PARAM_DATA 0x01
#define PARAM_Name 0x02
#define PARAM_Pass 0x03

#endif //_CONST_H_