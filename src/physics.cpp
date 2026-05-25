#include "physics.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

// Forward declaration
static void scalar_rk4(double& x, double& y, double& vx, double& vy,
                       double mu, double g, double h);

// ═══════════════════════════════════════════════════════════════════════════
//  PHYSICAL MODEL  (quadratic drag)
//
//  dx/dt  = vx
//  dy/dt  = vy
//  dvx/dt = -mu * v * vx
//  dvy/dt = -g  - mu * v * vy      v = sqrt(vx²+vy²)
//
//  VARIATIONAL EQUATIONS  for free parameter c_i:
//
//  Let  px  = ∂x/∂c_i   py  = ∂y/∂c_i
//       pvx = ∂vx/∂c_i  pvy = ∂vy/∂c_i
//
//  d(px)/dt  = pvx
//  d(py)/dt  = pvy
//
//  For c_i = mu:
//    d(pvx)/dt = -mu*v*pvx - mu*(vx*pvx+vy*pvy)/v * vx - v*vx
//    d(pvy)/dt = -mu*v*pvy - mu*(vx*pvx+vy*pvy)/v * vy - v*vy
//
//  For c_i = v0:
//    initial conditions: pvx(0)=cos(θ), pvy(0)=sin(θ)
//    d(pvx)/dt = -mu*v*pvx - mu*(vx*pvx+vy*pvy)/v * vx
//    d(pvy)/dt = -mu*v*pvy - mu*(vx*pvx+vy*pvy)/v * vy
//
//  For c_i = theta0 (degrees):
//    initial conditions: pvx(0)=-v0*sin(θ)*π/180, pvy(0)=v0*cos(θ)*π/180
//    d(pvx)/dt = same drag term (no explicit θ in RHS)
//    d(pvy)/dt = same
//
//  For c_i = x0:
//    initial conditions: px(0)=1, others 0
//    d(pvx)/dt = d(pvy)/dt = 0  (x0 doesn't appear in acceleration)
//    → pvx=0, pvy=0 always, px=1, py=0 always
//
//  For c_i = y0: symmetric, py=1, px=0 always
// ═══════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────

double Physics::get_param(const Params& p, ParamIndex idx)
{
    switch(idx) {
        case IDX_X0:    return p.x0;
        case IDX_Y0:    return p.y0;
        case IDX_V0:    return p.v0;
        case IDX_THETA0:return p.theta0;
        case IDX_MU:    return p.mu;
    }
    return 0.0;
}

void Physics::set_param(Params& p, ParamIndex idx, double val)
{
    switch(idx) {
        case IDX_X0:    p.x0    = val; break;
        case IDX_Y0:    p.y0    = val; break;
        case IDX_V0:    p.v0    = val; break;
        case IDX_THETA0:p.theta0= val; break;
        case IDX_MU:    p.mu    = val; break;
    }
}

std::string Physics::param_name(ParamIndex idx)
{
    switch(idx) {
        case IDX_X0:    return "x0";
        case IDX_Y0:    return "y0";
        case IDX_V0:    return "v0";
        case IDX_THETA0:return "theta0";
        case IDX_MU:    return "mu";
    }
    return "?";
}

// ─────────────────────────────────────────────────────────────────────────
//  make_initial_state
//  Extended state: [ x, y, vx, vy,  px_0,py_0,pvx_0,pvy_0, ... ]
// ─────────────────────────────────────────────────────────────────────────
Eigen::VectorXd Physics::make_initial_state(
    const Params& p,
    const std::vector<ParamIndex>& free) const
{
    const int np = (int)free.size();
    Eigen::VectorXd s = Eigen::VectorXd::Zero(4 + 4*np);

    const double theta = p.theta0 * M_PI / 180.0;
    s[0] = p.x0;
    s[1] = p.y0;
    s[2] = p.v0 * std::cos(theta);
    s[3] = p.v0 * std::sin(theta);

    // Initial conditions for variational part
    for (int i = 0; i < np; ++i)
    {
        int base = 4 + 4*i;
        // px, py, pvx, pvy  all zero by default
        switch(free[i])
        {
        case IDX_MU:
            // mu doesn't appear in initial position/velocity
            break;
        case IDX_V0:
            // ∂vx/∂v0 = cos(θ),  ∂vy/∂v0 = sin(θ)
            s[base+2] = std::cos(theta);
            s[base+3] = std::sin(theta);
            break;
        case IDX_THETA0:
            // ∂vx/∂θ0 = -v0*sin(θ)*π/180,  ∂vy/∂θ0 = v0*cos(θ)*π/180
            s[base+2] = -p.v0 * std::sin(theta) * M_PI / 180.0;
            s[base+3] =  p.v0 * std::cos(theta) * M_PI / 180.0;
            break;
        case IDX_X0:
            s[base+0] = 1.0;   // ∂x/∂x0 = 1
            break;
        case IDX_Y0:
            s[base+1] = 1.0;   // ∂y/∂y0 = 1
            break;
        }
    }
    return s;
}

