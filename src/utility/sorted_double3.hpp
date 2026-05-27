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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "./../mathematics/double3.hpp"

namespace Utility {

	// Mathness:: Terrible name for a class. >_<

	// Values added to the list are sorted (xyz order), if not already in the list.
	// Axis differences in value by existing and value (Double3) being added,
	// are considered equal if below EPSILON_EQUAL (defined in the Double3 class).
	// For instance A=(0.0.0), B=(0,0,-epsilon), C=(0,0,+epsilon).
	// If A is added first, B and C are equal to A, and data only have one value (A).
	// If B or C is added first, data have two values (B and C).
	class SortedDouble3 final {

	private:

		std::vector<Double3> data;

	public:

		SortedDouble3() {};

		~SortedDouble3() {
			// Garbage collection, vector is released at end of scope,
			// but hey, just to be safe.
			data.clear();
			data.shrink_to_fit();
		};

		// Adds a Double3 to the ordered data, if it does not exist.
		void add(
			Double3 const& value
		) {
			// Find lowest index which contain value
			auto index = std::lower_bound( data.begin(), data.end(), value, IsLesser );
			// End of list reach, e.g. value is not in the list
			if ( index == data.end() ) {
				// Everything is less than value, add it at the end
				data.emplace_back( value );
				return;
			}

			// Lesser, but not equal; insert it
			if ( value != data[index - data.begin()] )
				data.insert( index, value );
		};

		// Returns index for 'search', if found in data.
		std::optional<std::size_t> get_index(
			Double3 const& search
		) const {
			// Find lowest index which contain value
			auto index = std::lower_bound( data.begin(), data.end(), search, IsLesser );
			// End of list reach, e.g. value is not in the list
			if ( index == data.end() )
				return std::nullopt;

			std::size_t const result = index - data.begin();

			// Lesser, but not equal
			if ( search != data[result] )
				return std::nullopt;

			return result;
		};

		// Read only access to internal data.
		std::vector<Double3> const& access_data() const {
			return data;
		};

	private:

		// Compare evaluation, used for sorted list insertion.
		static bool IsLesser(
			Double3 const& l,
			Double3 const& r
		) {
			Double3 const diff = l - r;

			// Test X
			if ( diff.x < -EPSILON_EQUAL )
				return true;

			// X is equal
			if ( std::abs( diff.x ) <= EPSILON_EQUAL ) {
				// Test Y
				if ( diff.y < -EPSILON_EQUAL )
					return true;
				// Y is equal
				if ( std::abs( diff.y ) <= EPSILON_EQUAL )
					// Test Z
					if ( diff.z < -EPSILON_EQUAL )
						return true;
			}

			// Larger or equal
			return false;
		};

	};

};
