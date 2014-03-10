/**
 * ACCEPTCommand.cc
 *
 * 27/04/2012 - Gergo Focze <gergo_f@yahoo.com>
 * Initial Version.
 *
 * Channel accept command from IRC 
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

const char ACCEPTCommand_cc_rcsId[] = "$Id: ACCEPTCommand.cc,v 1.0 2012/04/27 20:48:00 Seven Exp $" ;

namespace gnuworld
{
using std::pair ;
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

using namespace gnuworld;

bool ACCEPTCommand::Exec( iClient* theClient, const string& Message )
{
	bot->incStat("COMMANDS.ACCEPT");
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

 	/* First, check the channel is registered/real. */

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
	/*  Check the user has sufficient access for this command.. */
	if (level < level::accept)
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
	unsigned int lastdays = (unsigned int)bot->currentTime() - ((unsigned int)bot->daySeconds*5);
	stringstream theQuery;
	/********** Channel ID query ***********/
	theQuery	<< "SELECT id,status FROM channels,pending WHERE "
			<< "channels.registered_ts = 0 AND lower(channels.name) = '"
			<< escapeSQLChars(string_lower(st[1]))
			<< "'"
			<< " AND channels.id = pending.channel_id AND (pending.decision_ts=0 OR (pending.decision_ts>0 AND pending.decision_ts>="
			<< lastdays
			<< "))"
			<< ends;
	if (!bot->SQLDb->Exec(theQuery, true))
	{	bot->logDebugMessage("Error on ACCEPT: channel is not UnClaimed");
	#ifdef LOG_SQL
		//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
		elog << "ACCEPT.pending.status query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false;
	} else if (bot->SQLDb->Tuples() == 0) 
	{ 
		bot->Notice(theClient,"Channel %s is not in my applications list",st[1].c_str());
		return false;
	}
	chanId = atoi(bot->SQLDb->GetValue(0,0));
	unsigned short status = atoi(bot->SQLDb->GetValue(0,1));
	if (status == 3) //if we're here, the channel was previously Rejected
	{
		bot->Notice(theClient,"Channel %s is not in my applications list",st[1].c_str());
		return false;
	}
	/* Quick query to set registered_ts back for this chan. */
	stringstream reclaimQuery;
	reclaimQuery	<< "UPDATE channels SET registered_ts = now()::abstime::int4,"
			<< " last_updated = now()::abstime::int4, "
			<< " flags = 0, description = '', url = '', comment = '', keywords = '', channel_mode = '+tn' "
			//<< " WHERE lower(name) = '"
			//<< escapeSQLChars(string_lower(st[1]))
			//<< "'"
			<< " WHERE id = "
			<< chanId 
			<< ends;

	if (!bot->SQLDb->Exec(reclaimQuery))
	{ 
		bot->logDebugMessage("Error on ACCEPT.UpdateReclaim query");
	#ifdef LOG_SQL
	elog << "sqlQuery> " << reclaimQuery.str().c_str() << endl;
	#endif
		return false; 
	} 
	/********** Update pending status ***********/
	theQuery.str("");
	theQuery	<< "UPDATE pending SET status=3,last_updated=now()::abstime::int4,decision_ts=now()::abstime::int4,reviewed='Y',decision='"
			<< decision
			<< "',reviewed_by_id="
			<< theUser->getID()
			<<" WHERE channel_id="
			<< chanId
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on ACCEPT.UpdatePending query");
	#ifdef LOG_SQL
		elog << "ACCEPT.UpdatePending query> SQL Error: " 
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
	bot->logDebugMessage("Error on ACCEPT.Manager_ID query");
	#ifdef LOG_SQL
		elog << "ACCEPT.Manager_ID query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	} 
	if (bot->SQLDb->Tuples() == 0) 
	{ 
		bot->logDebugMessage("Result NOT Found ACCEPT.Manager_ID query");
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
	bot->logDebugMessage("Error on ACCEPT.UsersLastseen last_updated");
	#ifdef LOG_SQL
		elog << "ACCEPT.UsersLastseen last_updated> SQL Error: " 
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
	bot->logDebugMessage("Error on ACCEPT.Manager UserName Query");
	#ifdef LOG_SQL
		elog << "ACCEPT.Manager UserName Query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		return false; 
	} 
	mngrUserName = bot->SQLDb->GetValue(0,0);
	//sqlUser* tmpUser = bot->getUserRecord(mngrUserName);	
	/*
	 * Create the new channel and insert it into the cache.
	 * If the channel exists on IRC, grab the creation timestamp
	 * and use this as the channel_ts in the Db.
	 */
	unsigned int channel_ts = 0;
	Channel* tmpChan = Network->findChannel(st[1]);
	channel_ts = tmpChan ? tmpChan->getCreationTime() : ::time(NULL);

	sqlChannel* newChan = new (std::nothrow) sqlChannel(bot->SQLDb);
	newChan->setName(st[1]);
	newChan->setID(chanId);
	newChan->setChannelTS(channel_ts);
	newChan->setRegisteredTS(bot->currentTime());
	newChan->setChannelMode("+tn");
	newChan->setLastUsed(bot->currentTime());

	bot->sqlChannelCache.insert(cservice::sqlChannelHashType::value_type(newChan->getName(), newChan));
	bot->sqlChannelIDCache.insert(cservice::sqlChannelIDHashType::value_type(newChan->getID(), newChan));

	/*
	 *  Finally, commit a channellog entry.
	 */

	bot->writeChannelLog(newChan, theClient, sqlChannel::EV_REGISTER, " (*** REGPROC ***) to " + mngrUserName);

	/*
	 * Create the new manager.
	 */

	sqlLevel* newManager = new (std::nothrow) sqlLevel(bot->SQLDb);
	newManager->setChannelId(newChan->getID());
	newManager->setUserId(mngrId);
	newManager->setAccess(500);
	newManager->setAdded(bot->currentTime());
	newManager->setAddedBy("*** REGPROC ***");
	newManager->setLastModif(bot->currentTime());
	newManager->setLastModifBy("*** REGPROC ***");

	if (!newManager->insertRecord())
		{
			bot->Notice(theClient, "Couldn't automatically add the level 500 Manager, check it doesn't already exist.");
			delete(newManager);
			return (false);
		}

	/*
	 * Insert this new 500 into the level cache.
	 */

	pair<int, int> thePair( newManager->getUserId(), newManager->getChannelId());
	bot->sqlLevelCache.insert(cservice::sqlLevelHashType::value_type(thePair, newManager));

	bot->logAdminMessage("%s (%s) has accepted %s to %s", theClient->getNickName().c_str(),
			theUser->getUserName().c_str(), st[1].c_str(), mngrUserName.c_str());
	bot->Notice(theClient,"Accepted channel %s",st[1].c_str());

	/* set channel mode R - tmpChan is created further above */
	stringstream tmpTS;
	tmpTS << channel_ts;
	string channelTS = tmpTS.str();

	if (tmpChan)
		bot->getUplink()->Mode(NULL, tmpChan, string("+R"), channelTS );
	bot->getUplink()->RegisterChannelEvent(st[1],bot);
   return true;
}

} // namespace gnuworld.