// ─────────────────────────────────────────────────────────────────────────
//  variational_rhs  – d/dt of (px, py, pvx, pvy) for one free parameter
// ─────────────────────────────────────────────────────────────────────────
void Physics::variational_rhs(
    double vx, double vy,
    double pvx_i, double pvy_i,
    double px_i,  double py_i,
    double mu, double /*g*/,
    ParamIndex idx,
    double& dpvx, double& dpvy,
    double& dpx,  double& dpy) const
{
    const double v     = std::sqrt(vx*vx + vy*vy);
    const double vdotp = (v > 1e-12) ? (vx*pvx_i + vy*pvy_i) / v : 0.0;

    dpx  = pvx_i;
    dpy  = pvy_i;

    // Drag contribution (common to all params that affect velocity)
    double drag_vx = -mu*v*pvx_i - mu*vdotp*vx;
    double drag_vy = -mu*v*pvy_i - mu*vdotp*vy;

    if (idx == IDX_MU)
    {
        // Extra explicit ∂/∂mu of (-mu*v*vx) = -v*vx
        dpvx = drag_vx - v*vx;
        dpvy = drag_vy - v*vy;
    }
    else if (idx == IDX_X0 || idx == IDX_Y0)
    {
        // x0,y0 don't appear in accelerations → pvx,pvy stay 0
        dpvx = 0.0;
        dpvy = 0.0;
        // px,py propagate via pvx=0,pvy=0 → constant
        dpx  = pvx_i;   // = 0
        dpy  = pvy_i;   // = 0
    }
    else
    {
        // v0, theta0: only drag terms (no explicit param in accel)
        dpvx = drag_vx;
        dpvy = drag_vy;
    }
    (void)px_i; (void)py_i;
}

// ─────────────────────────────────────────────────────────────────────────
//  f_ext  – full extended ODE RHS
// ─────────────────────────────────────────────────────────────────────────
Eigen::VectorXd Physics::f_ext(
    const Eigen::VectorXd& s,
    const Params& p,
    const std::vector<ParamIndex>& free) const
{
    const int np = (int)free.size();
    Eigen::VectorXd ds = Eigen::VectorXd::Zero(4 + 4*np);

    const double vx = s[2], vy = s[3];
    const double v  = std::sqrt(vx*vx + vy*vy);

    // Trajectory
    ds[0] =  vx;
    ds[1] =  vy;
    ds[2] = -p.mu * v * vx;
    ds[3] = -p.g  - p.mu * v * vy;

    // Variational blocks
    for (int i = 0; i < np; ++i)
    {
        int base = 4 + 4*i;
        double px_i  = s[base+0], py_i  = s[base+1];
        double pvx_i = s[base+2], pvy_i = s[base+3];

        double dpx, dpy, dpvx, dpvy;
        variational_rhs(vx, vy, pvx_i, pvy_i, px_i, py_i,
                        p.mu, p.g, free[i],
                        dpvx, dpvy, dpx, dpy);
        ds[base+0] = dpx;
        ds[base+1] = dpy;
        ds[base+2] = dpvx;
        ds[base+3] = dpvy;
    }
    return ds;
}

