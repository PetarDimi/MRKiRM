#include <stdio.h>

#include "const.h"
#include "ServerAuto.h"
#include "dirent.h"
#include <thread.h>

#include <string.h>
#include <conio.h>

bool g_ProgramEnd = false;

#define StandardMessageCoding 0x00

char* users[4] = { "Aleksa\0", "Katarina\0", "Mitar\0", "Petar\0" };
char* passwords[4] = { "IgramSeAdvokata\0", "KlupciZakon\0", "Krajisnici015\0", "PetarDimi28\0" };
int user_index;


/*
*params:
*3. number of time control
*4. max states for one automate
*5. max number of transitions functions
*/
SrAuto::SrAuto() : FiniteStateMachine(SR_AUTOMATE_TYPE_ID, SR_AUTOMATE_MBX_ID, 0, FSM_SERVER_STATE_COUNT, 3) {
}

SrAuto::~SrAuto() {
}

/* This function actually connnects the ClAuto with the mailbox. */
uint8 SrAuto::GetMbxId() {
	return SR_AUTOMATE_MBX_ID;
}

uint32 SrAuto::GetObject() {
	return GetObjectId();
}

MessageInterface *SrAuto::GetMessageInterface(uint32 id) {
	return &StandardMsgCoding;
}

void SrAuto::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}

void SrAuto::SetDefaultFSMData() {
	SetDefaultHeader(StandardMessageCoding);
}

void SrAuto::NoFreeInstances() {
	printf("[%d] SrAuto::NoFreeInstances()\n", GetObjectId());
}

void SrAuto::Reset() {
	printf("[%d] SrAuto::Reset()\n", GetObjectId());
}


void SrAuto::Initialize() {
	SetState(FSM_SERVER_IDLE);

	//intitialization message handlers
	InitEventProc(FSM_SERVER_IDLE, MSG_Conn_req, (PROC_FUN_PTR)&SrAuto::FSM_Server_Idle_Set_All);

	//Username login
	InitEventProc(FSM_SERVER_USER_AUTORISATION, MSG_user, (PROC_FUN_PTR)&SrAuto::FSM_Server_Authorising_username);
	InitEventProc(FSM_SERVER_USER_AUTORISATION, MSG_quit, (PROC_FUN_PTR)&SrAuto::FSM_Server_Disconnect);
	InitEventProc(FSM_SERVER_USER_AUTORISATION, MSG_other, (PROC_FUN_PTR)&SrAuto::FSM_Server_Invalid_Request);

	//Password login
	InitEventProc(FSM_SERVER_PASS_AUTORISATION, MSG_pass, (PROC_FUN_PTR)&SrAuto::FSM_Server_Authorising_pass);
	InitEventProc(FSM_SERVER_PASS_AUTORISATION, MSG_quit, (PROC_FUN_PTR)&SrAuto::FSM_Server_Disconnect);
	InitEventProc(FSM_SERVER_PASS_AUTORISATION, MSG_other, (PROC_FUN_PTR)&SrAuto::FSM_Server_Invalid_Request);

	//Actions
	InitEventProc(FSM_SERVER_ACTIVE, MSG_state, (PROC_FUN_PTR)&SrAuto::FSM_Server_Mail_Check);
	InitEventProc(FSM_SERVER_ACTIVE, MSG_quit, (PROC_FUN_PTR)&SrAuto::FSM_Server_Disconnect);
	InitEventProc(FSM_SERVER_ACTIVE, MSG_other, (PROC_FUN_PTR)&SrAuto::FSM_Server_Invalid_Request);
}


void Monoalphabetic_Encryption(char* message) {
	char ptc[36] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','R','S','T','U','V',' ','+','-','.',',','1','2','3','4','5','6','7','8','9','0' };
	char ctc[36] = { 'G','H','I','J','K','L','M','N','O','P','R','S','T','U','V','A','B','C','D','E','F','Q','Y','W','X','Z','3','4','5','6','7','8','9','0','1','2' };

	char ptl[36] = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','r','s','t','u','v' };
	char ctl[36] = { 'g','h','i','j','k','l','m','n','o','p','r','s','t','u','v','a','b','c','d','e','f' };

	int i = 0;
	int j;

	while (message[i] != '\0') {
		for (j = 0; j < 36; j++) {
			if (ptc[j] == message[i]) {
				message[i] = ctc[j];
				break;
			}
			else if (ptl[j] == message[i]) {
				message[i] = ctl[j];
				break;
			}
		}
		i++;
	}
}

