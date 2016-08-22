/**
 * REGISTERCommand.cc
 *
 * 26/12/2000 - Greg Sikorski <gte@atomicrevs.demon.co.uk>
 * Initial Version.
 *
 * Registers a channel.
 *
 * Caveats: None
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
 * $Id: REGISTERCommand.cc,v 1.24 2009/07/31 07:29:13 mrbean_ Exp $
 */

#include	<map>
#include	<string>
#include	<sstream>
#include	<iostream>

#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"cservice.h"
#include	"levels.h"
#include	"dbHandle.h"
#include	"Network.h"
#include	"responses.h"


const char REGISTERCommand_cc_rcsId[] = "$Id: REGISTERCommand.cc,v 1.24 2009/07/31 07:29:13 mrbean_ Exp $" ;

namespace gnuworld
{
using std::pair ;
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

using namespace gnuworld;

bool REGISTERCommand::Exec( iClient* theClient, const string& Message )
{
	bot->incStat("COMMANDS.REGISTER");

	StringTokenizer st( Message ) ;

	/*
	 *  Fetch the sqlUser record attached to this client. If there isn't one,
	 *  they aren't logged in - tell them they should be.
	 */

	sqlUser* theUser = bot->isAuthed(theClient, true);
	if (!theUser) return false;

	if( st.size() < 2 )
	{
		Usage(theClient);
		return true;
	}

	string::size_type pos = st[1].find_first_of( ',' ); /* Don't allow comma's in channel names. :) */

	if ( (st[1][0] != '#') || (string::npos != pos))
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::inval_chan_name,
				string("Invalid channel name.")));
		return false;
	}

	/*
	 *  First, check the channel isn't already registered.
	 */

	sqlChannel* theChan;
	theChan = bot->getChannelRecord(st[1]);
	if (theChan)
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::chan_already_reg,
				string("%s is already registered with me.")).c_str(),
			st[1].c_str());
		return false;
	}

	/*
	 *  Check the user has sufficient access for this command..
	 */
	bool hasApp = false;
	sqlIncompleteChannel *chanApp = 0;
	cservice::incompleteChanRegsType::iterator theApp = bot->incompleteChanRegs.find(theUser->getID());
	if (theApp != bot->incompleteChanRegs.end())
	{
		hasApp = true;
		chanApp = theApp->second;
	}

	int level = bot->getAdminAccessLevel(theUser);
	if (((level < level::registercmd) && (level > 0)) && ((st.size() > 2) && (!hasApp)))
	{
		bot->Notice(theClient,
			bot->getResponse(theUser,
				language::insuf_access,
				string("You have insufficient access to perform that command.")));
		return false;
	}
#ifndef ALLOW_IRC_CHANREG
	if (level == 0)
	{
		bot->Notice(theClient, "Channel registration via IRC is disabled. Use website instead.");
		return false;
	}
