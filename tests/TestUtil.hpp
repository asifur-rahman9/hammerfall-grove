#pragma once
#include <doctest/doctest.h>

#include <glm/glm.hpp>

inline bool near(const glm::vec3& a, const glm::vec3& b, float eps = 1e-3f) { return glm::length(a - b) <= eps; }

#define CHECK_VEC3_NEAR(a, b, eps)                                                                                      \
    do {                                                                                                               \
        const glm::vec3 va_ = (a), vb_ = (b);                                                                          \
        INFO("got (" << va_.x << ", " << va_.y << ", " << va_.z << ") expected (" << vb_.x << ", " << vb_.y << ", " << vb_.z \
                     << ")");                                                                                         \
        CHECK(near(va_, vb_, eps));                                                                                    \
    } while (0)
