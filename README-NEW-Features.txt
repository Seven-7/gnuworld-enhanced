==============================================================================
2020.06.26 * FIX/UPDATE: Newly created users are created with no last_seen data
             so without login within one day they expire.
------------------------------------------------------------------------------
Beside the usual update procedure, you must enter into postgresql command line:

/usr/local/pgsql/bin/psql cservice

and execute the command:

cservice=# DROP TRIGGER t_new_user ON users;

In case of new installation this step is not necessary!
==============================================================================
2020.06.12 * cservice HELLO now sends out a real email with the generated password
           * cservice NEWPASS now clears TOTP authentication data as well!
------------------------------------------------------------------------------
The feature has to be enabled in cservice.conf:

hello_sendmail_enabled = 1

 ** IMPORTANT: mailutils or mailx package must be installed on your system!

For normal users NEWPASS command now can act as Password recovery in case the user is not logged in!
(For admins no change in the behavior)
If the user is logged in, the original new password generation mechanism applies.
So the new syntax is:

NEWPASS <new passphrase|username>

So for example a user lost it's password and is NOT logged in, 
can recover it's password by passing the username as parameter, for example:

/msg x@channels.yournetwork.org NEWPASS SomeUser

otherwise if IS logged in, just want to change it's password:

/msg x@channels.yournetwork.org NEWPASS NewPassword
==============================================================================
2020.04.26 * SETHOST: Prevent setting some restricted/reserved hostnames
------------------------------------------------------------------------------
Restricted/reserved hostnames are set in cservice.conf.
Any number of "reservedHost = " entry is allowed.

To restrict admins/opers to set restricted/reserved hostnames edit
mod.cservice/cservice_config.h

#undef ADMINS_USE_RESTRICTED_SETHOSTS
==============================================================================
2016.02.18 * Added halfop support: mod.cservice HALFOPCommand and HALFDEOPCommand
------------------------------------------------------------------------------
To enable halfop command in mod.cservice you must
	#define USING_NEFARIOUS
	#define USE_HALFOPS
in mod.cservice/cservice_config.h

The syntax is very similar to op and deop.
	/msg X halfop <#channel> [nick1 nick2 nick3]
	/msg X haldefop <#channel> [nick1 nick2 nick3]
	
Warning! This feature requires Nefarious2 with it's halfop support.
Warning! Don't forget to enable halfops in ircd.conf:
	"HALFOPS" = "TRUE";
If you have an existing installation, to upgrade read README-DO-UPDATES.txt
==============================================================================
2016.02.04 * Recognizing and handling Nefarious2's cloaked IP and cloaked Host
------------------------------------------------------------------------------
If you set "HOST_HIDING_STYLE" greater than 1 in Nefarious2's ircd.conf
gnuworld now will recognize the cloaked style
==============================================================================
2016.01.08 * Prevent users to set 'exotic' hostnames, like with colors, etc.
------------------------------------------------------------------------------
You might find lame to see colored/bolded/etc. hostnames, so if you enable

	#define VALIDATE_SET_HOSTNAME

in mod.cservice/cservice_config.h only normal characters will be allowed.
Also it is a possibility of confusion/abuse to allow users to set hosts
with the network's hidden-host-suffix(eg. something.users.yournetwork.org)
so this is not allowed from now on.
============================================================================
2016.01.07 * Added mod.cservice SCANCommand
	First puropse: find owner user of a nickname
----------------------------------------------------------------------------
To find an owner of a nickname:
   /msg X scan nick|nickname <nick>
==============================================================================
2016.01.07 * mod.cservice NEWPASS: Admins can set target users a new password
------------------------------------------------------------------------------
Admins above access level level::newpass can set target users a new password:
 Syntax:
   /msg X@channels.yournetwork.org newpass <targetUser> <target New Password>

Note: Changing own password remained valid as originally:
   /msg X@channels.yournetwork.org newpass <your new password>
==============================================================================
2016.01.06 * Nick protection through mod.cservice (nickreg/nickserv function)
------------------------------------------------------------------------------
Warning! This feature requires Nefarious2 with it's svsnick support.
Warning! This feature requires to patch Nefarious2 for a proper svsnick handling. Read README-DO-UPDATES.txt!
Also USING_NEFARIOUS must be #define in mod.cservice/cservice_config.h
If you have an existing installation, to upgrade read README-DO-UPDATES.txt
This nick protection works through an svsnick nick changing mechanism by gnuworld.
For the cservice user structure is added a nickname field, so gnuworld takes care any nick on the network is used by the 'right person'
by force-changing and generating an arbitrary 4 digits numbers ended nickname.

