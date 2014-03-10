/**
 * OBJECTCommand.cc
 *
 * 01/07/2012 - Gergo Focze <gergo_f@yahoo.com>
 * Initial Version.
 *
 * Make an objection for a channel under registration from IRC 
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
#include	"levels.h"
#include	"responses.h"

const char OBJECTCommand_cc_rcsId[] = "$Id: OBJECTCommand.cc,v 1.0 2012/07/01 12:11:10 Seven Exp $" ;

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

using namespace gnuworld;

bool OBJECTCommand::Exec( iClient* theClient, const string& Message )
{
	bot->incStat("COMMANDS.OBJECT");

        StringTokenizer st( Message ) ;
        if ( st.size() < 3 )
        {
                Usage(theClient);
                return true;
        }

	sqlUser* theUser = bot->isAuthed(theClient, true);
        /*
         *  Fetch the sqlUser record attached to this client. If there isn't one,
         *  they aren't logged in - tell them they should be.
         */
        if (!theUser) return false;

 	/*
	 *  First, check the channel is registered/real.
	 */

	if ( (st[1][0] != '#') )
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::inval_chan_name,
				string("Invalid channel name.")));
		return false;
	}
	sqlChannel* theChan = bot->getChannelRecord(st[1]);
	if (theChan)
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::chan_already_reg,
				string("%s is already registered with me.")).c_str(),
			st[1].c_str());
		return false;
	} 

	string objection = st.assemble(2);
	if ((objection.size() < 2) || (objection.size() > 450))
	{
		bot->Notice(theClient, bot->getResponse(theUser,language::reason_must).c_str(),2,450);
		return false;
	}
	stringstream theQuery;
		/********** Channel ID query ***********/
	theQuery	<< "SELECT id,status FROM channels,pending WHERE "
			<< "channels.registered_ts = 0 AND lower(channels.name) = '"
			<< escapeSQLChars(string_lower(st[1]))
			<< "'"
			<< " AND channels.id = pending.channel_id AND pending.decision_ts=0"
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on OBJECT.pending.status query");
	#ifdef LOG_SQL
		elog << "OBJECT.pending.status query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	} 
	if (bot->SQLDb->Tuples() == 0) 
	{
		bot->Notice(theClient,"Channel %s is not in my applications list",st[1].c_str());
		return false;
	}
	unsigned int chanId = atoi(bot->SQLDb->GetValue(0,0));
	unsigned short status = atoi(bot->SQLDb->GetValue(0,1));
	if ((status >= 3) && (status <= 9))
        {
                bot->Notice(theClient,"Channel %s is not in my applications list",st[1].c_str());
                return false;
        }
	/*** Check if already has a comment ***/
	theQuery.str("");
	theQuery	<< "SELECT * FROM objections WHERE user_id="
			<< theUser->getID()
			<< " and channel_id = "
			<< chanId
			<< " AND admin_only='N'"
			<< ends;
	if (!bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on OBJECT.AlreadyComment query");
	#ifdef LOG_SQL
		elog << "OBJECT.AlreadyComment query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	}
	if (bot->SQLDb->Tuples() > 0) 
	{
		bot->Notice(theClient,"You have already posted an objection for that channel.");
		return false;
	}
	/* *** Insert the objection *** */
	theQuery.str("");
	theQuery	<< "INSERT INTO objections (channel_id,user_id,comment,created_ts,admin_only) VALUES ("
			<< chanId
			<< ", "
			<< theUser->getID()
			<< ", '"
			<< objection
			<< "', now()::abstime::int4, 'N')"
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on OBJECT.InsertObjection query");
	#ifdef LOG_SQL
		elog << "OBJECT.InsertObjection query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		bot->Notice(theClient,"An unknown error occured while posting objection.");
		return false; 
	}
	bot->Notice(theClient,"Objection posted successfully.");
	return true;
}

} // namespace gnuworld.
