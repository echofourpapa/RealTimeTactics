//============================================================================
// Name        : RTT_Client.cpp
// Author      : AltF4
// Copyright   : 2011, GNU GPLv3
// Description : Code for handling socket IO on the server side
//============================================================================

#include "ClientProtocolHandler.h"
#include "messages/AuthMessage.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string.h>

#define SHA256_DIGEST_LENGTH 32

using namespace std;
using namespace RTT;

bool RTT::AuthToServer(int connectFD, string username, unsigned char *hashedPassword)
{
	//***************************
	// Send client Hello
	//***************************
	AuthMessage *client_hello = new AuthMessage();
	client_hello->type = CLIENT_HELLO;
	client_hello->softwareVersion.major = CLIENT_VERSION_MAJOR;
	client_hello->softwareVersion.minor = CLIENT_VERSION_MINOR;
	client_hello->softwareVersion.rev = CLIENT_VERSION_REV;

	if( Message::WriteMessage(client_hello, connectFD) == false)
	{
		//Error in write
		delete client_hello;
		return false;
	}
	delete client_hello;

	//***************************
	// Receive Server Hello
	//***************************
	Message *server_hello_init = Message::ReadMessage(connectFD);
	if( server_hello_init == NULL)
	{
		SendError(connectFD, PROTOCOL_ERROR);
		return false;
	}
	if( server_hello_init->type != SERVER_HELLO)
	{
		SendError(connectFD, PROTOCOL_ERROR);
		delete server_hello_init;
		return false;
	}
	AuthMessage *server_hello = (AuthMessage*)server_hello_init;

	//Check version compatibility
	if ((server_hello->softwareVersion.major != CLIENT_VERSION_MAJOR) ||
		(server_hello->softwareVersion.minor != CLIENT_VERSION_MINOR) ||
		(server_hello->softwareVersion.rev != CLIENT_VERSION_REV) )
	{
		//Incompatible software versions.
		//The server should have caught this, though.

		SendError(connectFD, AUTHENTICATION_ERROR);
		delete server_hello_init;
		return false;
	}
	delete server_hello_init;

	//***************************
	// Send Client Auth
	//***************************
	AuthMessage *client_auth = new AuthMessage();
	client_auth->type = CLIENT_AUTH;
	strncpy( client_auth->username, username.data(), USERNAME_MAX_LENGTH);
	memcpy(client_auth->hashedPassword, hashedPassword, SHA256_DIGEST_LENGTH);

	if( Message::WriteMessage(client_auth, connectFD) == false)
	{
		//Error in write
		delete client_auth;
		return false;
	}
	delete client_auth;

	//***************************
	// Receive Server Auth Reply
	//***************************
	Message *server_auth_reply_init = Message::ReadMessage(connectFD);
	if( server_auth_reply_init == NULL)
	{
		return false;
	}
	if( server_auth_reply_init->type != SERVER_AUTH_REPLY)
	{
		delete server_auth_reply_init;
		return false;
	}
	AuthMessage *server_auth_reply = (AuthMessage*)server_auth_reply_init;

	if( server_auth_reply->authSuccess != AUTH_SUCCESS)
	{
		return false;
	}

	return true;
}

//Informs the server that we want to exit
//	connectFD: Socket File descriptor of the server
//	Returns true if we get a successful acknowledgment back
bool RTT::ExitServer(int connectFD)
{
	//********************************
	// Send Exit Server Notification
	//********************************
	LobbyMessage *exit_server_notice = new LobbyMessage();
	exit_server_notice->type = MATCH_EXIT_SERVER_NOTIFICATION;
	if( Message::WriteMessage(exit_server_notice, connectFD) == false)
	{
		//Error in write
		delete exit_server_notice;
		return false;
	}
	delete exit_server_notice;

	//**********************************
	// Receive Exit Server Acknowledge
	//**********************************
	Message *exit_server_ack = Message::ReadMessage(connectFD);
	if( exit_server_ack == NULL)
	{
		return false;
	}
	if( exit_server_ack->type != MATCH_EXIT_SERVER_ACKNOWLEDGE)
	{
		delete exit_server_ack;
		return false;
	}

	return true;
}

//Get a page of match descriptions from the server
//	Writes the data into the matchArray array.
//	connectFD: Socket File descriptor of the server
//	page: What page of results to ask for? (>0)
//	matchArray: An array of MatchDescription's, of length MATCHES_PER_PAGE
//	Returns: The number of descriptions actually found
uint RTT::ListMatches(int connectFD, uint page, MatchDescription *matchArray)
{
	if(page < 1)
	{
		return 0;
	}

	//********************************
	// Send Match List Request
	//********************************
	LobbyMessage *list_request = new LobbyMessage();
	list_request->type = MATCH_LIST_REQUEST;
	list_request->requestedPage = page;
	if( Message::WriteMessage(list_request, connectFD) == false)
	{
		//Error in write
		delete list_request;
		return 0;
	}
	delete list_request;

	//**********************************
	// Receive Match List Reply
	//**********************************
	Message *list_reply_init = Message::ReadMessage(connectFD);
	if( list_reply_init == NULL)
	{
		return 0;
	}
	if( list_reply_init->type != MATCH_LIST_REPLY)
	{
		delete list_reply_init;
		return 0;
	}
	LobbyMessage *list_reply = (LobbyMessage*)list_reply_init;
	if( list_reply->returnedMatchesCount > MATCHES_PER_PAGE)
	{
		delete list_reply;
		return 0;
	}

	//Copy each Match Description in
	for( uint i = 0; i < list_reply->returnedMatchesCount; i++ )
	{
		matchArray[i] = list_reply->matchDescriptions[i];
	}

	return list_reply->returnedMatchesCount;

}