Syntax: 
   /msg X set nick|nickname <TheNick>
   
To clear any nickname:
   /msg X set nick|nickname OFF
   or simply
   /msg X set nick|nickname
   
Also coders/admins with (levels.h -> level::nickset) access can set nickname for a target username.

   /msg X set nick|nickname <targetUser> <targetNewNickname>

There is a new user flag too, AUTONICK:

/msg X set autonick <ON|OFF>

If it is ON, on login the saved nickname will be set for the client.
==============================================================================
2016.01.06 * Set welcome message-notice for channel joins through X
------------------------------------------------------------------------------
The command syntax for setting welcome message for a channel:

/msg X set #channel_name welcome <welcome_message>

To clear any welcome message:

/msg X set #channel_name welcome OFF
or simply
/msg X set #channel_name welcome

Done.
You can see the welcome message on any channel join.

==============================================================================
2015.08.06 * Set (fake)hostname for users
------------------------------------------------------------------------------
Warning! This feature requires Nefarious2 with it's Fake host support.
If you have an existing installation, to upgrade read README-DO-UPDATES.txt

The command syntax for setting a hostname for a username:

/msg X set hostname <anyhost.youwant.com>

To clear your hostname use:

/msg X set hostname off

The set hostname will be applied instantaneously (if +x), and on every login with +x  
insted of the default users hiddenhost suffix
==============================================================================
 * Status * command shows currently logged in officials
------------------------------------------------------------------------------
/msg X status *

 -X- Status of currently logged '*' officials:
 -X- Auth: SomeUser1/SomeNick1 (1000) SomeUser2/SomeNick2 (750) SomeUser3/SomeNick31 SomeNick32 (750) SomeUser4/SomeNick4 (600)
 
 Note: The original function of status * is moved to stats * 
==============================================================================
* Suspension of users on channels with reason
------------------------------------------------------------------------------
-X- SYNTAX: SUSPEND <#channel> <username> [duration] [level] [reason]
------------------------------------------------------------------------------
-X- Your reason must be 2 - 300 charcters long.
------------------------------------------------------------------------------
 -> *x* access #coder-com Seven
 -X- USER: Seven ACCESS: 499 L
 -X- CHANNEL: #coder-com -- AUTOMODE: None
 -X- *** SUSPENDED *** - Expires in 0 days, 00:09:48 (Level 400)
 -X- Reason: Not proper behavior
 -X- LAST SEEN: 0 days, 10:13:18 ago.
 -X- End of access list

-X- SYNTAX: UNSUSPEND <#channel> <username> [reason]

 -> *x* unsuspend #coder-com Seven I forgive you this time
 -X- USER: Seven ACCESS: 499 L
 -X- CHANNEL: #coder-com -- AUTOMODE: None
 -X- UNSUSPENDED - I forgive you this time
 -X- LAST SEEN: 0 days, 10:16:09 ago.
 -X- End of access list

If the suspension expires, no reason will be show.
If no reason is supplied, reason will be "No reason supplied"

==============================================================================
 * NOTEs is accessible for normal users too.
------------------------------------------------------------------------------
Notes can be used to bother the another, and maybe admins don't even want to get notes from normal users, for
this I introduced config lines in "cservice_config.h"

#define USE_NOTES

/**
 * Do you want normal users to use the Noteserv functionality?
 */

#define USE_USERS_NOTES

/**
 * Normal users can send notes to admins?
 */

#define USERS_NOTE_ADMINS

It can be choose between only admins use notes, admins and users, and admins can or cannot get notes from users.
An ignore option might be advisable.

==============================================================================
 * Target users get notification about a new access modification.
------------------------------------------------------------------------------
... and if the target user is not logged in in the moment of modification, the notification will be saved and displayed when the user logs in.

-X- You have been added to channel #coder-com with access level 499
-X- Your access on #coder-com has been suspended.
-X- Your suspension on #coder-com has been cancelled.
-X- Your access on #coder-com has been modified from 499 to 400
-X- Your access from #coder-com has been removed by Admin.

The maxmimum amount of notification can be set up in "cservice.example.conf" at the "max_notes" line. Only the latest "max_notes" count will be stored.

