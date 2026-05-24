#include "physics.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════
//  EQUATIONS OF MOTION  (quadratic drag)
//
//  The physical model:
//      dx/dt  = vx
//      dy/dt  = vy
//      dvx/dt = -mu * v * vx          (drag proportional to v²)
//      dvy/dt = -g  - mu * v * vy
//  where  v = sqrt(vx² + vy²)
//
//  VARIATIONAL EQUATIONS  w.r.t. mu  (derivative of the above w.r.t. mu)
//  Let  px = ∂x/∂mu,  py = ∂y/∂mu,  pvx = ∂vx/∂mu,  pvy = ∂vy/∂mu
//
//      d(px)/dt  = pvx
//      d(py)/dt  = pvy
//      d(pvx)/dt = -mu * v * pvx
//                  - mu * (vx*pvx + vy*pvy)/v * vx
//                  - v * vx
//      d(pvy)/dt = -mu * v * pvy
//                  - mu * (vx*pvx + vy*pvy)/v * vy
//                  - v * vy
//
//  Initial conditions for the variational part: all zero.
// ═══════════════════════════════════════════════════════════════════════════

using Vec8 = Eigen::Matrix<double, 8, 1>;

// ─────────────────────────────────────────────────────────────────────────
//  f : right-hand side of the extended ODE
// ─────────────────────────────────────────────────────────────────────────
Vec8 Physics::f(const Vec8& s, const Params& p) const
{
    const double x   = s[0];  (void)x;
    const double y   = s[1];  (void)y;
    const double vx  = s[2];
    const double vy  = s[3];
    const double px  = s[4];  (void)px;
    const double py  = s[5];  (void)py;
    const double pvx = s[6];
    const double pvy = s[7];

    const double v   = std::sqrt(vx*vx + vy*vy);

    // ---- trajectory equations ----
    const double dvx = -p.mu * v * vx;
    const double dvy = -p.g  - p.mu * v * vy;

    // ---- variational equations ----
    // dot product  vx*pvx + vy*pvy  (used in both components)
    double vdotp = 0.0;
    if (v > 1e-12)
        vdotp = (vx * pvx + vy * pvy) / v;

    const double dpvx = -p.mu * v * pvx  -  p.mu * vdotp * vx  -  v * vx;
    const double dpvy = -p.mu * v * pvy  -  p.mu * vdotp * vy  -  v * vy;

    Vec8 ds;
    ds[0] = vx;
    ds[1] = vy;
    ds[2] = dvx;
    ds[3] = dvy;
    ds[4] = pvx;
    ds[5] = pvy;
    ds[6] = dpvx;
    ds[7] = dpvy;

    return ds;
}

// ─────────────────────────────────────────────────────────────────────────
//  RK4 step
// ─────────────────────────────────────────────────────────────────────────
Vec8 Physics::rk4_step(const Vec8& s, const Params& p, double dt) const
{
    const Vec8 k1 = f(s,               p);
    const Vec8 k2 = f(s + 0.5*dt*k1,  p);
    const Vec8 k3 = f(s + 0.5*dt*k2,  p);
    const Vec8 k4 = f(s +     dt*k3,  p);
    return s + (dt / 6.0) * (k1 + 2.0*k2 + 2.0*k3 + k4);
}

// ─────────────────────────────────────────────────────────────────────────
//  Initial extended state from Params
// ─────────────────────────────────────────────────────────────────────────
static Vec8 make_initial_state(const Params& p)
{
    const double theta = p.theta0 * M_PI / 180.0;
    Vec8 s;
    s[0] = p.x0;
    s[1] = p.y0;
    s[2] = p.v0 * std::cos(theta);
    s[3] = p.v0 * std::sin(theta);
    s[4] = 0.0;   // ∂x/∂mu  = 0 initially
    s[5] = 0.0;   // ∂y/∂mu  = 0 initially
    s[6] = 0.0;   // ∂vx/∂mu = 0 initially
    s[7] = 0.0;   // ∂vy/∂mu = 0 initially
    return s;
}

// ─────────────────────────────────────────────────────────────────────────
//  simulate  – full trajectory sampled at every RK4 step
// ─────────────────────────────────────────────────────────────────────────
std::vector<Point> Physics::simulate(
    const Params& p,
    double dt,
    double tmax) const
{
    std::vector<Point> traj;
    Vec8 s = make_initial_state(p);
    double t = 0.0;

    while (t <= tmax + 1e-12)
    {
        traj.push_back({t, s[0], s[1]});
        if (s[1] < -1.0 && t > 0.1) break;   // stop below ground
        s = rk4_step(s, p, dt);
        t += dt;
    }
    return traj;
}

