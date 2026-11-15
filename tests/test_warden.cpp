#include "TestUtil.hpp"
#include "core/MathUtil.hpp"
#include "game/Warden.hpp"

using namespace ew;

namespace {

constexpr float kEye = 4.0f;
constexpr float kDt = 1.0f / 120.0f;

// Runs until `pred` holds or `maxSeconds` elapse. Returns the elapsed time.
template <typename Pred>
float runUntil(Warden& w, const glm::vec3& playerEye, const WardenParams& p, float maxSeconds, Pred pred) {
    float t = 0.0f;
    while (t < maxSeconds && !pred()) {
        w.update(kDt, playerEye, kEye, p);
        t += kDt;
    }
    return t;
}

}  // namespace

TEST_CASE("the Warden sleeps while the player is far away") {
    Warden w;
    const WardenParams p;
    const glm::vec3 far = w.config().base + glm::vec3(p.detectionRadius + 10.0f, kEye, 0.0f);
    runUntil(w, far, p, 5.0f, [] { return false; });
    CHECK(w.state() == WardenState::Dormant);
    CHECK(w.alertness() < 0.05f);
}

TEST_CASE("full attack cycle: track -> wind-up -> strike lands near the locked target") {
    Warden w;
    const WardenParams p;
    const glm::vec3 eye = w.config().base + glm::vec3(-12.0f, kEye, 15.0f);

    w.update(kDt, eye, kEye, p);
    CHECK(w.state() == WardenState::Tracking);

    runUntil(w, eye, p, 10.0f, [&] { return w.state() == WardenState::WindUp; });
    REQUIRE(w.state() == WardenState::WindUp);
    // While tracking it turned to face the player.
    CHECK(std::fabs(wrapDegrees(w.pose().yaw - yawToward(w.config(), eye))) < 25.0f);

    runUntil(w, eye, p, 10.0f, [&] { return w.state() == WardenState::Strike; });
    REQUIRE(w.state() == WardenState::Strike);
    CHECK(w.isStriking());
    CHECK(w.alertness() > 0.6f);

    glm::vec3 impact;
    float t = 0.0f;
    bool landed = false;
    while (t < 2.0f && !landed) {
        w.update(kDt, eye, kEye, p);
        t += kDt;
        landed = w.consumeImpact(impact);
    }
    REQUIRE(landed);
    CHECK(w.state() == WardenState::Recover);
    // The hammer comes down within a unit of where the player was standing.
    CHECK(horizontalDistance(impact, eye) < 1.0f);
    CHECK(impact.y == doctest::Approx(3.0f).epsilon(0.2));
    CHECK_FALSE(w.consumeImpact(impact));  // reported exactly once
}

TEST_CASE("the strike target is locked: moving after the wind-up dodges the hammer") {
    Warden w;
    const WardenParams p;
    glm::vec3 eye = w.config().base + glm::vec3(18.0f, kEye, 0.0f);
    runUntil(w, eye, p, 10.0f, [&] { return w.state() == WardenState::Strike; });
    REQUIRE(w.state() == WardenState::Strike);
    const glm::vec3 locked = w.strikeTarget();
    eye += glm::vec3(0.0f, 0.0f, 12.0f);  // sidestep
    glm::vec3 impact;
    runUntil(w, eye, p, 2.0f, [&] { return w.consumeImpact(impact); });
    CHECK(horizontalDistance(impact, locked) < 1.0f);
    CHECK(horizontalDistance(impact, eye) > 10.0f);
}
