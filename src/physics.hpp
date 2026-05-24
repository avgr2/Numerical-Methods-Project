#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>

// ─────────────────────────────────────────────
//  Data structures
// ─────────────────────────────────────────────

struct Point
{
    double t;
    double x;
    double y;
};

struct Params
{
    double x0     = 0.0;
    double y0     = 0.0;
    double v0     = 200.0;
    double theta0 = 45.0;   // degrees
    double mu     = 0.0;    // drag coefficient (unknown for dataset1)
    double g      = 9.80665;
};

// ─────────────────────────────────────────────
//  Physics class
// ─────────────────────────────────────────────
//
//  State vector for dataset1 (1 unknown: mu)
//  s = [ x, y, vx, vy,
//        dx/dmu, dy/dmu, dvx/dmu, dvy/dmu ]   (8 components)
//
//  The variational equations are integrated alongside the
//  trajectory equations so that the Jacobian matrix B
//  required by the least-squares algorithm is available
//  at every observation time.
// ─────────────────────────────────────────────

class Physics
{
public:
    // ---------------------------------------------------
    // RK4 step on the 8-component extended state vector.
    // ---------------------------------------------------
    Eigen::Matrix<double, 8, 1> rk4_step(
        const Eigen::Matrix<double, 8, 1>& s,
        const Params& p,
        double dt
    ) const;

    // ---------------------------------------------------
    // Derivative of the extended state (equations of motion
    // + variational equations w.r.t. mu).
    // ---------------------------------------------------
    Eigen::Matrix<double, 8, 1> f(
        const Eigen::Matrix<double, 8, 1>& s,
        const Params& p
    ) const;

    // ---------------------------------------------------
    // Simulate the trajectory from t=0 to tmax with step dt.
    // Returns positions sampled at every step.
    // ---------------------------------------------------
    std::vector<Point> simulate(
        const Params& p,
        double dt,
        double tmax
    ) const;

    // ---------------------------------------------------
    // Simulate and return the extended state evaluated
    // exactly at the observation times t_obs.
    // Uses RK4 with fixed step dt, interpolating between
    // steps to land precisely on each t_obs.
    // ---------------------------------------------------
    std::vector<Eigen::Matrix<double, 8, 1>> simulate_at(
        const Params& p,
        double dt,
        const std::vector<double>& t_obs
    ) const;

    // ---------------------------------------------------
    // One iteration of the linearised least-squares fit.
    // Returns the correction Delta_mu.
    // ---------------------------------------------------
    double least_squares_step(
        const std::vector<Point>& observations,
        const Params& p,
        double dt,
        double& residual_rms
    ) const;

    // ---------------------------------------------------
    // Full iterative least-squares solver.
    // Modifies p.mu in-place and returns sigma_mu.
    // ---------------------------------------------------
    double solve(
        const std::vector<Point>& observations,
        Params& p,
        double dt            = 0.001,
        double eps           = 1e-10,
        int    max_iter      = 100,
        double sigma_obs     = 1.0
    ) const;

    // ---------------------------------------------------
    // Post-processing helpers (called after solve())
    // ---------------------------------------------------

    // Continue integration past observations until y <= 0;
    // returns (x_impact, t_impact) via linear interpolation.
    std::pair<double, double> find_impact(
        const Params& p,
        double dt,
        double t_start,
        double tmax = 300.0
    ) const;

    // Returns (y_max, t_ymax)
    std::pair<double, double> find_max_altitude(
        const Params& p,
        double dt,
        double tmax
    ) const;

    // Returns (v_min, t_vmin)
    std::pair<double, double> find_min_velocity(
        const Params& p,
        double dt,
        double tmax
    ) const;

    // Read dataset CSV (skips lines starting with '#').
    // Expected columns: t, x, y
    static std::vector<Point> read_csv(const std::string& filename);
};