// ─────────────────────────────────────────────────────────────────────────
//  rk4_step
// ─────────────────────────────────────────────────────────────────────────
Eigen::VectorXd Physics::rk4_step(
    const Eigen::VectorXd& s,
    const Params& p,
    const std::vector<ParamIndex>& free,
    double dt) const
{
    auto k1 = f_ext(s,               p, free);
    auto k2 = f_ext(s + 0.5*dt*k1,  p, free);
    auto k3 = f_ext(s + 0.5*dt*k2,  p, free);
    auto k4 = f_ext(s +     dt*k3,  p, free);
    return s + (dt/6.0)*(k1 + 2.0*k2 + 2.0*k3 + k4);
}

// ─────────────────────────────────────────────────────────────────────────
//  simulate  (trajectory only, for rendering)
// ─────────────────────────────────────────────────────────────────────────
std::vector<Point> Physics::simulate(
    const Params& p, double dt, double tmax) const
{
    const double h = std::max(dt, 0.01);
    double theta=p.theta0*M_PI/180.0;
    double x=p.x0, y=p.y0;
    double vx=p.v0*std::cos(theta), vy=p.v0*std::sin(theta);
    double t=0.0;

    std::vector<Point> traj;
    int max_steps=(int)(tmax/h)+2;
    traj.reserve(max_steps);

    while(t<=tmax+1e-9){
        traj.push_back({t,x,y});
        if(y<p.y0-1.0&&t>0.5)break;
        scalar_rk4(x,y,vx,vy,p.mu,p.g,h);
        t+=h;
    }
    return traj;
}

