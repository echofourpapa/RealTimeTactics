//============================================================================
// Name        : Unit.h
// Author      : AltF4
// Copyright   : 2011, GNU GPLv3
// Description : Message class pertaining to client authentication
//============================================================================

#ifndef AUTHMESSAGE_H_
#define AUTHMESSAGE_H_

#include "Message.h"

using namespace std;

namespace LoF
{

enum AuthMechanism
{
	HASHED_SALTED_PASS = 0,
	SSH_KEY,
};

class AuthMessage: public Message
{

public:
	enum AuthMechanism authMechanism;
	struct VersionNumber serverVersion;
	char username[USERNAME_MAX_LENGTH];
	bool authSuccess;

	AuthMessage();
	AuthMessage(char *buffer, uint length);
	char *Serialize(uint *length);

};

}



#endif /* AUTHMESSAGE_H_ */