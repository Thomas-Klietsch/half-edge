#pragma once

#include <memory>
#include <string>

#include "./../mesh/half-edge.hpp"
#include "./../mesh/file_format/common.hpp"
// Supported file formats
#include "./../mesh/file_format/stanford.hpp"
#include "./../mesh/file_format/wavefront.hpp"

namespace Mesh {

	std::unique_ptr<Mesh::HalfEdge> Import(
		std::string const file_name,
		Mesh::Type const file_format,
		// Set to true to validate that all edges have a twin
		bool const f_connected = true
	) {
		if ( file_name.size() == 0 ) {
			std::cout << "Import file name is missing.\n";
			return nullptr;
		}

		switch ( file_format ) {
			case Mesh::Type::Stanford:
				return Mesh::FileFormat::Stanford::Import( file_name, f_connected );
			case Mesh::Type::Wavefront:
				return Mesh::FileFormat::Wavefront::Import( file_name, f_connected );
		}

		std::cout << "Failed to import file: \"" << file_name << "\"\n";
		return nullptr;
	};

};
