#include "TestUtil.hpp"
#include "core/MathUtil.hpp"
#include "game/Player.hpp"

using namespace ew;

namespace {

void step(Player& p, const PlayerInput& in, float seconds, float dt = 1.0f / 120.0f) {
    for (float t = 0.0f; t < seconds; t += dt) p.update(in, dt);
}

Player makePlayer() {
    Player p;
    p.reset(glm::vec3(-20.0f, 4.0f, 5.0f), 0.0f, 0.0f);  // facing +X, toward the arena centre
    return p;
}

}  // namespace

TEST_CASE("walking forward moves along the look direction at walk speed") {
    Player p = makePlayer();
    PlayerInput in;
    in.forward = true;
    const glm::vec3 start = p.eye();
    step(p, in, 0.5f);
    const glm::vec3 moved = p.eye() - start;
    CHECK(moved.x > 0.0f);
    CHECK(std::fabs(moved.z) < 1e-3f);
    CHECK(glm::length(glm::vec2(p.velocity().x, p.velocity().z)) == doctest::Approx(p.params().walkSpeed).epsilon(0.02));
}

TEST_CASE("jumping goes up and lands back on the ground") {
    Player p = makePlayer();
    PlayerInput jump;
    jump.jump = true;
    p.update(jump, 1.0f / 120.0f);
    CHECK_FALSE(p.onGround());
    float peak = 0.0f;
    PlayerInput none;
    for (int i = 0; i < 240; ++i) {
        p.update(none, 1.0f / 120.0f);
        peak = std::max(peak, p.eye().y);
    }
    const PlayerParams& prm = p.params();
    const float expectedPeak = prm.eyeHeight + prm.jumpSpeed * prm.jumpSpeed / (2.0f * -prm.gravity);
    CHECK(peak == doctest::Approx(expectedPeak).epsilon(0.03));
    CHECK(p.onGround());
    CHECK(p.eye().y == doctest::Approx(prm.eyeHeight));
}

TEST_CASE("the player can't leave the clearing or walk into the plinth") {
    Player p = makePlayer();
    PlayerInput back;
    back.back = true;
    back.sprint = true;
    step(p, back, 10.0f);
    const glm::vec3& c = p.params().arenaCenter;
    CHECK(horizontalDistance(p.eye(), c) <= p.params().arenaRadius + 1e-3f);

    p.reset(glm::vec3(-20.0f, 4.0f, 5.0f), 0.0f, 0.0f);
    PlayerInput fwd;
    fwd.forward = true;
    step(p, fwd, 5.0f);
    CHECK(horizontalDistance(p.eye(), c) >= p.params().obstacleRadius - 1e-3f);
}
