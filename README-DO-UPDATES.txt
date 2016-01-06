============================================================================
2016.01.06  Nick protection through mod.cservice (nickreg/nickserv function)
----------------------------------------------------------------------------
Warning! You need to patch Nefarious2 to handle correctly svsnicks!
 - Edit nefarious2/ircd/s_user.c
 - Find set_nick_name function
   On the section where writes:
      /*
       * Refuse nick change if the last nick change was less
       * then 30 seconds ago. This is intended to get rid of
       ... ...
There is:
      if (CurrentTime < cli_nextnick(cptr))
      {
        cli_nextnick(cptr) += 2;
        send_reply(cptr, ERR_NICKTOOFAST, parv[1],
                   cli_nextnick(cptr) - CurrentTime);
        /* Send error message */
        sendcmdto_one(cptr, CMD_NICK, cptr, "%s", cli_name(cptr));
        /* bounce NICK to user */
        return 0;                /* ignore nick change! */
      }
      else {
        /* Limit total to 1 change per NICK_DELAY seconds: */
        cli_nextnick(cptr) += NICK_DELAY;
        /* However allow _maximal_ 1 extra consecutive nick change: */
        if (cli_nextnick(cptr) < CurrentTime)
          cli_nextnick(cptr) = CurrentTime;
      }
      /* Invalidate all bans against the user so we check them again */
      for (member = (cli_user(cptr))->channel; member;
	   member = member->next_channel) {
        ClearBanValid(member);
        ClearBanValidNick(member);
        ClearExceptValid(member);
        ClearExceptValidNick(member);
      }
    }
Change it to:
      if (!svsnick)
      {
        if (CurrentTime < cli_nextnick(cptr))
        {
          cli_nextnick(cptr) += 2;
          send_reply(cptr, ERR_NICKTOOFAST, parv[1],
                   cli_nextnick(cptr) - CurrentTime);
          /* Send error message */
          sendcmdto_one(cptr, CMD_NICK, cptr, "%s", cli_name(cptr));
          /* bounce NICK to user */
          return 0;                /* ignore nick change! */
        }
        else {
          /* Limit total to 1 change per NICK_DELAY seconds: */
          cli_nextnick(cptr) += NICK_DELAY;
          /* However allow _maximal_ 1 extra consecutive nick change: */
          if (cli_nextnick(cptr) < CurrentTime)
            cli_nextnick(cptr) = CurrentTime;
        }
      }
 - Save the file
 - Do a make; make install again
 - Restart ircd!
 
 Now updating gnuworld:
 - Update the database 'users' table structure:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~$ cd gnuworld-enhanced/doc
gnuworld@ubuntu:~/gnuworld-enhanced/doc$ /usr/local/pgsql/bin/psql cservice < update_cservice_users_nickname.sql
gnuworld@ubuntu:~/gnuworld-enhanced/doc$ cd ..
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done. 
============================================================================
2016.01.06  New feature: Set welcome notice-message on channel joins through X
----------------------------------------------------------------------------
If you have an existing installation, you can upgrade with the following steps:
 - Update the database 'channels' table structure:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~/gnuworld-enhanced$ cd doc
gnuworld@ubuntu:~/gnuworld-enhanced/doc$ /usr/local/pgsql/bin/psql cservice < update_cservice_channel_welcome.sql
gnuworld@ubuntu:~/gnuworld-enhanced/doc$ cd ..
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...

Done. You can see the welcome message on any channel join.
============================================================================
2015.08.06  New feature: Set (fake)hostname for users
----------------------------------------------------------------------------
Warning! This feature requires Nefarious2 with it's Fake host support.
If you have an existing installation, you can upgrade with the following steps:
 - Update the database 'users' table structure:
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~$ cd gnuworld-enhanced/doc
gnuworld@ubuntu:~/gnuworld-enhanced/doc$ /usr/local/pgsql/bin/psql cservice < update_cservice_users_hostname.sql
 - This update probably requires a full reconfigure, so note down your configure line from
~/gnuworld-enhanced/config.log
after that, do a full reconfigure and remake
gnuworld@ubuntu:~/gnuworld-enhanced$ make clean; make distclean
gnuworld@ubuntu:~/gnuworld-enhanced$ ./configure ....
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...

The command syntax for setting a hostname for a username:

/msg X set hostname <anyhost.youwant.com>

To clear your hostname use:

/msg X set hostname off
