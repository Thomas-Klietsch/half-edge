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
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "./../mesh/data.hpp"

namespace Mesh {

	class HalfEdge final {

	public:

		// List of unique sorted 3D vertices
		std::vector<std::shared_ptr<Mesh::Data::Vertex>> vertex;

		// List of unique sorted 3D texture vertices
		std::vector<std::shared_ptr<Mesh::Data::Texture>> texture;

		// Unordered, adding edges directly is not recommended
		std::vector<std::shared_ptr<Mesh::Data::Edge>> edge;

		// Unordered, using add_polygon() is recommended
		std::vector<std::shared_ptr<Mesh::Data::Polygon>> polygon;

		// Material library file name(s)
		std::vector<std::string> material_library;

		// Materials
		std::vector<std::string> material_name;

	public:

		HalfEdge() {};

		// Constructor with copy of material data
		HalfEdge(
			std::unique_ptr<HalfEdge> const& p_input
		)
			: material_library( p_input->material_library )
			, material_name( p_input->material_name )
		{
		};

		// Adds a Double3 to the order vertex vector, if it does not exist.
		// Return is the memory address of the (added) vertex.
		std::shared_ptr<Mesh::Data::Vertex>& add_vertex(
			Double3 const& value
		) {
			// Find lowest index which contain value
			auto index = std::lower_bound( vertex.begin(), vertex.end(), value, IsLesser<Mesh::Data::Vertex> );
			// End of list reach, e.g. value is not in the list
			if ( index == vertex.end() ) {
				// Everything is less than value, add it at the end
				vertex.emplace_back( std::make_shared<Mesh::Data::Vertex>( value ) );
				return vertex[vertex.size() - 1];
			}

			// Memory location used by list (vector) might change on insert,
			// so result is calculated first
			std::size_t const result = index - vertex.begin();

			// Lesser, but not equal; insert it
			if ( value != vertex[result]->location )
				vertex.insert( index, std::make_shared<Mesh::Data::Vertex>( value ) );

			return vertex[result];
		};

		// Adds a Double3 to the texture vector, if it does not exist
		// Return is the memory address of the (added) texture.
		std::shared_ptr<Mesh::Data::Texture>& add_texture(
			Double3 const& value
		) {
			// Find lowest index which contain value
			auto index = std::lower_bound( texture.begin(), texture.end(), value, IsLesser<Mesh::Data::Texture> );
			// End of list reach, e.g. value is not in the list
			if ( index == texture.end() ) {
				// Everything is less than value, add it at the end
				texture.emplace_back( std::make_shared<Mesh::Data::Texture>( value ) );
				return texture[texture.size() - 1];
			}

			// Memory location used by list (vector) might change on insert,
			// so result is calculated first
			std::size_t const result = index - texture.begin();

			// Lesser, but not equal; insert it
			if ( value != texture[result]->location )
				texture.insert( index, std::make_shared<Mesh::Data::Texture>( value ) );

			return texture[result];
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

		// Imported data might not be sorted, sort it.
		void sort_data() {
			std::sort( vertex.begin(), vertex.end(), IsLesserSort<Mesh::Data::Vertex> );
			std::sort( texture.begin(), texture.end(), IsLesserSort<Mesh::Data::Texture> );
		};

		// Find all shared edges, and set twin state.
		bool connect_shared_edges(
			// Set to true to validate that all edges have a twin
			bool const f_connected = true
		) {
			// Check for nullptr in list, should never happen
			for ( auto const& e : edge )
				if ( e == nullptr ) {
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
							// Set edge sharp state, if either is sharp
							if ( edge[i]->is_sharp() || edge[k]->is_sharp() ) {
								edge[i]->f_sharp = true;
								edge[k]->f_sharp = true;
							}
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

		// Returns index for 'search', if found in vertex list.
		std::optional<std::size_t> index_vertex(
			std::shared_ptr<Mesh::Data::Vertex> const& search
		) const {
			// Find lowest index which contain value
			auto index = std::lower_bound( vertex.begin(), vertex.end(), search->location, IsLesser<Mesh::Data::Vertex> );
			// End of list reach, e.g. value is not in the list
			if ( index == vertex.end() )
				return std::nullopt;

			std::size_t const result = index - vertex.begin();

			// Lesser, but not equal
			if ( search != vertex[result] )
				return std::nullopt;

			return result;
		};

		// Returns index for 'search', if found in texture list.
		std::optional<std::size_t> index_texture(
			std::shared_ptr<Mesh::Data::Texture> const& search
		) const {
			// Find lowest index which contain value
			auto index = std::lower_bound( texture.begin(), texture.end(), search->location, IsLesser<Mesh::Data::Texture> );
			// End of list reach, e.g. value is not in the list
			if ( index == texture.end() )
				return std::nullopt;

			std::size_t const result = index - texture.begin();

			// Lesser, but not equal
			if ( search != texture[result] )
				return std::nullopt;

			return result;
		};

	private:

		// Compare evaluation, used for sorted list insertion.
		template<typename T>
		static bool IsLesser(
			std::shared_ptr<T> const& l,
			Double3 const& r
		) {
			Double3 const diff = l->location - r;

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

		// Compare evaluation, used for sorting list.
		template<typename T>
		static bool IsLesserSort(
			std::shared_ptr<T> const& l,
			std::shared_ptr<T> const& r
		) {
			Double3 const diff = l->location - r->location;

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
