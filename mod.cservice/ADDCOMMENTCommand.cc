/**
 * ADDCOMMENTCommand.cc
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
 * $Id: ADDCOMMENTCommand.cc,v 1.5 2012/01/01 19:12:22 Seven Exp $
 */

#include	<string>

#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"cservice.h"
#include	"levels.h"
#include	"responses.h"

const char ADDCOMMENTCommand_cc_rcsId[] = "$Id: ADDCOMMENTCommand.cc,v 1.5 2012/01/01 19:12:22 Seven Exp $" ;

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

bool ADDCOMMENTCommand::Exec( iClient* theClient, const string& Message )
{
bot->incStat("COMMANDS.ADDCOMMENT");

StringTokenizer st( Message ) ;
if( st.size() < 2 )
	{
	Usage(theClient);
	return true;
	}

sqlUser* theUser = bot->isAuthed(theClient, false);
if (!theUser)
	{
	return false;
	}

int admLevel = bot->getAdminAccessLevel(theUser);
if (admLevel < level::addcommentcmd) 
{
	if (admLevel > 0) 
		bot->Notice(theClient,bot->getResponse(theUser,language::insuf_access).c_str());
	return false;
}

string comment = st.assemble(2);

/* *** Performing Admin Comment for a channel under registration *** */
if ( (st[1][0] == '#') )
{
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

	if ((comment.size() < 2) || (comment.size() > 450))
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
	bot->logDebugMessage("Error on ADDCOMMENT.pending.status query");
	#ifdef LOG_SQL
		elog << "ADDCOMMENT.pending.status query> SQL Error: " 
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
	if (((status >= 3) && (status < 8)) || (status == 9))
        {
                bot->Notice(theClient,"Channel %s is not in my applications list",st[1].c_str());
                return false;
        }
        //if ((status > 3) and (status < 8)) return false;

	/* *** Insert the objection *** */
	theQuery.str("");
	theQuery	<< "INSERT INTO objections (channel_id,user_id,comment,created_ts,admin_only) VALUES ("
			<< chanId
			<< ", "
			<< theUser->getID()
			<< ", '"
			<< comment
			<< "', now()::abstime::int4, 'Y')"
			<< ends;
	if( !bot->SQLDb->Exec(theQuery, true ) ) 
	{ 
	bot->logDebugMessage("Error on ADDCOMMENT.InsertObjection query");
	#ifdef LOG_SQL
		elog << "ADDCOMMENT.InsertObjection query> SQL Error: " 
		     << bot->SQLDb->ErrorMessage() 
		     << endl ; 
	#endif
		bot->Notice(theClient,"An unknown error occured while posting comment.");
		return false; 
	}
	bot->Notice(theClient,"Comment posted successfully.");
	return true;
}

/* *** Performing User Comment *** */

/*
 *  Check the person we're trying to add is actually registered.
 */

sqlUser* targetUser = bot->getUserRecord(st[1]);
if (!targetUser)
	{
	bot->Notice(theClient,
		bot->getResponse(theUser,
			language::not_registered).c_str(),
		st[1].c_str()
	);
	return false;
	}

/*
 * Add the comment.
 */

targetUser->writeEvent(sqlUser::EV_COMMENT, theUser, comment);

if (st.size() == 2)
{
	bot->Notice(theClient, "Comment Blanked for %s.", targetUser->getUserName().c_str());
} else
{
	bot->Notice(theClient, "Done. Added comment to %s", targetUser->getUserName().c_str());
}

return true ;
}

} // namespace gnuworld.
