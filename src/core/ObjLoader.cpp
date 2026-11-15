#include "core/ObjLoader.hpp"

#include <array>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>

namespace ew {
namespace {

struct FaceVertex {
    int position = 0;  // 1-based after resolution, 0 = missing
    int uv = 0;
    int normal = 0;
};

// Resolve an OBJ index (1-based, or negative = relative to the end) to 0-based.
int resolveIndex(int idx, std::size_t count) {
    if (idx > 0) return idx - 1;
    if (idx < 0) return static_cast<int>(count) + idx;
    return -1;
}

bool parseFaceVertex(const std::string& token, FaceVertex& fv) {
    // Accepts: v, v/t, v//n, v/t/n
    const std::size_t s1 = token.find('/');
    if (s1 == std::string::npos) {
        fv.position = std::atoi(token.c_str());
        return fv.position != 0;
    }
    fv.position = std::atoi(token.substr(0, s1).c_str());
    const std::size_t s2 = token.find('/', s1 + 1);
    if (s2 == std::string::npos) {
        fv.uv = std::atoi(token.substr(s1 + 1).c_str());
    } else {
        if (s2 > s1 + 1) fv.uv = std::atoi(token.substr(s1 + 1, s2 - s1 - 1).c_str());
        fv.normal = std::atoi(token.substr(s2 + 1).c_str());
    }
    return fv.position != 0;
}

void computeBounds(MeshData& m) {
    if (m.positions.empty()) return;
    m.boundsMin = glm::vec3(std::numeric_limits<float>::max());
    m.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const auto& p : m.positions) {
        m.boundsMin = glm::min(m.boundsMin, p);
        m.boundsMax = glm::max(m.boundsMax, p);
    }
}

}  // namespace

bool parseObj(std::istream& in, MeshData& out, std::string* error) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    out = MeshData{};

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        std::string tag;
        if (!(ls >> tag) || tag[0] == '#') continue;

        if (tag == "v") {
            glm::vec3 p;
            ls >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vt") {
            glm::vec2 t(0.0f);
            ls >> t.x >> t.y;
            // OBJ has V pointing up, stb_image rows go top-down: flip V.
            t.y = 1.0f - t.y;
            uvs.push_back(t);
        } else if (tag == "vn") {
            glm::vec3 n;
            ls >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "f") {
            std::vector<FaceVertex> face;
            std::string tok;
            while (ls >> tok) {
                FaceVertex fv;
                if (!parseFaceVertex(tok, fv)) {
                    if (error) *error = "line " + std::to_string(lineNo) + ": bad face token '" + tok + "'";
                    return false;
                }
                face.push_back(fv);
            }
            if (face.size() < 3) {
                if (error) *error = "line " + std::to_string(lineNo) + ": face with < 3 vertices";
                return false;
            }
            // Fan triangulation handles quads and convex n-gons.
            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                const std::array<FaceVertex, 3> tri = {face[0], face[i], face[i + 1]};
                std::array<glm::vec3, 3> triPos{};
                bool triHasNormals = true;
                for (int k = 0; k < 3; ++k) {
                    const int pi = resolveIndex(tri[k].position, positions.size());
                    if (pi < 0 || pi >= static_cast<int>(positions.size())) {
                        if (error) *error = "line " + std::to_string(lineNo) + ": position index out of range";
                        return false;
                    }
                    triPos[k] = positions[pi];
                    out.positions.push_back(triPos[k]);

                    const int ti = resolveIndex(tri[k].uv, uvs.size());
                    if (ti >= 0 && ti < static_cast<int>(uvs.size())) {
                        out.uvs.push_back(uvs[ti]);
                        out.sourceHadUVs = true;
                    } else {
                        out.uvs.emplace_back(0.0f);
                    }

                    const int ni = resolveIndex(tri[k].normal, normals.size());
                    if (ni >= 0 && ni < static_cast<int>(normals.size())) {
                        out.normals.push_back(glm::normalize(normals[ni]));
                    } else {
                        out.normals.emplace_back(0.0f);
                        triHasNormals = false;
                    }
                }
                if (triHasNormals) {
                    out.sourceHadNormals = true;
                } else {
                    // Flat normal from the triangle's winding.
                    glm::vec3 n = glm::cross(triPos[1] - triPos[0], triPos[2] - triPos[0]);
                    const float len = glm::length(n);
                    n = len > 1e-12f ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
                    const std::size_t base = out.normals.size() - 3;
                    for (int k = 0; k < 3; ++k) out.normals[base + k] = n;
                }
            }
        }
        // o, g, s, usemtl, mtllib... are ignored.
    }

    if (out.positions.empty()) {
        if (error) *error = "no faces found";
        return false;
    }
    computeBounds(out);
    return true;
}

bool loadObj(const std::string& path, MeshData& out, std::string* error) {
    std::ifstream file(path);
    if (!file) {
        if (error) *error = "cannot open '" + path + "'";
        return false;
    }
    std::string localError;
    if (!parseObj(file, out, &localError)) {
        if (error) *error = path + ": " + localError;
        return false;
    }
    return true;
}

MeshData makePlane(float halfSize, float uvTile) {
    MeshData m;
    const glm::vec3 corners[4] = {{-halfSize, 0, -halfSize}, {halfSize, 0, -halfSize}, {halfSize, 0, halfSize}, {-halfSize, 0, halfSize}};
    // Two counter-clockwise (seen from +Y) triangles.
    const int idx[6] = {0, 2, 1, 0, 3, 2};
    for (int i : idx) {
        m.positions.push_back(corners[i]);
        m.normals.emplace_back(0.0f, 1.0f, 0.0f);
        m.uvs.emplace_back(corners[i].x / uvTile, corners[i].z / uvTile);
    }
    m.sourceHadNormals = m.sourceHadUVs = true;
    computeBounds(m);
    return m;
}

MeshData makeUnitCube() {
    MeshData m;
    // For each face: normal, and two tangent axes (u, v) such that cross(u, v) == normal.
    const glm::vec3 faces[6][3] = {
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}}, {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
        {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},  {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},  {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
    };
    const glm::vec2 quad[6] = {{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}};
    for (const auto& f : faces) {
        for (const auto& q : quad) {
            m.positions.push_back(f[0] + f[1] * q.x + f[2] * q.y);
            m.normals.push_back(f[0]);
            m.uvs.emplace_back(q.x * 0.5f + 0.5f, 1.0f - (q.y * 0.5f + 0.5f));
        }
    }
    m.sourceHadNormals = m.sourceHadUVs = true;
    computeBounds(m);
    return m;
}

}  // namespace ew
