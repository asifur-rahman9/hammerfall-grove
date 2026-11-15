#include <cmath>

#include "TestUtil.hpp"
#include "game/Kinematics.hpp"
#include "game/Warden.hpp"

using namespace ew;

TEST_CASE("rest pose points the arm straight up") {
    ArmConfig cfg;
    const ArmTransforms t = forwardKinematics(cfg, ArmPose{});
    CHECK_VEC3_NEAR(t.elbowPosition, cfg.base + glm::vec3(0, cfg.bicepLength, 0), 1e-4f);
    CHECK_VEC3_NEAR(t.hammerPosition, cfg.base + glm::vec3(0, cfg.bicepLength + cfg.forearmLength, 0), 1e-4f);
}

TEST_CASE("segment lengths are preserved by every pose") {
    ArmConfig cfg;
    const ArmTransforms t = forwardKinematics(cfg, ArmPose{77.0f, -33.0f, 88.0f});
    CHECK(glm::length(t.elbowPosition - cfg.base) == doctest::Approx(cfg.bicepLength).epsilon(1e-4));
    CHECK(glm::length(t.hammerPosition - t.elbowPosition) == doctest::Approx(cfg.forearmLength).epsilon(1e-4));
}

TEST_CASE("yawToward points the arm's forward axis at the target") {
    ArmConfig cfg;
    CHECK(yawToward(cfg, cfg.base + glm::vec3(0, 0, 10)) == doctest::Approx(0.0f));
    CHECK(yawToward(cfg, cfg.base + glm::vec3(10, 0, 0)) == doctest::Approx(90.0f));
    CHECK(std::fabs(yawToward(cfg, cfg.base + glm::vec3(0, 0, -10))) == doctest::Approx(180.0f));
}

TEST_CASE("strike poses put the hammer near the ground at the requested distance") {
    ArmConfig cfg;
    for (float d = 12.0f; d <= 28.0f; d += 2.0f) {
        const glm::vec3 hp = hammerPosition(cfg, strikePoseForDistance(d, 90.0f));
        CHECK(hp.x - cfg.base.x == doctest::Approx(d).epsilon(0.03));  // yaw 90 = along +X
        CHECK(hp.y == doctest::Approx(3.0f).epsilon(0.15));
    }
}
