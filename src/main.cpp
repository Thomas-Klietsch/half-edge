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

#include <iostream>
#include <memory>
#include <string>

#include "./mesh/export.hpp"
#include "./mesh/half-edge.hpp"
#include "./mesh/import.hpp"

// Convert of file format, currently only Wavefront is supported
void ConvertFile() {
	// Load mesh into half-data, without twin edge validation
	std::unique_ptr<Mesh::HalfEdge> p_input = Mesh::Import( "input.obj", Mesh::Type::Wavefront, false );
	// Save input to a file
	Mesh::Export( "test.obj", p_input, Mesh::Type::Wavefront );
};

// Create a new, minimal, half-edge mesh,
// with copy of material data from input file
void MinimalMesh() {
	// Load mesh into half-data
	std::unique_ptr<Mesh::HalfEdge> p_input = Mesh::Import( "input.obj", Mesh::Type::Wavefront, false );

	// Create work structure, including material data copy
	std::unique_ptr<Mesh::HalfEdge> p_output = std::make_unique<Mesh::HalfEdge>( p_input );

	// Add three non-static vertices to half-edge data
	auto v1 = p_output->add_vertex( Double3( 1, 0, 0 ), false );
	auto v2 = p_output->add_vertex( Double3( 0, 1, 0 ), false );
	auto v3 = p_output->add_vertex( Double3( 0, 0, 1 ), false );

	// Create 3 edges, they are not stored in half-edge data,
	// all are non-static, with no texture, and no polygon id
	auto edge1 = std::make_shared<Mesh::Data::Edge>( v1 );
	auto edge2 = std::make_shared<Mesh::Data::Edge>( v2 );
	auto edge3 = std::make_shared<Mesh::Data::Edge>( v3 );

	// Add a triangle (and edges) to half-edge data; if polygon is valid
	std::uint32_t const material_index = 0; // First material, or default if none are defined
	bool const f_smooth{ false }; // Flat shading
	// Add polygon to data, this also adds edges to data
	p_output->add_polygon( { edge1, edge2, edge3 }, material_index, f_smooth );

	// Save half-data as Wavefront data
	if ( !Mesh::Export( "minimal.obj", p_output, Mesh::Type::Wavefront ) )
		std::cout << "Failed to export half edge data.\n";
};

// Create a cube with all edges connected
std::unique_ptr<Mesh::HalfEdge> CreateCube() {
	auto p_mesh = std::make_unique<Mesh::HalfEdge>();
	// Cube vertices, added to half-edge data
	auto v1 = p_mesh->add_vertex( Double3( +1, +1, +1 ) );
	auto v2 = p_mesh->add_vertex( Double3( -1, +1, +1 ) );
	auto v3 = p_mesh->add_vertex( Double3( +1, -1, +1 ) );
	auto v4 = p_mesh->add_vertex( Double3( -1, -1, +1 ) );
	auto v5 = p_mesh->add_vertex( Double3( +1, +1, -1 ) );
	auto v6 = p_mesh->add_vertex( Double3( -1, +1, -1 ) );
	auto v7 = p_mesh->add_vertex( Double3( +1, -1, -1 ) );
	auto v8 = p_mesh->add_vertex( Double3( -1, -1, -1 ) );
	// Cube polygons, default material, smooth shaded
	p_mesh->add_polygon( {
		std::make_shared<Mesh::Data::Edge>( v1 ),
		std::make_shared<Mesh::Data::Edge>( v2 ),
		std::make_shared<Mesh::Data::Edge>( v4 ),
		std::make_shared<Mesh::Data::Edge>( v3 ),
		}, 0, true );
	p_mesh->add_polygon( {
		std::make_shared<Mesh::Data::Edge>( v5 ),
		std::make_shared<Mesh::Data::Edge>( v7 ),
		std::make_shared<Mesh::Data::Edge>( v8 ),
		std::make_shared<Mesh::Data::Edge>( v6 ),
		}, 0, true );
	p_mesh->add_polygon( {
		std::make_shared<Mesh::Data::Edge>( v1 ),
		std::make_shared<Mesh::Data::Edge>( v3 ),
		std::make_shared<Mesh::Data::Edge>( v7 ),
		std::make_shared<Mesh::Data::Edge>( v5 ),
		}, 0, true );
	p_mesh->add_polygon( {
		std::make_shared<Mesh::Data::Edge>( v2 ),
		std::make_shared<Mesh::Data::Edge>( v6 ),
		std::make_shared<Mesh::Data::Edge>( v8 ),
		std::make_shared<Mesh::Data::Edge>( v4 ),
		}, 0, true );
	p_mesh->add_polygon( {
		std::make_shared<Mesh::Data::Edge>( v3 ),
		std::make_shared<Mesh::Data::Edge>( v4 ),
		std::make_shared<Mesh::Data::Edge>( v8 ),
		std::make_shared<Mesh::Data::Edge>( v7 ),
		}, 0, true );
	p_mesh->add_polygon( {
		std::make_shared<Mesh::Data::Edge>( v1 ),
		std::make_shared<Mesh::Data::Edge>( v5 ),
		std::make_shared<Mesh::Data::Edge>( v6 ),
		std::make_shared<Mesh::Data::Edge>( v2 ),
		}, 0, true );

	// Connect shared edges and test for closed surface,
	// returned value of true if it is.
	if ( !p_mesh->connect_shared_edges( true ) ) {
		std::cout << "Not a unique closed surface mesh.\n";
		return nullptr;
	}

	return p_mesh;
};