-X- AUTHENTICATION SUCCESSFUL as Seven
-X- Remember: Nobody from CService will ever ask you for your password, do NOT give out your password to anyone claiming to be CService.

-X- You have been added to channel #coder-com with access level 499
-
-X- Your access on #coder-com has been suspended.
-
-X- Your suspension on #coder-com has been cancelled.
-
-X- Your access on #coder-com has been modified from 499 to 400
-
-X- Your access from #coder-com has been removed. 

==============================================================================
 * Completed users url, useable as a short description, motto, etc of the user (like we do with the Real Name).
------------------------------------------------------------------------------
It can be used with 3 keywords:

/msg X set URL <urlstring>
/msg X set DESC || DESCRIPTION <descstring>
/msg X set MOTTO <mottostring>

-> *x* set motto Escape from Paradise
-X- Set your MOTTO to Escape from Paradise.

-> *x* info seven
-X- Information about: Seven (2)
-X- Escape from Paradise
-X- Currently logged on via:

To clear a motto/desc/url must type:
/msg X set url off
-X- Cleared your URL

=============================================================================
 * Chaninfo shows information about channels under registration
------------------------------------------------------------------------------
Admins get fullinformation about the channel:

-> *x* info #anychan
-X- Channel #anychan is in applications list at stage:
-X-  *** PENDING SUPPORTERS CONFIRMATION ***
-X- Applicant: Seven - last seen: 0 days, 00:31:32 ago
-X- Real Name: Seven RealName
-X- Description: Channel for nobody
-X- Application posted on: Sun Jun 17 10:01:55 2012
-X- Supporters: johndoe/johndoe (0), NoBody (0)

Where johndoe/johndoe (0) means
     username/nick (joincount)

If user has supported channel with YES, it will appear with bold letters
If the nick is currently on the channel it will appear with bold letters, if not, in plain, if not logged in then skipped.
If a user set it's support to NO, the username will appear with bold upper, and it will appear the decision.

Applicant and supporters don't get the joincount. 
Non-supporters get's information Only about Status, Applicant and Description:

 -X- Channel #anychan is in applications list at stage:
 -X-  *** PENDING SUPPORTERS CONFIRMATION ***
 -X- Applicant: Seven - last seen: 0 days, 01:56:49 ago
 -X- Description: Channel for nobody

=============================================================================
 * Officials with *1+ can set nopurge for their user form IRC
-----------------------------------------------------------------------------
 -> *x* set nopurge on
 -X- Your NOPURGE setting is now ON

=============================================================================
 * Manual commands for channel ACCEPT/REJECT from IRC 
-----------------------------------------------------------------------------

 -> *x* reject #anychan For some reason rejected
 -channels.homenetwork.org:#coder-com- [X] Admin (Admin) has rejected #anychan from Seven
 -X- Rejected channel #anychan

 -> *x* info #anychan
 -X- Channel #anychan is in applications list at stage:
 -X-    *** REJECTED ***
 -X- Applicant: Seven - last seen: 0 days, 00:00:50 ago
 -X- Real Name: Seven RealName
 -X- Description: Channel for nobody
 -X- Application posted on: Sun Jun 17 11:28:30 2012
 -X- Decision by CService Admin: For some reason rejected
 -X- Supporters: johndoe/johndoe (0), NoBody (0)

 -> *x* accept #anychan For some reason Still Accept
 -channels.homenetwork.org:#coder-com- [X] Admin (Admin) has accepted #anychan to Seven
 -X- Accepted channel #anychan

The from-IRC accept is superior realtive to web-accept, because it's no needed any sychronization time betweeen gnuworld and web,
the channel chache is refreshed instantaneously, +R appear, X can be asked to join immediately.

 -X- Your reason must be 2 - 300 charcters long.
=============================================================================
 * Completed HELLO command with verification answer requirement
