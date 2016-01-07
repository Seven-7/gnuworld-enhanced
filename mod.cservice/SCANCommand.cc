/*
 * SCANCommand.cc
 *
 *  Created on: Jan 07, 2016
 *      Author: Seven
 *      gergo_f@yahoo.com
 *
 * General purpose scan command:
 *  - Find owner user of a nickname
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
 * SCANCommand.cc, v 1.0 2016.01.07 - Seven
 */

#include        <string>
#include        <sstream>

#include        "StringTokenizer.h"
#include        "cservice.h"
#include        "levels.h"
#include        "responses.h"
#include        "dbHandle.h"
#include		"Network.h"

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

bool SCANCommand::Exec( iClient* theClient, const string& Message )
{

	bot->incStat("COMMANDS.SCAN");

	StringTokenizer st( Message ) ;
	if( st.size() < 2 )
	{
		Usage(theClient);
		return true;
	}

	/*
	 *  Fetch the sqlUser record attached to this client. If there isn't one,
	 *  they aren't logged in - tell them they should be.
	 */
	sqlUser* theUser = bot->isAuthed(theClient, true);
	if (!theUser)
	{
		return false;
	}

	string option, value;
	option = string_upper(st[1]);
	if (st.size() > 2)
		value = st[2];

	if ((option == "NICK") || (option == "NICKNAME"))
	{
		string nickOwner = bot->NickIsRegisteredTo(value);
		if (!nickOwner.empty())
			bot->Notice(theClient, "Nickname is registered to %s", nickOwner.c_str());
		else
			bot->Notice(theClient, bot->getResponse(theUser, language::no_match).c_str());
	}

	return true;
}
} //namespace gnuworld
