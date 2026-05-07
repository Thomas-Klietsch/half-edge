// Copyright (c) 2026 Thomas Klietsch, all rights reserved.
//
// Licensed under the GNU Lesser General Public License, version 3.0 or later
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or ( at your option ) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General
// Public License along with this program.If not, see < https://www.gnu.org/licenses/>. 

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Utility {

	// Split text into token(s), depending on delimiter(s).
	std::vector<std::string> Tokenise(
		std::string_view const& text,
		std::string_view const& delimiter,
		// True: Multiple sequential delemiters create tokens.
		// For example "1,,2"; true->{"1","","2"}; false->{"1","2"}
		bool const f_keep_empty = false
	) {
		// text is too short to split
		if ( text.size() < 2 )
			return{ std::string( text ) };

		// Result
		std::vector<std::string> tokens;

		// Running index in text
		std::size_t position{ 0 };

		while ( position < text.size() ) {
			std::size_t end = text.find_first_of( delimiter, position );
			if ( end >= std::string::npos )
				end = text.size();
			if ( end - position > 0 )
				// Add non-empty token
				tokens.push_back( std::string( text.substr( position, end - position ) ) );
			else
				// Add empty token, if requested
				if ( f_keep_empty )
					tokens.push_back( "" );
			position = end + 1;
		}

		return tokens;
	};

};
