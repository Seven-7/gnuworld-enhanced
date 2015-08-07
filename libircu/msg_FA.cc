/**
 * msg_FA.cc
 * Copyright (C) 2002 Daniel Karrels <dan@karrels.com>
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

#include	<new>
#include	<string>
#include	<iostream>

#include	<cassert>

#include	"gnuworld_config.h"
#include	"server.h"
#include	"iClient.h"
#include	"events.h"
#include	"ip.h"
#include	"Network.h"
#include	"ELog.h"
#include	"xparameters.h"
#include	"ServerCommandHandler.h"
#include	"StringTokenizer.h"


namespace gnuworld
{

using std::string ;
using std::endl ;

CREATE_HANDLER(msg_FA)

/**
 * Any server sends out the FA handler, need to be handled by servers/services,
 *
 * Syntax:
 * Az FA AzAAA new.fake.host.com
 *
 * [0] Az    - Numeric of the sending server
 * [0] FA    - Fake Host Token
 * [1] AzAAA - target client's numeric
 * [2] new.fake.host.com - the desired new host
 */
bool msg_FA::Execute( const xParameters& params )
{
if( params.size() < 3 )
{
	// Error
	elog	<< "msg_FA> Invalid format: "
		<< params
		<< endl ;
	return false ;
}

iClient* Target = Network->findClient(params[1]);
if( NULL == Target )
	{
	elog	<< "msg_FA> Unable to find client for: %s"
			<< params[ 1 ]
			<< endl;
	return false ;
	}

Target->setFakeHost(params[2]);

return true;
}

} // namespace gnuworld
