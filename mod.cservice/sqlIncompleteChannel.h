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
 */

#ifndef __SQLINCOMPLETECHANNEL_H
#define __SQLINCOMPLETECHANNEL_H "$Id: sqlIncompleteChannel.h,v 1.0 2012/22/11 09:53:16 Seven Exp $"

#include	<map>

#include	"dbHandle.h"
#include	"sqlUser.h"

namespace gnuworld
{ 

using std::string ;
 
class sqlIncompleteChannel
{
public:
	sqlIncompleteChannel(dbHandle*);

	//bool commit();
	bool commitRealName();
	bool commitDesc();
	bool commitSupporters();
	bool commitNewPending(unsigned int, unsigned int);
	void loadIncompleteChans();

	unsigned int chanId;
	string chanName;
	string RealName;
	string Description;
	typedef std::map <unsigned int, sqlUser*> supporterListType;
	supporterListType supps;
	
	dbHandle* SQLDb;
};

}
#endif // __sqlIncompleteChannel_H
