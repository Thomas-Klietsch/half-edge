#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace Utility {

	// Returns index for 'search', if found in 'list'.
	// Class template ensures compatible data
	template<class T>
	std::optional<std::size_t> GetIndex(
		T const& search,
		std::vector<T> const& list
	) {
		// Slow method
		for ( std::size_t i{ 0 }; i < list.size(); ++i )
			if ( search == list[i] )
				return i;
		// Not found
		return std::nullopt;
	};

};
