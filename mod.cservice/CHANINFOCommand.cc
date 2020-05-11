/**
 * CHANINFOCommand.cc
 *
 * 29/12/2000 - Greg Sikorski <gte@atomicrevs.demon.co.uk>
 * Initial Template.
 *
 * 30/12/2000 - David Henriksen <david@itwebnet.dk>
 * Started and finished the command. Showing all owners by a
 * SQL Query which returns all the level 500s of the channel.
 *
 * Caveats: Need to determine if the query is aimed at a #
 * or a user. :)
 *
 * Command is aliased "INFO".
 *
 * 12/05/2012 - Gergo Focze <gergo_f@yahoo.com>
 * Implemented channel application status information.
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
 * $Id: CHANINFOCommand.cc,v 1.7 2012/05/12 09:36:42 Seven Exp $
 */

#include	<string>
#include	<sstream>
#include	<iostream>

#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"Network.h"
#include	"cservice.h"
#include	"levels.h"
#include	"responses.h"
#include	"dbHandle.h"
#include	"cservice_config.h"

const char CHANINFOCommand_cc_rcsId[] = "$Id: CHANINFOCommand.cc,v 1.7 2012/05/12 09:36:42 Seven Exp $" ;

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

static const char* queryHeader = "SELECT channels.name,users.user_name,levels.access,users_lastseen.last_seen FROM levels,channels,users,users_lastseen ";
static const char* queryString = "WHERE levels.channel_id=channels.id AND users.id=users_lastseen.user_id AND levels.access = 500 AND levels.user_id = users.id ";