// ─────────────────────────────────────────────────────────────────────────
//  simulate_at  – extended state evaluated at observation times t_obs
//  Uses fine RK4 steps (dt) and records the state whenever the
//  integration time crosses a t_obs value.
// ─────────────────────────────────────────────────────────────────────────
std::vector<Vec8> Physics::simulate_at(
    const Params& p,
    double dt,
    const std::vector<double>& t_obs) const
{
    std::vector<Vec8> result;
    result.reserve(t_obs.size());

    Vec8   s = make_initial_state(p);
    double t = 0.0;
    size_t k = 0;   // index into t_obs

    while (k < t_obs.size())
    {
        const double t_target = t_obs[k];

        // Advance until we are just past t_target
        while (t < t_target - 1e-14)
        {
            double step = std::min(dt, t_target - t);
            s = rk4_step(s, p, step);
            t += step;
        }

        result.push_back(s);
        ++k;
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
//  least_squares_step  – one linearised iteration
//
//  Builds:
//    Y  (2K × 1) : residual vector  (x_obs - x_model, y_obs - y_model)
//    B  (2K × 1) : partial derivatives  (∂x/∂mu, ∂y/∂mu) at each time
//
//  Solves:  Delta_mu = (Bᵀ B)⁻¹ Bᵀ Y   (scalar least squares)
// ─────────────────────────────────────────────────────────────────────────
double Physics::least_squares_step(
    const std::vector<Point>& obs,
    const Params& p,
    double dt,
    double& residual_rms) const
{
    const int K = static_cast<int>(obs.size());

    // Collect observation times
    std::vector<double> t_obs(K);
    for (int i = 0; i < K; ++i) t_obs[i] = obs[i].t;

    // Integrate extended state at observation times
    auto states = simulate_at(p, dt, t_obs);

    // Build B and Y
    Eigen::VectorXd Y(2 * K);
    Eigen::VectorXd B(2 * K);

    double rms = 0.0;
    for (int i = 0; i < K; ++i)
    {
        const double x_model  = states[i][0];
        const double y_model  = states[i][1];
        const double dx_dmu   = states[i][4];
        const double dy_dmu   = states[i][5];

        Y[2*i    ] = obs[i].x - x_model;
        Y[2*i + 1] = obs[i].y - y_model;

        B[2*i    ] = dx_dmu;
        B[2*i + 1] = dy_dmu;

        rms += Y[2*i]*Y[2*i] + Y[2*i+1]*Y[2*i+1];
    }
    residual_rms = std::sqrt(rms / (2.0 * K));

    // Scalar least squares:  Delta_mu = (Bᵀ B)⁻¹ Bᵀ Y
    const double BtB = B.dot(B);
    if (std::abs(BtB) < 1e-30)
        throw std::runtime_error("Singular system in least_squares_step: B^T B ≈ 0");

    return B.dot(Y) / BtB;
}

// ─────────────────────────────────────────────────────────────────────────
//  solve  – iterative least-squares until convergence
// ─────────────────────────────────────────────────────────────────────────
double Physics::solve(
    const std::vector<Point>& obs,
    Params& p,
    double dt,
    double eps,
    int    max_iter,
    double sigma_obs) const
{
    std::cout << "\n=== Iterative Least-Squares (dataset1: unknown = mu) ===\n";
    std::cout << std::scientific;

    double residual_rms = 0.0;

    for (int iter = 0; iter < max_iter; ++iter)
    {
        const double delta_mu = least_squares_step(obs, p, dt, residual_rms);
        p.mu += delta_mu;

        std::cout << "iter " << iter + 1
                  << "  mu = "        << p.mu
                  << "  Delta_mu = "  << delta_mu
                  << "  RMS = "       << residual_rms
                  << '\n';

        if (std::abs(delta_mu) < eps) {
            std::cout << "Converged after " << iter + 1 << " iterations.\n";
            break;
        }
    }

    // ── Uncertainty on mu ──────────────────────────────────────────────
    // After convergence the covariance of the estimated parameter is:
    //   Var(mu) = sigma_obs² * (Bᵀ B)⁻¹
    // We recompute B one last time to get the final (Bᵀ B).
    const int K = static_cast<int>(obs.size());
    std::vector<double> t_obs(K);
    for (int i = 0; i < K; ++i) t_obs[i] = obs[i].t;

    auto states = simulate_at(p, dt, t_obs);

    double BtB = 0.0;
    for (int i = 0; i < K; ++i)
    {
        const double dx_dmu = states[i][4];
        const double dy_dmu = states[i][5];
        BtB += dx_dmu * dx_dmu + dy_dmu * dy_dmu;
    }

    const double sigma_mu = sigma_obs * std::sqrt(1.0 / BtB);

    std::cout << "\n--- Result ---\n";
    std::cout << "mu        = " << p.mu     << " m⁻¹\n";
    std::cout << "sigma_mu  = " << sigma_mu << " m⁻¹\n";
    std::cout << "RMS final = " << residual_rms << " m\n";

    return sigma_mu;
}

// ─────────────────────────────────────────────────────────────────────────
//  find_impact  – integrate until y = 0, return (x, t)
// ─────────────────────────────────────────────────────────────────────────
std::pair<double,double> Physics::find_impact(
    const Params& p,
    double dt,
    double t_start,
    double tmax) const
{
    // Start from t_start (end of dataset) and integrate forward
    // We need the full trajectory from t=0 to t_start first to get
    // the correct initial state at t_start.
    Vec8   s = make_initial_state(p);
    double t = 0.0;

    // Advance to t_start
    while (t < t_start - 1e-14)
    {
        double step = std::min(dt, t_start - t);
        s = rk4_step(s, p, step);
        t += step;
    }

    // Now march forward until y crosses 0
    Vec8   s_prev = s;
    double t_prev = t;

    while (t < tmax)
    {
        s_prev = s;
        t_prev = t;

        s = rk4_step(s, p, dt);
        t += dt;

        if (s[1] <= 0.0 && s_prev[1] > 0.0)
        {
            // Linear interpolation for t_impact and x_impact
            const double alpha   = s_prev[1] / (s_prev[1] - s[1]);
            const double t_imp   = t_prev + alpha * dt;
            const double x_imp   = s_prev[0] + alpha * (s[0] - s_prev[0]);
            return {x_imp, t_imp};
        }
    }
    std::cerr << "Warning: projectile did not reach y=0 within tmax="
              << tmax << " s\n";
    return {s[0], t};
}

// ─────────────────────────────────────────────────────────────────────────
//  find_max_altitude  – scan trajectory for y_max
// ─────────────────────────────────────────────────────────────────────────
std::pair<double,double> Physics::find_max_altitude(
    const Params& p,
    double dt,
    double tmax) const
{
    Vec8   s = make_initial_state(p);
    double t = 0.0;
    double y_max  = s[1];
    double t_ymax = 0.0;

    while (t <= tmax)
    {
        if (s[1] > y_max) {
            y_max  = s[1];
            t_ymax = t;
        }
        if (s[1] < 0.0 && t > 0.1) break;
        s = rk4_step(s, p, dt);
        t += dt;
    }
    return {y_max, t_ymax};
}

// ─────────────────────────────────────────────────────────────────────────
//  find_min_velocity  – scan trajectory for v_min
// ─────────────────────────────────────────────────────────────────────────
std::pair<double,double> Physics::find_min_velocity(
    const Params& p,
    double dt,
    double tmax) const
{
    Vec8   s = make_initial_state(p);
    double t = 0.0;

    double vx0   = s[2], vy0 = s[3];
    double v_min  = std::sqrt(vx0*vx0 + vy0*vy0);
    double t_vmin = 0.0;

    while (t <= tmax)
    {
        const double vx = s[2], vy = s[3];
        const double v  = std::sqrt(vx*vx + vy*vy);
        if (v < v_min) {
            v_min  = v;
            t_vmin = t;
        }
        if (s[1] < 0.0 && t > 0.1) break;
        s = rk4_step(s, p, dt);
        t += dt;
    }
    return {v_min, t_vmin};
}

// ─────────────────────────────────────────────────────────────────────────
//  read_csv  – parse dataset file
//  Format:  lines starting with '#' are comments (parameter headers)
//           data lines: t,x,y
// ─────────────────────────────────────────────────────────────────────────
std::vector<Point> Physics::read_csv(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + filename);

    std::vector<Point> data;
    std::string line;

    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string tok;
        std::vector<double> vals;

        while (std::getline(ss, tok, ','))
        {
            try { vals.push_back(std::stod(tok)); }
            catch (...) { }
        }

        if (vals.size() >= 3)
            data.push_back({vals[0], vals[1], vals[2]});
    }

    if (data.empty())
        throw std::runtime_error("No data found in: " + filename);

    return data;
}