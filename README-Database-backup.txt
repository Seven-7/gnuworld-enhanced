Often might be needed to save your database, you want to do a migration, a reinstall, etc.
Creating and restoring a database is easy:

Creating a backup:
-----------------
/usr/local/pgsql/bin/pg_dump cservice > cservice-backup.sql
/usr/local/pgsql/bin/pg_dump ccontrol > ccontrol-backup.sql

Restoring a database from backup:
--------------------------------
First drop the current databases what we want to replace:
/usr/local/pgsql/bin/dropdb cservice
/usr/local/pgsql/bin/dropdb ccontrol

Create back the databases, they will be empty:
/usr/local/pgsql/bin/createdb --template=template0 -E=SQL_ASCII cservice
/usr/local/pgsql/bin/createdb --template=template0 -E=SQL_ASCII ccontrol

Copy back the backup data:
/usr/local/pgsql/bin/psql cservice < cservice-backup.sql
/usr/local/pgsql/bin/psql ccontrol < ccontrol-backup.sql
