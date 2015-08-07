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
