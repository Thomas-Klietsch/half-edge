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

#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "./../../mathematics/double3.hpp"
#include "./../../mesh/data.hpp"
#include "./../../mesh/half-edge.hpp"
#include "./../../utility/convert.hpp"
#include "./../../utility/tokenise.hpp"

// Format created by Chuck Hull (1987)

namespace Mesh::FileFormat::STL {

	bool Export(
		std::string const& file_name,
		std::unique_ptr<Mesh::HalfEdge> const& p_mesh,
		// True; triangle normals are written to file
		bool const f_normal = true
	) {
		if ( p_mesh == nullptr ) {
			std::cout << "Fatal error! Export of nullptr. File name: \"" << file_name << "\"\n";
			std::exit( EXIT_FAILURE );
		}

		if ( file_name.size() == 0 ) {
			std::cout << "Export file name is missing.\n";
			return false;
		}

		if ( p_mesh->polygon.size() < 1 || p_mesh->vertex.size() < 3 ) {
			std::cout << "Not enough data for export: \"" << file_name << "\"\n";
			return false;
		}

		std::ofstream file;
		file.open( file_name, std::ios::trunc );
		// Check if file is open
		if ( !file.is_open() ) {
			std::cout << "Export could not open output file: \"" << file_name << "\"\n";
			return false;
		}

		// Write header
		file << "solid half-edge export\n";

		// Write triangles from polygons
		for ( auto const& p : p_mesh->polygon ) {
			if ( f_normal )
				file << "facet normal " << p->unit_normal() << '\n';
			else
				file << "facet normal \n"; // Trailing space is required
			file << " outer loop\n";
			for ( auto const& e : p->edge )
				file << "  vertex " << e->vertex->location << '\n';
			file << " endloop\n";
			file << "endfacet\n";
		}

		// End of file
		file << "endsolid half-edge export\n";

		file.close();

		std::cout << "Exported : " << file_name << '\n';
		std::cout << "Vertex   : " << p_mesh->vertex.size() << '\n';
		std::cout << "Polygon  : " << p_mesh->polygon.size() << '\n';

		return true;
	};

	std::unique_ptr<Mesh::HalfEdge> Import(
		std::string const file_name,
		// Set to true to validate that all edges have a twin
		bool const f_connected = true
	) {
		// Open file for reading
		std::ifstream file;
		file.open( file_name );
		if ( !file.is_open() ) {
			std::cout << "Could not open the input file: \"" << file_name << "\"\n";
			return nullptr;
		}

		// Data storage
		std::unique_ptr<Mesh::HalfEdge> p_mesh = std::make_unique<Mesh::HalfEdge>();

		std::string line;

		// Check STL header
		std::getline( file, line );
		{
			auto token = Utility::Tokenise( line, " \t" );
			if ( token[0] != "solid" ) {
				std::cout << "Error. File is not a STL (ASCII) file.\n";
				return nullptr;
			}
		}

		// Parse triangle data, this is a fixed format
		/*
		facet normal nx ny nz
			outer loop
				vertex x y z
				vertex x y z
				vertex x y z
			endloop
		endfacet
		*/
		while ( std::getline( file, line ) ) {
			auto token = Utility::Tokenise( line, " \t" );

			// End of file data
			if ( token[0] == "endsolid" )
				break;

			// Note: Line might contain a "- " before nx.
			if ( token[0] != "facet" ) {
				std::cout << "Expected data: facet. Got: " << line << '\n';
				break;
			}

			std::getline( file, line );
			token = Utility::Tokenise( line, " \t" );
			if ( token[0] != "outer" ) {
				std::cout << "Expected data: outer loop. Got: " << line << '\n';
				break;
			}

			// Temporary polygon data
			std::vector<std::shared_ptr<Mesh::Data::Edge>> data;

			// Get triangle vertices
			for ( std::uint8_t i{ 0 }; i < 3; ++i ) {
				std::getline( file, line );
				token = Utility::Tokenise( line, " \t" );
				if ( token[0] == "vertex" && token.size() == 4 ) {
					auto x = Utility::Decimal( token[1] );
					auto y = Utility::Decimal( token[2] );
					auto z = Utility::Decimal( token[3] );
					if ( !x.has_value() || !y.has_value() || !z.has_value() ) {
						std::cout << "Invalid vertex data. Got: " << line << '\n';
						return nullptr;
					}
					auto v = p_mesh->add_vertex( Double3( x.value(), y.value(), z.value() ) );
					Mesh::Data::Edge edge( v, false );
					data.emplace_back( std::make_shared<Mesh::Data::Edge>( edge ) );
				}
				else {
					std::cout << "Expected data: vertex x y z. Got: " << line << '\n';
					return nullptr;
				}
			}

			// Add triangle
			p_mesh->add_polygon( data );

			std::getline( file, line );
			token = Utility::Tokenise( line, " \t" );
			if ( token[0] != "endloop" ) {
				std::cout << "Expected data: endloop. Got: " << line << '\n';
				return nullptr;
			}

			std::getline( file, line );
			token = Utility::Tokenise( line, " \t" );
			if ( token[0] != "endfacet" ) {
				std::cout << "Expected data: endfacet. Got: " << line << '\n';
				return nullptr;
			}

		} // End of triangle data

		file.close();

		std::cout << "Imported : " << file_name << '\n';
		std::cout << "Vertex   : " << p_mesh->vertex.size() << '\n';
		std::cout << "Edge     : " << p_mesh->edge.size() << '\n';
		std::cout << "Polygon  : " << p_mesh->polygon.size() << '\n';

		p_mesh->sort_data();
		p_mesh->connect_shared_edges( f_connected );
		return p_mesh;
	};

};