#endif
	// Admin registration case !
	if ((level >= level::registercmd) && ((st.size() > 2) && (!hasApp)))
	{
		sqlUser* tmpUser = bot->getUserRecord(st[2]);
		if (!tmpUser)
		{
			bot->Notice(theClient,
					bot->getResponse(theUser,
							language::not_registered,
							string("The user %s doesn't appear to be registered.")).c_str(),
							st[2].c_str());
			return true;
		}

		if (level < level::immune::registercmd)
		{
			if (!bot->isValidChannel(st[1]))
			{
				if (!bot->validResponseString.empty())
				{
					bot->Notice(theClient, "Cannot register, channel is %s",bot->getCurrentValidResponse().c_str());
					bot->validResponseString.clear();
				}
	           	return false;
			}

			if (!bot->isValidUser(tmpUser->getUserName()))
			{
				if (!bot->validResponseString.empty())
				{
					bot->Notice(theClient, "Cannot register, invalid target user (%s)",bot->getCurrentValidResponse().c_str());
					bot->validResponseString.clear();
				}
	           	return false;
			}

			if (!bot->isValidApplicant(tmpUser))
			{
				if (!bot->validResponseString.empty())
				{
					if (bot->getCurrentValidResponse() == "ALREADY_HAVE_CHAN")
					{
						bot->Notice(theClient, "Target user already have a channel registered.");
						bot->validResponseString.clear();
						return false;
					}
					if (bot->getCurrentValidResponse() == "ALREADY_HAVE_PENDINGCHAN")
					{
						bot->Notice(theClient, "Target user already have a channel pending registration.");
						bot->validResponseString.clear();
						return false;
					}
					bot->Notice(theClient, bot->getCurrentValidResponse().c_str());
					bot->validResponseString.clear();
				}
	           	return false;
			}
		} //level < immune::registercomd

		if (bot->sqlRegisterChannel(theClient, tmpUser,st[1].c_str()))
		{
			//sqlChannel* newChan = bot->getChannelRecord(st[1]);
			theChan = bot->getChannelRecord(st[1]);
			bot->writeChannelLog(theChan, theClient, sqlChannel::EV_REGISTER, "to " + tmpUser->getUserName());
			bot->logAdminMessage("%s (%s) has registered %s to %s", theClient->getNickName().c_str(),
					theUser->getUserName().c_str(), st[1].c_str(), tmpUser->getUserName().c_str());

			bot->Notice(theClient,
                bot->getResponse(theUser,
                	language::regged_chan,
                    	string("Registered channel %s to %s")).c_str(), st[1].c_str(),tmpUser->getUserName().c_str());
			//return true;
		} else {
			if (!bot->validResponseString.empty())
			{
				bot->Notice(theClient, bot->validResponseString.c_str());
				bot->validResponseString.clear();
			}
                	return false;
			}
		return true;
	} // end of Admin registration case

	//Normal user application case
	if (hasApp)
	{
		//bot->logDebugMessage("Performing normal registration");

		if ((string_lower(chanApp->chanName) != (string_lower(st[1]))))
		{
			string chanName = chanApp->chanName;
			bot->Notice(theClient, "You have an incomplete channel application of %s", chanName.c_str());
			if (!chanApp->RealName.empty())
			{
				bot->Notice(theClient, "So far you've enumerated the following:");
				bot->Notice(theClient, "Your Real Name: %s", chanApp->RealName.c_str());
			}
			else
			{
				bot->Notice(theClient,"Now specify your Real Name by typing /msg %s REGISTER %s REALNAME <Your RealName>", bot->getNickName().c_str(), chanName.c_str());
				bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(), chanName.c_str());
			}
			if ((!chanApp->RealName.empty()) && (chanApp->Description.empty()))
			{
				bot->Notice(theClient,"Now specify your channel's description by typing /msg %s REGISTER %s DESCRIPTION <description>", bot->getNickName().c_str(),chanName.c_str());
				if (chanApp->supps.empty())
					bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(),chanName.c_str());
			}
			if (!chanApp->Description.empty()) bot->Notice(theClient, "Description: %s", chanApp->Description.c_str());
			if ((!chanApp->RealName.empty()) && (!chanApp->Description.empty()))
			{
				bot->Notice(theClient,"Now enumerate your supporters by typing /msg %s REGISTER %s SUPPORTERS <supporter1 supporter2 supporter3 ...>", bot->getNickName().c_str(),chanName.c_str());
				bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s", bot->getNickName().c_str(),chanName.c_str());
			}
			return false;
		}
	}
	if (!hasApp)
	{
		if (!bot->isValidChannel(st[1]))
		{
			if (!bot->validResponseString.empty())
			{
				bot->Notice(theClient, "Cannot register, channel is %s",bot->getCurrentValidResponse().c_str());
				bot->validResponseString.clear();
			}
          		return false;
		}

		if (!bot->isValidUser(theUser->getUserName()))
		{
			if (!bot->validResponseString.empty())
			{
				bot->Notice(theClient, "Cannot register, Your username is invalid (%s)",bot->getCurrentValidResponse().c_str());
				bot->validResponseString.clear();
			}
          		return false;
		}

		if (!bot->isValidApplicant(theUser))
		{
			if (!bot->validResponseString.empty())
			{
				if (bot->getCurrentValidResponse() == "TOO_NEW")
				{
					bot->Notice(theClient, "Sorry, You can't register new channels at the moment.");
					bot->Notice(theClient, "Your username must have been created since at least %i days in order to apply for a new channel.",bot->MinDaysBeforeReg);
					bot->validResponseString.clear();
					return false;
				}
				if (bot->getCurrentValidResponse() == "ALREADY_HAVE_CHAN")
				{
					bot->Notice(theClient, "Sorry, you already have a channel registered to you");
					bot->Notice(theClient, "You can only register \002ONE\002 channel.");
					bot->validResponseString.clear();
					return false;
				}
				if (bot->getCurrentValidResponse() == "ALREADY_HAVE_PENDINGCHAN")
				{
					bot->Notice(theClient, "Sorry, you already have a channel pending registration to you");
					bot->Notice(theClient, "You can only register \002ONE\002 channel.");
					bot->validResponseString.clear();
					return false;
				}
				bot->Notice(theClient, bot->getCurrentValidResponse().c_str());
				bot->validResponseString.clear();
			}
           	return false;
		} //end of isValidApplicant

		//The instant registration case
		if (bot->RequiredSupporters == 0)
		{
			if (bot->sqlRegisterChannel(bot->getInstance(), theUser, st[1].c_str()))
			{
				sqlChannel* newChan = bot->getChannelRecord(st[1]);
				bot->writeChannelLog(newChan, bot->getInstance(), sqlChannel::EV_REGISTER, "to " + theUser->getUserName());
				bot->logAdminMessage("%s (%s) has registered %s to %s", bot->getNickName().c_str(),
						bot->getInstance()->getNickUserHost().c_str(), st[1].c_str(), theUser->getUserName().c_str());

				bot->Notice(theClient,
			                bot->getResponse(theUser,
	        		        	language::regged_chan,
	                    		string("Registered channel %s to %s")).c_str(), st[1].c_str(), theUser->getUserName().c_str());
				//return true;
			} else {
				if (!bot->validResponseString.empty())
				{
					bot->Notice(theClient, bot->validResponseString.c_str());
					bot->logDebugMessage("The Judge Failed to (instant) register channel %s with reason:", st[1].c_str());
					bot->logDebugMessage(bot->validResponseString.c_str());
					bot->validResponseString.clear();
				}
	                	return false;
				}
			return true;
		}

		//bot->logDebugMessage("Entering REGISTER.CheckReclaimQuery");
		//Add the registration query into the db (fresh insert, or reclaim)
		//Check if the channel exists in the 'channels' table with the registered_ts=0 than it was wiped,
		//and we need to do a reclaim
		bool reclaim = false;
		int chanId = bot->getPendingChanId(st[1]);
		if (chanId)	reclaim = true;

		if (reclaim)
		{
			bot->wipeChannel(chanId);
			stringstream theQuery;
			theQuery	<< "UPDATE channels SET name = '"
						<< escapeSQLChars(st[1])
						<< "', mass_deop_pro=0, flood_pro=0, flags=0, limit_offset=3, "
						<< "limit_period=20, limit_grace=1, limit_max=0, userflags=0, "
						<< "url='', description='', keywords='', registered_ts=0, "
						<< "channel_ts=0, channel_mode='', comment='', last_updated=now()::abstime::int4 "
						<< "WHERE id = " << chanId
						<< ends;
			if (!bot->SQLDb->Exec(theQuery, true))
			{
				bot->logDebugMessage("Error on REGISTER.DoReclaimQuery");
				#ifdef LOG_SQL
				//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
				elog 	<< "REGISTER.DoReclaimQuery> SQL Error: "
				   		<< bot->SQLDb->ErrorMessage()
				   		<< endl ;
				#endif
				return false;
			} //else bot->logDebugMessage("Successfully reclaimed channel %s", st[1].c_str());
		}
		else //reclaim
		{
			stringstream theQuery;
			theQuery	<< "INSERT INTO channels (name,url,description,keywords,registered_ts,"
						<< "channel_ts,channel_mode,comment,last_updated,mass_deop_pro,flood_pro,"
						<< "flags,limit_offset,limit_period,limit_grace,limit_max,userflags"
						<< ") VALUES ('"
						<< escapeSQLChars(st[1])
						<< "','','','',0,0,'','',now()::abstime::int4,0,0,0,3,20,1,0,0)"
						<< ends;
			if (!bot->SQLDb->Exec(theQuery, true))
			{
				bot->logDebugMessage("Error on REGISTER.InsertIntoQuery");
				#ifdef LOG_SQL
				//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
				elog 	<< "REGISTER.InsertIntoQuery> SQL Error: "
				   		<< bot->SQLDb->ErrorMessage()
				   		<< endl ;
				#endif
				return false;
			} 
			else
			{	//Also here we need to find out the new channels's id
				chanId = bot->getPendingChanId(st[1]);
				//bot->logDebugMessage("Successfully inserted new channel %s with id = %i", st[1].c_str(), chanId);
			}
		}
		
		sqlIncompleteChannel* newApp = new (std::nothrow) sqlIncompleteChannel(bot->SQLDb);
		assert(newApp != 0);

		bot->incompleteChanRegs.insert(cservice::incompleteChanRegsType::value_type(theUser->getID(),newApp));
		
		newApp->chanName = st[1];
		newApp->chanId = chanId;
		if (!newApp->commitNewPending(theUser->getID(), chanId))
		{
			bot->logDebugMessage("Error on REGISTER.commitNewPending");
			return false;
		}

		//Ask after the Real Name
		bot->Notice(theClient,"Now specify your Real Name by typing /msg %s REGISTER %s REALNAME <Your RealName>", bot->getNickName().c_str(), st[1].c_str());
		bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(), st[1].c_str());
		return true;
	} //end of !hasApp, begin of hasApp

	string chanName = chanApp->chanName; //bot->getPendingChanName(chanApp->chanId);
	string option;
	if (st.size() < 3) //No option specified, so we enumearate the current state
	{
		if (!chanApp->RealName.empty())
		{
			bot->Notice(theClient, "So far you've enumerated the following:");
			bot->Notice(theClient, "Your Real Name: %s", chanApp->RealName.c_str());
		}
		else
		{
			bot->Notice(theClient,"Now specify your Real Name by typing /msg %s REGISTER %s REALNAME <Your RealName>", bot->getNickName().c_str(), chanName.c_str());
			bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(), chanName.c_str());
		}
		if ((!chanApp->RealName.empty()) && (chanApp->Description.empty()))
		{
			bot->Notice(theClient,"Now specify your channel's description by typing /msg %s REGISTER %s DESCRIPTION <description>", bot->getNickName().c_str(),chanName.c_str());
			if (chanApp->supps.empty())
				bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(),chanName.c_str());
		}
		if (!chanApp->Description.empty()) bot->Notice(theClient, "Description: %s", chanApp->Description.c_str());
		if ((!chanApp->RealName.empty()) && (!chanApp->Description.empty()))
		{
			bot->Notice(theClient,"Now enumerate your supporters by typing /msg %s REGISTER %s SUPPORTERS <supporter1 supporter2 supporter3 ...>", bot->getNickName().c_str(),chanName.c_str());
			bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s", bot->getNickName().c_str(),chanName.c_str());
		}
	}
	if (st.size() > 2) option = string_upper(st[2]);
	if (option == "DESC") option = "DESCRIPTION";
	if (option == "REALNAME")
	{
		string mngrName = st.assemble(3);
		if ((mngrName.size() < 2) || (mngrName.size() > 80))
		{
			bot->Notice(theClient,"Your realname must be 2 - 80 charcters long. Try again");
			return true;
		}
		chanApp->RealName = mngrName;
		chanApp->commitRealName();
		if (chanApp->Description.empty())
		{
			bot->Notice(theClient,"Now specify your channel's description by typing /msg %s REGISTER %s DESCRIPTION <description>", bot->getNickName().c_str(),chanName.c_str());
				bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(),chanName.c_str());
				return true;
		}
		if (chanApp->supps.empty())
		{
			bot->Notice(theClient,"Now enumerate your supporters by typing /msg %s REGISTER %s SUPPORTERS <supporter1 supporter2 supporter3 ...>", bot->getNickName().c_str(),chanName.c_str());
			bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(),chanName.c_str());
			return true;
		}
		return true;
	}
	if (option == "DESCRIPTION")
	{
		string desc = st.assemble(3);
		if ((desc.size() < 2) || (desc.size() > 300))
		{
			bot->Notice(theClient,"Your description must be 2 - 300 charcters long. Try again");
			return true;
		}
		chanApp->Description = desc;
		chanApp->commitDesc();
		if (chanApp->RealName.empty())
		{
			bot->Notice(theClient,"Now specify your Real Name by typing /msg %s REGISTER %s REALNAME <Your RealName>", bot->getNickName().c_str(), chanName.c_str());//st[1].c_str());
			bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(), chanName.c_str());//st[1].c_str());
			return true;
		}
		if (chanApp->supps.empty())
		{
			bot->Notice(theClient,"Now enumerate your supporters by typing /msg %s REGISTER %s SUPPORTERS <supporter1 supporter2 supporter3 ...>", bot->getNickName().c_str(),st[1].c_str());
			bot->Notice(theClient,"To cancel/abort your application any time, type /msg %s CANCEL %s YES", bot->getNickName().c_str(),st[1].c_str());
			return true;
		}
		return true;
	}

	//Check the validity of the list of the supporters
	if (option == "SUPPORTERS")
	{
		for (unsigned int i = chanApp->supps.size(); i < bot->RequiredSupporters; i++)
		{
			string suppUserName;
			if (st.size() > (i+3)) suppUserName = st[3+i]; else suppUserName.empty();
			if ((suppUserName.empty()) && (i < bot->RequiredSupporters))
			{
				bot->Notice(theClient,"You need to enumerate %i supporters in order to complete your channel application.", bot->RequiredSupporters);
				return false;
			}
			sqlUser* suppUser = bot->getUserRecord(suppUserName);
			if (!suppUser)
			{
				bot->Notice(theClient, "Supporter \002%s\002 is Inexistent. Please check if you spelt the username correctly and try again.",suppUserName.c_str());
				return false;
			}
			if (!bot->isValidUser(suppUserName))
			{
				bot->Notice(theClient,"Invalid supporter %s (%s)", suppUser->getUserName().c_str(), bot->getCurrentValidResponse().c_str());
				return false;
			}
			if (!bot->isValidSupporter(suppUserName))
				if (!bot->validResponseString.empty())
				{
					if (bot->getCurrentValidResponse() == "NEVER_LOGGED")
					{
						bot->Notice(theClient, "Supporter \002%s\002 has never logged in.", suppUser->getUserName().c_str());
						bot->Notice(theClient, "All your supporters must login to %s on IRC to apply to register a channel.",bot->getNickName().c_str());
						return false;
					}
					if (bot->getCurrentValidResponse() == "TOO_NEW")
					{
						bot->Notice(theClient, "Supporter \002%s\002 is too newly created.", suppUser->getUserName().c_str());
						bot->Notice(theClient, "All your supporters must login to %s on IRC at least %i days before to apply to register a channel.",bot->getNickName().c_str(), bot->MinDaysBeforeSupport);
						return false;
					}
					if (bot->getCurrentValidResponse() == "SEEN_LONG_AGO")
					{
						bot->Notice(theClient, "Supporter \002%s\002 was seen too long ago.", suppUser->getUserName().c_str());
						bot->Notice(theClient, "All your supporters must login to %s on IRC at least 21 days ago to apply to register a channel.", bot->getNickName().c_str());
						return false;
					}
				}
			sqlIncompleteChannel::supporterListType::iterator ptr = chanApp->supps.find(suppUser->getID());
			if (ptr != chanApp->supps.end())
			{
				bot->Notice(theClient, "Supporter \002%s\002 is enumerated multiple times", ptr->second->getUserName().c_str());
				chanApp->supps.clear();
				return false;
			}
			else
				chanApp->supps.insert(sqlIncompleteChannel::supporterListType::value_type(suppUser->getID(),suppUser));
		}
		chanApp->commitSupporters();
		delete(chanApp); chanApp = 0;
		bot->incompleteChanRegs.erase(theUser->getID());

		//We cannot write to a non existing NULL sqlChannel, so we nedd to create:
        unsigned int channel_ts = 0;
        Channel* tmpChan = Network->findChannel(chanName);
        channel_ts = tmpChan ? tmpChan->getCreationTime() : ::time(NULL);
        theChan = new (std::nothrow) sqlChannel(bot->SQLDb);
        theChan->setName(chanName);
        theChan->setChannelTS(channel_ts);
        theChan->setRegisteredTS(bot->currentTime());
        theChan->setChannelMode("+tn");
        theChan->setLastUsed(bot->currentTime());

		bot->writeChannelLog(theChan, theClient, sqlChannel::EV_NEWAPP, "by " + theUser->getUserName());

		bot->logAdminMessage("New channel application %s by (%s) %s", st[1].c_str(), theUser->getUserName().c_str(), theClient->getRealNickUserHost().c_str());
		bot->Notice(theClient,"Your application has been recorded.");
		bot->Notice(theClient,"Please allow 10-12 days for processing");
	}
	return true;
}

} // namespace gnuworld.
