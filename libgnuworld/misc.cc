/**
 * misc.cc
 * Copyright (C) 2002 Daniel Karrels <dan@karrels.com>
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
 * $Id: misc.cc,v 1.6 2007/12/27 20:45:15 kewlio Exp $
 */

#include	<string>
#include	<algorithm>

#include	<cctype>
#include	<cstdio>
#include	<cstdlib>
#include	<cstdarg>
#include	<cstring>

#include	<sstream>

#include	"misc.h"
#include	"StringTokenizer.h"

const char rcsId[] = "$Id: misc.cc,v 1.6 2007/12/27 20:45:15 kewlio Exp $" ;

namespace gnuworld
{

using std::string ;

/**
 * Create a string and copy into it (Key), but convert to all
 * lower case.
 */
string string_lower( const string& Key )
{
// This variable will be the string to be returned
// Make a copy of the original string to speed up
// allocation.
string retMe( Key ) ;

// Iterate through the new string, converting each character
// to lower case
for( string::size_type i = 0, end = retMe.size() ; i < end ; ++i )
	{
	retMe[ i ] = tolower( retMe[ i ] ) ;
	}

// Return by value the new string (this will create a copy of
// the new string)
return retMe ;
}

/**
 * Create a string and copy into it (Key), but convert to all
 * upper case.
 */
string string_upper( const string& Key )
{

// This variable will be the string to be returned
// Make a copy of the original string to speed up
// allocation.
string retMe( Key ) ;

// Iterate through the new string, converting each character
// to upper case
for( string::size_type i = 0, end = retMe.size() ; i < end ; i++ )
	{
	retMe[ i ] = toupper( retMe[ i ] ) ;
	}

// Return by value the new string (this will create a copy of
// the new string)
return retMe ;
}

/**
 * Convert the given string to all lowercase.  This method
 * mutates the original string, yet is much faster than the
 * equivalent method which receives the string by const
 * reference.
 */
void string_tolower( string& Key )
{
for( string::size_type i = 0, end = Key.size() ; i < end ; i++ )
	{
	Key[ i ] = tolower( Key[ i ] ) ;
	}
}

/**
 * Convert the given string to all uppercase.  This method
 * mutates the original string, yet is much faster than the
 * equivalent method which receives the string by const
 * reference.
 */
void string_toupper( string& Key )
{
for( string::size_type i = 0, end = Key.size() ; i < end ; i++ )
	{
	Key[ i ] = toupper( Key[ i ] ) ;
	}
}

/**
 * Return true if this string consists of all numerical
 * [0,9] characters.
 * Return false otherwise. */
bool IsNumeric( const string& s )
{
for( string::const_iterator ptr = s.begin(), endPtr = s.end() ;
	ptr != endPtr ; ++ptr )
	{
	if( !isdigit( *ptr ) )
		{
		return false ;
		}
	}
return true ;
}

/**
 * Return true if this string consists of a time specification
 * [0123456789SsMmHhDd] - it also must begin with a digit.
 */
bool IsTimeSpec( const string& s )
{
	char c;
	int count = 0;

	for ( string::const_iterator ptr = s.begin(), endPtr = s.end() ;
		ptr != endPtr ; ++ptr )
	{
		c = tolower(*ptr);
		/* if this is the start, it must be a digit */
		if ((ptr == s.begin()) && (!isdigit(c)))
			return false;
		/* check for valid characters (digits, smhd) */
		if (!isdigit(c) && c != 's' && c != 'm' && c != 'h' && c != 'd')
			return false;
		/* keep a count of non-numeric characters */
		if (!isdigit(c))
			count++;
		/* maximum of 1 is allowed */
		if (count > 1)
			return false;
	}
	/* if we reach here, this is a valid time specification */
	return true;
}

int strcasecmp( const string& s1, const string& s2 )
{
return ::strcasecmp( s1.c_str(), s2.c_str() ) ;
}

/**
 * Returns the time which is given as #<d/h/m/s> as seconds */

time_t extractTime( string Length, unsigned int defaultUnits )
{
unsigned int Units;

if (defaultUnits == 0)
	Units = 1;
else
	Units = defaultUnits;

if (!strcasecmp(Length.substr(Length.length()-1).c_str(),"d"))
	{
	Units = (24*3600);
	Length.resize(Length.length()-1);
	}
else if(!strcasecmp(Length.substr(Length.length()-1).c_str(),"h"))
        {
        Units = 3600;
        Length.resize(Length.length()-1);
        }
else if(!strcasecmp(Length.substr(Length.length()-1).c_str(),"m"))
        {
        Units = 60;
        Length.resize(Length.length()-1);
        }
else if(!strcasecmp(Length.substr(Length.length()-1).c_str(),"s"))
        {
        Units = 1;
        Length.resize(Length.length()-1);
        }

return atoi(Length.c_str()) * Units;
}

int atoi( const string& val )
{
return ::atoi( val.c_str() ) ;
}

string itoa(int n)
{
	string Result;
	std::ostringstream convert;
	convert << n;
	Result = convert.str();
	return Result;
}

string extractNick(const string& NickUserHostIP)
{
	StringTokenizer st( NickUserHostIP, '!' ) ;

	// Make sure there are exactly two tokens
	if( st.size() != 2 )
		return string("");

	return st[0];
}

string extractUser(const string& NickUserHostIP)
{
	string NickUser = extractNickUser(NickUserHostIP);

	StringTokenizer st( NickUser, '!' ) ;

	if( st.size() != 2 )
		return string("");

	return st[1];
}

string extractNickUser(const std::string& NickUserHostIP)
{
	StringTokenizer st( NickUserHostIP, '@' ) ;

	if( st.size() != 2 )
		return string("");

	return st[0];
}

string extractHostIP(const string& NickUserHostIP)
{
	StringTokenizer st( NickUserHostIP, '@' ) ;

	if( st.size() != 2 )
		return string("");

	return st[1];
}

bool validUserMask(const string& userMask)
{
	// Check that a '@' exists
	StringTokenizer st( userMask, '@' ) ;

	if (st.size() != 2)
	{
		return false ;
	}

	// Check that a '!' exists
	StringTokenizer st1( st[0], '!' ) ;
	if (st1.size() != 2)
	{
		return false ;
	}

	// Be sure that the hostname is no more than 255 characters
	if( st[ 1 ].size() > 255 )
	{
		return false ;
	}

	// Tests have passed
	return true ;
}

bool checkAllValidChars(const string& theString)
{
	const char validChars[]
		= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-";
	for( string::const_iterator ptr = theString.begin() ;
		ptr != theString.end() ; ++ptr )
	{
		/*
		 * 62 entries in the table. 26 + 26 + 2 + 10 digits.
		 */
		bool found = false;
		for (int f = 0; f < 64; f++)
		{
			if(*ptr == validChars[f])
			{
				found = true;
			}
		}
		if (!found)
			return false;
	}

	return true;
}

bool validHostName(const string& hostname)
{
	string _hostname = string_lower(hostname);
	StringTokenizer st( _hostname, '.' ) ;
	if (st.size() == 1)	//no dot found ...
		return false;
	//Domains are at least 2 characters long ...
	if (string(st[st.size()-1]).length() < 2)
		return false;
	if (hostname[0] == '.')	//hostname cannot start with dot
		return false;
	return true;
}

const string prettyDuration( int duration )
{
	// Pretty format a 'duration' in seconds to
	// x day(s), xx:xx:xx.
	char tmpBuf[ 64 ] = {0};

	if (duration == 0)
	{
		sprintf(tmpBuf, "Never");
		return string( tmpBuf ) ;
	}

	int	res = ::time(NULL) - duration,
		secs = res % 60,
		mins = (res / 60) % 60,
		hours = (res / 3600) % 24,
		days = (res / 86400) ;

	sprintf(tmpBuf, "%i day%s, %02d:%02d:%02d",
		days,
		(days == 1 ? "" : "s"),
		hours,
		mins,
		secs );

	return string( tmpBuf ) ;
}

const string TokenStringsParams(const char* format,...)
{
	char buf[ 1024 ] = { 0 } ;
	va_list _list ;

	va_start( _list, format ) ;
	vsnprintf( buf, 1024, format, _list ) ;
	va_end( _list ) ;
	return string(buf);
}
} // namespace gnuworld
