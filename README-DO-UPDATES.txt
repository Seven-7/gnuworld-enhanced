============================================================================
2015.08.06  New feature: Set (fake)hostname for users
----------------------------------------------------------------------------
Update the database 'users' table structure:
linux:/home/gnuworld$ cd gnuworld-enhanced/doc
linux:/home/gnuworld/doc$ /usr/local/pgsql/bin/psql cservice < update_cservice_users_hostname.sql

The command syntax for setting a hostname for a username:

/msg X set hostname <anyhost.youwant.com>

To clear your hostname use:

/msg X set hostname off
