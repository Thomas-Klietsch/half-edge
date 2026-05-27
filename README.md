##### Introduction

Half edge data structure, with mesh import and export. Supports materials, textures (3D uv map), flat/smooth polygon shading, and vertex normal.

Default operation is an unique closed surfaces (all edges are shared by two polygons). Options for hard and boundary (non-shared) edges.

This project is intended as a minimal implementation of the half edge data structure. Speed and low memory usage is not considered.

##### Usage

Load a Wavefront mesh into half-data:

    std::unique_ptr<Mesh::HalfEdge> p_mesh = Mesh::Import( "input.obj", Mesh::FileType::Wavefront );

Save half-data as a Wavefront mesh object, using weighted (area and angle) vertex normal calculation:

    Mesh::Export( "output.obj", p_mesh, Mesh::FileType::Wavefront, Mesh::VertexNormal::Weighted );

More examples of basic half-edge usage are included in the main.cpp file.

##### Mesh file support

| format | material | normal | texture | edge state |
| --- | --- | --- | --- | --- |
| Stanford ASCII .ply | No | Yes | Yes | No |
| STL ASCII .stl | No | No* | No | No |
| Wavefront .obj | Yes | Yes | Yes | Yes |

*normal*: Vertex normal. Stanford will export triangle normal.\
*texture*: Texture coordinate, UV mapping.

Note: STL can only handle triangles.

##### Dependencies

- C++17 (C++20 when using std::numbers)

##### To do

Support more file formats. Currently unlikely due to complexity of remaining file formats.

Improved edge connecting, and testing.

Test if polygon is convex, and near planar.

More mesh operations.
