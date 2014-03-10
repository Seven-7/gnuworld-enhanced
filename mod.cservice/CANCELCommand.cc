/**
 * CANCELCommand.cc
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
 * $Id: CANCELCommand.cc,v 1.0 2013/01/22 11:11:20 Seven Exp $
 */

#include	<string>
#include	<sstream>
#include	<utility>

#include	"cservice.h"
//#include	"sqlUser.h"
#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"responses.h"

const char CANCELCommand_cc_rcsId[] = "$Id: CANCELCommand.cc,v 1.0 2013/01/22 11:11:20 Seven Exp $" ;

namespace gnuworld
{

using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

bool CANCELCommand::Exec( iClient* theClient, const string& Message )
{
bot->incStat("COMMANDS.CANCEL");

StringTokenizer st( Message ) ;
if( st.size() < 3 )
	{
	Usage(theClient);
	return true;
	}

sqlUser* theUser = bot->isAuthed(theClient, true);
if (!theUser)
	{
	return false;
 	}

unsigned int chanId = bot->getPendingChanId(st[1]);

if (!chanId)
{
	bot->Notice(theClient, "Sorry, channel %s doesn't appear to be under registration.",st[1].c_str());
	bot->Notice(theClient, "Check you spelt the name correctly and try again.");
	return true;
}

unsigned int mngrId = 0;

// Check if user has a pending channel applicaion
stringstream queryString;
queryString << "SELECT manager_id FROM pending WHERE status <> 3 AND status <> 9 AND status <> 4 AND channel_id = "
			<< chanId
			<< ends;

#ifdef LOG_SQL
	elog	<< "CANCELCommand::manageridQuery> "
			<< queryString.str().c_str()
			<< endl;
#endif

if( !bot->SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
{
	elog	<< "CANCELCommand::manageridQuery> Something went wrong: "
			<< bot->SQLDb->ErrorMessage()
			<< endl;

	return false ;
}

if (bot->SQLDb->Tuples() == 0)
{
	bot->Notice(theClient, "You don't have a pending channel registration to you or you mistyped the channel name.");
	bot->Notice(theClient, "Try again, or if you want to register a new channel type /msg %s REGISTER <#channel>",bot->getNickName().c_str());
	return true;
}

mngrId = atoi(bot->SQLDb->GetValue(0,0));

if ((mngrId != theUser->getID()) && (mngrId != 0))
{
	bot->Notice(theClient, "Sorry, the channel is under registration by someone else.");
	return true;
}

//Not found any manager
if (mngrId == 0) return false;

if (string_upper(st[2]) != "YES")
{
	bot->Notice(theClient, "Your channel application of %s has \002not\002 been cancelled.",st[1].c_str());
	return true;
}

queryString.str("");
queryString	<< "UPDATE pending SET status=4,last_updated=now()::abstime::int4,decision='Cancelled by applicant',decision_ts=now()::abstime::int4 "
			<< "WHERE channel_id = "
			<< chanId
			<< endl;

if( !bot->SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
{
	elog	<< "CANCELCommand::sqlCancelQuery> Something went wrong: "
			<< bot->SQLDb->ErrorMessage()
			<< endl;

	bot->logDebugMessage("FAILED to Cancel channel application %s",st[1].c_str());
	return false ;
}

string sup_list = "", tmpstr = "";

queryString.str("");
queryString	<< "SELECT supporters.user_id,users.user_name FROM supporters,pending,users "
			<< "WHERE supporters.user_id=users.id AND pending.channel_id=supporters.channel_id AND pending.channel_id="
			<< chanId
			<< endl;

if( !bot->SQLDb->Exec(queryString ) )
//if( PGRES_COMMAND_OK != status )
{
	elog	<< "CANCELCommand::sqlSupportersQuery> Something went wrong: "
			<< bot->SQLDb->ErrorMessage()
			<< endl;

	return false ;
}

for (unsigned int i=0; i < bot->SQLDb->Tuples(); i++)
{
	if (sup_list.empty())
		sup_list += bot->SQLDb->GetValue(i,0) + " (" + bot->SQLDb->GetValue(i,1) + ")";
	else
		sup_list += " " + bot->SQLDb->GetValue(i,0) + " (" + bot->SQLDb->GetValue(i,1) + ")";
}

//If it was an incomplete channel registration there were no supporters
if (sup_list.empty()) sup_list = "No supporters";

string applicant = theUser->getUserName() + " ("+ theUser->getEmail() + ")"; //bot->TokenStringsParams("%s (%s)",theUser->getUserName().c_str(),theUser->getEmail().c_str());

//We have sqlChannel only if the application is not an sqlIncompleteChannel
sqlChannel* theChan = bot->getChannelRecord(st[1]);
if (theChan)
{
	bot->writeChannelLog(theChan, theClient, sqlChannel::EV_WITHDRAW,
			"Application Cancelled by applicant - Applicant was: " + applicant +", Supporters were: " + sup_list);
}

/**
 * If we HAVE an incomplete channel, than probably we weren't an sqlChannel
 * so anyway we destroy & cleanup
 */
cservice::incompleteChanRegsType::iterator theApp = bot->incompleteChanRegs.find(theUser->getID());
if (theApp != bot->incompleteChanRegs.end())
{
	delete(theApp->second);
	bot->incompleteChanRegs.erase(theUser->getID());
}
	bot->Notice(theClient, "Successfully cancelled you channel application of %s", st[1].c_str());
	return true ;
}

} // namespace gnuworld.
