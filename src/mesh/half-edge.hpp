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
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "./../mesh/data.hpp"

namespace Mesh {

	class HalfEdge final {

	public:

		// List of unique 3D vertices
		std::vector<std::shared_ptr<Mesh::Data::Vertex>> vertex;

		// List of unique 3D texture vertices
		std::vector<std::shared_ptr<Mesh::Data::Texture>> texture;

		// Unordered
		std::vector<std::shared_ptr<Mesh::Data::Edge>> edge;

		// Unordered
		std::vector<std::shared_ptr<Mesh::Data::Polygon>> polygon;

		// Material library file name(s)
		std::vector<std::string> material_library;

		// Materials
		std::vector<std::string> material_name;

	public:

		HalfEdge() {};

		// Constructor which copy material data
		HalfEdge(
			std::unique_ptr<HalfEdge> const& p_input
		)
			: material_library( p_input->material_library )
			, material_name( p_input->material_name )
		{};

		// Adds a Double3 to the unorder vertex vector, if it does not exist.
		// Return is the memory address of the (added) vertex.
		std::shared_ptr<Mesh::Data::Vertex>& add_vertex(
			Double3 const& value,
			bool const f_static = false
		) {
			// TODO ordered vector, and lower_bound search

			// Slow search
			for ( std::size_t index{ 0 }; index < vertex.size(); ++index )
				if ( vertex[index]->location == value )
					return vertex[index];

			vertex.emplace_back( std::make_shared<Mesh::Data::Vertex>( value, f_static ) );
			return vertex[vertex.size() - 1];
		};

		// Adds a Double3 to the texture vector, if it does not exist
		// Return is the memory address of the (added) texture.
		std::shared_ptr<Mesh::Data::Texture>& add_texture(
			Double3 const& value
		) {
			// TODO ordered vector, and lower_bound search

			// Slow search
			for ( std::size_t index{ 0 }; index < texture.size(); ++index )
				if ( texture[index]->location == value )
					return texture[index];

			texture.emplace_back( std::make_shared<Mesh::Data::Texture>( value ) );
			return texture[texture.size() - 1];
		};

		// Create and add polygon to list
		void add_polygon(
			std::vector<std::shared_ptr<Mesh::Data::Edge>> const& p_edge,
			std::uint32_t const material_id = 0,
			bool const f_smooth = false
		) {
			if ( p_edge.size() < 3 ) {
				std::cout << "Invalid add_polygon, need at least 3 edges\n";
				return;
			}

			// Create polygon
			auto poly = std::make_shared<Mesh::Data::Polygon>( p_edge, material_id, f_smooth );

			for ( auto& e : poly->edge ) {
				// Set poly id on edge
				e->polygon = poly;
				// Add edge to half-edge data
				edge.emplace_back( e );
			}

			polygon.emplace_back( poly );
		};

		// TODO use index?
		// Releases all data associated with the polygon,
		// sets it to a nullptr, this includes input reference.
		void remove_polygon(
			std::shared_ptr<Mesh::Data::Polygon>& p_polygon
		) {
			if ( p_polygon == nullptr )
				return;

			p_polygon->release();
			p_polygon = nullptr;
		};

		// Removes nullptr and unused data
		void clean_up() {
			// For a use_count of one, only the smart pointer
			// containing the data is used, so it can be removed.

			// Multiple usage
			std::size_t index;
			// .size() is the number of elements.
			// If zero (0), then index-->0 evaluate to false, and loop is skipped/exited.
			// If (for example) 5, then index becomes 4, which is the 5th element.
			// When index is one (1), then index-->0 is true, after evaluation
			// index becomes zero, which is the first element.

			index = texture.size();
			for ( ; index-- > 0;) {
				std::cout << index << '\n';
				if ( texture[index] == nullptr || texture[index].use_count() < 2 )
					texture.erase( texture.begin() + index );
			}

			index = vertex.size();
			for ( ; index-- > 0;) {
				if ( vertex[index] == nullptr || vertex[index].use_count() < 2 )
					vertex.erase( vertex.begin() + index );
			}

			index = edge.size();
			for ( ; index-- > 0;) {
				if ( edge[index] == nullptr || edge[index].use_count() < 2 )
					edge.erase( edge.begin() + index );
			}

			index = polygon.size();
			for ( ; index-- > 0;) {
				if ( polygon[index] == nullptr || polygon[index].use_count() < 2 )
					polygon.erase( polygon.begin() + index );
			}
		};

		// Find all shared edges, and set twin state.
		bool connect_shared_edges(
			// Set to true to validate that all edges have a twin
			bool const f_connected = true
		) {
			// Check for nullptr, should never happen
			for ( auto& e : edge )
				if ( e == nullptr )
				{
					std::cout << "Fatal error! Connect shared edges.\n";
					std::cout << "One (or more) edge(s) is a nullptr!\n";
					std::cout << "Use .clean_up() on data first, to avoid this happening.\n";
					std::exit( EXIT_FAILURE );
				}

			for ( std::size_t i{ 0 }; i < edge.size(); ++i ) {
				if ( !edge[i]->twin ) {
					// To find a twin, vertices for the edge is needed
					std::shared_ptr<Mesh::Data::Vertex> const& vertex1 = edge[i]->vertex;
					std::shared_ptr<Mesh::Data::Vertex> const& vertex2 = edge[i]->next->vertex;
					// Only need to search in k(i+1), since all half edges below are matched.
					// This is faster than k(0), for a valid mesh.
					// However, this might not detect multiple shared edges,
					// TODO add better method
					for ( std::size_t k{ i + 1 }; k < edge.size(); ++k ) {
						std::shared_ptr<Mesh::Data::Vertex> const& twin_vertex1 = edge[k]->vertex;
						std::shared_ptr<Mesh::Data::Vertex> const& twin_vertex2 = edge[k]->next->vertex;
						// Twin vertices are in reverse order
						if ( ( vertex1 == twin_vertex2 ) && ( vertex2 == twin_vertex1 ) ) {
							// Check if edge is shared more than once, not an unique closed surface.
							if ( edge[k]->twin ) {
								std::cout << "Mesh has edge(s) shared by more than two polygons!\n";
								return false;
							}
							// Define twins
							edge[i]->twin = edge[k];
							edge[k]->twin = edge[i];
							break;
						}
					}
				}
			}

			// Test for twined edges
			if ( f_connected )
				for ( auto const& e : edge )
					if ( e->twin == nullptr ) {
						std::cout << "Mesh has unconnected edge(s)!\n";
						return false;
					}

			return true;
		};

	};

};