// ─────────────────────────────────────────────────────────────────────────
//  simulate_at  – extended state at observation times
// ─────────────────────────────────────────────────────────────────────────
std::vector<Eigen::VectorXd> Physics::simulate_at(
    const Params& p,
    const std::vector<ParamIndex>& free,
    double dt,
    const std::vector<double>& t_obs) const
{
    std::vector<Eigen::VectorXd> result;
    result.reserve(t_obs.size());

    Eigen::VectorXd s = make_initial_state(p, free);
    double t = 0.0;

    for (size_t k = 0; k < t_obs.size(); ++k)
    {
        double t_target = t_obs[k];
        while (t < t_target - 1e-14) {
            double step = std::min(dt, t_target - t);
            s = rk4_step(s, p, free, step);
            t += step;
        }
        result.push_back(s);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────
//  least_squares_step
//
//  Builds  B (2K × np)  and  Y (2K × 1),  solves  ΔC = (BᵀB)⁻¹ BᵀY
// ─────────────────────────────────────────────────────────────────────────
Eigen::VectorXd Physics::least_squares_step(
    const std::vector<Point>& obs,
    const Params& p,
    const std::vector<ParamIndex>& free,
    double dt,
    double& residual_rms) const
{
    const int K  = (int)obs.size();
    const int np = (int)free.size();

    std::vector<double> t_obs(K);
    for (int i=0;i<K;++i) t_obs[i]=obs[i].t;

    auto states = simulate_at(p, free, dt, t_obs);

    Eigen::MatrixXd B(2*K, np);
    Eigen::VectorXd Y(2*K);

    double rms = 0.0;
    for (int i = 0; i < K; ++i)
    {
        Y[2*i  ] = obs[i].x - states[i][0];
        Y[2*i+1] = obs[i].y - states[i][1];
        rms += Y[2*i]*Y[2*i] + Y[2*i+1]*Y[2*i+1];

        for (int j = 0; j < np; ++j)
        {
            int base = 4 + 4*j;
            B(2*i,   j) = states[i][base+0];   // ∂x/∂c_j
            B(2*i+1, j) = states[i][base+1];   // ∂y/∂c_j
        }
    }
    residual_rms = std::sqrt(rms / (2.0*K));

    // Normal equations: ΔC = (BᵀB)⁻¹ BᵀY
    Eigen::MatrixXd BtB = B.transpose() * B;
    Eigen::VectorXd BtY = B.transpose() * Y;

    // Use LDLT (robust for symmetric positive semi-definite)
    return BtB.ldlt().solve(BtY);
}

// ─────────────────────────────────────────────────────────────────────────
//  solve  – iterative least-squares
// ─────────────────────────────────────────────────────────────────────────
Eigen::VectorXd Physics::solve(
    const std::vector<Point>& obs,
    Params& p,
    const std::vector<ParamIndex>& free,
    double dt, double eps, int max_iter, double sigma_obs) const
{
    const int np = (int)free.size();

    std::cout << "\n=== Least-squares  (" << np << " unknown"
              << (np>1?"s":"") << ": ";
    for (int i=0;i<np;++i)
        std::cout << param_name(free[i]) << (i<np-1?", ":"");
    std::cout << ") ===\n" << std::scientific;

    double rms = 0.0;

    for (int iter = 0; iter < max_iter; ++iter)
    {
        Eigen::VectorXd dc = least_squares_step(obs, p, free, dt, rms);

        for (int j=0;j<np;++j)
            set_param(p, free[j], get_param(p,free[j]) + dc[j]);

        std::cout << "iter " << iter+1 << "  |dc|=" << dc.norm()
                  << "  RMS=" << rms << '\n';

        if (dc.norm() < eps) {
            std::cout << "Converged after " << iter+1 << " iterations.\n";
            break;
        }
    }

    // ── Uncertainty: diag of sigma_obs² * (BᵀB)⁻¹ ──────────────────────
    const int K = (int)obs.size();
    std::vector<double> t_obs(K);
    for (int i=0;i<K;++i) t_obs[i]=obs[i].t;
    auto states = simulate_at(p, free, dt, t_obs);

    Eigen::MatrixXd B(2*K, np);
    for (int i=0;i<K;++i)
        for (int j=0;j<np;++j) {
            int base=4+4*j;
            B(2*i,   j)=states[i][base+0];
            B(2*i+1, j)=states[i][base+1];
        }

    Eigen::MatrixXd BtB     = B.transpose()*B;
    Eigen::MatrixXd BtB_inv = BtB.ldlt().solve(
                                Eigen::MatrixXd::Identity(np,np));
    Eigen::VectorXd sigmas(np);
    for (int j=0;j<np;++j)
        sigmas[j] = sigma_obs * std::sqrt(std::abs(BtB_inv(j,j)));

    std::cout << "\n--- Results ---\n" << std::fixed << std::setprecision(6);
    for (int j=0;j<np;++j)
        std::cout << "  " << param_name(free[j])
                  << " = " << get_param(p,free[j])
                  << "  ±  " << sigmas[j] << '\n';
    std::cout << "  RMS = " << rms << " m\n";

    return sigmas;
}

// ─────────────────────────────────────────────────────────────────────────
//  Post-processing
// ─────────────────────────────────────────────────────────────────────────
std::pair<double,double> Physics::find_impact(
    const Params& p, double, double, double tmax) const
{
    const double h = 0.05;
    double theta = p.theta0 * M_PI / 180.0;
    double x=p.x0, y=p.y0;
    double vx=p.v0*std::cos(theta), vy=p.v0*std::sin(theta);
    double x_prev=x, y_prev=y, t_prev=0.0, t=0.0;
    int max_steps=(int)(tmax/h)+1;
    for(int step=0;step<max_steps;++step){
        double v=std::sqrt(vx*vx+vy*vy);
        double ax1=-p.mu*v*vx,          ay1=-p.g-p.mu*v*vy;
        double vx2=vx+0.5*h*ax1,        vy2=vy+0.5*h*ay1;
        double v2=std::sqrt(vx2*vx2+vy2*vy2);
        double ax2=-p.mu*v2*vx2,        ay2=-p.g-p.mu*v2*vy2;
        double vx3=vx+0.5*h*ax2,        vy3=vy+0.5*h*ay2;
        double v3=std::sqrt(vx3*vx3+vy3*vy3);
        double ax3=-p.mu*v3*vx3,        ay3=-p.g-p.mu*v3*vy3;
        double vx4=vx+h*ax3,            vy4=vy+h*ay3;
        double v4=std::sqrt(vx4*vx4+vy4*vy4);
        double ax4=-p.mu*v4*vx4,        ay4=-p.g-p.mu*v4*vy4;
        x_prev=x; y_prev=y; t_prev=t;
        x +=(h/6.0)*(vx +2*vx2+2*vx3+vx4);
        y +=(h/6.0)*(vy +2*vy2+2*vy3+vy4);
        vx+=(h/6.0)*(ax1+2*ax2+2*ax3+ax4);
        vy+=(h/6.0)*(ay1+2*ay2+2*ay3+ay4);
        t+=h;
        if(t>1.0 && y<=p.y0 && y_prev>p.y0){
            double alpha=(y_prev-p.y0)/(y_prev-y);
            return {x_prev+alpha*(x-x_prev), t_prev+alpha*h};
        }
    }
    std::cerr << "Warning: find_impact tmax reached\n";
    return {x, t};
}

// ── Shared scalar RK4 step (no Eigen) ────────────────────────────────────
static void scalar_rk4(double& x, double& y, double& vx, double& vy,
                       double mu, double g, double h)
{
    auto ax=[&](double vx_,double vy_){
        double v=std::sqrt(vx_*vx_+vy_*vy_); return -mu*v*vx_;};
    auto ay=[&](double vx_,double vy_){
        double v=std::sqrt(vx_*vx_+vy_*vy_); return -g-mu*v*vy_;};

    double ax1=ax(vx,vy),          ay1=ay(vx,vy);
    double vx2=vx+0.5*h*ax1,       vy2=vy+0.5*h*ay1;
    double ax2=ax(vx2,vy2),         ay2=ay(vx2,vy2);
    double vx3=vx+0.5*h*ax2,       vy3=vy+0.5*h*ay2;
    double ax3=ax(vx3,vy3),         ay3=ay(vx3,vy3);
    double vx4=vx+h*ax3,            vy4=vy+h*ay3;
    double ax4=ax(vx4,vy4),         ay4=ay(vx4,vy4);

    x  += (h/6.0)*(vx +2*vx2+2*vx3+vx4);
    y  += (h/6.0)*(vy +2*vy2+2*vy3+vy4);
    vx += (h/6.0)*(ax1+2*ax2+2*ax3+ax4);
    vy += (h/6.0)*(ay1+2*ay2+2*ay3+ay4);
}

std::pair<double,double> Physics::find_max_altitude(
    const Params& p, double, double tmax) const
{
    const double h = 0.05;
    double theta=p.theta0*M_PI/180.0;
    double x=p.x0,y=p.y0,vx=p.v0*std::cos(theta),vy=p.v0*std::sin(theta);
    double t=0.0, ymax=y, tymax=0.0;
    int max_steps=(int)(tmax/h)+1;
    for(int i=0;i<max_steps;++i){
        if(y>ymax){ymax=y;tymax=t;}
        if(y<p.y0-1.0&&t>0.5)break;
        scalar_rk4(x,y,vx,vy,p.mu,p.g,h);
        t+=h;
    }
    return {ymax,tymax};
}

std::pair<double,double> Physics::find_min_velocity(
    const Params& p, double, double tmax) const
{
    const double h = 0.05;
    double theta=p.theta0*M_PI/180.0;
    double x=p.x0,y=p.y0,vx=p.v0*std::cos(theta),vy=p.v0*std::sin(theta);
    double t=0.0, vmin=std::sqrt(vx*vx+vy*vy), tvmin=0.0;
    int max_steps=(int)(tmax/h)+1;
    for(int i=0;i<max_steps;++i){
        double v=std::sqrt(vx*vx+vy*vy);
        if(v<vmin){vmin=v;tvmin=t;}
        if(y<p.y0-1.0&&t>0.5)break;
        scalar_rk4(x,y,vx,vy,p.mu,p.g,h);
        t+=h;
    }
    return {vmin,tvmin};
}

// ─────────────────────────────────────────────────────────────────────────
//  read_csv
// ─────────────────────────────────────────────────────────────────────────
std::vector<Point> Physics::read_csv(const std::string& filename)
{
    std::ifstream file(filename);
    if(!file.is_open())
        throw std::runtime_error("Cannot open: "+filename);

    std::vector<Point> data;
    std::string line;
    while(std::getline(file,line)){
        if(line.empty()||line[0]=='#') continue;
        std::istringstream ss(line);
        std::string tok;
        std::vector<double> vals;
        while(std::getline(ss,tok,','))
            try{vals.push_back(std::stod(tok));}catch(...){}
        if(vals.size()>=3)
            data.push_back({vals[0],vals[1],vals[2]});
    }
    if(data.empty())
        throw std::runtime_error("No data in: "+filename);
    return data;
}