/**
 * NEWPASSCommand.cc
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
 * $Id: NEWPASSCommand.cc,v 1.17 2005/11/17 22:20:57 kewlio Exp $
 */

#include	<string>
#include	<sstream>
#include	<iostream>
#include	<iomanip>
#include	<inttypes.h>

#include	"md5hash.h"
#include	"StringTokenizer.h"
#include	"ELog.h"
#include	"cservice.h"
#include	"responses.h"
#include	"networkData.h"
#include	"cservice_config.h"
#include	"levels.h"

const char NEWPASSCommand_cc_rcsId[] = "$Id: NEWPASSCommand.cc,v 1.17 2005/11/17 22:20:57 kewlio Exp $" ;

namespace gnuworld
{
using std::string ;
using std::endl ;
using std::ends ;
using std::stringstream ;

const char validChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.$*_";

bool NEWPASSCommand::Exec( iClient* theClient, const string& Message )
{
bot->incStat("COMMANDS.NEWPASS");

#ifndef USE_NEWPASS
bot->Notice(theClient, "To change your account password, please use the web interface.");
return true;
#else

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

sqlUser* tmpUser = bot->isAuthed(theClient, false);
sqlUser* targetUser = tmpUser;
if (!tmpUser)
{
	if (bot->helloSendmailEnabled)
	{
		targetUser = bot->getUserRecord(st[1]);
		if (!targetUser)
		{
			bot->Notice(theClient, "I don't know who %s is.", st[1].c_str());
			return false;
		}
		targetUser->generateRecoveryPassword();
		targetUser->setInstantiatedTS(::time(NULL));
		// Send the mail
		stringstream mailstream;
		mailstream << "The generated password is: " << targetUser->getRecoveryPassword().c_str() << endl;
		mailstream << theClient->getNickUserHost().c_str() << " requested a password reset to your username at this email address." << endl;
		mailstream << "If it wasn't you, you can safely ignore this message, your current password remains unchanged without a login with the generated password." << endl;
		mailstream << "Login via IRC using /msg " << bot->getNickName().c_str() << "@" << bot->getUplinkName().c_str() << " LOGIN " << targetUser->getUserName().c_str() << " " << targetUser->getRecoveryPassword().c_str() << endl;
		mailstream << "Then change your password using /msg " << bot->getNickName().c_str() << "@" << bot->getUplinkName().c_str() << " NEWPASS <new_password>" << endl;
		mailstream << ends;
		if (!bot->SendMail(targetUser->getEmail(), "Your CService account", mailstream))
		{
			bot->Notice(theClient, "An error occurred while sending newpass email, contact a CService representative.");
			return false;
		}
		else
		{
			bot->Notice(theClient, "Password reset successful.");
			bot->Notice(theClient, "Check your email (also in junk/spam) for the generated password.");
			if (targetUser->getFlag(sqlUser::F_TOTP_ENABLED))
			{
				targetUser->clearTotpKey();
				targetUser->clearTotpHexKey();
				targetUser->removeFlag(sqlUser::F_TOTP_ENABLED);
				if (!targetUser->commit(theClient))
				{
					bot->Notice(theClient, "Failed to disable TOTP authentication, please contact a CService representative.");
					return false;
				}
				bot->Notice(theClient, "\002 *** Your TOTP authentication data is also cleared! ***\002");
			}
			return true;
		}
	}
	else
	{
		bot->Notice(theClient, "Sorry, You must be logged in to use this command.");
		return false;
	}
}

int admAccess = (int)bot->getAdminAccessLevel(tmpUser);
string newpass = st.assemble(1);

//bool notMe = false;
if ((admAccess) && (st.size() > 2))
{
	targetUser = bot->getUserRecord(st[1]);
	if (targetUser)
	{
		if (admAccess < level::newpass)
		{
			bot->Notice(theClient,
				bot->getResponse(tmpUser,
					language::insuf_access,
					string("You have insufficient access to perform that command.")));
			return false;
		}
		newpass = st.assemble(2);
		sqlUser::networkClientListType::iterator clientItr = targetUser->networkClientList.begin();
		if (targetUser->networkClientList.size() > 0)
		{
			for ( ; clientItr != targetUser->networkClientList.end(); clientItr++)
			{
				iClient* tmpClient = *clientItr;
				if ((string_lower(newpass) == string_lower(targetUser->getUserName()))
					|| (string_lower(newpass) == string_lower(tmpClient->getNickName())))
				{
					bot->Notice(theClient,"The passphrase cannot be the target user's username or current nickname - syntax is: NEWPASS <targetUser> <new passphrase>");
					return false;
				}
			}
		}
		else if ((string_lower(newpass) == string_lower(targetUser->getUserName())))
		{
			bot->Notice(theClient,"The passphrase cannot be the target user's username - syntax is: NEWPASS <targetUser> <new passphrase>");
			return false;
		}
	}
	else
		targetUser = tmpUser;
}
/* Try and stop people using an invalid syntax.. */
if ( (string_lower(newpass) == string_lower(targetUser->getUserName()))
	  || (string_lower(newpass) == string_lower(theClient->getNickName())) )
	{
	bot->Notice(theClient,
		bot->getResponse(targetUser,
			language::pass_cant_be_nick,
			string("Your passphrase cannot be your username or current nick - syntax is: NEWPASS <new passphrase>")));
	return false;
	}

if (newpass.length() > 50)
	{
	bot->Notice(theClient, "Your passphrase cannot exceed 50 characters.");
	return false;
	}

if (newpass.length() < 6)
	{
	bot->Notice(theClient, "Your passphrase cannot be less than 6 characters.");
	return false;
	}


/* Work out some salt. */
string salt;

// TODO: Why calling srand() here?
srand(clock() * 1000000);

// TODO: What is the significance of '8' here?
// Schema states a fixed 8 bytes of random salt are used in generating the
// passowrd.
for ( unsigned short int i = 0 ; i < 8; i++)
	{
	int randNo = 1+(int) (64.0*rand()/(RAND_MAX+1.0));
	salt += validChars[randNo];
	}

/* Work out a MD5 hash of our salt + password */

md5	hash; // MD5 hash algorithm object.
md5Digest digest; // MD5Digest algorithm object.

// Prepend the salt to the password
string newPass = salt + newpass;

// Take the md5 hash of this newPass string
hash.update( (const unsigned char *)newPass.c_str(), newPass.size() );
hash.report( digest );

/* Convert to Hex */

int data[ MD5_DIGEST_LENGTH ] = { 0 } ;
for( size_t i = 0 ; i < MD5_DIGEST_LENGTH ; ++i )
	{
	data[ i ] = digest[ i ] ;
	}

stringstream output;
output << std::hex;
output.fill('0');
for( size_t ii = 0; ii < MD5_DIGEST_LENGTH; ii++ )
	{
	output << std::setw(2) << data[ii];
	}
output << ends;

// Prepend the md5 hash to the salt
string finalPassword = salt + output.str().c_str();

targetUser->setPassword(finalPassword);

bool hasTotp = false;
if (targetUser->getFlag(sqlUser::F_TOTP_ENABLED))
{
	hasTotp = true;
	targetUser->clearTotpKey();
	targetUser->clearTotpHexKey();
	targetUser->removeFlag(sqlUser::F_TOTP_ENABLED);
}

if (targetUser->commit(theClient))
{
	bot->Notice(theClient,
		bot->getResponse(tmpUser,
			language::pass_changed,
			string("Password successfully changed.")));
	if (hasTotp)
		bot->Notice(theClient, "\002 *** Your TOTP authentication data is also cleared! ***\002");
}
else
{
	// TODO
	bot->Notice(theClient, "NEWPASS: Unable to commit to database") ;
}
return true;

#endif

}

} // namespace gnuworld.