bool CHANINFOCommand::Exec( iClient* theClient, const string& Message )
{
bot->incStat("COMMANDS.INFO");

StringTokenizer st( Message ) ;
if( st.size() < 2 )
	{
	Usage(theClient);
	return true;
	}

sqlUser* tmpUser = bot->isAuthed(theClient, true);
if (!tmpUser)
{
	return false;
}

int adminAccess = 0;
if (tmpUser) adminAccess = bot->getAdminAccessLevel(tmpUser);

/*
 *  Are we checking info about a user or a channel?
 */

// Did we find a '#' ?
if( string::npos == st[ 1 ].find_first_of( '#' ) )
	{
	// Nope, look by user then.
	sqlUser* theUser = bot->getUserRecord(st[1]);

	if (!theUser)
		{
		bot->Notice(theClient,
			bot->getResponse(tmpUser,
				language::not_registered,
				string("The user %s doesn't appear to be registered.")).c_str(),
			st[1].c_str());
		return true;
		}

	/* fetch admin access level (manually to avoid various checks) for use for IP hiding ONLY */
	unsigned short tmpadminLevel;
	sqlChannel* adminChan = bot->getChannelRecord("*");
	if (!adminChan)
	{
		/* cant find admin channel for some reason, assume no access of course */
		tmpadminLevel = 0;
	} else {
		/* found admin channel, try to get the level record */
		sqlLevel* adminLev = bot->getLevelRecord(theUser, adminChan);
		if (!adminLev)
		{
			/* no level record, assume no access */
			tmpadminLevel = 0;
		} else {
			/* found it, set it */
			tmpadminLevel = adminLev->getAccess();
		}
	}

	/* get the language STRING */
	stringstream s;
	int langId = theUser->getLanguageId();
	s	<< langId
		<< ends ;
	string langString = s.str();

	for (cservice::languageTableType::iterator ptr = bot->languageTable.begin();
		ptr != bot->languageTable.end(); ptr++)
	{
		if (ptr->second.first == langId)
		{
			langString = ptr->first;
			break;
		}
	}

	/* build up a flag string */
	string flagsSet;

	if (theUser->getFlag(sqlUser::F_GLOBAL_SUSPEND)) 
		flagsSet += "SUSPEND ";
	if (theUser->getFlag(sqlUser::F_INVIS))
		flagsSet += "INVISIBLE ";
	if (theUser->getFlag(sqlUser::F_AUTONICK))
		flagsSet += "AUTONICK ";
	if (theUser->getFlag(sqlUser::F_NOADDUSER))
		flagsSet += "NOADDUSER ";
	/* flags only visible to *1+ users */
	if (adminAccess)
	{
		if ((tmpUser->getFlag(sqlUser::F_POWER)) && (theUser->getFlag(sqlUser::F_POWER)))
			flagsSet += "POWER ";
		if (theUser->getFlag(sqlUser::F_FRAUD))
			flagsSet += "FRAUD ";
		if (theUser->getFlag(sqlUser::F_NOPURGE))
			flagsSet += "NOPURGE ";
		if (theUser->getFlag(sqlUser::F_ALUMNI))
			flagsSet += "ALUMNI ";
		if (theUser->getFlag(sqlUser::F_NOADMIN))
			flagsSet += "DISABLEAUTH ";
		if (theUser->getFlag(sqlUser::F_TOTP_ENABLED))
                        flagsSet += "TOTP ";
	}
	else if (tmpUser == theUser)
	{
		if (theUser->getFlag(sqlUser::F_NOPURGE))
			flagsSet += "NOPURGE ";
		if (theUser->getFlag(sqlUser::F_TOTP_ENABLED))
			flagsSet += "TOTP ";
		if (theUser->getFlag(sqlUser::F_ALUMNI))
			flagsSet += "ALUMNI ";
	}

	/* flags with variables */
	if (langString.size() > 0)
		flagsSet += "LANG=" + langString + " ";
	/* flags with variables for admins (or self-viewing) only */
	if (adminAccess || (tmpUser == theUser))
	{
		int maxLogins = theUser->getMaxLogins();
		stringstream ss;
		ss	<< maxLogins;
//			<< ends ;
		if (maxLogins > 1)
			flagsSet += "MAXLOGINS=" + ss.str() + " ";
	
		stringstream autoInviteQuery;

		autoInviteQuery	<< "SELECT channel_id from levels"
				<< " where user_id = " << theUser->getID()
				<< " and flags & "
				<< sqlLevel::F_AUTOINVITE
				<< " > 0"
				<< " and deleted = 0"
				<< ends;

		#ifdef LOG_SQL
			elog	<< "CHANINFO::sqlQuery> "
				<< autoInviteQuery.str().c_str()
				<< endl;
		#endif


		if( !bot->SQLDb->Exec(autoInviteQuery, true ) )
//	if( PGRES_TUPLES_OK != status )
			{

			elog	<< "CHANINFO> SQL Error: "
				<< bot->SQLDb->ErrorMessage()
				<< endl ;
			return  false;
			}
		if(bot->SQLDb->Tuples() > 0)
		{
			flagsSet+= "INVITE ";
		}

	}
	/* set 'NONE' if no flags */
	if (flagsSet.size() == 0)
		flagsSet = "NONE ";

	/* Keep details private. */
	if (theUser->getFlag(sqlUser::F_INVIS))
		{

		/* If they don't have * access, deny. */
		if( !((tmpUser) && bot->getAdminAccessLevel(tmpUser)) && (tmpUser != theUser))
			{
			static char resBuf[1024];
			sprintf(resBuf, bot->getResponse(tmpUser,
						language::info_about,
						string("Information about: %s (%i)")).c_str(),
						theUser->getUserName().c_str(), 0);
			bot->Notice(theClient,"%s - %s",resBuf,bot->getResponse(tmpUser,
					language::no_peeking,
					string("Unable to view user details (Invisible)")).c_str());

			/* Show flags even when invisible */
			bot->Notice(theClient,
				bot->getResponse(tmpUser, language::status_flags,
				string("Flags set: %s")).c_str(),
				flagsSet.c_str());

			/*
			 * Show the channels this guy owns to opers.
			 */

			if (theClient->isOper())
			{
				bot->outputChannelAccesses(theClient, theUser, tmpUser, 500);
			}

			return false;
			}
		}

	bot->Notice(theClient,
		bot->getResponse(tmpUser,
			language::info_about,
			string("Information about: %s (%i)")).c_str(),
		theUser->getUserName().c_str(), theUser->getID());

        if (theUser->getID() == 1)
        {
                bot->Notice(theClient," - The one that was, the one that is, the one that will be.");
        }

	if (theUser->getID() == 42)
	{
		bot->Notice(theClient," - What do you mean you want to demolish the Undernet to make way for a new"
		" hyperspace expressway?");
	}

#ifdef USE_USERS_URL
	if (!theUser->getUrl().empty())
		bot->Notice(theClient, theUser->getUrl());
#endif
	/*
	 * Loop over all people we might be logged in as.
	 */

	bot->Notice(theClient, "Currently logged on via:");
	int aCount = 0;

	for( sqlUser::networkClientListType::iterator ptr = theUser->networkClientList.begin() ;
		ptr != theUser->networkClientList.end() ; ++ptr )
		{
			if ((tmpadminLevel > 0 || theUser->getFlag(sqlUser::F_OPER)) && adminAccess < 800)
			{
				bot->Notice(theClient, "  " + (*ptr)->getNickName() + "!" +
					(*ptr)->getUserName() + "@" +
					(*ptr)->getAccount() + (*ptr)->getHiddenHostSuffix());
			} else {
				bot->Notice(theClient, "  " + (*ptr)->getNickUserHost());
			}
			aCount++;
		}

	if (!aCount) bot->Notice(theClient, "  OFFLINE");

	bot->Notice(theClient,
		bot->getResponse(tmpUser, language::status_flags,
			string("Flags set: %s")).c_str(),
			flagsSet.c_str());

	bot->Notice(theClient,
		bot->getResponse(tmpUser,
			language::last_seen,
			string("Last Seen: %s")).c_str(),
		prettyDuration(theUser->getLastSeen()).c_str());

	if(adminAccess)
	{
	/*
	 * Show admins some more details about the user.
	 */

	stringstream queryString;

	queryString	<< "SELECT message,ts"
                        << " FROM userlog WHERE user_id = "
                        << theUser->getID()
                        << " AND event = "
                        << sqlUser::EV_COMMENT
                        << " ORDER BY ts DESC LIMIT 3"
                        << ends;

	if( bot->SQLDb->Exec(queryString, true ) )
        {
		for (unsigned int i = 0 ; i < bot->SQLDb->Tuples(); i++) {
		        string userComments = bot->SQLDb->GetValue(i, 0);
		        unsigned int theTime = atoi(bot->SQLDb->GetValue(i, 1));
			bot->Notice(theClient,"\002Admin Comment\002: %s ago (%s)", prettyDuration(theTime).c_str(),
				userComments.c_str());
		}
        }


	if (theUser->getFlag(sqlUser::F_FRAUD))
		{
		bot->Notice(theClient, "\002** This account has been tagged as being used in a fraudulent channel application **\002");
		}

	if (theUser->getFlag(sqlUser::F_GLOBAL_SUSPEND))
		{
		/*
		 * Perform a lookup to get the last SUSPEND event from the userlog.
		 */
		unsigned int theTime;
		string reason = theUser->getLastEvent(sqlUser::EV_SUSPEND, theTime);

		/* alter the reason for web-based suspensions */
		if (adminAccess < 800)
		{
			/* check if this is a web-based suspension */
			if ((reason.size() > 7) && (reason.substr(0,7)=="[Web]: "))
			{
				/* tokenize the string and reconstruct it without 2nd token */
				StringTokenizer rst(reason);
				reason = rst[0];
				reason += " ";
				reason += rst.assemble(2);
			}
		}

		bot->Notice(theClient, "Account suspended %s ago, Reason: %s", prettyDuration(theTime).c_str(),
			reason.c_str());
		} else
		{
		/*
		 *  Maybe they where unsuspended recently..
		 */

		unsigned int theTime;
		string reason = theUser->getLastEvent(sqlUser::EV_UNSUSPEND, theTime);
		if (!reason.empty())
			{
				/* alter the reason for web-based suspensions */
				if (adminAccess < 800)
				{
					/* check if this is a web-based suspension */
					if ((reason.size() > 7) && (reason.substr(0,7)=="[Web]: "))
					{
						/* tokenize the string and reconstruct it without 2nd token */
						StringTokenizer rst(reason);
						reason = rst[0];
						reason += " ";
						reason += rst.assemble(2);
					}
				}
				bot->Notice(theClient, "Account was unsuspended %s ago %s",
					prettyDuration(theTime).c_str(),
					reason.c_str());
			}
		}
	}

	/*
	 * Run a query to see what channels this user has access on. :)
	 * Only show to those with admin access, or the actual user.
	 */

	if( adminAccess || (tmpUser == theUser) )
		{
		bot->Notice(theClient, "EMail: %s",
			theUser->getEmail().c_str());

#ifdef USING_NEFARIOUS
		bot->Notice(theClient, "Nickname: %s",
			theUser->getNickName().c_str());
#endif
		bot->Notice(theClient, "Saved hostname: %s",
			theUser->getHostName().c_str());

		if ((tmpUser != theUser) && ((tmpadminLevel > 0 || theUser->getFlag(sqlUser::F_OPER))) && (adminAccess < 800))
		{
			bot->Notice(theClient, "Last Hostmask: Not Available");
		} else {
			bot->Notice(theClient, "Last Hostmask: %s",
				theUser->getLastHostMask().c_str());
			//Show ip only to admins
			if(adminAccess > 0) 
				{
				bot->Notice(theClient, "Last IP: %s",
					theUser->getLastIP().c_str());			
				}
		}

#ifdef USE_NOTES
		if(theUser->getFlag(sqlUser::F_NONOTES))
			{
			bot->Notice(theClient, "%s doesn't accept Notes.", theUser->getUserName().c_str());
			} else
			{
			bot->Notice(theClient, "%s happily accepts Notes.", theUser->getUserName().c_str());
			}
#endif
		string createdOn = (string)ctime(&theUser->getCreatedTS());
		createdOn.erase(createdOn.length()-1);
		string createdAgo = prettyDuration((int)theUser->getCreatedTS());
		bot->Notice(theClient,"User was created on: %s (%s ago)",createdOn.c_str(), createdAgo.c_str());
		bot->outputChannelAccesses(theClient, theUser, tmpUser, 0);

		}

	/*
	 * Show the channels this guy owns to opers.
	 */

	if (theClient->isOper() && !adminAccess && (tmpUser != theUser))
	{
		bot->outputChannelAccesses(theClient, theUser, tmpUser, 500);
	}

	
	return true;
}
stringstream theQuery;
sqlUser* theUser = bot->isAuthed(theClient, false);
Channel* tmpChan = Network->findChannel(st[1]);
sqlChannel* theChan = bot->getChannelRecord(st[1]);
if( !theChan )
	{
	cservice::incompleteChanRegsType::iterator theApp = bot->incompleteChanRegs.end();
	unsigned int lastdays = (unsigned int)bot->currentTime() - ((unsigned int)bot->daySeconds*5);	
	theQuery << "SELECT id,status,manager_id,managername,pending.description,decision,created_ts FROM channels,pending WHERE "
		 << "registered_ts = 0 AND status <> 3 AND lower(channels.name) = '"
		 << escapeSQLChars(string_lower(st[1]))
		 << "'"
		 << " AND channels.id = pending.channel_id AND (pending.decision_ts=0 OR (pending.decision_ts>0 AND pending.decision_ts>="
			<< lastdays
			<< "))"
			<< ends;
	#ifdef LOG_SQL
		elog << "sqlQuery> " << theQuery.str().c_str() << endl;
	#endif
	if (!bot->SQLDb->Exec(theQuery, true))
	{	bot->logDebugMessage("Error on CHANInfo.status query: %s",theQuery.str().c_str());
		return false;
	} else if (bot->SQLDb->Tuples() != 0) 
	{
		unsigned int chanID = atoi(bot->SQLDb->GetValue(0,0));
		unsigned int status = atoi(bot->SQLDb->GetValue(0,1));
		string mngrID = bot->SQLDb->GetValue(0,2);
		string mngrName = bot->SQLDb->GetValue(0,3);
		string chandesc = bot->SQLDb->GetValue(0,4);
		string decision = bot->SQLDb->GetValue(0,5);
		time_t posted = atoi(bot->SQLDb->GetValue(0,6));
		string suppUserName;
		string supported;
		string nick;
		string supplist;
		string joincount;
		bool showsupplist = false;
		string tmpUserName = tmpUser->getUserName();
		theQuery.str("");
		theQuery << "SELECT user_name,last_seen FROM users,users_lastseen WHERE id="
			 << mngrID
			 << " AND id = user_id"
			 << ends; 		
		#ifdef LOG_SQL
			elog << "CHANINFO.STATUS.mngr.sqlQuery> " << theQuery.str().c_str() << endl;
		#endif
		if (!bot->SQLDb->Exec(theQuery, true))
		{	bot->logDebugMessage("Error on CHANInfo.status managerquery: %s",theQuery.str().c_str());
			return false;
		}
		string mngrUserName = bot->SQLDb->GetValue(0,0);
		unsigned int mngrlastseen = atoi(bot->SQLDb->GetValue(0,1));
		theApp = bot->incompleteChanRegs.find(atoi(mngrID));
		bot->Notice(theClient,"Channel %s is in applications list at stage:",st[1].c_str());
		switch (status)
		{
			case 0: /* Pending Supporters Confirmation = Incoming */
			{
				if (theApp != bot->incompleteChanRegs.end())
					bot->Notice(theClient," \002*** NEW PENDING INCOMPLETE APPLICATION ***\002");
				else
					bot->Notice(theClient," \002*** PENDING SUPPORTERS CONFIRMATION ***\002");
				break;
			}
			case 1: /* Traffic Check */
			{
				bot->Notice(theClient,"  \002*** TRAFFIC CHECKING ***\002");
				break;
			}
			case 2: /* Notification */
			{
				bot->Notice(theClient,"   \002*** NOTIFICATION ***\002");
				break;
			}
			/* This case were filtered out on query, it means the channel was purged
			   (registered_ts = 0 AND status = 3 Accepted) 
			case 3: // Completed 
			{	
				bot->Notice(theClient,"   \002*** ACCEPTED ***\002");
				break;
			} */
			case 4: /* Cancelled by applicant */
			{
				bot->Notice(theClient,"  \002CANCELLED BY APPLICANT\002");
				break;
			}
			case 8: /* Pending Admin Review */
			{
				bot->Notice(theClient," \002*** PENDING ADMIN REVIEW ***\002");
				break;
			}
			case 9: /* Rejected */
			{
				bot->Notice(theClient,"   \002*** REJECTED ***\002");
				break;
			}
		}
		bot->Notice(theClient,"Applicant: %s - last seen: %s ago",mngrUserName.c_str(),
					prettyDuration(mngrlastseen).c_str());
		if (adminAccess > 0) bot->Notice(theClient,"Real Name: %s",mngrName.c_str());
		bot->Notice(theClient,"Description: %s",chandesc.c_str());
		if ((status == 9) && (decision.find("<br>") != string::npos))
			decision.replace(decision.find("<br>"),4,": ");
		theQuery.str("");
		theQuery << "SELECT user_name,support,join_count FROM users,supporters WHERE channel_id="
			 << chanID
			 <<" AND users.id = supporters.user_id"
			 << ends;
		if (!bot->SQLDb->Exec(theQuery, true))
		{	bot->logDebugMessage("Error on CHANINFO.supporters.usernames query");
			#ifdef LOG_SQL
			//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
			elog << "CHANINFO.supporters query> SQL Error: " 
			     << bot->SQLDb->ErrorMessage() 
			     << endl ; 
			#endif
		}
		if (bot->SQLDb->Tuples() == 0) 
		{
			/*
			if (theApp == bot->incompleteChanRegs.end())
			{
				bot->Notice(theClient,"No results returned on CHANINFO.supporters query");
				return false;
			}*/
			bot->Notice(theClient,"Supporters: ");
			return true;
		}
		for (unsigned int i = 0 ; i<bot->SQLDb->Tuples(); i++)
		{
			suppUserName = bot->SQLDb->GetValue(i,0);
			supported = bot->SQLDb->GetValue(i,1);
			joincount = bot->SQLDb->GetValue(i,2);
			sqlUser* suppUser = bot->getUserRecord(suppUserName);
			if ((tmpUserName == suppUserName) || (tmpUserName == mngrUserName) || (adminAccess > 0)) showsupplist = true;
			if (supported == "Y") suppUserName = "\002" + suppUserName + "\002";
			if (supported == "N") suppUserName = "\002" + string_upper(suppUserName) + "\002";
			if (supplist == "") supplist += "Supporters: " + suppUserName;
				else supplist += ", " + suppUserName;
			nick = "";
			for (sqlUser::networkClientListType::iterator ptr = suppUser->networkClientList.begin();
				ptr != suppUser->networkClientList.end() ; ++ptr )
				{
					if (nick == "") nick += "/" + (*ptr)->getNickName(); 
						else nick += " " + (*ptr)->getNickName(); 
					if ((tmpChan) && (tmpChan->findUser(*ptr)))
					{ 
						nick = "\002" + nick + "\002";
						nick.replace(nick.find("\002/"),2,"/\002");
					}
				}
			supplist += nick;
			if (adminAccess > 0) supplist += " ("+joincount+")";
			// Quick buffer overload protection
			// TODO: This will show to anyone if buffer overload,also decision must be solved
			if (supplist.size() >= 450) 
			{
				bot->Notice(theClient,supplist.c_str());
				supplist.erase(supplist.begin(), supplist.end());
			}
		}
		if (showsupplist) 
		{
			bot->Notice(theClient,"Application posted on: %s",ctime(&posted));
			if (status == 9) bot->Notice(theClient,"Decision %s",decision.c_str());
			bot->Notice(theClient,supplist.c_str());
			return true;
		}
	return true;
	}	
	else	
	{	bot->Notice(theClient,
			bot->getResponse(tmpUser,
				language::chan_not_reg,
				string("The channel %s is not registered")).c_str(),
			st[1].c_str());
		return true;
	}
}

/*
 * Receiving all the level 500's of the channel through a sql query.
 * The description and url, are received from the cache. --Plexus
 */

theQuery.str("");
theQuery	<< queryHeader
		<< queryString
		<< "AND levels.channel_id = "
		<< theChan->getID()
		<< ends;

#ifdef LOG_SQL
	elog	<< "CHANINFO::sqlQuery> "
		<< theQuery.str().c_str()
		<< endl;
#endif

bot->Notice(theClient,
	bot->getResponse(theUser,
		language::reg_by,
		string("%s is registered by:")).c_str(),
	st[1].c_str());

if( bot->SQLDb->Exec(theQuery, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	for(unsigned int i = 0; i < bot->SQLDb->Tuples(); i++)
		{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::last_seen_info,
				string("%s - last seen: %s ago")).c_str(),
			bot->SQLDb->GetValue(i, 1).c_str(),
			prettyDuration(atoi(bot->SQLDb->GetValue(i, 3))).c_str());
		} // for()
	}

