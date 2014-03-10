/**
 * snoop.cc
 */

#include	<string>
#include	<sstream>
#include	<iostream>

#include	<cctype>

#include	"server.h"
#include	"client.h"
#include	"snoop.h"
#include	"Channel.h"
#include	"iClient.h"
#include	"Network.h"
#include	"EConfig.h"
#include	"StringTokenizer.h"
#include	"misc.h"

namespace gnuworld
{

using std::endl ;
using std::string ;
using std::stringstream ;

extern "C"
{
xClient* _gnuwinit( const string& args )
	{
	return new snoop( args ) ;
	}
}

snoop::snoop( const string& confFileName )
 : xClient( confFileName )
{
	EConfig conf( confFileName ) ;
	cmdchar = conf.Require( "cmdchar" )->second ;
	adminChanName = conf.Require( "adminchan" )->second ;
	relayChanName = conf.Require( "relaychan" )->second ;
	defaultQuitMessage = conf.Require( "defaultquitmessage" )->second ;
	maxnicklen = ::atoi( conf.Require( "maxnicklen" )->second.c_str() ) ;

	EConfig::const_iterator ptr = conf.Find( "permit_user" ) ;
	while( ptr != conf.end() && ptr->first == "permit_user" )
	{
		allowAccess.push_back( ptr->second ) ;
		++ptr ;
	}

	ptr = conf.Find( "joinchan" ) ;
	while( ptr != conf.end() && ptr->first == "joinchan" )
	{
		joinchans.push_back( ptr->second ) ;
		++ptr ;
	}
	ptr = conf.Find( "address" ) ;
	while( ptr != conf.end() && ptr->first == "address" )
	{
		addresses.push_back( ptr->second ) ;
		++ptr ;
	}
}

snoop::~snoop()
{}

void snoop::BurstChannels()
{
xClient::BurstChannels() ;

// It's ok if admin and relay chans are the same,
// xServer::JoinChannel() will not join more than once
Join( adminChanName ) ;
Join( relayChanName ) ;
//MyUplink->RegisterTimer( ::time( 0 ) + 1, this ) ;
for (std::vector<string>::const_iterator addr = addresses.begin(); addr < addresses.end(); addr++)
{
	StringTokenizer straddr(*addr);
	string nick = straddr[0];
	string user = straddr[1];
	string host = straddr[2];
	string real = straddr.assemble(3);
	string fulladdr = nick + '!' + user + '@' +host;

	elog    << "snoop::handleSpawnClient> "
			<< fulladdr
			<< endl ;

	if ((validNickname(nick)) && ( Network->findNick( nick ) == 0 ))
	{
		char newCharYY[ 6 ] ;
		newCharYY[ 2 ] = 0 ;
		inttobase64( newCharYY, MyUplink->getIntYY(), 2 ) ;

		iClient* newClient = new (std::nothrow) iClient(
			MyUplink->getIntYY(), // intYY
			newCharYY, // charYYXXX
		        nick,
			user,
			"AAAAAA", // host base 64
			host,
			"realInsecureHost.com", // realInsecureHost
			"+i", // mode
			string(), // account
			0, // account_ts
			real,
			31337 // connect time
			) ;
		assert( newClient != 0 ) ;

		if( !getUplink()->AttachClient( newClient, this ) )
			{
			elog    << "snoop::handleSpawnClient> Failed to add new client: "
				<< *newClient
				<< endl ;

			//Notice( srcClient, "Failed to create new fake client" ) ;
			delete newClient ; newClient = 0 ;
			continue;
			}

		// Each relay (spawn) client must join the relay chan
		getUplink()->JoinChannel( newClient, relayChanName );
		clones.push_back( newClient ) ;
	}
	joinAll();
}
}

void snoop::OnChannelMessage( iClient* srcClient,
	Channel* theChan,
	const string& Message )
{
if( strcasecmp( theChan->getName(), adminChanName ) )
	{
	// Not the admin chan, ignore
	return ;
	}

bool userHasAccess = hasAccess( srcClient->getAccount() ) ;
if ((!srcClient->isOper()) && (!userHasAccess))
{
	return ;
}

StringTokenizer st( Message ) ;
if( st.empty() || st[ 0 ] != cmdchar )
	{
	return ;
	}

elog	<< "snoop::OnChannelMessage> Received chan message: "
	<< st.assemble( 0 )
	<< endl ;

if( st.size() < 2 )
	{
	return ;
	}

string command( st[ 1 ] ) ;
string_lower( command ) ;

if( command == "spawnclient" )
	{
	handleSpawnClient( srcClient, theChan, st ) ;
	}
else if( command == "join" )
	{
	handleSpawnJoin( srcClient, theChan, st ) ;
	}
else if( command == "joinall" )
	{
	if (st.size() < 3)
		joinAll();
	else
		joinAll( srcClient, theChan, st ) ;
	}
else if( command == "part" )
	{
	handleSpawnPart( srcClient, theChan, st ) ;
	}
else if( command == "partall" )
	{
	partAll( srcClient, theChan, st ) ;
	}
else if( command == "quit" )
	{
	handleSpawnQuit( srcClient, st ) ;
	}
else if( command == "quitall" )
	{
	quitAll(srcClient, st ) ;
	}
else if( (command == "say") || (command == "do") )
	{
	say(srcClient,st);
	}
else if( (command == "sayall") || (command == "doall") )
	{
	sayAll(srcClient,st);
	}
else if( command == "reload" )
	{
	getUplink()->UnloadClient( this,
		"Something has changed in the matrix..." ) ;
	getUplink()->LoadClient( "libsnoop.la",
		getConfigFileName() ) ;
	}
else if( command == "shutdown" )
	{
	getUplink()->Shutdown() ;
	}
}

void snoop::handleSpawnClient( iClient* srcClient,
	Channel* theChan,
	const StringTokenizer& st )
{
// snoop spawnclient nick!user@host realname
if( st.size() < 4 )
	{
	usage( srcClient, "spawnclient" ) ;
	return ;
	}

const string realname( st.assemble( 3 ) ) ;

//elog	<< "snoop::OnChannelMessage> nickTokens, st[ 2 ]: "
//	<< st[ 2 ]
//	<< endl ;

// st[ 2 ] is nick!user@host:realname
StringTokenizer nickTokens( st[ 2 ], '!' ) ;
if( nickTokens.size() != 2 )
	{
	usage( srcClient, "spawnclient" ) ;
	return ;
	}

StringTokenizer userTokens( nickTokens[ 1 ], '@' ) ;
if( userTokens.size() != 2 )
	{
	usage( srcClient, "spawnclient" ) ;
	return ;
	}

const string nickname( nickTokens[ 0 ] ) ;
const string username( userTokens[ 0 ] ) ;
const string hostname( userTokens[ 1 ] ) ;

if( nickname.empty() || username.empty() || hostname.empty() )
	{
	Notice( srcClient, "Please specify non-empty nick/user/host names" ) ;
	return ;
	}

// Verify that the nickname is valid
if( !validNickname( nickname ) )
	{
	Notice( srcClient, "Invalid nickname" ) ;
	return ;
	}

if( Network->findNick( nickname ) != 0 )
	{
	Notice( srcClient, "SPAWNCLIENT: Nickname %s already exists",
		nickname.c_str() ) ;//elog << "cloner::OnPrivateMessage> " << Message << endl ;
	return ;
	}

if( string::npos == hostname.find( '.' ) )
	{
	Notice( srcClient, "SPAWNCLIENT: Hostname must appear valid" ) ;
	return ;
	}

char newCharYY[ 6 ] ;
newCharYY[ 2 ] = 0 ;
inttobase64( newCharYY, MyUplink->getIntYY(), 2 ) ;

iClient* newClient = new (std::nothrow) iClient(
	MyUplink->getIntYY(), // intYY
	newCharYY, // charYYXXX
        nickname,
	username,
	"AAAAAA", // host base 64
	hostname,
	"realInsecureHost.com", // realInsecureHost
	"+i", // mode
	string(), // account
	0, // account_ts
	realname,
	31337 // connect time
	) ;
assert( newClient != 0 ) ;

if( !getUplink()->AttachClient( newClient, this ) )
	{
	elog    << "snoop::handleSpawnClient> Failed to add new client: "
		<< *newClient
		<< endl ;

	Notice( srcClient, "Failed to create new fake client" ) ;
	delete newClient ; newClient = 0 ;
	}

// Each relay (spawn) client must join the relay chan
if( !getUplink()->JoinChannel( newClient, relayChanName ) )
	{
	Notice( srcClient, "Unable to make \'%s\' join channel "
		"\'%s\', killing off client",
		st[ 2 ].c_str(),
		st[ 3 ].c_str() ) ;

	getUplink()->DetachClient( newClient ) ;
	delete newClient ; newClient = 0 ;
	return ;
	}
clones.push_back( newClient ) ;
Message( theChan, "Successfully created nick %s and joined "
	"relay channel %s",
	nickname.c_str(),
	relayChanName.c_str() ) ;
}

void snoop::handleSpawnJoin( iClient* srcClient,
	Channel* theChan,
	const StringTokenizer& st )
{
// snoopy spawnjoin <nick> <chan>
if( st.size() != 4 )
	{
	usage( srcClient, "join" ) ;
	return ;
	}

// Find the client
iClient* fakeClient = Network->findNick( st[ 2 ] ) ;
if( 0 == fakeClient )
	{
	Notice( srcClient, "Nick \'%s\' does not exist",
		st[ 2 ].c_str() ) ;
	return ;
	}

// Verify that it is a fake client, and owned by this module
xClient* ownerClient = Network->findFakeClientOwner( fakeClient ) ;
if( ownerClient != this )
	{
	Notice( srcClient, "I don't own that client!" ) ;
	return ;
	}

// fakeClient is valid, and owned by this module

if( !getUplink()->JoinChannel( fakeClient, st[ 3 ] ) )
	{
	Notice( srcClient, "Unable to make \'%s\' join channel "
		"%s",
		st[ 2 ].c_str(),
		st[ 3 ].c_str() ) ;
	}
else
	{
	Message( theChan, "%s successfully joined %s",
		st[ 2 ].c_str(),
		st[ 3 ].c_str() ) ;
	}
}

void snoop::joinAll()
{
	if (joinchans.empty()) return;
	for (std::vector<string>::const_iterator jchan = joinchans.begin(); jchan < joinchans.end(); jchan++)
		for( std::list< iClient* >::const_iterator ptr = clones.begin(),
			endPtr = clones.end() ; ptr != endPtr ; ++ptr )
			{
			//if( !getUplink()->JoinChannel( *ptr, relayChanName ) )	return;
			getUplink()->JoinChannel( *ptr, *jchan );
			}
}

void snoop::joinAll(iClient* srcClient, Channel* theChan, const StringTokenizer& st)
{
	// snoopy joinall <chan>
	if( st.size() != 3 )
	{
		usage( srcClient, "joinall" ) ;
		return ;
	}

	string chanName = st[2]; //st.assemble(2); //( st[ 0 ] ) ;
	if( chanName[ 0 ] != '#' )
	{
		chanName.insert( chanName.begin(), '#' ) ;
	}

	for( std::list< iClient* >::const_iterator ptr = clones.begin(),
		endPtr = clones.end() ; ptr != endPtr ; ++ptr )
	{
		if( !getUplink()->JoinChannel( *ptr, st.assemble(2) ) )
		{
			Message(theChan, "%s unable to join channel %s",(*ptr)->getNickName().c_str(),chanName.c_str());
			//Notice( srcClient, "Unable to make \'%s\' join channel "
			//	"\'%s\', killing off client",
			//	chanName.c_str(),
			//	(*ptr)->getNickName().c_str() ) ;
			//getUplink()->DetachClient( *ptr ) ;
			//delete (*ptr); (*ptr) = 0;
			//delete newClient ; newClient = 0 ;
			//return ;
		}
	}
}

void snoop::handleSpawnPart( iClient* srcClient,
	Channel* theChan,
	const StringTokenizer& st )
{
// snoopy part <nick> <chan> [message]
if( st.size() < 4 )
	{
	usage( srcClient, "part" ) ;
	return ;
	}

// Find the client
iClient* fakeClient = Network->findNick( st[ 2 ] ) ;
if( 0 == fakeClient )
	{
	Notice( srcClient, "Nick \'%s\' does not exist",
		st[ 2 ].c_str() ) ;
	return ;
	}

// Verify that it is a fake client, and owned by this module
xClient* ownerClient = Network->findFakeClientOwner( fakeClient ) ;
if( ownerClient != this )
	{
	Notice( srcClient, "I don't own that client!" ) ;
	return ;
	}

// fakeClient is valid, and owned by this module

getUplink()->PartChannel( fakeClient, st.assemble(3) ) ; //st[3]
Message( theChan, "%s successfully parted %s",
	st[ 2 ].c_str(),
	st[ 3 ].c_str() ) ;
}

void snoop::partAll(iClient* srcClient, Channel* theChan, const StringTokenizer& st)
{
	// snoopy partall <chan> [message]
	if( st.size() < 3 )
	{
		usage( srcClient, "partall" ) ;
		return ;
	}

	for( std::list< iClient* >::const_iterator ptr = clones.begin(),
		endPtr = clones.end() ; ptr != endPtr ; ++ptr )
	{
	// Find the client
	iClient* fakeClient = Network->findFakeClient(*ptr); //->findNick( st[ 2 ] ) ;
	if( 0 == fakeClient )
		{
		Notice( srcClient, "Nick \'%s\' does not exist",
			st[ 2 ].c_str() ) ;
		return ;
		}

	// Verify that it is a fake client, and owned by this module
	xClient* ownerClient = Network->findFakeClientOwner( fakeClient ) ;
	if( ownerClient != this )
		{
		Notice( srcClient, "I don't own that client!" ) ;
		return ;
		}

	// fakeClient is valid, and owned by this module

	getUplink()->PartChannel( fakeClient, st.assemble(2) ) ; //st[2]
	//Message( theChan, "%s successfully parted %s",	st[ 2 ].c_str(), st[ 3 ].c_str() ) ;
	}
	Message(theChan, "Successfully parted all channels");
}

void snoop::usage( iClient* srcClient,
	const string& command )
{
if( command == "spawnclient" )
	{
	Notice( srcClient, "Usage: spawnclient <nick!user@host <realname>" ) ;
	}
else if( command == "join" )
	{
	Notice( srcClient, "Usage: join <nick> <chan> [key]" ) ;
	}
else if( command == "joinall" )
	{
	Notice( srcClient, "Usage: joinall <chan> [key]" ) ;
	}
else if( command == "part" )
	{
	Notice( srcClient, "Usage: part <nick> <chan> [message]" ) ;
	}
else if( command == "partall" )
	{
	Notice( srcClient, "Usage: partall <chan> [message]" ) ;
	}
else if( command == "quit" )
	{
	Notice( srcClient, "Usage: quit <nick> [quit message]" ) ;
	}
else if( command == "quitall" )
	{
	Notice( srcClient, "Usage: quitall [quit message]" ) ;
	}
else if( command == "say" )
	{
	Notice( srcClient, "Usage: say <nick> <chan|nick> <message>" ) ;
	}
else if( command == "sayall" )
	{
	Notice( srcClient, "Usage: sayall <chan|nick> <message>" ) ;
	}
else if( command == "do" )
	{
	Notice( srcClient, "Usage: do <nick> <chan> <message>" ) ;
	}
else if( command == "doall" )
	{
	Notice( srcClient, "Usage: doall <chan> <message>" ) ;
	}
}
/*	//From now on, snooping is not our primary task
void snoop::OnFakeChannelMessage( iClient* srcClient,
	iClient* fakeClient,
	Channel* theChan,
	const string& Message )
{
// A fake client received a channel message

// Ignore if it's the admin or relay channel
if( !strcasecmp( theChan->getName(), adminChanName ) ||
	!strcasecmp( theChan->getName(), relayChanName ) )
	{
	return ;
	}

// For now simply relay to the relay chan without checking for
// state, etc
Channel* relayChan = Network->findChannel( relayChanName ) ;
if( 0 == relayChan )
	{
	elog	<< "snoop::OnFakeChannelMessage> Fake client: "
		<< *fakeClient
		<< " unable to find relay chan: "
		<< relayChanName
		<< endl ;
	return ;
	}

stringstream s ;
s	<< "[RELAY "
	<< theChan->getName()
	<< "] "
	<< srcClient->getNickName()
	<< "!"
	<< srcClient->getUserName()
	<< "@"
	<< srcClient->getInsecureHost()
	<< ": "
	<< Message ;

FakeMessage( relayChan,
	fakeClient,
	s.str().c_str() ) ;
}
*/
void snoop::handleSpawnQuit( iClient* srcClient,
	const StringTokenizer& st )
{
// snoopy quit <nick> [message]
if( st.size() < 3 )
	{
	usage( srcClient, "quit" ) ;
	return ;
	}

string quitMessage( defaultQuitMessage ) ;
if( st.size() >= 4 )
	{
	quitMessage = st.assemble( 3 ) ;
	}

// Find the client
iClient* fakeClient = Network->findFakeNick( st[ 2 ] ) ;
if( 0 == fakeClient )
	{
	Notice( srcClient, "Nick \'%s\' does not exist",
		st[ 2 ].c_str() ) ;
	return ;
	}

// Verify that it is a fake client, and owned by this module
xClient* ownerClient = Network->findFakeClientOwner( fakeClient ) ;
if( ownerClient != this )
	{
	Notice( srcClient, "I don't own that client!" ) ;
	return ;
	}

getUplink()->DetachClient( fakeClient, quitMessage ) ;
Notice( srcClient, "%s detached",
	st[ 2 ].c_str() ) ;

// This module allocated the iClient, and it must therefore deallocate
// it.
delete fakeClient ; fakeClient = 0 ;
}

void snoop::quitAll(iClient* srcClient, const StringTokenizer& st)
{
	// snoopy quitall [message]
	if( st.size() < 3 )
	{
		usage( srcClient, "quitall" ) ;
		return ;
	}

	string quitMessage( defaultQuitMessage ) ;
	if( st.size() >= 3 )
	{
		quitMessage = st.assemble( 2 ) ;
	}

	//std::vector<iClient*> toDie;
	for( std::list< iClient* >::const_iterator ptr = clones.begin(),
		endPtr = clones.end() ; ptr != endPtr ; ++ptr )
	{
	// Find the client
	iClient* fakeClient = Network->findFakeClient(*ptr); //->findFakeNick( st[ 2 ] ) ;
	if( 0 == fakeClient )
		{
		Notice( srcClient, "Nick \'%s\' does not exist",
			st[ 2 ].c_str() ) ;
		return ;
		}

	// Verify that it is a fake client, and owned by this module
	xClient* ownerClient = Network->findFakeClientOwner( fakeClient ) ;
	if( ownerClient != this )
		{
		Notice( srcClient, "I don't own that client!" ) ;
		return ;
		}

	getUplink()->DetachClient( fakeClient, quitMessage ) ;
	//Notice( srcClient, "%s detached", st[ 2 ].c_str() ) ;

	// This module allocated the iClient, and it must therefore deallocate
	// it.
	//toDie.push_back(*ptr);
	delete fakeClient ; fakeClient = 0 ;
	}
	clones.clear();
}

void snoop::say(iClient* srcClient, const StringTokenizer& st)
{
	// snoopy say <fakenick> <channel|nick> [message]
	if( st.size() < 5 )
	{
		if (st[1] == "say") usage( srcClient, "say" ) ;
		if (st[1] == "do" ) usage( srcClient, "do" ) ;
		return ;
	}
	string chanOrNickName( st[ 3 ] ) ;
	string privMsg( st.assemble(4).c_str() ) ;

        if( chanOrNickName[ 0 ] != '#' )
                { // Assume nickname
                iClient* Target = Network->findNick( st[ 3 ] ) ;
                if( NULL == Target )
                        {
                        Notice( srcClient, "Unable to find nick: %s"
	                         , st[ 3 ].c_str() ) ;
	                return ;
	                }
		chanOrNickName = Target->getCharYYXXX();
	        }

    	iClient* fakeClient = Network->findFakeNick( st[ 2 ] ) ;
    	if( 0 == fakeClient )
    		{
    		Notice( srcClient, "Nick \'%s\' does not exist",
    			st[ 2 ].c_str() ) ;
    		return ;
    		}
    	// Verify that it is a fake client, and owned by this module
    	xClient* ownerClient = Network->findFakeClientOwner( fakeClient ) ;
    	if( ownerClient != this )
    		{
    		Notice( srcClient, "I don't own that client!" ) ;
    		return ;
    		}
		stringstream s ;
		s	<< fakeClient->getCharYYXXX()
			<< " P "
			<< chanOrNickName
			<< " :"
			<< privMsg ;

		MyUplink->Write( s ) ;
}

void snoop::sayAll(iClient* srcClient, const StringTokenizer& st)
{
	// snoopy say <chan|nick> [message]
	if( st.size() < 4 )
	{
		if (st[1] == "sayall") usage( srcClient, "sayall" ) ;
		if (st[1] == "doall" ) usage( srcClient, "doall" ) ;
		return ;
	}
	string chanOrNickName( st[ 2 ] ) ;
	string privMsg( st.assemble(3).c_str() ) ;

        if( chanOrNickName[ 0 ] != '#' )
                { // Assume nickname
                iClient* Target = Network->findNick( st[ 2 ] ) ;
                if( NULL == Target )
                        {
                        Notice( srcClient, "Unable to find nick: %s"
	                         , st[ 2 ].c_str() ) ;
	                return ;
	                }
		chanOrNickName = Target->getCharYYXXX();
	        }

	for( std::list< iClient* >::const_iterator ptr = clones.begin(),
		endPtr = clones.end() ; ptr != endPtr ; ++ptr )
		{
		stringstream s ;
		s	<< (*ptr)->getCharYYXXX()
			<< " P "
			<< chanOrNickName
			<< " :"
			<< privMsg ;

		MyUplink->Write( s ) ;
		}
}

bool snoop::validNickname( const string& nickname ) const
{
if( nickname.empty() || nickname.size() > maxnicklen )
	{
	return false ;
	}

/*
 * From ircu:
 * Nickname characters are in range 'A'..'}', '_', '-', '0'..'9'
 *  anything outside the above set will terminate nickname.
 * In addition, the first character cannot be '-' or a Digit.
 */
if( isdigit( nickname[ 0 ] ) )
	{
	return false ;
	}

for( string::const_iterator sItr = nickname.begin() ;
	sItr != nickname.end() ; ++sItr )
	{
	if( *sItr >= 'A' && *sItr <= '}' )
		{
		// ok
		continue ;
		}
	if( '_' == *sItr || '-' == *sItr )
		{
		// ok
		continue ;
		}
	if( *sItr >= '0' && *sItr <= '9' )
		{
		// ok
		continue ;
		}
	// bad char
	return false ;
	}
return true ;
}

bool snoop::hasAccess( const string& accountName ) const
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

