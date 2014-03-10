/**
 * cloner.cc
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
 * $Id: cloner.cc,v 2.0 2012/10/20 08:40:58 Seven Exp $
 */

#include	<new>
#include	<list>
#include	<vector>
#include	<iostream>
#include	<sstream>
#include	<string>

#include	<ctime>
#include	<cstdlib>

#include	"server.h"
#include	"client.h"
#include	"iClient.h"
#include	"cloner.h"
#include	"Channel.h"
#include	"EConfig.h"
#include	"ip.h"
#include	"Network.h"
#include	"StringTokenizer.h"
#include	"misc.h"
#include	"ELog.h"

namespace gnuworld
{
using std::vector ;
using std::endl ;
using std::stringstream ;
using std::string ;
/*
 *  Exported function used by moduleLoader to gain an
 *  instance of this module.
 */

extern "C"
{
  xClient* _gnuwinit(const std::string& args)
  { 
    return new cloner( args );
  }

} 
 
/**
 * This constructor calls the base class constructor.  The xClient
 * constructor will open the configuration file given and retrieve
 * basic client info (nick/user/host/etc).
 * Any additional processing must be done here.
 */
cloner::cloner( const std::string& configFileName )
 : xClient( configFileName )
{
	EConfig conf( configFileName ) ;

	cloneDescription = conf.Require( "clonedescription" )->second ;
	cloneMode = conf.Require( "clonemode" )->second ;
	fakeServerName = conf.Require( "fakeservername" )->second ;
	fakeServerDescription = conf.Require( "fakeserverdescription" )->second ;

	allowOpers = false ;
	string stringOperAccess = conf.Require( "allow_opers" )->second ;
	if( !strcasecmp( stringOperAccess, "yes" ) ||
		!strcasecmp( stringOperAccess, "true" ) )
		{
		allowOpers = true ;
		}

	EConfig::const_iterator ptr = conf.Find( "permit_user" ) ;
	while( ptr != conf.end() && ptr->first == "permit_user" )
		{
		allowAccess.push_back( ptr->second ) ;
		++ptr ;
		}

	cloneBurstCount = atoi( conf.Require( "cloneburstcount" )->second.c_str() ) ;
	if( cloneBurstCount < 1 )
		{
		elog	<< "cloner> cloneBurstCount must be at least 1"
			<< endl ;
		::exit( 0 ) ;
		}

	ptr = conf.Find( "fakehost" ) ;
	while( ptr != conf.end() && ptr->first == "fakehost" )
		{
		hostNames.push_back( ptr->second ) ;
		++ptr ;
		}

	if( hostNames.empty() )
		{
		elog	<< "cloner> Must specify at least one hostname"
			<< endl ;
		::exit( 0 ) ;
		}

	ptr = conf.Find( "fakeuser" ) ;
	while( ptr != conf.end() && ptr->first == "fakeuser" )
		{
		userNames.push_back( ptr->second ) ;
		++ptr ;
		}

	if( userNames.empty() )
		{
		elog	<< "cloner> Must specify at least one username"
			<< endl ;
		::exit( 0 ) ;
		}

	makeCloneCount = 0 ;

	minNickLength = atoi( conf.Require( "minnicklength" )->second.c_str() ) ;
	maxNickLength = atoi( conf.Require( "maxnicklength" )->second.c_str() ) ;

	if( minNickLength < 1 )
		{
		elog	<< "cloner> minNickLength must be at least 1"
			<< endl ;
		::exit( 0 );
		}
	if( maxNickLength <= minNickLength )
		{
		elog	<< "cloner> minNickLength must be less than maxNickLength"
			<< endl ;
		::exit( 0 ) ;
		}
}

cloner::~cloner()
{
/* No heap space allocated */
}

void cloner::OnAttach()
{
xClient::OnAttach() ;

// Register for all events
for( eventType whichEvent = 0 ; whichEvent != EVT_NOOP ; ++whichEvent )
	{
	switch( whichEvent )
		{
		case EVT_RAW:
			break ;
		default:
			MyUplink->RegisterEvent( whichEvent, this ) ;
			break ;
		} // switch()
	} // for()

MyUplink->RegisterChannelEvent( "*", this ) ;

// Register to receive timed events every minute
// This event will be used to flush data to the log files
MyUplink->RegisterTimer( ::time( 0 ) + 1, this ) ;
}

void cloner::OnChannelEvent( const channelEventType& whichEvent,
	Channel* theChan,
	void* arg1,
	void* arg2,
	void* arg3,
	void* arg4 )
{
	xClient::OnChannelEvent( whichEvent, theChan,
		arg1, arg2, arg3, arg4 ) ;
}

void cloner::OnEvent( const eventType& whichEvent,
	void* arg1,
	void* arg2,
	void* arg3,
	void* arg4 )
{

assert( whichEvent <= EVT_CREATE ) ;


// NEVER uncomment this line on a large network heh
elog	<< "stats::OnEvent> Event number: "
	<< whichEvent
	<< endl ;

xClient::OnEvent( whichEvent, arg1, arg2, arg3, arg4 ) ;
}

void cloner::OnChannelMessage( iClient* theClient, Channel* theChan, const std::string& Message)
{
	xClient::OnChannelMessage(theClient, theChan, Message);
	Notice( theClient, "Howdy :)" ) ;
//	return xClient::OnChannelMessage(theClient, theChan, Message);
}

// Burst any channels.
void cloner::BurstChannels()
{
	xClient::BurstChannels() ;
	Join( "#coder-com" ) ;
	MyUplink->RegisterChannelEvent("#coder-com",this);
}

void cloner::OnTimer( const xServer::timerID&, void* )
{
//elog	<< "cloner::OnTimer> makeCloneCount: "
//	<< makeCloneCount
//	<< endl ;

if( 0 == makeCloneCount )
	{
	return ;
	}

size_t cloneCount = makeCloneCount ;
if( cloneCount > cloneBurstCount )
	{
	// Too many
	cloneCount = cloneBurstCount ;
	}

makeCloneCount -= cloneCount ;

//elog	<< "cloner::OnTimer> cloneCount: "
//	<< cloneCount
//	<< endl ;

for( size_t i = 0 ; i < cloneCount ; ++i )
	{
	addClone() ;
	}

if( makeCloneCount > 0 )
	{
	MyUplink->RegisterTimer( ::time( 0 ) + 1, this, 0 ) ;
	}
}

void cloner::addClone()
{
// The XXX doesn't matter here, the core will choose an
// appropriate value.
string yyxxx( fakeServer->getCharYY() + "]]]" ) ;

iClient* newClient = new iClient(
		fakeServer->getIntYY(),
		yyxxx,
		randomNick( minNickLength, maxNickLength ),
		randomUser(),
		randomNick( 6, 6 ),
		randomHost(),
		randomHost(),
		cloneMode,
		string(),
		0,
		cloneDescription,
		::time( 0 ) ) ;
assert( newClient != 0 );

if( MyUplink->AttachClient( newClient, this ) )
	{
	clones.push_back( newClient ) ;
	}
}

string cloner::randomUser()
{
return userNames[ rand() % userNames.size() ] ;
}

string cloner::randomHost()
{
return hostNames[ rand() % hostNames.size() ] ;
}

string cloner::randomNick( int minLength, int maxLength )
{
string retMe ;

// Generate a random number between minLength and maxLength
// This will be the length of the nickname
int randomLength = minLength + (rand() % (maxLength - minLength + 1) ) ;

for( int i = 0 ; i < randomLength ; i++ )
        {
        retMe += randomChar() ;
        }

//elog << "randomNick: " << retMe << endl ;
return retMe ;
}

// ascii [65,122]
char cloner::randomChar()
{
char c = ('A' + (rand() % ('z' - 'A')) ) ;
//elog << "char: returning: " << c << endl ;
return c ;

//return( (65 + (rand() % 122) ) ;
//return (char) (1 + (int) (9.0 * rand() / (RAND_MAX + 1.0) ) ) ;
}

bool cloner::hasAccess( const string& accountName ) const
{
for( std::list< string >::const_iterator itr = allowAccess.begin() ;
	itr != allowAccess.end() ; ++itr )
	{
	if( !strcasecmp( accountName, *itr ) )
		{
		return true ;
		}
	}
return false ;
}

} // namespace gnuworld
