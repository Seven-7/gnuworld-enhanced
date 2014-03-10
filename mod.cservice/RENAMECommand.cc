/**
 * RENAMECommand.cc
 *
 * 10/06/2012 - Gergo Focze <gergo_f@yahoo.com>
 * Initial Version.
 *
 * Admin user rename command from IRC 
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
 */

#include	<string>
#include	<sstream>
#include	<utility>

#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"cservice.h"
#include	"dbHandle.h"
#include	"Network.h"
#include	"levels.h"
#include	"responses.h"

const char RENAMECommand_cc_rcsId[] = "$Id: RENAMECommand.cc,v 1.0 2012/06/10 09:25:13 Seven Exp $" ;

namespace gnuworld
{
using std::pair ;
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

using namespace gnuworld;

bool RENAMECommand::Exec( iClient* theClient, const string& Message )
{
	bot->incStat("COMMANDS.RENAME");

	const char validChars[] 
		= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	int level = 0;

	sqlUser* theUser = bot->isAuthed(theClient, true);
	if (theUser) level = bot->getAdminAccessLevel(theUser);
	if (level == 0) return true; //don't show anything to normal users ;)
	StringTokenizer st( Message ) ;
	if ( st.size() < 3 )
	{
		Usage(theClient);
		return true;
	}
	/*
	 *  Fetch the sqlUser record attached to this client. If there isn't one,
	 *  they aren't logged in - tell them they should be.
	 */

	if (!theUser) return false;

	if (level < level::rename)
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::insuf_access,
				string("You have insufficient access to perform that command.")));
		return false;
	}

	sqlUser* oldUser = bot->getUserRecord(st[1].c_str());
	if (!oldUser)
	{
		bot->Notice(theClient, bot->getResponse(theUser,language::not_registered).c_str(),st[1].c_str());
		return false;
	}
	
	string oldUserName = oldUser->getUserName();
	string newUserName = st[2];

	/*** Check the new usernames validity ***/
	bool badName = false;

	for( string::const_iterator ptr = newUserName.begin() ;
		ptr != newUserName.end() ; ++ptr )
	{
		/*
		 * 62 entries in the table. 26 + 26 + 10 digits.
		 */

		bool found = false;
		for (int f = 0; f < 62; f++)
			{
			if(*ptr == validChars[f])
				{
				found = true;
				}
			}
		if (!found)
		{
			badName = true;
		}
	}
	if (badName)
	{
		bot->Notice(theClient, "The new useraname must be made of letters (A-Z, a-z) and numbers (0-9).");
		return false;
	}
	
	newUserName = escapeSQLChars(newUserName);

	/*** Check if the new username is already in use ***/
	sqlUser* newUser = bot->getUserRecord(newUserName);
	if (newUser)
	{ 
		bot->Notice(theClient, "Username %s already exists!",newUserName.c_str());
		return false;
	}

	/* Also Update(rename) the username in the database */
	stringstream theQuery;
	theQuery.str("");
	theQuery	<< "UPDATE users SET user_name = '"
			<< newUserName
			<< "'"
			<< "WHERE lower(user_name) = '"
			<< string_lower(oldUserName)
			<< "'"
			<< ends;

	if (!bot->SQLDb->Exec(theQuery, true))
	{
	#ifdef LOG_SQL
		//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
		elog << "RENAME UPDATE SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		bot->logDebugMessage("RENAME UPDATE SQL Error: ",bot->SQLDb->ErrorMessage().c_str());
		return false;
	} 
	bot->updateUserCacheUserName(oldUser,newUserName);
	bot->Notice(theClient,"Successfully renamed username %s to %s", 
			oldUserName.c_str(), newUserName.c_str());

   return true;
}

} // namespace gnuworld.
