==============================================================================
2020.04.26 * SETHOST: Prevent setting some restricted/reserved hostnames
------------------------------------------------------------------------------
If you have an existing installation, you can upgrade with the following steps:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 - Add the new restricted/reserved hostnames section from cservice.example.conf to your cservice.conf
 ... restart gnuworld ...

 - If you just edited/updated the restricted hostnames in cservice.conf just:
 ... restart gnuworld ...
Done.

 - If you don't want to allow admins/opers to set restricted/reserved hosts, edit

 mod.cservice/cservice_config.h

#undef ADMINS_USE_RESTRICTED_SETHOSTS

gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done.
==============================================================================
2016.02.18 Added halfop support: mod.cservice HALFOPCommand and HALFDEOPCommand
------------------------------------------------------------------------------
If you have an existing installation, you can upgrade with the following steps:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ make clean; make distclean
 - Reconfigure again:
gnuworld@ubuntu:~/gnuworld-enhanced$ ./configure ......
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done.
Note: If doesn't works, follow the steps like in SCANCommand, saving libltdl, and doing the autoreconf
============================================================================
2016.02.04 Recognizing and handling Nefarious2's cloaked IP and cloaked Host
----------------------------------------------------------------------------
If you have an existing installation, you can upgrade with the following steps:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done.
Also don't forget to set "HOST_HIDING_STYLE" > 1 in Nefarious2's ircd.conf if you intend to see cloaked IPs/hosts
============================================================================
2016.01.08 Prevent users to set 'exotic' hostnames, like with colors, etc.
----------------------------------------------------------------------------
If you have an existing installation, you can upgrade with the following steps:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done.
============================================================================
2016.01.07  Added mod.cservice SCANCommand
			First puropse: find owner user of a nickname
----------------------------------------------------------------------------
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
 - Check and note down your current configure line:
gnuworld@ubuntu:~/gnuworld-enhanced$ vim config.log
In my case:
./configure --with-extra-includes=/usr/local/include --with-extra-includes=/usr/include/postgresql/ --with-pgsql-home=/usr/local/pgsql/ --enable-modules=cservice,ccontrol
 - Do a full clean:
gnuworld@ubuntu:~/gnuworld-enhanced$ make clean; make distclean
 - Save libltdl:
gnuworld@ubuntu:~/gnuworld-enhanced$ cp -rf libltdl/ libltdl-save
 - Do a full autoreconf:
gnuworld@ubuntu:~/gnuworld-enhanced$ autoreconf -Wall -i
 - Delete libltdl/, rename back libltdl-save:
gnuworld@ubuntu:~/gnuworld-enhanced$ rm -rf libltdl; mv libltdl-save libltdl
 - Configure again:
gnuworld@ubuntu:~/gnuworld-enhanced$ ./configure --with-extra-includes=/usr/local/include --with-extra-includes=/usr/include/postgresql/ --with-pgsql-home=/usr/local/pgsql/ --enable-modules=cservice,ccontrol
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done.
============================================================================
2016.01.07  mod.cservice NEWPASS: Admins can set target users a new password
----------------------------------------------------------------------------
 - The usual update procedure:
gnuworld@ubuntu:~$ cd gnuworld-enhanced
gnuworld@ubuntu:~/gnuworld-enhanced$ git pull
gnuworld@ubuntu:~/gnuworld-enhanced$ make; make install
 ... restart gnuworld ...
Done.
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
