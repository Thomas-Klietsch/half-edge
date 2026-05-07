##### Introduction

Half edge data structure, with mesh import and export. Supports materials, textures, smooth shading and vertex normal data.

Default operation is an unique closed surfaces (all edges are shared by two polygons). Options for hard and boundary (non-shared) edges.

This project is intended as a minimal implementation of the half edge data structure. Speed and low memory usage is not considered.

##### Usage

Load mesh into half-data:

    std::unique_ptr<Mesh::HalfEdge> p_mesh = Mesh::Import( "input.obj", Mesh::Type::Wavefront );

Save half-data as Wavefront mesh object, using weighted (area and angle) vertex normal calculation:

    Mesh::Export( "output.obj", p_mesh, Mesh::Type::Wavefront, Mesh::VertexNormal::Weighted );

More examples of basic half-edge usage are included in the main.cpp file.

##### Dependencies

- C++17

##### To do

Support more file formats.

Pre-calculation of flat shaded vertex normals, when exporting. 

Sorted lists of 3D data, for faster processing.

Improved edge connecting, and testing.

Test if polygon is convex, and near planar.

More mesh operations.
