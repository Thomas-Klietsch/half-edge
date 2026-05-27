#pragma once

#include <memory>
#include <string>

#include "./../mesh/half-edge.hpp"
#include "./../mesh/file_format/common.hpp"
// Supported file formats
#include "./../mesh/file_format/stanford.hpp"
#include "./../mesh/file_format/stl.hpp"
#include "./../mesh/file_format/wavefront.hpp"

namespace Mesh {

	std::unique_ptr<Mesh::HalfEdge> Import(
		std::string const file_name,
		Mesh::FileType const file_format,
		// Set to true to validate that all edges have a twin
		bool const f_connected = true
	) {
		if ( !file_name.size() ) {
			std::cout << "Import file name is missing.\n";
			return nullptr;
		}

		switch ( file_format ) {
			case Mesh::FileType::Stanford:
				return Mesh::FileFormat::Stanford::Import( file_name, f_connected );
			case Mesh::FileType::STL:
				return Mesh::FileFormat::STL::Import( file_name, f_connected );
			case Mesh::FileType::Wavefront:
				return Mesh::FileFormat::Wavefront::Import( file_name, f_connected );
		}

		std::cout << "Failed to import file: \"" << file_name << "\"\n";
		return nullptr;
	};

};
