/**
 * NOTESCommand.cc
 * Leave a note for another user account, or read your own Notes.
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
 * $Id: NOTECommand.cc,v 1.8 2007/08/28 16:10:11 dan_karrels Exp $
 */

#include	<string>
#include	<sstream>
#include	<iostream>

#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"Network.h"
#include	"responses.h"
#include	"cservice.h"
#include	"cservice_config.h"

const char NOTECommand_cc_rcsId[] = "$Id: NOTECommand.cc,v 1.8 2007/08/28 16:10:11 dan_karrels Exp $" ;

namespace gnuworld
{

using std::endl ;
using std::ends ;
using std::string ;
using std::stringstream ;

bool NOTECommand::Exec( iClient* theClient, const string& Message )
{

//If Admin's don't play, noone's playin' ...
#ifndef USE_USERS_NOTES
   #ifndef USE_NOTES
	return true;
   #endif
#endif

bot->incStat("COMMANDS.NOTE");

/* Is the user authorised? */
sqlUser* theUser = bot->isAuthed(theClient, false);
if(!theUser)
	{
	return false;
	}
/* No, from now on, All of Us is Playin' :P - Seven :P
/* Only let admins play for now 
int level = bot->getAdminAccessLevel(theUser);
if(!level) return false;
 */

StringTokenizer st( Message ) ;
if( st.size() < 3 )
	{
	Usage(theClient);
	return true;
	}

/*
 * Sending a Note?
 */
if (string_lower(st[1]) == "send")
{
	/*
	 * First, check this person exists.
	 */
	sqlUser* targetUser = bot->getUserRecord(st[2]);
	string message = st.assemble(3);

	if (!targetUser)
		{
		bot->Notice(theClient, bot->getResponse(theUser, language::not_registered,
				string("I don't know who %s is")).c_str(), st[2].c_str());
		return false;
		}

	/*
	 * They do? Great! lets see if we're currently allowed to send
	 * a note.
	 *
	 * Firstly, does the target wish to have a nice quiet inbox?
	 */
	if (targetUser->getFlag(sqlUser::F_NONOTES))
		{
		bot->Notice(theClient, bot->getResponse(theUser, language::noaccpt_notes,
				string("%s doesn't accept Notes.")).c_str(),
			targetUser->getUserName().c_str());
			return false;
		}

	/* Do Admins gets notes from users? */
#ifndef USERS_NOTE_ADMINS
	bot->Notice(theClient, bot->getResponse(theUser, language::noaccpt_notes,
				string("%s doesn't accept Notes.")).c_str(),
			targetUser->getUserName().c_str());
	return false;
#endif
	/*
	 * Have we exceeded the maximum number of Notes we can
	 * send right now?
	 */

	if( (unsigned int)(bot->currentTime() - theUser->getLastNote()) >= bot->noteDuration )
		{
		theUser->setLastNote(bot->currentTime());
		theUser->setNotesSent(0);
		}

	if( ((unsigned int)(bot->currentTime() - theUser->getLastNote()) <= bot->noteDuration) && (theUser->getNotesSent() >= bot->noteLimit) )
		{
		bot->Notice(theClient, bot->getResponse(theUser, language::exc_max_notes, string("You have exceeded the maximum number of notes you can send at this time, please try later.")).c_str());
		return false;
		}

	/*
	 * Dump the note into the database.
	 */

	/* .. SQL here .. */

	static const char* queryHeader = "INSERT INTO notes (user_id,from_user_id,message,last_updated) VALUES (";

	stringstream queryString;
	queryString	<< queryHeader
				<< targetUser->getID() << ", "
				<< theUser->getID() << ", '"
				<< escapeSQLChars(message) << "', "
				<< "now()::abstime::int4);"
				<< ends;

	#ifdef LOG_SQL
		elog	<< "NOTECommand::Insert Note> "
				<< queryString.str().c_str()
				<< endl;
	#endif

	if( !bot->SQLDb->Exec(queryString ) )
//	if( PGRES_COMMAND_OK != status )
		{
		elog	<< "sqlBan::commit> Something went wrong: "
				<< bot->SQLDb->ErrorMessage()
				<< endl;

		bot->Notice(theClient, "An unknown error occured delivering the note.");
		bot->Notice(theClient, bot->SQLDb->ErrorMessage().c_str());
		return false ;
		}
	bot->Notice(theClient, bot->getResponse(theUser, language::note_sended,string("Successfully delivered message to %s!")).c_str(), targetUser->getUserName().c_str());

	bot->noticeAllAuthedClients(targetUser, bot->getResponse(targetUser, language::read_note_help,string("%s has just sent you a note. Type /msg %s notes read all to read it.")).c_str(),
		theUser->getUserName().c_str(), bot->getNickName().c_str());

	theUser->setNotesSent(theUser->getNotesSent() + 1);
	return true;
}

/*
 * Reading a Note?
 */
if (string_lower(st[1]) == "read")
{
	if(string_lower(st[2]) == "all")
	{
	/*
	 * Perform a query to list all notes belonging to this user.
	 */
	stringstream allNotesQuery;
	allNotesQuery	<< "SELECT users.user_name, notes.message, notes.last_updated, message_id "
					<< "FROM notes,users "
					<< "WHERE notes.from_user_id = users.id "
					<< "AND notes.user_id = "
					<< theUser->getID()
					<< " ORDER BY notes.last_updated ASC"
					<< ends;

	if( !bot->SQLDb->Exec( allNotesQuery, true ) )
//	if( PGRES_TUPLES_OK != status )
		{
		elog	<< "SUPPORTCommand> SQL Error: "
				<< bot->SQLDb->ErrorMessage()
				<< endl ;

		bot->Notice(theClient, "An unknown error occured while reading your notes.");
		return false ;
		}

	if (bot->SQLDb->Tuples() <= 0)
		{
		bot->Notice(theClient, bot->getResponse(theUser, language::have_nonotes,string("You have no notes.")).c_str());
		return false;
		}

	unsigned int noteCount = bot->SQLDb->Tuples();

	for (unsigned int i = 0 ; i < noteCount; i++)
		{
		string from = bot->SQLDb->GetValue(i,0);
		string theMessage = bot->SQLDb->GetValue(i,1);
		unsigned int when = atoi(bot->SQLDb->GetValue(i,2));
		unsigned int message_id = atoi(bot->SQLDb->GetValue(i,3));
		bot->Notice(theClient, bot->getResponse(theUser, language::message_id,string("\002NOTE\002 (Message-Id: %i): Recieved from %s, %s ago: %s")).c_str(),
			message_id, from.c_str(), prettyDuration(when).c_str(), theMessage.c_str());
		}

	}
	bot->Notice(theClient, bot->getResponse(theUser, language::erase_note_help,string("To erase an individual note, type /msg %s notes erase <message-id>. To erase all your notes, type /msg %s notes erase all")).c_str(), bot->getNickName().c_str(), bot->getNickName().c_str());

	return true;
}

/*
 * Erasing a Note?
 */
if (string_lower(st[1]) == "erase")
{
	if(string_lower(st[2]) == "all")
		{
		stringstream queryString;
		queryString	<< "DELETE FROM notes where user_id = "
					<< theUser->getID()
					<< ends;

		#ifdef LOG_SQL
			elog	<< "NOTECommand::Delete Notes> "
					<< queryString.str().c_str()
					<< endl;
		#endif

		if( !bot->SQLDb->Exec(queryString ) )
//		if( PGRES_COMMAND_OK != status )
			{
			bot->Notice(theClient, "An unknown error occured while deleting your notes.");
			return false;
			}
		bot->Notice(theClient, bot->getResponse(theUser, language::all_notes_erased,string("Successfully erased all your notes.")).c_str());
		return true;
		}

	/*
	 * TOFINISH: Delete by message-id.
	 */

	unsigned int messageId = atoi(st[2].c_str());
	if(!messageId)
		{
		bot->Notice(theClient, bot->getResponse(theUser, language::inv_messageid,string("Invalid message-id.")).c_str());
		return false;
		}

		stringstream queryString;
		queryString	<< "DELETE FROM notes where user_id = "
					<< theUser->getID()
					<< " AND message_id = "
					<< messageId
					<< ends;

		#ifdef LOG_SQL
			elog	<< "NOTECommand::Delete Notes> "
					<< queryString.str().c_str()
					<< endl;
		#endif

		if( !bot->SQLDb->Exec(queryString, true ) )
//		if( PGRES_COMMAND_OK != status )
			{
			bot->Notice(theClient, "An error occured while deleting note-id %i.", messageId);
			return false;
			}
		bot->Notice(theClient, bot->getResponse(theUser, language::erased_messageid,string("Successfully erased note with message-id %i.")).c_str(), messageId);
		return true;

	return true;
}


return true ;
}

} // namespace gnuworld.
