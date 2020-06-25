/**
 * sqlUser.cc
 *
 * Storage class for accessing user information either from the backend
 * or internal storage.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 *
 * $Id: sqlUser.cc,v 1.49 2012/05/28 20:40:28 Seven Exp $
 */

#include	<sstream>
#include	<string>
#include	<iostream>

#include	<cstring>

#include	"ELog.h"
#include	"misc.h"
#include	"sqlUser.h"
#include	"constants.h"
#include	"cservice.h"
#include	"cservice_config.h"
#ifdef HAVE_LIBOATH
extern "C"
{
	#include <liboath/oath.h>
}
#endif

namespace gnuworld
{

using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

const sqlUser::flagType sqlUser::F_GLOBAL_SUSPEND =	0x01 ;
const sqlUser::flagType sqlUser::F_LOGGEDIN =		0x02 ;
const sqlUser::flagType sqlUser::F_INVIS =		0x04 ;
const sqlUser::flagType sqlUser::F_FRAUD =		0x08 ;
const sqlUser::flagType sqlUser::F_NONOTES =		0x10 ;
const sqlUser::flagType sqlUser::F_NOPURGE =		0x20 ;
const sqlUser::flagType sqlUser::F_NOADMIN =		0x40 ;
const sqlUser::flagType sqlUser::F_ALUMNI =		0x80 ;
const sqlUser::flagType sqlUser::F_OPER =		0x100 ;
const sqlUser::flagType sqlUser::F_NOADDUSER =		0x200 ;
const sqlUser::flagType sqlUser::F_TOTP_ENABLED = 	0x400;
const sqlUser::flagType sqlUser::F_AUTONICK = 	0x800;
const sqlUser::flagType sqlUser::F_POWER = 	0x1000;

const unsigned int sqlUser::EV_SUSPEND		= 1;
const unsigned int sqlUser::EV_UNSUSPEND	= 2;
const unsigned int sqlUser::EV_ADMINMOD		= 3;
const unsigned int sqlUser::EV_MISC		= 4;
const unsigned int sqlUser::EV_COMMENT		= 5;

sqlUser::sqlUser(dbHandle* _SQLDb)
 : id( 0 ),
   user_name(),
   password(),
   last_seen( 0 ),
   url(),
   nickname(),
   language_id( 0 ),
   flags( 0 ),
   last_used( 0 ),
   created_ts(0),
   instantiated_ts( ::time(NULL) ),
   signup_ip(),
   email(),
   verifNr(0),
   verifdata(),
   last_hostmask(),
   maxlogins(0),
   last_note(0),
   notes_sent(0),
   failed_logins(0),
   failed_login_ts(0),
   totp_key(),
   totp_hex_key(),
   hostname(),
   SQLDb( _SQLDb )
{
}

/*
 *  Load all data for this user from the backend. (Key: userID)
 */

bool sqlUser::loadData(int userID)
{
/*
 *  With the open database handle 'SQLDb', retrieve information about
 *  'userID' and fill our member variables.
 */

#ifdef LOG_DEBUG
	elog	<< "sqlUser::loadData> Attempting to load data for user-id: "
		<< userID
		<< endl;
#endif

stringstream queryString;
queryString	<< "SELECT "
		<< sql::user_fields
		<< " FROM users WHERE id = "
		<< userID
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::loadData> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( SQLDb->Exec(queryString, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	/*
	 *  If the user doesn't exist, we won't get any rows back.
	 */

	if(SQLDb->Tuples() < 1)
		{
		return (false);
		}

	setAllMembers(0);

	return (true);
	}

return (false);
}

bool sqlUser::loadData(const string& userName)
{
/*
 *  With the open database handle 'SQLDb', retrieve information about
 *  'userID' and fill our member variables.
 */

#ifdef LOG_DEBUG
	elog	<< "sqlUser::loadData> Attempting to load data for user-name: "
		<< userName
		<< endl;
#endif

stringstream queryString;
queryString	<< "SELECT "
		<< sql::user_fields
		<< " FROM users WHERE lower(user_name) = '"
		<< escapeSQLChars(string_lower(userName))
		<< "'"
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::loadData> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( SQLDb->Exec(queryString, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	/*
	 *  If the user doesn't exist, we won't get any rows back.
	 */

	if(SQLDb->Tuples() < 1)
		{
		return (false);
		}

	setAllMembers(0);

	return (true);
	}

return (false);
}


void sqlUser::setAllMembers(int row)
{
/*
 *  Support function for both loadData's.
 *  Assumes SQLDb contains a valid results set for all user information.
 */

id = atoi(SQLDb->GetValue(row, 0));
user_name = SQLDb->GetValue(row, 1);
password = SQLDb->GetValue(row, 2);
url = SQLDb->GetValue(row, 3);
nickname = SQLDb->GetValue(row, 4);
language_id = atoi(SQLDb->GetValue(row, 5));
flags = atoi(SQLDb->GetValue(row, 6));
last_updated_by = SQLDb->GetValue(row, 7);
last_updated = atoi(SQLDb->GetValue(row, 8));
email = SQLDb->GetValue(row, 9);
maxlogins = atoi(SQLDb->GetValue(row, 10));
verifdata = SQLDb->GetValue(row, 11);
totp_key = SQLDb->GetValue(row, 12);
hostname = SQLDb->GetValue(row, 13);
failed_logins = 0;
failed_login_ts = 0;
instantiated_ts = atoi(SQLDb->GetValue(row, 14));
signup_ip = SQLDb->GetValue(row, 15);
created_ts = atoi(SQLDb->GetValue(row, 16));

/* Fetch the "Last Seen" time from the users_lastseen table. */

}

bool sqlUser::commit(iClient* who)
{
/*
 *  Build an SQL statement to commit the transient data in this storage class
 *  back into the database.
 */
if(who)
{
	last_updated_by = who->getNickUserHost();
} else {
	last_updated_by = "Marvin, the paranoid android.";
}

static const char* queryHeader =    "UPDATE users ";
static const char* queryCondition = "WHERE id = ";

stringstream queryString;
queryString	<< queryHeader
		<< "SET flags = " << flags << ", "
		<< "password = '" << password << "', "
		<< "url = '" << url << "', "
		<< "nickname = '" << escapeSQLChars(nickname) << "', "
//		<< "question_id = " << verifNr << ", "
//		<< "verificationdata = '" << verifdata << "', "
		<< "language_id = " << language_id << ", "
		<< "maxlogins = " << maxlogins << ", "
		<< "last_updated = now()::abstime::int4, "
		<< "last_updated_by = '" << escapeSQLChars(last_updated_by) << "', "
		<< "totp_key = '" << escapeSQLChars(totp_key) << "', "
		<< "hostname = '" << escapeSQLChars(hostname) << "' "
		<< queryCondition << id
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::commit> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( !SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
	{
	// TODO: Log to msgchan here.
	elog	<< "sqlUser::commit> Something went wrong: "
		<< SQLDb->ErrorMessage()
		<< endl;

	return false;
 	}

return true;
}

bool sqlUser::commitLastSeen()
{
/*
 *  Build an SQL statement to write the last_seen field to a seperate table.
 */

static const char* queryHeader =    "UPDATE users_lastseen ";
static const char* queryCondition = "WHERE user_id = ";

stringstream queryString;
queryString	<< queryHeader
		<< "SET last_seen = "
		<< last_seen
		<< ", "
		<< "last_hostmask = '"
		<< escapeSQLChars(last_hostmask)
		<< "', "
		<< "last_ip = '"
		<< escapeSQLChars(last_ip)
		<< "', "
		<< "last_updated = now()::abstime::int4 "
		<< queryCondition
		<< id
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::commitLastSeen> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( !SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
	{
	// TODO: Log to msgchan here.
	elog	<< "sqlUser::commit> Something went wrong: "
		<< SQLDb->ErrorMessage()
		<< endl;

	return false;
 	}

return true;
}

bool sqlUser::commitLastSeenWithoutMask()
{
/*
 *  -- Boy, we need a masked Commit() method in these classes. ;)
 */

static const char* queryHeader =    "UPDATE users_lastseen ";
static const char* queryCondition = "WHERE user_id = ";

stringstream queryString;
queryString	<< queryHeader
		<< "SET last_seen = "
		<< last_seen
		<< ", last_updated = now()::abstime::int4 "
		<< queryCondition
		<< id
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::commitLastSeenWithoutMask> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( !SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
	{
	elog	<< "sqlUser::commit> Something went wrong: "
		<< SQLDb->ErrorMessage()
		<< endl;

	return false;
 	}

return true;
}

time_t sqlUser::getLastSeen()
{
stringstream queryString;
queryString	<< "SELECT last_seen"
		<< " FROM users_lastseen WHERE user_id = "
		<< id
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::getLastSeen> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( SQLDb->Exec(queryString, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	/*
	 *  If the user doesn't exist, we won't get any rows back.
	 */

	if(SQLDb->Tuples() < 1)
		{
		return 0;
		}

	last_seen = atoi(SQLDb->GetValue(0, 0));

	return (last_seen);
	}

return (false);

}

const string sqlUser::getLastHostMask()
{
stringstream queryString;
queryString	<< "SELECT last_hostmask"
		<< " FROM users_lastseen WHERE user_id = "
		<< id
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::getLastHostMask> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( SQLDb->Exec(queryString, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	/*
	 *  If the user doesn't exist, we won't get any rows back.
	 */

	if(SQLDb->Tuples() < 1)
		{
		return ("");
		}

	last_hostmask = SQLDb->GetValue(0, 0);

	return (last_hostmask);
	}

return ("");

}

const string sqlUser::getLastIP()
{
stringstream queryString;
queryString	<< "SELECT last_ip"
		<< " FROM users_lastseen WHERE user_id = "
		<< id
		<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::getLastIP> "
		<< queryString.str().c_str()
		<< endl;
#endif

if( SQLDb->Exec(queryString, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	/*
	 *  If the user doesn't exist, we won't get any rows back.
	 */

	if(SQLDb->Tuples() < 1)
		{
		return ("");
		}

	last_ip = SQLDb->GetValue(0, 0);

	return (last_ip);
	}

return ("");

}
void sqlUser::writeEvent(unsigned short eventType, sqlUser* theUser, const string& theMessage)
{
string userExtra = theUser ? theUser->getUserName() : "Not Logged In";

stringstream theLog;
theLog	<< "INSERT INTO userlog (ts, user_id, event, message, "
	<< "last_updated) VALUES "
	<< "("
	<< "now()::abstime::int4"
	<< ", "
	<< id
	<< ", "
	<< eventType
	<< ", "
 	<< "'"
	<< escapeSQLChars(theMessage)
	<< " (By " << userExtra << ")"
	<< "', now()::abstime::int4)"
	<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::writeEvent> "
		<< theLog.str().c_str()
		<< endl;
#endif

// TODO: Is this ok?
SQLDb->Exec(theLog);
//SQLDb->ExecCommandOk(theLog.str().c_str());
}

const string sqlUser::getLastEvent(unsigned short eventType, unsigned int& eventTime)
{
stringstream queryString;

queryString	<< "SELECT message,ts"
			<< " FROM userlog WHERE user_id = "
			<< id
			<< " AND event = "
			<< eventType
			<< " ORDER BY ts DESC LIMIT 1"
			<< ends;

#ifdef LOG_SQL
	elog	<< "sqlUser::getLastEvent> "
			<< queryString.str().c_str()
			<< endl;
#endif

if( SQLDb->Exec(queryString, true ) )
//if( PGRES_TUPLES_OK == status )
	{

	if(SQLDb->Tuples() < 1)
		{
		return("");
		}

	string reason = SQLDb->GetValue(0, 0);
	eventTime = atoi(SQLDb->GetValue(0, 1));

	return (reason);
	}

return ("");
}

void sqlUser::generateRecoveryPassword()
{
	const char validChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	setRecoveryPassword(string());

	for (unsigned short int i = 0 ; i < 8 ; i++)
	{
		int randNo = 1 + (int)(62.0*rand()/(RAND_MAX + 1.0));
		recovery_password += validChars[randNo];
	}
}

bool sqlUser::generateTOTPKey()
{
#ifdef TOTP_AUTH_ENABLED
	//Create a random hex string 168bit long
	char str_key2[20];
	char hex_key[41];
	srand(clock()*745+time(NULL));
	hex_key[0] = 0;
	for(int i=0; i < 20; i++) {
		str_key2[i]=((rand() %95) + 32);
		sprintf(hex_key+(i*2),"%02x",str_key2[i] & 0xFF);
	}
	char* key;
	int res = oath_base32_encode(str_key2, 20, &key, NULL);
	if (res == OATH_OK)
	{
		totp_key = string(key);
		totp_hex_key = string(hex_key);
		free(key);
		return true;
	}
	free(key);
#endif
	return false;
}

OathResult::OATH_RESULT_TYPE sqlUser::validateTOTP(const string& totp)
{
#ifdef TOTP_AUTH_ENABLED
	char* key;
	size_t len;
	int res = oath_base32_decode(getTotpKey().c_str(), getTotpKey().size(), &key, &len);
	if (res != OATH_OK)
	{
		elog << "ERROR while decoding base32 (" << getTotpKey().c_str() << ") " << oath_strerror(res) << "\n";
		free(key);
		return OathResult::ERROR;
	}
	res = oath_totp_validate(key, len, time(NULL), 30, 0, 1, totp.c_str());
	free(key);
	if (res < 0)
	{
		return OathResult::INVALID_TOKEN;
	}
#endif
	return OathResult::OK;
}

sqlUser::~sqlUser()
{
// No heap space allocated
}

bool sqlUser::Insert()
{
/*
 * Build an SQL statement to insert the transient data in 
 * this storage class back into the database.
 */
static const char* queryHeader =  "INSERT INTO users "
	"(user_name,password,question_id,verificationdata,language_id,flags,last_updated_by,last_"
	"updated,email,created_ts,signup_ts,signup_ip) VALUES ('";

stringstream queryString;
queryString	<< queryHeader
		<< escapeSQLChars(user_name)
		<< "'," << "'"
		<< password << "',"
		<< verifNr << ",'"
		<< verifdata << "',"
		<< 1 << ","
		<< 0 << ",'"
		<< escapeSQLChars(last_updated_by)
		<< "',"
		<< "now()::abstime::int4, '"
		<< escapeSQLChars(email)
		<< "', "
		<< created_ts	<< ", "
		<< instantiated_ts	<< ", '"
		<< signup_ip << "')"
		<< ends;

#ifdef LOG_SQL
        elog    << "sqlUser::insert> "
                << queryString.str()
                << endl;
#endif

if( !SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
        {
        // TODO: Log to msgchan here.
        elog    << "sqlUser::insert> Something went wrong: "
                << SQLDb->ErrorMessage()
                << endl;

        return false;
        }

return true;
}

} // namespace gnuworld.
