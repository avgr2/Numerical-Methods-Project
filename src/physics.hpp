#pragma once

#include <vector>

#include <Eigen/Dense>

struct Point
{
    float x;
    float y;
};

struct Params
{
    double x0;
    double y0;
    double v0;
    double theta0;
    double mu;
    double g;
};

class Physics
{
public:

    Eigen::Vector4d step(
        const Eigen::Vector4d& s,
        const Params& p,
        float dt
    );

    std::vector<Point> simulate(
        const Params& p,
        float dt,
        float tmax
    );
};