void Monoalphabetic_Decryption(char* message) {
	char ptc[36] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','R','S','T','U','V',' ','+','-','.',',','1','2','3','4','5','6','7','8','9','0' };
	char ctc[36] = { 'G','H','I','J','K','L','M','N','O','P','R','S','T','U','V','A','B','C','D','E','F','Q','Y','W','X','Z','3','4','5','6','7','8','9','0','1','2' };

	char ptl[36] = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','r','s','t','u','v' };
	char ctl[36] = { 'g','h','i','j','k','l','m','n','o','p','r','s','t','u','v','a','b','c','d','e','f' };


	int i = 0;
	int j;

	while (message[i] != '\0') {
		for (j = 0; j < 36; j++) {
			if (ctc[j] == message[i]) {
				message[i] = ptc[j];
				break;
			}
			else if (ctl[j] == message[i]) {
				message[i] = ptl[j];
				break;
			}
		}
		i++;
	}
}

void SrAuto::FSM_Server_Idle_Set_All() {
	int c;

	WSADATA wsaData;

	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return;
	}

	//Create socket
	m_Server_Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_Server_Socket == -1)
	{
		printf("TCP : Could not create socket");
	}
	puts("TCP : Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(m_Server_Socket, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		//print the error message
		perror("TCP : bind failed. Error");
		return;
	}
	puts("TCP : bind done");

	//Listen
	listen(m_Server_Socket, 3);

	//Accept and incoming connection
	puts("TCP : Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	//accept connection from an incoming client
	m_Client_Socket = accept(m_Server_Socket, (struct sockaddr *)&client, (int*)&c);
	if (m_Client_Socket < 0)
	{
		perror("TCP : accept failed");
		return;
	}
	puts("TCP : Client connected");


	char message[] = "+OK\r\n\0";
	Monoalphabetic_Encryption(message);

	if (send(m_Client_Socket, message, strlen(message), 0) < 0)
	{
		puts("Send failed");
		return;
	}


	SetState(FSM_SERVER_USER_AUTORISATION);

	/* Then, start the thread that will listen on the the newly created socket. */
	m_hThread = CreateThread(NULL, 0, ServerListener, (LPVOID) this, 0, &m_nThreadID);
	if (m_hThread == NULL) {
		/* Cannot create thread.*/
		closesocket(m_Server_Socket);
		m_Server_Socket = INVALID_SOCKET;
		return;
	}
}

void SrAuto::FSM_Server_Authorising_username() {
	int user_valid = 0;

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];
	memcpy(data, buffer + 4, size);
	data[size - 2] = 0;
	//printf("Encrypted received message: %s\n", data);
	Monoalphabetic_Decryption(data + 5);
	printf("User: %s\n", data);

	for (int i = 0; i < NUM_USERS; i++)
	{
		if (strcmp(data + 5, users[i]) == 0) //check username
		{
			user_valid = 1;
			user_index = i;
			break;
		}
	}

	
	if (user_valid == 1)
	{
		char message[] = "+OK\r\n\0";
		printf("SENT: %s", message);
		Monoalphabetic_Encryption(message);
		//printf("Encrypted sent message: %s", message);
		if (send(m_Client_Socket, message, strlen(message), 0) < 0)
		{
			puts("Send failed");
			return;
		}

		SetState(FSM_SERVER_PASS_AUTORISATION);

	}
	else {
		char message[] = "ERROR Wrong user\n\0";
		printf("SENT: %s", message);
		Monoalphabetic_Encryption(message);
		//printf("Encrypted sent message: %s", message);
		if (send(m_Client_Socket, message, strlen(message), 0) < 0)
		{
			puts("Send failed");
			return;
		}

		SetState(FSM_SERVER_USER_AUTORISATION);
	}
	delete[] data;
}

void SrAuto::FSM_Server_Authorising_pass() {
	int pass_valid = 0;

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];

	memcpy(data, buffer + 4, size);
	data[size - 2] = 0;
	//printf("Encrypted received message: %s\n", data);
	Monoalphabetic_Decryption(data + 5);
	printf("Password: %s\n", data);

	if (strcmp(data + 5, passwords[user_index]) == 0) //provera user-a
	{
		pass_valid = 1;
	}

	if (pass_valid == 1)
	{

		char message[] = "+OK\r\n\0";
		printf("SENT: %s", message);
		Monoalphabetic_Encryption(message);
		//printf("Encrypted sent message: %s", message);
		if (send(m_Client_Socket, message, strlen(message), 0) < 0)
		{
			puts("Send failed");
			return;
		}

		SetState(FSM_SERVER_ACTIVE);

	}
	else {
		char message[] = "ERROR Wrong password\n\0";
		printf("SENT: %s\n", message);
		Monoalphabetic_Encryption(message);
		//printf("Encrypted sent message: %s\n", message);
		if (send(m_Client_Socket, message, strlen(message), 0) < 0)
		{
			puts("Send failed");
			return;
		}

		SetState(FSM_SERVER_PASS_AUTORISATION);
	}
	delete[] data;
}