//Create a new Match on the server, and join that Match
//	connectFD: Socket File descriptor of the server
//	Returns: true if the match is created successfully
bool RTT::CreateMatch(int connectFD, struct MatchOptions options)
{
	//********************************
	// Send Match Create Request
	//********************************
	LobbyMessage *create_request = new LobbyMessage();
	create_request->type = MATCH_CREATE_REQUEST;
	if( Message::WriteMessage(create_request, connectFD) == false)
	{
		//Error in write
		delete create_request;
		return false;
	}
	delete create_request;

	//**********************************
	// Receive Match Options Available
	//**********************************
	Message *ops_available_init = Message::ReadMessage(connectFD);
	if( ops_available_init == NULL)
	{
		return false;
	}
	if( ops_available_init->type != MATCH_CREATE_OPTIONS_AVAILABLE)
	{
		delete ops_available_init;
		return false;
	}

	LobbyMessage *ops_available = (LobbyMessage*)ops_available_init;
	if( ops_available->options.maxPlayers < options.maxPlayers )
	{
		return false;
	}

	//********************************
	// Send Match Create Request
	//********************************
	LobbyMessage *ops_chosen = new LobbyMessage();
	ops_chosen->type = MATCH_CREATE_OPTIONS_CHOSEN;
	ops_chosen->options = options;
	if( Message::WriteMessage(ops_chosen, connectFD) == false)
	{
		//Error in write
		delete ops_chosen;
		return false;
	}
	delete ops_chosen;

	//**********************************
	// Receive Match Create Reply
	//**********************************
	Message *create_reply = Message::ReadMessage(connectFD);
	if( create_reply == NULL)
	{
		return false;
	}
	if( create_reply->type != MATCH_CREATE_REPLY)
	{
		delete create_reply;
		return false;
	}
	delete create_reply;

	return true;

}

//Joins the match at the given ID
//	connectFD: Socket File descriptor of the server
//	matchID: The server's unique ID for the chosen match
//	Returns: true if the match is joined successfully
bool RTT::JoinMatch(int connectFD, uint matchID)
{

	//********************************
	// Send Match Join Request
	//********************************
	LobbyMessage *join_request = new LobbyMessage();
	join_request->type = MATCH_JOIN_REQUEST;
	join_request->ID = matchID;
	if( Message::WriteMessage(join_request, connectFD) == false)
	{
		//Error in write
		delete join_request;
		return false;
	}
	delete join_request;

	//**********************************
	// Receive Match Join Reply
	//**********************************
	LobbyMessage *join_reply = (LobbyMessage*)Message::ReadMessage(connectFD);
	if( join_reply == NULL)
	{
		return false;
	}
	if( join_reply->type != MATCH_JOIN_REPLY)
	{
		delete join_reply;
		return false;
	}
	delete join_reply;

	return true;
}

//Leaves the match at the given ID
//	connectFD: Socket File descriptor of the server
//	matchID: The server's unique ID for the chosen match
//	Returns: true if the match is left cleanly
bool RTT::LeaveMatch(int connectFD, uint matchID)
{
	//********************************
	// Send Match Leave Notification
	//********************************
	LobbyMessage *leave_note = new LobbyMessage();
	leave_note->type = MATCH_LEAVE_NOTIFICATION;
	leave_note->ID = matchID;
	if( Message::WriteMessage(leave_note, connectFD) == false)
	{
		//Error in write
		delete leave_note;
		return false;
	}
	delete leave_note;

	//**********************************
	// Receive Match Leave Acknowledge
	//**********************************
	LobbyMessage *leave_ack = (LobbyMessage*)Message::ReadMessage(connectFD);
	if( leave_ack == NULL)
	{
		return false;
	}
	if( leave_ack->type != MATCH_LEAVE_ACKNOWLEDGE)
	{
		delete leave_ack;
		return false;
	}
	delete leave_ack;

	return true;
}

//Send a message of type Error to the client
void  RTT::SendError(int connectFD, enum ErrorType errorType)
{
	ErrorMessage *error_msg = new ErrorMessage();
	error_msg->type = MESSAGE_ERROR;
	error_msg->errorType = errorType;
	if(  Message::WriteMessage(error_msg, connectFD) == false)
	{
		cerr << "ERROR: Error message send returned failure.\n";
	}
	delete error_msg;
}