-----------------------------------------------------------------------------

 -X- SYNTAX: HELLO <username> <email> <email> <1-3> <verification answer>

 -X- Valid verification answer numbers are: 1 to 3
 -X- 1: What's your mother's maiden name ?
 -X- 2: What's your dog's(or cat's) name ?
 -X- 3: What's your father's birth date ?

 -X- Your verification answer must be 4 - 300 charcters long.

=============================================================================
 * Channel REGISTER and HELLO commands take into account NOREG/LOCKED's
-----------------------------------------------------------------------------

 -> *x* hello duckyjoe duckyjoe_2001@yahoo.com duckyjoe_2001@yahoo.com 2 Rexy
 -X- Invalid username (NOREG)
 -X- Usernames matching *ducky* are disallowed for the following reason:
 -X- Ducks are not allowed here

 -> *x* hello duckijoe duckyjoe_2001@yahoo.com duckyjoe_2001@yahoo.com 2 Rexy
 -X- Invalid email address (NOREG)
 -X- Email addresses matching *ducky* are disallowed for the following reason:
 -X- Ducks are not allowed here

 -> *x* hello duckyjoe duckyjoe_2001@yahoo.com duckyjoe_2001@yahoo.com 2 Rexy
 -X- Invalid username (LOCKED)
 -X- Usernames matching *ducky* are disallowed for the following reason:
 -X- Ducks must stay out from here

 *** Also for locked verification answer ***

In cservice_config.h i'v separated 2 config line:

#define ALLOW_HELLO	  ==> enable HELLO command only for admins
#define ALLOW_USERS_HELLO ==> enable HELLO command for admins and users

Note: If HELLO is not enabled for admins, than also will not be for users.
Note2: Admins with access level above level::hello defined in mod.cservice/levels.h can bypass NOREG/LOCKED's, etc.

Tha same story for REGISTER command. (cheking for NOREG/LOCKED email, username, verification answer
+ target user must be logged in at least once on IRC

Note: Admins with access level above level::immune::registercmd defined in levels.h can bypass NOREG/LOCKED's, etc.

=============================================================================
 * POWER user flag, enables almighty admins
-----------------------------------------------------------------------------
/msg X set POWER [targetuser] ON|OFF

Many networks asked for a possbility of 'special' admins, with absolute administrative power, to be able to set another 1000 admin,
remove, etc. 
Userid 1 always has the capability to set itself as POWER, other users can get only from an another POWER-ed user.
The POWER flag is visible in user info only to an another POWER-ed admin, for normal admins is not listed.

=============================================================================
 * SPECIAL channel flag is used for prevention of setting normal accesses above 499
-----------------------------------------------------------------------------
Logic dictate to not to allow to set accesses above 500, except for some special cases, those with SPECIAL channel flag:

 -> *x* adduser #cservice Seven 501
 -X- Access levels on regular channels cannot exceed 499 (except SPECIAL)
 -> *x* adduser #cservice Seven 499
 -X- Added user Seven to #cservice with access level 499
 -> *x* modinfo #cservice access Seven 501
 -X- Access levels on regular channels cannot exceed 499 (except SPECIAL)

=============================================================================
 * Expiration of users for more than 60 days are automatical from the gnuworld's internal timer
-----------------------------------------------------------------------------

In mod.cservice/ in cservice.h/cservice.cc it is declared a new timer looking after users expiration.
This can be fine tuned within cservice.example.conf 

"users_expire_days" 

config line.
For this, and for the channel expiration it is valid the 

"hour_seconds" and "day_seconds" config lines.

The "users_db_idle" is the time interval in hours to check the users for the lastseen value.

=============================================================================
* Expiration of channels within gnuworld, with setting the proper MIA, LOCKED, DESC, URL's
-----------------------------------------------------------------------------
It is well known the steps of channel expiration, this does the same in automatic way.
In cservice.example.conf there are config lines for this

#
# Frequncy for check database for for channels for missing managers(in hours)
# cannot be 0, default 3 will be used

channels_db_idle = 1

# Set hour length in seconds
# (appliabale only for "new" features user and channel expires
# cannot be 0; 1 will be used

hour_seconds = 3600

# Set day length in seconds
# (appliabale only for "new" features user and channel expires
# cannot be 0; 1 will be used

day_seconds = 86400

#
# Set MIA flag on channels where manager missing for
# 0 = never expire

MIA_start_days = 21

#
# Purge MIA flagged channels after
#

MIA_end_days = 30

#
# Description for MIA starting period
#

MIA_start_desc = Manager has failed to login. To vote for new manager send mail to cservice@undernet.org

#
# MIA URL
#

MIA_URL = Only 400+ Ops

#
# Description for MIA ending period (manager has logged in)
#

MIA_end_desc = Manager has logged in. This was the last warning for failure to login

If the manager has logged in in the MIA period, a CAUTION channel flag will be set (LOCKED,MIA will be removed), 
what will cause an immediate channel purge next time when the manager fails to log in in time.

=============================================================================
* Automode VOICE/OP are Not allowed in any kind below the required access levels.
-----------------------------------------------------------------------------
Everyone knows that automode voice/op can be set starting with 1 access. This is an issue to fix,
So:

 -> *x* modinfo #coder-com access Seven 24
 -X- Modified Seven's access level on channel #coder-com to 24
 -> *x* modinfo #coder-com automode seven op
 -X- Target user Seven on channel #coder-com has insufficient access for an automode OP
 -> *x* modinfo #coder-com automode seven voice
 -X- Target user Seven on channel #coder-com has insufficient access for an automode VOICE
 -> *x* modinfo #coder-com access Seven 25
 -X- Modified Seven's access level on channel #coder-com to 25
 -> *x* modinfo #coder-com automode seven voice
 -X- Set AUTOMODE to VOICE for Seven on channel #coder-com
 -> *x* modinfo #coder-com automode seven op
 -X- Target user Seven on channel #coder-com has insufficient access for an automode OP
 -> *x* modinfo #coder-com access Seven 99
 -X- Modified Seven's access level on channel #coder-com to 99
 -> *x* modinfo #coder-com automode seven op
 -X- Target user Seven on channel #coder-com has insufficient access for an automode OP
 -> *x* modinfo #coder-com access Seven 100
 -X- Modified Seven's access level on channel #coder-com to 100
 -> *x* modinfo #coder-com automode seven op
 -X- Set AUTOMODE to OP for Seven on channel #coder-com

In addition if channel has a userflag VOICE/OP, when the access level is modified to the required level, it will get the automode,
if the access is modified below, it will be lost, set to NONE.

=============================================================================
* RENAMECommand of the usernames
-----------------------------------------------------------------------------

 -X- SYNTAX: RENAME <old_username> <new_username>

 -> *x* rename seven George
 -X- Successfully renamed username Seven to George

It is required an admin access level level::rename in mod.cservice/levels.h

=============================================================================
 * Reimplemented NOFORCE channel flag
-----------------------------------------------------------------------------
Noforce channel flag prevents admins to force themselves on a channel.

 -> *x* set #cservice noforce on
 -X- NOFORCE for #cservice is ON

 -X- Channel #cservice has 2 users (2 operators)
 -X- Mode is: +tnR 
 -X- I'm currently in this channel.
 -X- MassDeopPro: 3
 -X- Flags set: NOFORCE AUTOJOIN 
 -X- Auth: 

 -> *x* force #cservice
 -X- The NOFORCE flag is set on #cservice

Requires an adminlevel level::set::noforce
=============================================================================
 * Implemented NOVOICE channel flag, homologous as NOOP
-----------------------------------------------------------------------------
/msg X set #anychan NOVOICE <on|off>

=============================================================================
 * REGISTER command is available to normal users, to start a new channel registration from irc
-----------------------------------------------------------------------------
/msg X register #channel

and than follow X's instructions!

The admin registration method remains:
/msg X register #channel [targetuser]

=============================================================================
 * NOTAKE channel flag prevents 'takeover' banning *!*@* through X
-----------------------------------------------------------------------------
/msg X set #channel NOTAKE <on|off>

The revenge action can be 3 types:

/msg X set #channel TAKEREVENGE <IGNORE|BAN|SUSPEND>

IGNORE - simply will do nothing on ban *!*@*
BAN - will ban and kick the person for 100 days at level 500
SUSPEND - will ban and kick and suspend for 100 days 500 level

=============================================================================
 * PURGE command can delete users too
-----------------------------------------------------------------------------
/msg X purge <#channel|username>

If a username is provided, then will delete it.
=============================================================================
 * Other minor enhancements
-----------------------------------------------------------------------------
 * On Channel join (bursting channels) X looks after NOOP and NOVOICE channel flag, and deop/devoice everyone.
 * Ban nicklength is now 50 instead of maximum of 15, ident legth is 25 instead of 12.
 * Channel description length is now can be 128 charcters long, url is 75 charcters long.
   Don't forget to adjust ircu's values too if needed.
 * On user suspension on a channel, all logged clients corresponding the suspended account will be deopped instantaneously.
 * On global user suspension, all clients corresponding to logged user, on all channels where is opped will be deopped instantaneously. 
   

