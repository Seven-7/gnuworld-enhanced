#!/bin/sh
/usr/local/pgsql/bin/createdb --template=template0 -E=SQL_ASCII -U gnuworld cservice
/usr/local/pgsql/bin/psql -U gnuworld cservice < cservice.sql
/usr/local/pgsql/bin/psql cservice < cservice.config.sql
/usr/local/pgsql/bin/psql cservice < languages.sql
/usr/local/pgsql/bin/psql cservice < language_table.sql
/usr/local/pgsql/bin/psql cservice < greeting.sql
/usr/local/pgsql/bin/psql cservice < cservice.help.sql
/usr/local/pgsql/bin/psql cservice < cservice.web.sql
/usr/local/pgsql/bin/psql cservice < cservice.addme.sql
/usr/local/pgsql/bin/createdb --template=template0 -E=SQL_ASCII ccontrol
/usr/local/pgsql/bin/psql ccontrol < ccontrol.sql
/usr/local/pgsql/bin/psql ccontrol < ccontrol.help.sql
/usr/local/pgsql/bin/psql ccontrol < ccontrol.addme.sql
/usr/local/pgsql/bin/psql ccontrol < ccontrol.commands.sql
/usr/local/pgsql/bin/createdb --template=template0 -E=SQL_ASCII dronescan
/usr/local/pgsql/bin/psql dronescan < dronescan.sql
/usr/local/pgsql/bin/createdb --template=template0 -E=SQL_ASCII chanfix
/usr/local/pgsql/bin/psql chanfix < ../mod.openchanfix/doc/chanfix.sql
/usr/local/pgsql/bin/psql chanfix < ../mod.openchanfix/doc/chanfix.languages.sql
/usr/local/pgsql/bin/psql chanfix < ../mod.openchanfix/doc/chanfix.language.english.sql
/usr/local/pgsql/bin/psql chanfix < ../mod.openchanfix/doc/chanfix.help.sql
/usr/local/pgsql/bin/psql chanfix < ../mod.openchanfix/doc/chanfix.addme.sql