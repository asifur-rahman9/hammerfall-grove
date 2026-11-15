#include <sstream>

#include "TestUtil.hpp"
#include "core/ObjLoader.hpp"

using namespace ew;

TEST_CASE("parses v/vt/vn triangles") {
    std::istringstream in(
        "# triangle\n"
        "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
        "vt 0 0\nvt 1 0\nvt 0 1\n"
        "vn 0 0 2\n"
        "f 1/1/1 2/2/1 3/3/1\n");
    MeshData m;
    REQUIRE(parseObj(in, m));
    CHECK(m.vertexCount() == 3);
    CHECK(m.sourceHadNormals);
    CHECK(m.sourceHadUVs);
    CHECK_VEC3_NEAR(m.normals[0], glm::vec3(0, 0, 1), 1e-6f);  // normalized
    CHECK(m.uvs[2].y == doctest::Approx(0.0f));                 // V flipped (1 - 1)
    CHECK(m.uvs[0].y == doctest::Approx(1.0f));
}

TEST_CASE("loads every model shipped with the game") {
    // Regression test for the original bug: models must always carry normals so
    // the lighting shader never normalizes a zero vector.
    for (const char* name : {"cube", "cylinder", "sphere", "grass1", "grass2", "tree1"}) {
        CAPTURE(name);
        MeshData m;
        std::string error;
        REQUIRE_MESSAGE(loadObj(std::string(EW_SOURCE_DIR) + "/models/" + name + ".obj", m, &error), error);
        CHECK(m.vertexCount() % 3 == 0);
        CHECK(m.normals.size() == m.vertexCount());
        for (const auto& n : m.normals) {
            REQUIRE(glm::length(n) == doctest::Approx(1.0f).epsilon(1e-3));
        }
    }
}
