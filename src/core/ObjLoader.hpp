#pragma once
// Wavefront OBJ parser (positions / normals / UVs, n-gon faces, negative indices).
// GL-free so it can be unit tested; gfx::Mesh uploads the result to the GPU.

#include <glm/glm.hpp>
#include <istream>
#include <string>
#include <vector>

namespace ew {

// De-indexed triangle soup: every three consecutive entries form one triangle.
// All three arrays always have the same length (missing normals are computed
// per face, missing UVs are filled with zeros).
struct MeshData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    bool sourceHadNormals = false;
    bool sourceHadUVs = false;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    std::size_t vertexCount() const { return positions.size(); }
};

bool parseObj(std::istream& in, MeshData& out, std::string* error = nullptr);
bool loadObj(const std::string& path, MeshData& out, std::string* error = nullptr);

// Procedural meshes used by the renderer.
MeshData makePlane(float halfSize, float uvTile);  // XZ plane facing +Y
MeshData makeUnitCube();                           // [-1, 1]^3 with face normals

}  // namespace ew
