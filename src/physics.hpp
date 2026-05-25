#pragma once

#include <vector>
#include <string>
#include <functional>
#include <Eigen/Dense>

// ─────────────────────────────────────────────────────────────────────────
//  Data structures
// ─────────────────────────────────────────────────────────────────────────

struct Point
{
    double t, x, y;
};

struct Params
{
    double x0     = 0.0;
    double y0     = 0.0;
    double v0     = 200.0;
    double theta0 = 45.0;   // degrees
    double mu     = 0.0;
    double g      = 9.80665;
};

// ─────────────────────────────────────────────────────────────────────────
//  ParamIndex – identifies which fields of Params are free unknowns.
//
//  The solver works on an Eigen::VectorXd of size p (number of unknowns).
//  ParamIndex maps each component of that vector to a field of Params,
//  so the same solver code handles all 4 datasets.
//
//  Example for dataset2 (mu, v0 unknown):
//    free = { IDX_MU, IDX_V0 }
// ─────────────────────────────────────────────────────────────────────────

enum ParamIndex { IDX_X0=0, IDX_Y0, IDX_V0, IDX_THETA0, IDX_MU };

// ─────────────────────────────────────────────────────────────────────────
//  Physics
//
//  Extended state vector for p free parameters:
//    s = [ x, y, vx, vy,
//          dx/dc_0, dy/dc_0, dvx/dc_0, dvy/dc_0,
//          dx/dc_1, dy/dc_1, dvx/dc_1, dvy/dc_1,
//          ...
//        ]   →  4 + 4*p  components
//
//  All methods accept a `free` vector that lists which parameters vary.
// ─────────────────────────────────────────────────────────────────────────

class Physics
{
public:
    // ── Extended ODE right-hand side ─────────────────────────────────────
    //  state size = 4 + 4*p  where p = free.size()
    Eigen::VectorXd f_ext(
        const Eigen::VectorXd&      s,
        const Params&               p,
        const std::vector<ParamIndex>& free
    ) const;

    // ── RK4 step on the extended state ───────────────────────────────────
    Eigen::VectorXd rk4_step(
        const Eigen::VectorXd&      s,
        const Params&               p,
        const std::vector<ParamIndex>& free,
        double dt
    ) const;

    // ── Simulate full trajectory (x,y only, no variational part) ─────────
    std::vector<Point> simulate(
        const Params& p,
        double dt,
        double tmax
    ) const;

    // ── Simulate extended state at observation times ──────────────────────
    std::vector<Eigen::VectorXd> simulate_at(
        const Params&               p,
        const std::vector<ParamIndex>& free,
        double dt,
        const std::vector<double>&  t_obs
    ) const;

    // ── One linearised least-squares iteration ───────────────────────────
    //  Returns correction vector Delta_c  (size = free.size())
    Eigen::VectorXd least_squares_step(
        const std::vector<Point>&   obs,
        const Params&               p,
        const std::vector<ParamIndex>& free,
        double dt,
        double& residual_rms
    ) const;

    // ── Full iterative solver ─────────────────────────────────────────────
    //  Updates p in-place for the free parameters.
    //  Returns covariance diagonal (sigma for each free param).
    Eigen::VectorXd solve(
        const std::vector<Point>&   obs,
        Params&                     p,
        const std::vector<ParamIndex>& free,
        double dt        = 0.001,
        double eps       = 1e-10,
        int    max_iter  = 200,
        double sigma_obs = 1.0
    ) const;

    // ── Post-processing ───────────────────────────────────────────────────
    std::pair<double,double> find_impact(
        const Params& p, double dt, double t_start, double tmax=300.0
    ) const;

    std::pair<double,double> find_max_altitude(
        const Params& p, double dt, double tmax
    ) const;

    std::pair<double,double> find_min_velocity(
        const Params& p, double dt, double tmax
    ) const;

    // ── CSV reader ────────────────────────────────────────────────────────
    static std::vector<Point> read_csv(const std::string& filename);

    // ── Helpers ───────────────────────────────────────────────────────────
    // Get/set a free parameter value inside Params by its index
    static double  get_param(const Params& p, ParamIndex idx);
    static void    set_param(Params& p, ParamIndex idx, double val);
    static std::string param_name(ParamIndex idx);

private:
    // Build initial extended state (4 + 4*p components)
    Eigen::VectorXd make_initial_state(
        const Params& p,
        const std::vector<ParamIndex>& free
    ) const;

    // Variational equations for ONE free parameter c_i
    // Returns (d/dt)(d[vx,vy]/d[c_i]) given current state and partials
    void variational_rhs(
        double vx, double vy,
        double pvx_i, double pvy_i,
        double px_i,  double py_i,
        double mu, double g,
        ParamIndex idx,
        double& dpvx, double& dpvy,
        double& dpx,  double& dpy
    ) const;
};