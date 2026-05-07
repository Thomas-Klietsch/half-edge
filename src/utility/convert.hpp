#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

namespace Utility {

	// Convert text to integer, if valid
	std::optional<std::int32_t> Integer(
		std::string const& text
	) {
		// Nothing to convert
		if ( !text.size() )
			return std::nullopt;

		std::int32_t value;
		try {
			value = std::stoi( text );
		}
		catch ( ... ) {
			std::cout << "Utility::Integer error on text: \"" << text << "\"\n";
			return std::nullopt;
		}
		return value;
	};
	
	// Convert text to double, if valid
	std::optional<std::double_t> Decimal(
		std::string const& text
	) {
		// Nothing to convert
		if ( !text.size() )
			return std::nullopt;

		std::double_t value;
		try {
			value = std::stod( text );
		}
		catch ( ... ) {
			std::cout << "Utility::Decimal error on text: \"" << text << "\"\n";
			return std::nullopt;
		}
		return value;
	};

};
