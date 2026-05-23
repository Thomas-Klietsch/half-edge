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

// Format created Greg Turk (1990)

namespace Mesh::FileFormat::Stanford {

	bool Export(
		std::string const& file_name,
		std::unique_ptr<Mesh::HalfEdge> const& p_mesh,
		// True; if available, vertex normals are written to file
		bool const f_vertex_normal = true,
		// True; if available, vertex textures are written to file
		bool const f_vertex_texture = true
	) {
		if ( p_mesh == nullptr ) {
			std::cout << "Fatal error! Export of nullptr. File name: \"" << file_name << "\"\n";
			std::exit( EXIT_FAILURE );
		}

		if ( file_name.size() == 0 ) {
			std::cout << "Stanford::Export file name is missing.\n";
			return false;
		}

		if ( p_mesh->polygon.size() < 1 || p_mesh->vertex.size() < 3 ) {
			std::cout << "Not enough data for Stanford export: \"" << file_name << "\"\n";
			return false;
		}

		std::ofstream file;
		file.open( file_name, std::ios::trunc );
		// Check if file is open
		if ( !file.is_open() ) {
			std::cout << "Stanford::Export could not open output file: \"" << file_name << "\"\n";
			return false;
		}

		// Write header

		// Mandatory
		file << "ply\n";
		file << "format ascii 1.0\n";

		// Not really needed
		file << "comment half-edge export\n";

		// Vertex count and format
		if ( f_vertex_texture ) {
			// Get total number of vertices referenced, since each
			// shared vertex is assumed to have unique vertex texture coordinate
			std::uint64_t count{ 0 };
			for ( auto const& poly : p_mesh->polygon )
				count += poly->edge.size();
			file << "element vertex " << count << '\n';
		}
		else {
			file << "element vertex " << p_mesh->vertex.size() << '\n';
		}
		file << "property float x\n";
		file << "property float y\n";
		file << "property float z\n";

		if ( f_vertex_normal ) {
			// Vertex normal format
			file << "property float nx\n";
			file << "property float ny\n";
			file << "property float nz\n";
		}

		if ( f_vertex_texture ) {
			// Vertex texture format
			file << "property float s\n";
			file << "property float t\n";
		}

		// Polygon count and format
		file << "element face " << p_mesh->polygon.size() << '\n';
		file << "property list uchar uint vertex_indices\n";

		// End header
		file << "end_header\n";

		// Write data
		if ( f_vertex_texture ) {
			// Vertex texture needs special treatment during export,
			// since it is not a per vertex value (like normals).

			// Write vertices, using edges from polygons.
			for ( auto const& p : p_mesh->polygon ) {
				for ( auto& edge : p->edge ) {
					file << edge->vertex->location;
					if ( f_vertex_normal )
						file << ' ' << edge->vertex->normal;
					file << ' ' << edge->texture->location.x << ' ' << edge->texture->location.y;
					file << '\n';
				}
			}

			// Write polygons, using edges.
			std::uint32_t index{ 0 };
			for ( auto const& p : p_mesh->polygon ) {
				file << p->edge.size();
				for ( std::uint16_t i{ 0 }; i < p->edge.size(); ++i )
					file << ' ' << index++;
				file << '\n';
			}
		}
		else {
			// Write vertices
			for ( auto const& v : p_mesh->vertex ) {
				file << v->location;
				if ( f_vertex_normal )
					file << ' ' << v->normal;
				file << '\n';
			}

			// Write polygons
			for ( auto const& p : p_mesh->polygon ) {
				file << p->edge.size();
				for ( auto& edge : p->edge ) {
					auto vertex = p_mesh->index_vertex( edge->vertex );
					if ( vertex.has_value() )
						file << ' ' << vertex.value();
				}
				file << '\n';
			}
		}

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

		// Check PLY header
		std::getline( file, line );
		if ( line != "ply" ) {
			std::cout << "Error. File is not a PLY file.\n";
			return nullptr;
		}

		std::getline( file, line );
		{
			auto token = Utility::Tokenise( line, " " );
			if ( token.size() != 3 ) {
				std::cout << "Error. File is not a PLY file.\n";
				return nullptr;
			}
			if ( token[1] != "ascii" ) {
				std::cout << "Binary file format is not supported.\n";
				return nullptr;
			}
			if ( token[2] != "1.0" ) {
				std::cout << "Only version 1.0 is supported.\n";
				return nullptr;
			}
		}

		// Number, read from file, of vertices and polygons.
		std::uint32_t n_vertex{ 0 };
		std::uint32_t n_polygon{ 0 };

		// State read from file.
		bool f_vertex_normal{ false };
		bool f_vertex_texture{ false };

		// Finish header parsing
		while ( std::getline( file, line ) ) {
			if ( line == "end_header" )
				break;

			auto token = Utility::Tokenise( line, " " );

			// Vertices have normal and/or texture
			if ( token.size() == 3 && token[0] == "property" ) {
				if ( token[2] == "nx" || token[2] == "ny" || token[2] == "nz" )
					f_vertex_normal = true;
				if ( token[2] == "s" || token[2] == "t" )
					f_vertex_texture = true;
			}

			// Number of vertices and polygons
			if ( token.size() == 3 && token[0] == "element" ) {
				auto number = Utility::Integer( token[2] );
				if ( number.has_value() ) {
					if ( token[1] == "vertex" )
						n_vertex = number.value();
					else if ( token[1] == "face" )
						n_polygon = number.value();
				}
			}

		} // End of header parsing

		if ( n_vertex < 4 || n_polygon < 1 ) {
			std::cout << "Not enough data in file.\n";
			return nullptr;
		}

		// Number of vertex elements, mandatory 3 for location
		std::uint8_t const n_element = 3 + ( f_vertex_normal ? 3 : 0 ) + ( f_vertex_texture ? 2 : 0 );

		// Parse vertex data
		std::uint32_t c_vertex{ 0 };
		std::vector<Mesh::Data::Vertex> vertex_list;
		while ( std::getline( file, line ) ) {
			auto token = Utility::Tokenise( line, " " );
			if ( token.size() != n_element ) {
				std::cout << "Vertex data size mismatch.\n";
				return nullptr;
			}

			Mesh::Data::Vertex vertex;

			auto x = Utility::Decimal( token[0] );
			auto y = Utility::Decimal( token[1] );
			auto z = Utility::Decimal( token[2] );
			if ( x.has_value() && y.has_value() && z.has_value() )
				vertex.location = Double3( x.value(), y.value(), z.value() );
			else {
				std::cout << "Invalid vertex data.\n";
				return nullptr;
			}

			if ( f_vertex_normal ) {
				auto nx = Utility::Decimal( token[3] );
				auto ny = Utility::Decimal( token[4] );
				auto nz = Utility::Decimal( token[5] );

				if ( nx.has_value() && ny.has_value() && nz.has_value() )
					vertex.normal = Double3( nx.value(), ny.value(), nz.value() );
				else {
					std::cout << "Invalid vertex normal data.\n";
					return nullptr;
				}

				if ( f_vertex_texture ) {
					auto s = Utility::Decimal( token[6] );
					auto t = Utility::Decimal( token[7] );
					if ( s.has_value() && t.has_value() )
						p_mesh->texture.emplace_back( std::make_shared<Mesh::Data::Texture>( Double3( s.value(), t.value(), 0. ) ) );
					else {
						std::cout << "Invalid vertex texture data.\n";
						return nullptr;
					}
				}
			}
			else if ( f_vertex_texture ) {
				auto s = Utility::Decimal( token[3] );
				auto t = Utility::Decimal( token[4] );
				if ( s.has_value() && t.has_value() )
					p_mesh->texture.emplace_back( std::make_shared<Mesh::Data::Texture>( Double3( s.value(), t.value(), 0. ) ) );
				else {
					std::cout << "Invalid vertex texture data.\n";
					return nullptr;
				}
			}

			// Store data.
			vertex_list.emplace_back( vertex );

			// Check if all vertices have been read.
			if ( ++c_vertex == n_vertex )
				break;
		} // End of vertex data

		// Parse polygon data
		std::uint32_t c_polygon{ 0 };
		while ( std::getline( file, line ) ) {
			auto token = Utility::Tokenise( line, " " );

			auto const n_edge = Utility::Integer( token[0] );
			if ( !n_edge.has_value() || n_edge < 3 || static_cast<std::size_t>( n_edge.value() ) + 1 != token.size() ) {
				std::cout << "Polygon has mismatched vertex count.\n";
				return nullptr;
			}

			// Temporary polygon data
			std::vector<std::shared_ptr<Mesh::Data::Edge>> data;

			// Process vertices
			for ( std::size_t i{ 1 }; i < token.size(); ++i ) {
				auto v_index = Utility::Integer( token[i] );
				if ( !v_index.has_value() || v_index.value() < 0 || static_cast<std::uint32_t>( v_index.value() ) > n_vertex ) {
					std::cout << "Polygon data has invalid integer.\n";
					return nullptr;
				}

				auto& v = p_mesh->add_vertex( vertex_list[v_index.value()].location );
				if ( f_vertex_normal )
					v->normal = vertex_list[v_index.value()].normal;
				// No vertex texture (UV), not a hard edge.
				Mesh::Data::Edge edge( v, false );
				// If available add texture data.
				if ( f_vertex_texture )
					edge.texture = p_mesh->texture[v_index.value()];
				// Store data.
				data.emplace_back( std::make_shared<Mesh::Data::Edge>( edge ) );
			}

			// No material support, all polygons have the same shading.
			p_mesh->add_polygon( data, 0, f_vertex_normal );

			// Check if all polygons have been read.
			if ( ++c_polygon == n_polygon )
				break;
		} // End of polygon data

		// Any remaining data is not supported.
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