if(theChan->getFlag(sqlChannel::F_SPECIAL) && !adminAccess) return true;

if( !theChan->getDescription().empty() )
	{
	bot->Notice(theClient,
		bot->getResponse(theUser,
			language::desc,
			string("Desc: %s")).c_str(),
		theChan->getDescription().c_str());
	}

if ((adminAccess > 0) && (theChan->getFlag(sqlChannel::F_SUSPEND)))
{
	string suspender = "";
	string suspendreason = "";
	unsigned int suspendts = 0;
	stringstream queryString;

	queryString	<< "SELECT message,ts FROM channellog WHERE "
			<< "channelid = "
			<< theChan->getID()
			<< " AND event = "
			<< sqlChannel::EV_SUSPEND
			<< " AND ts <= "
			<< bot->currentTime()
			<< " ORDER BY ts DESC LIMIT 1"
			<< ends;

#ifdef LOG_SQL
	elog	<< "cservice::CHANINFOCommand> "
		<< queryString.str().c_str()
		<< endl;
#endif

	if (bot->SQLDb->Exec(queryString, true))
	{
		if (bot->SQLDb->Tuples() > 0)
		{
			string tempreason = bot->SQLDb->GetValue(0, 0);
			StringTokenizer suspendst(tempreason);
			if (suspendst.size() > 5)
			{
				if ( (suspendst[2].substr(0,1)=="(") &&
				(suspendst[2].substr(suspendst[2].size()-1,1)==")"))
					suspender = suspendst[2].substr(1,
						suspendst[2].size()-2);
				else
					suspender = suspendst[2];
				suspendreason = suspendst.assemble(5);
			}
			suspendts = atoi(bot->SQLDb->GetValue(0, 1));
		}
	}

	if (suspendreason != "")
	{
		bot->Notice(theClient, "Channel suspended %s ago by %s, Reason: %s",
			prettyDuration(suspendts).c_str(),
			suspender.c_str(),
			suspendreason.c_str());
	}
}

if( !theChan->getComment().empty() && adminAccess )
	{
	if((tmpUser) && bot->getAdminAccessLevel(tmpUser))
		{
		bot->Notice(theClient, "Comments: %s",
			theChan->getComment().c_str());
		}
	}

if( !theChan->getKeywords().empty() )
	{
	bot->Notice(theClient,
		bot->getResponse(theUser,
			language::keywords,
			string("Keywords: %s")).c_str(),
		theChan->getKeywords().c_str());
	}

if( !theChan->getURL().empty() )
	{
	bot->Notice(theClient,
		bot->getResponse(theUser,
			language::url,
			string("URL: %s")).c_str(),
		theChan->getURL().c_str());
	}

if (!theChan->getWelcome().empty())
{
	bot->Notice(theClient,
	   bot->getResponse(theUser,
		   language::welcome_status,
		   string("WELCOME for %s is: %s")).c_str(),
	   theChan->getName().c_str(),
	   theChan->getWelcome().c_str());
}

if (theChan->getFlag(sqlChannel::F_TEMP))
	{
	bot->Notice(theClient, "\002This channel has a temporary manager.\002");
	}

return true;
}

} // namespace gnuworld.
