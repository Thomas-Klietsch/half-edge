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

#include <memory>
#include <string>

#include "./../mesh/half-edge.hpp"
#include "./../mesh/file_format/common.hpp"
// Supported file formats
#include "./../mesh/file_format/stanford.hpp"
#include "./../mesh/file_format/wavefront.hpp"

namespace Mesh {

	// Vertex normal calculation method.
	enum class VertexNormal : std::uint8_t {
		// Vertex normals are not used.
		Off = 0,
		// Polygon normal, commonly used method.
		Common = 1,
		// Weighted polygon normal (normal scaled by polygon area).
		Area = 2,
		// Polygon normal scaled by edge angle at vertex.
		Angle = 4,
		// Weighted polygon normal and edge angle at vertex.
		Weighted = 8,
		// Experimental, generated using polygon edges.
		// Only valid if all polygon vertices are on the same plane.
		Experimental = 0xff
	};

	bool Export(
		std::string const& file_name,
		// Can not reference a const
		std::unique_ptr<Mesh::HalfEdge>& p_mesh,
		Mesh::Type const file_format,
		// If not set to ::Off, will calculate and export vertex normals,
		// requires that polygon(s) are marked f_smooth=true
		Mesh::VertexNormal const f_vertex_normal = Mesh::VertexNormal::Weighted,
		// True will export vertex texture coordinates
		bool const f_vertex_texture = true
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
			std::cout << "Not enough data to export: \"" << file_name << "\"\n";
			return false;
		}

		// Adds a default material, if none are defined.
		// TODO make this an optional feature?
		if ( p_mesh->material_name.size() == 0 )
			p_mesh->material_name = { "default_mesh_material" };

		// True if smooth shading (vertex normals) is used.
		bool const f_smooth = f_vertex_normal != Mesh::VertexNormal::Off;

		// TODO avoid recalculation for multiple exports?
		if ( f_smooth ) {
			// Initialise/reset data
			for ( auto& v : p_mesh->vertex )
				v->normal = Double3::Zero;

			// Overwrite flat shading (Stanford does not support mixed shading)
			bool const f_include_flat = ( file_format == Mesh::Type::Stanford );

			switch ( f_vertex_normal ) {
				default: { break; }
				case Mesh::VertexNormal::Off: { break; }
				case Mesh::VertexNormal::Common: {
					// Add all polygon normals to their vertices
					for ( auto p : p_mesh->polygon )
						// Skip flat, unless override
						if ( p->f_smooth || f_include_flat ) {
							Double3 const polygon_normal = p->unit_normal();
							// Go through edges to get vertices, and update normal
							for ( auto e : p->edge )
								e->vertex->normal += polygon_normal;
						}
					break;
				}
				case Mesh::VertexNormal::Area: {
					for ( auto p : p_mesh->polygon )
						if ( p->f_smooth || f_include_flat ) {
							Double3 const polygon_normal = p->weighted_normal();
							for ( auto e : p->edge )
								e->vertex->normal += polygon_normal;
						}
					break;
				}
				case Mesh::VertexNormal::Angle: {
					for ( auto p : p_mesh->polygon )
						if ( p->f_smooth || f_include_flat ) {
							Double3 const polygon_normal = p->unit_normal();
							for ( auto e : p->edge )
								e->vertex->normal += polygon_normal * e->angle();
						}
					break;
				}
				case Mesh::VertexNormal::Weighted: {
					for ( auto p : p_mesh->polygon )
						if ( p->f_smooth || f_include_flat ) {
							Double3 const polygon_normal = p->weighted_normal();
							for ( auto e : p->edge )
								e->vertex->normal += polygon_normal * e->angle();
						}
					break;
				}
				case Mesh::VertexNormal::Experimental: {
					for ( auto p : p_mesh->polygon )
						if ( p->f_smooth || f_include_flat ) {
							for ( auto e : p->edge ) {
								Double3 const v0 = e->vertex->location;
								Double3 const v1 = e->next->vertex->location;
								Double3 const v2 = e->previous->vertex->location;
								e->vertex->normal += ( ( v1 - v0 ).cross( v2 - v0 ) ).normalise();
							}
						}
					break;
				}
			}

			// Normalise vertex normals
			for ( auto v : p_mesh->vertex )
				v->normal = ( v->normal ).normalise();
		}

		// True if any polygon is fully textured
		bool f_texture{ false };
		if ( f_vertex_texture ) {
			// Check if any polygon is textured
			for ( auto const& p : p_mesh->polygon )
				if ( p->is_textured() ) {
					f_texture = true;
					break;
				}
		}

		switch ( file_format ) {
			case Mesh::Type::Stanford: {
				// Stanford does not support mixed texturing.
				if ( f_texture )
					// Check if all polygon are textured
					for ( auto const& p : p_mesh->polygon )
						if ( !p->is_textured() ) {
							f_texture = false;
							break;
						}

				return Mesh::FileFormat::Stanford::Export( file_name, p_mesh, f_smooth, f_texture );
			}
			case Mesh::Type::Wavefront:
				return Mesh::FileFormat::Wavefront::Export( file_name, p_mesh, f_smooth, f_texture );
		}

		return false;
	};

};
