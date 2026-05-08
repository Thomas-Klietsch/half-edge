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
#include "./../../utility/get_index.hpp"
#include "./../../utility/tokenise.hpp"

namespace Mesh::FileFormat::Wavefront {

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
			std::cout << "Wavefront::Export file name is missing.\n";
			return false;
		}

		if ( p_mesh->polygon.size() < 1 || p_mesh->vertex.size() < 3 ) {
			std::cout << "Not enough data for Wavefront export: \"" << file_name << "\"\n";
			return false;
		}

		std::ofstream file;
		file.open( file_name, std::ios::trunc );
		// Check if file is open
		if ( !file.is_open() ) {
			std::cout << "Wavefront::Export could not open output file: \"" << file_name << "\"\n";
			return false;
		}

		// Not really needed
		file << "# half-edge export\n";

		// Save material library, if defined. Multi lib support
		if ( p_mesh->material_library.size() )
			for ( auto& text : p_mesh->material_library )
				file << "mtllib " << text << '\n';

		// Write vertices to file
		for ( auto& v : p_mesh->vertex )
			file << "v " << v->location << '\n';

		if ( f_vertex_normal )
			// Write vertex normals to file, uses same index as vertices
			for ( auto& v : p_mesh->vertex )
				file << "vn " << v->normal << '\n';

		// Offset for polygon vertex normal, used for flat shading
		std::size_t offset_vertex_normal = p_mesh->vertex.size();

		if ( f_vertex_texture ) {
			// Write vertex texture data
			for ( auto& v : p_mesh->texture )
				file << "vt " << v->location << '\n';
		}

		// Number of materials defined in mesh
		std::size_t const n_material = p_mesh->material_name.size();

		// Values that might change during parsing of the file
		// Face(s) might be marked as smooth (render)
		bool state_smooth = p_mesh->polygon[0]->f_smooth;

		std::size_t state_material_index = p_mesh->polygon[0]->material_index;
		// Test for out of bound material index
		if ( state_material_index >= n_material )
			state_material_index = 0;

		// Write initial states
		file << "s " << state_smooth << '\n';
		// Only write if a material name is available
		if ( n_material )
			file << "usemtl " << p_mesh->material_name[state_material_index] << '\n';

		// Write faces (polygons) to file
		for ( auto& polygon : p_mesh->polygon ) {

			// Only write texture index if requested and available.
			bool const f_texture = f_vertex_texture & polygon->is_textured();

			if ( n_material )
				// Check for material change
				if ( state_material_index != polygon->material_index ) {
					state_material_index = polygon->material_index;
					// Test for out of bound material index
					if ( state_material_index >= n_material )
						state_material_index = 0;
					file << "usemtl " << p_mesh->material_name[state_material_index] << '\n';
				}

			// Check for smooth change
			if ( state_smooth != polygon->f_smooth ) {
				state_smooth = polygon->f_smooth;
				file << "s " << state_smooth << '\n';
			}

			// FIXME // TODO
			// Terrible, terrible hack, this should be done before looping polygons
			if ( f_vertex_normal && !state_smooth ) {
				++offset_vertex_normal;
				file << "vn " << polygon->unit_normal() << '\n';
			}

			file << "f";
			// Polygon vertices
			for ( auto& edge : polygon->edge ) {
				auto vertex = Utility::GetIndex( edge->vertex, p_mesh->vertex );
				// Vertex (mandatory)
				if ( vertex.has_value() ) {
					file << " " << vertex.value() + 1;
				}
				// Vertex texture (optional)
				if ( f_texture ) {
					auto vt = Utility::GetIndex( edge->texture, p_mesh->texture );
					if ( vt.has_value() )
						file << "/" << vt.value() + 1;
					else
						file << "/";
				}
				// Vertex normal (optional)
				if ( f_vertex_normal ) {
					// If no vertex texture, add empty separator
					if ( !f_texture )
						file << "/";

					if ( state_smooth ) {
						// Vertex and vertex normal have same index
						file << "/" << vertex.value() + 1;
					}
					else {
						// Flat shading
						file << "/" << offset_vertex_normal;
					}
				}
			}
			file << '\n';
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

		// Optional vertex normal data (from file)
		std::vector<Double3> vn;

		// Values that might change during parsing of the file
		// Face(s) might be marked as smooth (render)
		bool parse_state_smooth{ false };
		// Adding default material (file might not specify one)
		p_mesh->material_name.push_back( "default_mesh_material" );

		// If multiple materials are used, track which face(s) needs the select material
		std::uint32_t parse_state_material_index{ 0 };

		// Proccess file
		std::string line;
		while ( std::getline( file, line ) ) {
			// Split line into tokens
			std::vector<std::string> token = Utility::Tokenise( line, " " );

			// Line has no data, or is a comment
			if ( token.size() == 1 || token[0] == "#" )
				continue;

			if ( token[0] == "v" ) {
				// Vertex has either 3 or 4 values
				if ( token.size() < 3 || token.size() > 5 ) {
					std::cout << "Invalid vertex: \"" << line << "\"\n";
					return nullptr;
				}
				auto x = Utility::Decimal( token[1] );
				auto y = Utility::Decimal( token[2] );
				auto z = Utility::Decimal( token[3] );
				if ( !x.has_value() || !y.has_value() || !z.has_value() ) {
					std::cout << "Invalid vertex: \"" << line << "\"\n";
					return nullptr;
				}

				Double3 vertex = Double3( x.value(), y.value(), z.value() );

				// Optional w value, is a scalar
				if ( token.size() == 5 ) {
					auto w = Utility::Decimal( token[4] );
					if ( w.has_value() )
						vertex *= w.value();
				}

				p_mesh->vertex.emplace_back( std::make_shared<Mesh::Data::Vertex>( vertex ) );
			}
			else if ( token[0] == "vn" ) {
				// Vertex normal
				if ( token.size() != 4 ) {
					std::cout << "Invalid vertex normal: \"" << line << "\"\n";
					return nullptr;
				}
				auto x = Utility::Decimal( token[1] );
				auto y = Utility::Decimal( token[2] );
				auto z = Utility::Decimal( token[3] );
				if ( !x.has_value() || !y.has_value() || !z.has_value() ) {
					std::cout << "Invalid vertex normal: \"" << line << "\"\n";
					return nullptr;
				}

				vn.emplace_back( Double3( x.value(), y.value(), z.value() ) );
			}
			else if ( token[0] == "vt" ) {
				// Vertex texture, one value is mandatory, three is supported
				if ( token.size() < 2 || token.size() > 4 ) {
					std::cout << "Invalid vertex texture: \"" << line << "\"\n";
					return nullptr;
				}
				auto x = Utility::Decimal( token[1] );
				if ( !x.has_value() ) {
					std::cout << "Invalid vertex texture: \"" << line << "\"\n";
					return nullptr;
				}

				Double3 vertex = Double3( x.value(), 0, 0. );

				// Optional y value
				if ( token.size() == 3 ) {
					auto y = Utility::Decimal( token[2] );
					if ( y.has_value() )
						vertex.y = y.value();
				}
				// Optional z value
				if ( token.size() == 4 ) {
					auto z = Utility::Decimal( token[3] );
					if ( z.has_value() )
						vertex.z = z.value();
				}

				p_mesh->texture.emplace_back( std::make_shared<Mesh::Data::Texture>( vertex ) );
			}
			else if ( token[0] == "f" ) {
				// Face (polygon)
				if ( token.size() < 4 ) {
					// The half edge algorithms used are for triangles only.
					// Non-triangles might not be planar or convex.
					std::cout << "Only polygons with at least three edges are supported\n";
					return nullptr;
				}

				// Temporary polygon data
				std::vector<std::shared_ptr<Mesh::Data::Edge>> data;

				// Parse all polygon vertices
				for ( std::size_t i{ 1 }; i < token.size(); ++i ) {
					std::vector<std::string> vertex = Utility::Tokenise( token[i], "/" );
					// Face vertex is mandatory, only positive index is supported
					auto v_index = Utility::Integer( vertex[0] );
					if ( !v_index.has_value() || v_index.value() < 1 ) {
						std::cout << "Face (polygon) has invalid, or negative, integer\n";
						return nullptr;
					}

					auto v = p_mesh->vertex[v_index.value() - 1];

					// No vertex texture (UV) or vertex normal defined yet
					std::shared_ptr<Mesh::Data::Edge> edge
						= std::make_shared<Mesh::Data::Edge>( v, false, nullptr, nullptr );

					if ( vertex.size() == 2 ) {
						// Vertex texture
						if ( vertex[1].size() > 0 ) {
							auto vt_index = Utility::Integer( vertex[1] );
							if ( !vt_index.has_value() || vt_index.value() < 1 ) {
								std::cout << "Face (polygon) has invalid, or negative, integer\n";
								return nullptr;
							}
							edge->texture = p_mesh->texture[vt_index.value() - 1];
						}
					}
					else if ( vertex.size() == 3 ) {
						// Vertex normal and possible vertex texture
						// Vertex texture
						if ( vertex[1].size() > 0 ) {
							auto vt_index = Utility::Integer( vertex[1] );
							if ( !vt_index.has_value() || vt_index.value() < 1 ) {
								std::cout << "Face (polygon) has invalid, or negative, integer\n";
								return nullptr;
							}
							edge->texture = p_mesh->texture[vt_index.value() - 1];
						}
						// Vertex normal
						if ( vertex[2].size() > 0 ) {
							auto vn_index = Utility::Integer( vertex[2] );
							if ( !vn_index.has_value() || vn_index.value() < 1 ) {
								std::cout << "Face (polygon) has invalid, or negative, integer\n";
								return nullptr;
							}
							v->normal = vn[vn_index.value() - 1];
						}
					}
					// Add edge to temporary polygon data
					data.emplace_back( edge );
				}
				// Store polygon
				p_mesh->add_polygon( data, parse_state_material_index, parse_state_smooth );
			}
			else if ( token[0] == "s" ) {
				// Smooth on/off, set the smooth state for subsequent faces
				if ( token.size() != 2 ) {
					std::cout << "Invalid s (smooth): \"" << line << "\"\n";
					return nullptr;
				}
				if ( token[1] == "0" || token[1] == "off" )
					parse_state_smooth = false;
				else if ( token[1] == "1" || token[1] == "on" )
					parse_state_smooth = true;
				else {
					std::cout << "Invalid s (smooth): \"" << line << "\"\n";
					return nullptr;
				}
			}
			else if ( token[0] == "usemtl" ) {
				// usemtl (material), set the material index for subsequent faces
				if ( token.size() != 2 ) {
					// Spaces are not valid in a name
					std::cout << "Invalid usemtl: \"" << line << "\"\n";
					return nullptr;
				}

				// Lambda function
				// Returns the index for material name
				auto MaterialIndex = [&]( std::string text )->std::uint32_t
				{
					for ( std::uint32_t i{ 0 }; i < p_mesh->material_name.size(); ++i )
						if ( text == p_mesh->material_name[i] )
							return i;
					// If not found, add text as new material name, and return index of it
					p_mesh->material_name.push_back( text );
					return p_mesh->material_name.size() - 1;
				};

				parse_state_material_index = MaterialIndex( token[1] );
			}
			else if ( token[0] == "mtllib" && line.size() > 7 ) {
				// Material library file name, multiple files are supported
				p_mesh->material_library.push_back( line.substr( 7 ) ); // skip "mtllib "
			}
		} // End of file parsing

		// Clear vertex normal data
		vn.clear();
		vn.shrink_to_fit();

		std::cout << "Imported : " << file_name << '\n';
		std::cout << "Vertex   : " << p_mesh->vertex.size() << '\n';
		std::cout << "Edge     : " << p_mesh->edge.size() << '\n';
		std::cout << "Polygon  : " << p_mesh->polygon.size() << '\n';

		p_mesh->connect_shared_edges( f_connected );
		return p_mesh;
	};

};
