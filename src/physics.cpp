#include "physics.hpp"

#include <cmath>

Eigen::Vector4d Physics::step(
    const Eigen::Vector4d& s,
    const Params& p,
    float dt
)
{
    Eigen::Vector4d ns = s;

    double x  = s[0];
    double y  = s[1];
    double vx = s[2];
    double vy = s[3];

    double ax = -p.mu * vx;
    double ay = -p.g - p.mu * vy;

    ns[0] += vx * dt;
    ns[1] += vy * dt;
    ns[2] += ax * dt;
    ns[3] += ay * dt;

    return ns;
}

std::vector<Point> Physics::simulate(
    const Params& p,
    float dt,
    float tmax
)
{
    std::vector<Point> traj;

    double theta =
        p.theta0 * M_PI / 180.0;

    Eigen::Vector4d state;

    state <<
        p.x0,
        p.y0,
        p.v0 * std::cos(theta),
        p.v0 * std::sin(theta);

    float t = 0.0f;

    while (t < tmax)
    {
        traj.push_back({
            (float)state[0],
            (float)state[1]
        });

        state = step(state, p, dt);

        t += dt;
    }

    return traj;
}