void ReplacePolygon(
	std::unique_ptr<Mesh::HalfEdge>& p_mesh
) {
	// Select a polygon to replace
	auto& p_polygon = p_mesh->polygon[5];

	// Copy polygon vertex locations
	std::vector<Double3> vertex;
	for ( auto& e : p_polygon->edge )
		vertex.push_back( e->vertex->location );

	// Polygon centre, shifted along polygon normal
	Double3 centre = p_polygon->centre() + p_polygon->unit_normal();

	// Erase the polygon.
	// NOTE! p_polygon is now a nullptr!
	p_mesh->remove_polygon( p_polygon );

	// Clean is needed when data is removed,
	// as it might contain uninitialised (nullptr) data.
	p_mesh->clean_up();

	// Save intermediate mesh
	Mesh::Export( "cube_open.obj", p_mesh, Mesh::Type::Wavefront, Mesh::VertexNormal::Weighted );

	// Replace the deleted polygon with a triangle fan.
	std::size_t const n_vertex = vertex.size();
	auto const v0 = p_mesh->add_vertex( centre );
	for ( std::size_t i{ 0 }; i < n_vertex;++i ) {
		auto v1 = p_mesh->add_vertex( vertex[i] );
		auto v2 = p_mesh->add_vertex( vertex[( i + 1 ) % n_vertex] );
		p_mesh->add_polygon( {
			std::make_shared<Mesh::Data::Edge>( v0 ),
			std::make_shared<Mesh::Data::Edge>( v1 ),
			std::make_shared<Mesh::Data::Edge>( v2 )
			}, 0, true );
	}

	// Connect all edges, no unconnected edges should be present.
	if ( !p_mesh->connect_shared_edges( true ) )
		std::cout << "Uh oh, something went wrong\n";

	// Save final mesh
	Mesh::Export( "cube_modified.obj", p_mesh, Mesh::Type::Wavefront, Mesh::VertexNormal::Weighted );
};

int main(
	[[maybe_unused]] int argc,
	[[maybe_unused]] char* argv[]
) {
	ConvertFile();

	MinimalMesh();

	auto p_mesh = CreateCube();
	if ( !p_mesh ) {
		std::cout << "Valid connected mesh needed for ReplacePolygon().\n";
		return EXIT_FAILURE;
	}

	// Save half-data (cube) as Wavefront data, with weighted vertex normals
	if ( !Mesh::Export( "cube.obj", p_mesh, Mesh::Type::Wavefront, Mesh::VertexNormal::Weighted ) )
		std::cout << "Failed to export half edge data.\n";

	ReplacePolygon( p_mesh );

	return EXIT_SUCCESS;
};
