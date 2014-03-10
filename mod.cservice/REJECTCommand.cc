/**
 * REJECTCommand.cc
 *
 * 07/05/2012 - Gergo Focze <gergo_f@yahoo.com>
 * Initial Version.
 *
 * Channel reject command from IRC 
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

const char REJECTCommand_cc_rcsId[] = "$Id: REJECTCommand.cc,v 1.16 2012/05/07 19:23:36 Seven Exp $" ;

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

using namespace gnuworld;

bool REJECTCommand::Exec( iClient* theClient, const string& Message )
{
	bot->incStat("COMMANDS.REJECT");

	string decision;
	string mngrUserName;
	unsigned int chanId;
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

	if (level < level::reject)
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::insuf_access,
				string("You have insufficient access to perform that command.")));
		return false;
	}

	decision = st.assemble(2);
	if (decision.size() > 300)
	{
		bot->Notice(theClient,"Decision cannot exceed 300 charcters.");
		return false;
	}
	decision = "by CService Admin<br>" + decision;
	stringstream theQuery;
	unsigned int lastdays = (unsigned int)bot->currentTime() - ((unsigned int)bot->daySeconds*5);
	/********** Channel ID query ***********/
	theQuery	<< "SELECT id,status FROM channels,pending WHERE "
			<< "channels.registered_ts = 0 AND lower(channels.name) = '"
			<< escapeSQLChars(string_lower(st[1]))
			<< "'"
			<< " AND channels.id = pending.channel_id AND (pending.decision_ts=0 OR (pending.decision_ts>0 AND pending.decision_ts>="
			<< lastdays
			<< "))"
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on REJECT.pending.status query");
	#ifdef LOG_SQL
		elog << "REJECT.pending.status query> SQL Error: " 
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
	chanId = atoi(bot->SQLDb->GetValue(0,0));
	unsigned short status = atoi(bot->SQLDb->GetValue(0,1));
	if (status == 9) 
	{	bot->Notice(theClient,"Channel %s is already rejected.",st[1].c_str());
		return false;
	}
	if (status == 3) //Accepted, so channel was previously Rejected
	{
		bot->Notice(theClient,"Channel %s is not in my applications list",st[1].c_str());
		return false;
	}
	if ((status > 3) and (status < 8)) return false;	
	//if ((status == 0) or (status == 1) or (status == 2) or (status == 8))
	/********** Update pending status ***********/
	theQuery.str("");
	theQuery	<< "UPDATE pending SET status=9,last_updated=now()::abstime::int4,decision_ts=now()::abstime::int4,reviewed='Y',decision='"
			<< decision
			<< "',reviewed_by_id="
			<< theUser->getID()
			<<" WHERE channel_id="
			<< chanId
			//<< " AND (status='0' OR status='1' OR status='2' OR status='8')" //AND created_ts='
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on REJECT.UpdatePending query");
	#ifdef LOG_SQL
		elog << "REJECT.UpdatePending query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	}
	/********** Manager ID query ***********/
	theQuery.str("");
	theQuery	<< "SELECT manager_id FROM pending WHERE channel_id="
			<< chanId
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on REJECT.Manager_ID query");
	#ifdef LOG_SQL
		elog << "REJECT.Manager_ID query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	} 
	if (bot->SQLDb->Tuples() == 0) 
	{ 
		bot->logDebugMessage("Result NOT Found REJECT.Manager_ID query");
		return false;
	}
	unsigned int mngrId = atoi(bot->SQLDb->GetValue(0,0));	
	/********** Update users LastSeen last_updated ***********/
	theQuery.str("");
	theQuery	<< "UPDATE users_lastseen SET last_updated=now()::abstime::int4,last_seen=now()::abstime::int4 WHERE user_id="
			<< mngrId
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on REJECT.UsersLastseen last_updated");
	#ifdef LOG_SQL
		elog << "REJECT.UsersLastseen last_updated> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	} 
	/********** Manager username query ***********/
	theQuery.str("");
	theQuery	<< "SELECT user_name FROM users WHERE id="
			<< mngrId
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on REJECT.Manager UserName Query");
	#ifdef LOG_SQL
		elog << "REJECT.Manager UserName Query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	} 
	mngrUserName = bot->SQLDb->GetValue(0,0); 
	bot->logAdminMessage("%s (%s) has rejected %s from %s", theClient->getNickName().c_str(),
			theUser->getUserName().c_str(), st[1].c_str(), mngrUserName.c_str());
	bot->Notice(theClient,"Rejected channel %s",st[1].c_str());

return true;
}

} // namespace gnuworld.
