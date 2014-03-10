/**
 * Seven - 2012.11.22
 * sqlIncompleteChannel.h
 *
 * Stores information about incomplete pending channels.
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
 */
 
#include	<sstream>
#include	<string> 
#include	<iostream>

#include	<cstring> 
#include	<ctime>

#include	"ELog.h"
#include	"misc.h"
#include	"sqlLevel.h"
#include	"sqlUser.h"
#include	"sqlChannel.h"
#include	"constants.h"
#include	"cservice_config.h"
#include	"sqlIncompleteChannel.h"
#include	"sqlPendingTraffic.h"
 
namespace gnuworld
{
using std::string ; 
using std::endl ; 
using std::ends ;
using std::stringstream ;

sqlIncompleteChannel::sqlIncompleteChannel(dbHandle* _SQLDb)
:chanId(0), 
RealName(),
Description(),
SQLDb(_SQLDb)
{ 
}

extern const string escapeSQLChars(const string& theString);
/*{
string retMe ;

for( string::const_iterator ptr = theString.begin() ;
	ptr != theString.end() ; ++ptr )
	{
	if( *ptr == '\'' )
		{
		retMe += "\\\047" ;
		}
	else if ( *ptr == '\\' )
		{
		retMe += "\\\134" ;
		}
	else
		{
		retMe += *ptr ;
		}
	}
return retMe ;
}*/

bool sqlIncompleteChannel::commitRealName()
{
	stringstream queryString;
	queryString << "UPDATE pending SET managername = '"
				<< escapeSQLChars(RealName)
				<< "' WHERE channel_id = "
				<< chanId
				<< ends;

	if( !SQLDb->Exec(queryString ) )
	//if( PGRES_COMMAND_OK != status )
	{
		elog	<< "sqlIncompleteChannel::commitRealName> Something went wrong: "
				<< SQLDb->ErrorMessage()
				<< endl;

		return false ;
	}
	return true;
}

bool sqlIncompleteChannel::commitDesc()
{
	stringstream queryString;
	queryString << "UPDATE pending SET description = '"
				<< escapeSQLChars(Description)
				<< "' WHERE channel_id = "
				<< chanId
				<< ends;

	if( !SQLDb->Exec(queryString ) )
	//if( PGRES_COMMAND_OK != status )
	{
		elog	<< "sqlIncompleteChannel::commitRealName> Something went wrong: "
				<< SQLDb->ErrorMessage()
				<< endl;

		return false ;
	}
	return true;
}

bool sqlIncompleteChannel::commitSupporters()
{
	stringstream queryString;
	//We don't bother with updating the supporters table, simply delete the existing entries
	queryString << "DELETE FROM supporters WHERE channel_id = "
				<< chanId
				<< ends;

	#ifdef LOG_SQL
		elog	<< "sqlIncompleteChannel::commitSupportersDelete> "
				<< queryString.str().c_str()
				<< endl;
	#endif

	if( !SQLDb->Exec(queryString ) )
	//if( PGRES_COMMAND_OK != status )
	{
		elog	<< "sqlIncompleteChannel::commitSupportersDelete> Something went wrong: "
				<< SQLDb->ErrorMessage()
				<< endl;

		return false ;
	}

	//Now we simply insert back the supporters list we have
	for (supporterListType::iterator itr = supps.begin(); itr != supps.end(); ++itr)
	{
		queryString.str("");
		queryString << "INSERT INTO supporters (channel_id,user_id,reason,last_updated) VALUES ("
					<< chanId << ", " << itr->first << ", '', now()::abstime::int4)"
					<< ends;

		if( !SQLDb->Exec(queryString ) )
		//if( PGRES_COMMAND_OK != status )
		{
			elog	<< "sqlIncompleteChannel::commitSupporters> Something went wrong: "
					<< SQLDb->ErrorMessage()
					<< endl;

			return false ;
		}
	}
	return true;
}

/*
bool sqlIncompleteChannel::commit()
{
	stringstream queryString;

	if ((!Description.empty()) && (!supps.empty()))
	{
		//Inserting/Updating supporters list into the supporters table

		//We don't bother with updating the supporters table, simply delete the existing entries
		queryString << "DELETE FROM supporters WHRE channel_id = "
					<< chanId
					<< ends;

		#ifdef LOG_SQL
			elog	<< "sqlIncompleteChannel::commitSupportersDelete> "
					<< queryString.str().c_str()
					<< endl;
		#endif

		if( !SQLDb->Exec(queryString ) )
		//if( PGRES_COMMAND_OK != status )
		{
			elog	<< "sqlIncompleteChannel::commitSupportersDelete> Something went wrong: "
					<< SQLDb->ErrorMessage()
					<< endl;

			return false ;
		}

		//Now we simply insert back the supporters list we have
		for (suppsType::iterator itr = supps.begin(); itr != supps.end(); ++itr)
		{
			queryString.str("");
			queryString << "INSERT INTO supporters (channel_id,user_id,reason,last_updated) VALUES ("
						<< chanId << ", " << itr->first << ", '', now()::abstime::int4)"
						<< ends;

			if( !SQLDb->Exec(queryString ) )
			//if( PGRES_COMMAND_OK != status )
			{
				elog	<< "sqlIncompleteChannel::commitSupporters> Something went wrong: "
						<< SQLDb->ErrorMessage()
						<< endl;

				return false ;
			}
		}
		return true;
	}

	if ((!RealName.empty()) && (Description.empty()))
	{
		queryString.str("");
		queryString << "UPDATE pending SET managername = '"
					<< escapeSQLChars(RealName)
					<< "' WHERE channel_id = "
					<< chanId
					<< ends;

		if( !SQLDb->Exec(queryString ) )
		//if( PGRES_COMMAND_OK != status )
		{
			elog	<< "sqlIncompleteChannel::commitRealName> Something went wrong: "
					<< SQLDb->ErrorMessage()
					<< endl;

			return false ;
		}
	}

	if ((!Description.empty()) && (supps.empty()))
	{
		queryString.str("");
		queryString << "UPDATE pending SET description = '"
					<< escapeSQLChars(Description)
					<< "' WHERE channel_id = "
					<< chanId
					<< ends;

		if( !SQLDb->Exec(queryString ) )
		//if( PGRES_COMMAND_OK != status )
		{
			elog	<< "sqlIncompleteChannel::commitRealName> Something went wrong: "
					<< SQLDb->ErrorMessage()
					<< endl;

			return false ;
		}
	}

		return true;
}
*/
// Insert into the pendings table the new channel_id with it's managerId
bool sqlIncompleteChannel::commitNewPending(unsigned int mngrId, unsigned int chanId)
{
	stringstream theQuery;
	theQuery 	<< "INSERT INTO pending (channel_id,manager_id,created_ts,decision_ts,decision,comments,description,managername,last_updated,reg_acknowledged,check_start_ts) "
				<< "VALUES (" << chanId << "," << mngrId << ",now()::abstime::int4,0,'','','','',now()::abstime::int4,'N',0)"
				<< ends;

	#ifdef LOG_SQL
		elog	<< "sqlIncompleteChannel::commitNewPending> "
				<< theQuery.str().c_str()
				<< endl;
	#endif

	if( !SQLDb->Exec(theQuery, true ) )
	{
		elog	<< "sqlIncompleteChannel::commitNewPending> Something went wrong: "
				<< SQLDb->ErrorMessage()
				<< endl;

		return false ;
	}
	return true;
}

	
void sqlIncompleteChannel::loadIncompleteChans()
{

/*
stringstream theQuery;
		theQuery 	<< "SELECT ip_number, join_count FROM pending_traffic"
					<< " WHERE channel_id = " << channel_id
					<< ends;

#ifdef LOG_SQL
	elog	<< "sqlIncompleteChannel::loadIncompleteChans> "
		<< theQuery.str().c_str()
		<< endl; 
#endif

if( SQLDb->Exec(theQuery, true ) )
//if( PGRES_TUPLES_OK == status )
	{
	for (unsigned int i = 0 ; i < SQLDb->Tuples(); i++)
		{ 
			unsigned int theIp = atoi(SQLDb->GetValue(i, 0));
//			elog << "IP: " << theIp << endl;

			sqlPendingTraffic* trafRecord = new sqlPendingTraffic(SQLDb);
			trafRecord->ip_number = theIp;
			trafRecord->join_count = atoi(SQLDb->GetValue(i, 1));
			trafRecord->channel_id = channel_id; 

			trafficList.insert(trafficListType::value_type(theIp, trafRecord));

		}
	}
*/
}

}
