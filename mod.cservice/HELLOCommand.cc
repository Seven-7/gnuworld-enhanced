/* HELLOCommand.cc */

#include	<string>

#include	"cservice_config.h"
#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"Network.h"
#include	"ip.h"
#include	"levels.h"
#include	"dbHandle.h"
#include	"cservice.h"

const char HELLOCommand_cc_rcsId[] = "$Id: HELLOCommand.cc,v 1.1 2005/04/03 22:11:42 dan_karrels Exp $" ;

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

bool HELLOCommand::Exec( iClient* theClient, const string& Message )
{
#ifdef ALLOW_HELLO

sqlUser* theUser = bot->isAuthed(theClient, false);
int admLevel = 0;
if (theUser)
{
	admLevel = bot->getAdminAccessLevel(theUser);
	if (admLevel == 0)
	{
	#ifndef ALLOW_USERS_HELLO
		bot->Notice(theClient, "HELLO command is disabled. Use webinterface instead (if available)");
		return true;
	#endif
		bot->Notice(theClient, "You can't create another "
			"account when you already have one!");
	        return false;
        }
}
else
{
#ifndef ALLOW_USERS_HELLO
	bot->Notice(theClient, "HELLO command is disabled. Use webinterface instead (if available)");
	return true;
#endif
}

bot->incStat("COMMANDS.HELLO");

StringTokenizer st( Message ) ;
if( st.size() < 6 )
	{
	Usage(theClient);
	bot->Notice(theClient,"Valid verification answer numbers are: 1 to 3");
	bot->Notice(theClient,"1: What's your mother's maiden name ?");
	bot->Notice(theClient,"2: What's your dog's(or cat's) name ?");
	bot->Notice(theClient,"3: What's your father's birth date ?");
	return true;
	}

const char validChars[] 
	= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/*
 * Check this IP hasn't already tried in the last 48 hours.
 */
cservice::helloIPListType::iterator itr = bot->helloIPList.find( theClient->getIP() ) ;

if (admLevel == 0) //(&& !theUser)) //only normal users has the one IP/user restriction, any Admin don't
if (itr != bot->helloIPList.end())
{
	/*
	 * Has it been 48hours already?
	 */
	if (bot->currentTime() < itr->second)
	{
		bot->Notice(theClient, "Sorry, an account has "
			"already been registered from this IP within "
			"the last %i hours. "
			"Please wait and try again, or contact cservice.", 
			bot->helloBlockPeriod / 3600);

		return false;
	}
}

sqlUser* newUser = bot->getUserRecord(st[1].c_str());
if(newUser)
	{
	bot->Notice(theClient, "This username already exists!");
	return false;
	}

/*
 * Check the username contains valid characters/correct 
length.
 */
if (  (st[1].size() < 2) || (st[1].size() > 12)  )
	{
	bot->Notice(theClient, "Your username must be 2 to 12 chars long.");
	return false;
	}

string theUserName = st[1];
bool badName = false;

for( string::const_iterator ptr = theUserName.begin() ;
	ptr != theUserName.end() ; ++ptr )
	{
	/*
	 * 62 entries in the table. 26 + 26 + 10 digits.
	 */

	bool found = false;
	for (int f = 0; f < 62; f++)
		{
		if(*ptr == validChars[f])
			{
			found = true;
			}
		}
	if (!found)
		{
		badName = true;
		}
	}

if (badName)
	{
	bot->Notice(theClient, "Your useraname must be made of letters (A-Z, a-z) and numbers (0-9).");
	return false;
	}

/*
 * Do some basic validation of the email address.
 */

if (st[2].size() > 128)
	{
	bot->Notice(theClient, "E-mail address is too long.");
	return false;
	}

if( string::npos == st[2].find_first_of('@') )
	{
	/*
	 * No @?
	 */
	bot->Notice(theClient, "Invalid e-mail address.");
	return false;
	}

/*
 * Check if they've at least vaugly attempted to specify a 
 * proper domain structure
 * and we have the right number of @'s.
 */
StringTokenizer emailCheck( st[2], '@' ) ;

if (  (emailCheck.size() != 2) ||
	(string::npos == emailCheck[1].find_first_of('.')) )
	{
	bot->Notice(theClient, "Invalid e-mail address.");
	return false;
	}

if(strcasecmp(st[2], st[3]))
	{
        bot->Notice(theClient, "E-mail addresses don't match!");
        return false;
	}

if (admLevel < level::hello)
{
/*
 * Check this email address doesn't already exist in the database!
 */
stringstream theQuery;
theQuery	<< "SELECT email FROM users WHERE email IS NOT NULL" 
		<< ends;
if (!bot->SQLDb->Exec(theQuery, true))
{	bot->logDebugMessage("Error on HELLO.EmailQuery");
#ifdef LOG_SQL
	//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
	elog << "Hello.EmailQuery> SQL Error: " 
	     << bot->SQLDb->ErrorMessage() 
	     << endl ; 
#endif
	return false;
} else if (bot->SQLDb->Tuples() != 0) 
{ 
	for (unsigned int i = 0 ; i < bot->SQLDb->Tuples(); i++) 
		if (!match(bot->SQLDb->GetValue(i,0),st[2]))
		{
			bot->Notice(theClient,"There is already an account registered with that email address.");
			bot->Notice(theClient,"You can only have one account per person!");
			if (bot->helloSendmailEnabled)
				bot->Notice(theClient, "If you have lost your password and require a new one, request a password reset with /msg %s@%s NEWPASS <username>", bot->getNickName().c_str(), bot->getUplinkName().c_str());
			else
				bot->Notice(theClient,"If you have lost your password and require a new one, log in to webinterface and click on the New password link.");
			return false;
		}
}
} //admLevel
unsigned short verifID = atoi(st[4].c_str());

if ((verifID < 1) || (verifID > 3)) 
{
	bot->Notice(theClient,"Valid verification answer numbers are: 1 to 3");
	bot->Notice(theClient,"1: What's your mother's maiden name ?");
	bot->Notice(theClient,"2: What's your dog's(or cat's) name ?");
	bot->Notice(theClient,"3: What's your father's birth date ?");
	return false;
}

string verifAnswer = st.assemble(5);

if ((verifAnswer.size() < 4) || (verifAnswer.size() > 300)) {
	bot->Notice(theClient,"Your verification answer must be 4 - 300 charcters long.");
	return false;
}

/*
 * Check if user_name,email,verification answer is in NOREG/LOCKED
 */
if (admLevel < level::hello)
{
stringstream theQuery;
theQuery	<< "SELECT user_name,email,type,reason FROM noreg WHERE email IS NOT NULL OR user_name IS NOT NULL" 
		<< ends;
if (!bot->SQLDb->Exec(theQuery, true))
{	bot->logDebugMessage("Error on HELLO.NoregEmailQuery");
#ifdef LOG_SQL
	//elog << "sqlQuery> " << theQuery.str().c_str() << endl;
	elog << "Hello.NoregEmailQuery> SQL Error: " 
	     << bot->SQLDb->ErrorMessage() 
	     << endl ; 
#endif
	return false;
} else if (bot->SQLDb->Tuples() != 0) 
{ 
	unsigned short type;
	string user_name,email;
	string reason; 
	for (unsigned int i = 0 ; i < bot->SQLDb->Tuples(); i++) 
	{
		user_name = bot->SQLDb->GetValue(i,0);
		email = bot->SQLDb->GetValue(i,1);
		type = atoi(bot->SQLDb->GetValue(i,2));
		reason = bot->SQLDb->GetValue(i,3);
		if ((user_name.size() > 0) && (user_name != "*")) 
		{
			if ((type < 6) && (!match(user_name,st[1]))) 
			{
				if (type < 4) bot->Notice(theClient,"Invalid username (NOREG)");
				if (type == 4) bot->Notice(theClient,"Invalid username (FRAUD)"); //this is set automatically when for a user is set the F_FRAUD flag on the webinterface
				if (type == 5) bot->Notice(theClient,"Invalid username (LOCKED)");
				bot->Notice(theClient,"Usernames matching %s are disallowed for the following reason:",user_name.c_str());
				bot->Notice(theClient,reason.c_str());
				return false;
			}				
			if (type == 6) 
			{	//* This should be the matchcase - TODO: need to find a solution */
				if (user_name[0] == '!') user_name.erase(0,1);
				if (!match(user_name,verifAnswer))
				{
					bot->Notice(theClient,"Invalid verification answer (LOCKED)");
					bot->Notice(theClient,"Verification answers matching %s are disallowed for the following reason:",user_name.c_str());
					bot->Notice(theClient,reason.c_str());
					return false;
				}
			}
		}
		if (((email.size() > 0) && (email != "*")) && (!match(email,st[2])))
		{
			if (type < 4) bot->Notice(theClient,"Invalid email address (NOREG)");
			if (type == 5) bot->Notice(theClient,"Invalid email address (LOCKED)");
			bot->Notice(theClient,"Email addresses matching %s are disallowed for the following reason:",email.c_str());
			bot->Notice(theClient,reason.c_str());
			return false;
		}
	}
}

/*
bot->helloIPList.erase(theClient->getIP());
bot->helloIPList.insert(
	std::make_pair(theClient->getIP(),
		bot->currentTime() + bot->helloBlockPeriod) );
*/
} //admLevel

//bot->Notice(theClient,"Username %s created successfully.",st[1].c_str());
//return true;

/*
 * We need to give this user a password
 */
string plainpass;

for ( unsigned short int i = 0 ; i < 8 ; i++ )
        {
        int randNo = 1+(int) (62.0*rand()/(RAND_MAX+1.0));
        plainpass += validChars[randNo];
        }

string cryptpass = bot->CryptPass(plainpass);
string updatedBy = "HELLO used by: ";
updatedBy += theClient->getNickUserHost().c_str();

newUser = new (std::nothrow) sqlUser(bot->SQLDb);
newUser->setUserName(escapeSQLChars(st[1].c_str()));
newUser->setEmail(escapeSQLChars(st[2]));
newUser->setVerifNr(verifID);
newUser->setVerifData(verifAnswer);
newUser->setPassword(cryptpass.c_str());
newUser->setLastUpdatedBy(updatedBy);
newUser->setCreatedTS(bot->currentTime());
newUser->setInstantiatedTS(bot->currentTime());
newUser->setSignupIp(xIP(theClient->getIP()).GetNumericIP());
newUser->Insert();
delete (newUser);

if (bot->helloSendmailEnabled)
{
	string themail = escapeSQLChars(st[2]);
	stringstream mailstream;
	mailstream << "The generated password is: " << plainpass << endl;
	mailstream << "Login via IRC using /msg " << bot->getNickName().c_str() << "@" << bot->getUplinkName().c_str() << " LOGIN " << st[1].c_str() << " " << plainpass.c_str() << endl;
	mailstream << "Then change your password using /msg " << bot->getNickName().c_str() << "@" << bot->getUplinkName().c_str() << " NEWPASS <new_password>" << endl;
	mailstream << std::ends;
	if (!bot->SendMail(themail, "Your CService account", mailstream))
	{
		bot->Notice(theClient, "An error occurred while sending newuser email, contact a CService representative.");
		return false;
	}
	else
	{
		bot->Notice(theClient, "Username %s created successfully.", st[1].c_str());
		bot->Notice(theClient, "Check your email (also in junk/spam) for the generated password.");
	}
}
else
{
	bot->Notice(theClient, "I generated this password for you: \002%s\002",
		plainpass.c_str());
	bot->Notice(theClient, "Login using \002/msg %s@%s LOGIN %s %s\002",
		bot->getNickName().c_str(),
		bot->getUplinkName().c_str(),
		st[1].c_str(),
		plainpass.c_str());
	bot->Notice(theClient, "Then change your password using \002/msg "
		"%s@%s NEWPASS <new_password>\002",
		bot->getNickName().c_str(),
		bot->getUplinkName().c_str());
}

//Record the IP only if below the required level
if (admLevel < level::hello)
{
bot->helloIPList.erase(theClient->getIP());
bot->helloIPList.insert(
	std::make_pair(theClient->getIP(),
		bot->currentTime() + bot->helloBlockPeriod) );
}

#endif //ALLOW_HELLO
return true ;
}

} // namespace gnuworld.