void SrAuto::FSM_Server_Mail_Check() {

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];

	memcpy(data, buffer + 4, size);
	data[size - 2] = 0;
	//printf("Encrypted received message: %s\n", data);
	Monoalphabetic_Decryption(data + 5);
	printf("Request: %s\n", data);

	
	char message[] = "+OK 0 0 \r\n\0";
	printf("SENT: %s", message);
	Monoalphabetic_Encryption(message);
	//printf("Encrypted sent message: %s", message);
	if (send(m_Client_Socket, message, strlen(message), 0) < 0)
	{

		return;
	}

	SetState(FSM_SERVER_ACTIVE);
	delete[] data;
	
}

void SrAuto::FSM_Server_Invalid_Request()
{
	char message[50];
	strcpy(message, "ERR 404 Invalid request \r\n");
	Monoalphabetic_Encryption(message);
	puts("Waiting for another request");
	if (send(m_Client_Socket, message, strlen(message), 0) < 0)
	{
		puts("Send failed");
		return;
	}
}

void SrAuto::FSM_Server_Disconnect() {

	
	char message[] = "+OK\r\n\0";
	printf("SENT: %s", message);
	Monoalphabetic_Encryption(message);
	//printf("Encrypted sent message: %s", message);
	if (send(m_Client_Socket, message, strlen(message), 0) < 0)
	{
		return;
	}

	shutdown(m_Client_Socket, 2);
	shutdown(m_Server_Socket, 2);

	CloseHandle(m_hThread);
	closesocket(m_Client_Socket);
	closesocket(m_Server_Socket);

	g_ProgramEnd = true;

	SetState(FSM_SERVER_IDLE);
}

/* This method is sendig message to activate current state */
void SrAuto::NetMsg_2_FSMMsg(const char* apBuffer, uint16 anBufferLength) {
	
	int i = 0;
	char* operationBuffer = new char[16];
	for (int i = 0; i < 4; i++)
	{
		operationBuffer[i] = apBuffer[i];
	}
	if (!strncmp(operationBuffer, "user", 4))
	{
		PrepareNewMessage(0x00, MSG_user);
	}
	else if (!strncmp(operationBuffer, "pass", 4))
	{
		PrepareNewMessage(0x00, MSG_pass);
	}
	else if (!strncmp(operationBuffer, "state", 4))
	{
		PrepareNewMessage(0x00, MSG_state);
	}
	else if (!strncmp(operationBuffer, "retr", 4))
	{
		PrepareNewMessage(0x00, MSG_retr);
	}
	else if (!strncmp(operationBuffer, "quit", 4))
	{
		PrepareNewMessage(0x00, MSG_quit);
	}
	else 
	{
		PrepareNewMessage(0x00, MSG_other);
	}

	SetMsgToAutomate(SR_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	AddParam(PARAM_DATA, anBufferLength, (uint8 *)apBuffer);
	SendMessage(SR_AUTOMATE_MBX_ID);
	delete[] operationBuffer;

}

DWORD SrAuto::ServerListener(LPVOID param) {
	SrAuto* pParent = (SrAuto*)param;
	int nReceivedBytes;
	char* buffer = new char[255];


	/* Receive data from the network until the socket is closed. */
	do {
		nReceivedBytes = recv(pParent->m_Client_Socket, buffer, 255, 0);
		if (nReceivedBytes == 0)
		{
			printf("Client disconnected!\n");
			pParent->FSM_Server_Disconnect();
			break;
		}
		if (nReceivedBytes < 0) {
			printf("Client disconnected!\n");
			pParent->FSM_Server_Disconnect();
			break;
		}
		pParent->NetMsg_2_FSMMsg(buffer, nReceivedBytes);

		Sleep(1000);

	} while (1);



	delete[] buffer;
	return 1;
}

/* Automat sending message to itself for starting system */
void SrAuto::Start() {

	PrepareNewMessage(0x00, MSG_Conn_req);
	SetMsgToAutomate(SR_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(SR_AUTOMATE_MBX_ID);
}