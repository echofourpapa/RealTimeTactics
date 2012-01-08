//============================================================================
// Name        : RTT_Server.cpp
// Author      : AltF4
// Copyright   : 2011, GNU GPLv3
// Description : A single match (or game) that is being played.
//					A server can have many at one time
//============================================================================s

#include "Match.h"

using namespace std;
using namespace RTT;

Match::Match(Player *player)
{
	memset(&description, '\0', sizeof(description));
	struct timeval tv;
	gettimeofday(&tv,NULL);
	timeCreated = tv.tv_sec;
	description.timeCreated = tv.tv_sec;
	for(uint i = 0; i < MAX_TEAMS; i++)
	{
		teams[i] = new Team((enum TeamNumber)i);
	}
	leaderID = player->GetID();
	currentPlayerCount = 0;
	pthread_rwlock_init(&lock, NULL);
}

Match::~Match()
{
	pthread_rwlock_wrlock(&lock);
	for(uint i = 0; i < MAX_TEAMS; i++)
	{
		if( teams[i] != NULL )
		{
			delete teams[i];
		}
	}
	pthread_rwlock_unlock(&lock);
}

//SET methods
void Match::SetID(uint newID)
{
	pthread_rwlock_wrlock(&lock);
	ID = newID;
	description.ID = newID;
	pthread_rwlock_unlock(&lock);
}

void Match::SetStatus(enum Status newStatus)
{
	pthread_rwlock_wrlock(&lock);
	status = newStatus;
	description.status = newStatus;
	pthread_rwlock_unlock(&lock);
}

void Match::SetMaxPlayers(uint newMaxPlayers)
{
	pthread_rwlock_wrlock(&lock);
	maxPlayers = newMaxPlayers;
	description.maxPlayers = newMaxPlayers;
	pthread_rwlock_unlock(&lock);
}

void Match::SetName(string newName)
{
	pthread_rwlock_wrlock(&lock);
	name = newName;
	name.resize(MAX_MATCHNAME_LEN);
	strncpy(description.name, newName.c_str(), MAX_MATCHNAME_LEN);
	pthread_rwlock_unlock(&lock);
}

void Match::SetLeaderID(uint nextLeader)
{
	pthread_rwlock_wrlock(&lock);
	leaderID = nextLeader;
	pthread_rwlock_unlock(&lock);
}

void Match::SetMap(struct MapDescription newMap)
{
	pthread_rwlock_wrlock(&lock);
	map = newMap;
	pthread_rwlock_unlock(&lock);
}

void Match::SetVictoryCondition(enum VictoryCondition newVict)
{
	pthread_rwlock_wrlock(&lock);
	victoryCondition = newVict;
	pthread_rwlock_unlock(&lock);
}

void Match::SetGamespeed(enum GameSpeed newSpeed)
{
	pthread_rwlock_wrlock(&lock);
	gameSpeed = newSpeed;
	pthread_rwlock_unlock(&lock);
}

//GET methods
enum Status Match::GetStatus()
{
	pthread_rwlock_rdlock(&lock);
	enum Status tempStatus = status;
	pthread_rwlock_unlock(&lock);
	return tempStatus;
}

uint Match::GetID()
{
	pthread_rwlock_rdlock(&lock);
	uint tempID = ID;
	pthread_rwlock_unlock(&lock);
	return tempID;
}

uint Match::GetMaxPlayers()
{
	pthread_rwlock_rdlock(&lock);
	uint tempMax = maxPlayers;
	pthread_rwlock_unlock(&lock);
	return tempMax;
}

uint Match::GetCurrentPlayerCount()
{
	pthread_rwlock_rdlock(&lock);
	uint tempCurr = currentPlayerCount;
	pthread_rwlock_unlock(&lock);
	return tempCurr;
}

string Match::GetName()
{
	pthread_rwlock_rdlock(&lock);
	string tempName = name;
	pthread_rwlock_unlock(&lock);
	return tempName;
}

uint Match::GetLeaderID()
{
	pthread_rwlock_rdlock(&lock);
	uint tempID = ID;
	pthread_rwlock_unlock(&lock);
	return tempID;
}

struct MatchDescription Match::GetDescription()
{
	pthread_rwlock_rdlock(&lock);
	struct MatchDescription tempDesc = description;
	pthread_rwlock_unlock(&lock);
	return tempDesc;
}

struct MapDescription Match::GetMap()
{
	pthread_rwlock_rdlock(&lock);
	struct MapDescription tempMap = map;
	pthread_rwlock_unlock(&lock);
	return tempMap;
}

enum VictoryCondition Match::GetVictoryCondition()
{
	pthread_rwlock_rdlock(&lock);
	enum VictoryCondition tempVict = victoryCondition;
	pthread_rwlock_unlock(&lock);
	return tempVict;
}

enum GameSpeed Match::GetGamespeed()
{
	pthread_rwlock_rdlock(&lock);
	enum GameSpeed tempSpeed = gameSpeed;
	pthread_rwlock_unlock(&lock);
	return tempSpeed;
}

bool Match::AddPlayer(Player *player, enum TeamNumber teamNum)
{
	pthread_rwlock_rdlock(&lock);
	if( currentPlayerCount >= maxPlayers )
	{
		pthread_rwlock_unlock(&lock);
		return false;
	}
	if( teamNum > REFEREE)
	{
		pthread_rwlock_unlock(&lock);
		return false;
	}
	pthread_rwlock_unlock(&lock);

	teams[teamNum]->AddPlayer(player);

	pthread_rwlock_wrlock(&lock);
	currentPlayerCount++;
	description.currentPlayerCount++;
	pthread_rwlock_unlock(&lock);

	return true;
}

bool Match::RemovePlayer( uint playerID )
{

	for(uint i = 0; i < MAX_TEAMS; i++)
	{
		if( teams[i]->RemovePlayer(playerID))
		{
			uint nextID = GetFirstPlayerID();

			pthread_rwlock_wrlock(&lock);
			leaderID = nextID;
			currentPlayerCount--;
			description.currentPlayerCount--;
			pthread_rwlock_unlock(&lock);

			return true;
		}
	}
	return false;
}

Player* Match::GetPlayer( uint playerID )
{
	for(uint i = 0; i < MAX_TEAMS; i++)
	{
		Player *player = teams[i]->GetPlayer(playerID);
		if( player != NULL)
		{
			return player;
		}
	}
	return NULL;
}

//Get the first Player in the teams lists
//	For use in getting the next leader when one leaves
//	Returns 0 if there are no more players
uint Match::GetFirstPlayerID()
{
	for(uint i = 0; i < MAX_TEAMS; i++)
	{
		uint retID = teams[i]->GetFirstPlayerID();
		if( retID > 0 )
		{
			return retID;
		}
	}
	return 0;
}

bool Match::ChangeTeam(Player *player, enum TeamNumber newTeam)
{
	if( newTeam > REFEREE)
	{
		return false;
	}
	for(uint i = 0; i < MAX_TEAMS; i++)
	{
		if( teams[i]->RemovePlayer(player->GetID()) == true)
		{
			teams[newTeam]->AddPlayer(player);
			return true;
		}
	}
	return false;
}

bool Match::StartMatch()
{

}
