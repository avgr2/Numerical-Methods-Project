#include "heatmap_view.hpp"
#include "shader.hpp"

#include <cmath>
#include <algorithm>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────
HeatmapView::HeatmapView(int grid_n, float range_factor)
    : gridN_(grid_n), rangeFactor_(range_factor),
      p0min_(0), p0max_(1), p1min_(0), p1max_(1),
      chi2min_(0), chi2max_(1), p0conv_(0), p1conv_(0)
{
    prog_ = createShaderProgram("shaders/heatmap.vert",
                                "shaders/heatmap.frag");
    glGenVertexArrays(1, &vao_);
    glGenTextures(1, &tex_);

    // Pre-allocate texture
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

HeatmapView::~HeatmapView()
{
    glDeleteTextures(1, &tex_);
    glDeleteVertexArrays(1, &vao_);
}

// ─────────────────────────────────────────────────────────────────────────
//  chi2  – sum of squared residuals for a given parameter set
// ─────────────────────────────────────────────────────────────────────────
double HeatmapView::chi2(
    const std::vector<Point>& obs,
    Params p,
    const std::vector<ParamIndex>& free,
    double dt)
{
    // Simple scalar RK4 integration (no Eigen, no variational part)
    const double h = std::max(dt, 0.05);
    double theta = p.theta0 * M_PI / 180.0;
    double x=p.x0, y=p.y0;
    double vx=p.v0*std::cos(theta), vy=p.v0*std::sin(theta);
    double t=0.0;

    // Build a lookup: simulate up to last obs time
    double t_max_obs = obs.back().t;

    // Store trajectory at obs times
    size_t k = 0;
    double sum = 0.0;

    int max_steps = (int)(t_max_obs / h) + 2;
    for (int step = 0; step < max_steps && k < obs.size(); ++step)
    {
        // Advance to next obs time
        double t_target = obs[k].t;
        while (t < t_target - 1e-12)
        {
            double step_h = std::min(h, t_target - t);
            double v1=std::sqrt(vx*vx+vy*vy);
            double ax1=-p.mu*v1*vx, ay1=-p.g-p.mu*v1*vy;
            double vx2=vx+0.5*step_h*ax1, vy2=vy+0.5*step_h*ay1;
            double v2=std::sqrt(vx2*vx2+vy2*vy2);
            double ax2=-p.mu*v2*vx2, ay2=-p.g-p.mu*v2*vy2;
            double vx3=vx+0.5*step_h*ax2, vy3=vy+0.5*step_h*ay2;
            double v3=std::sqrt(vx3*vx3+vy3*vy3);
            double ax3=-p.mu*v3*vx3, ay3=-p.g-p.mu*v3*vy3;
            double vx4=vx+step_h*ax3, vy4=vy+step_h*ay3;
            double v4=std::sqrt(vx4*vx4+vy4*vy4);
            double ax4=-p.mu*v4*vx4, ay4=-p.g-p.mu*v4*vy4;
            x  +=(step_h/6.0)*(vx +2*vx2+2*vx3+vx4);
            y  +=(step_h/6.0)*(vy +2*vy2+2*vy3+vy4);
            vx +=(step_h/6.0)*(ax1+2*ax2+2*ax3+ax4);
            vy +=(step_h/6.0)*(ay1+2*ay2+2*ay3+ay4);
            t  += step_h;
        }
        double dx = obs[k].x - x;
        double dy = obs[k].y - y;
        sum += dx*dx + dy*dy;
        ++k;
    }
    return sum;
}

// ─────────────────────────────────────────────────────────────────────────
//  compute  – build chi2 grid and upload to texture
// ─────────────────────────────────────────────────────────────────────────
void HeatmapView::compute(
    const std::vector<Point>& obs,
    const Params& p_conv,
    const std::vector<ParamIndex>& free,
    double dt)
{
    const int N  = gridN_;
    const int np = (int)free.size();

    // ── Determine which parameter goes on which axis ──────────────────────
    // Convention (matches reference image): mu on Y, other param on X.
    // If mu is not present, use free[0] on X and free[1] on Y.

    int idx_x = 0, idx_y = 1;   // indices into free[]
    if (np >= 2) {
        // Find mu in free list
        for (int i = 0; i < np; ++i) {
            if (free[i] == IDX_MU) { idx_y = i; break; }
        }
        // X axis = first non-mu parameter
        for (int i = 0; i < np; ++i) {
            if (i != idx_y) { idx_x = i; break; }
        }
    } else {
        // Only 1 parameter: put it on X, make Y a thin band
        idx_x = 0; idx_y = 0;
    }

    ParamIndex px_idx = free[idx_x];
    ParamIndex py_idx = (np >= 2) ? free[idx_y] : free[0];

    // ── Axis ranges ───────────────────────────────────────────────────────
    auto half_range = [](ParamIndex idx, double val) -> double {
        switch(idx) {
            case IDX_MU:     return std::max(val*0.8,  1e-5);
            case IDX_V0:     return std::max(val*0.4,  20.0);
            case IDX_THETA0: return std::max(val*0.4,  10.0);
            default:         return std::max(std::abs(val)*0.8+1.0, 5.0);
        }
    };

    double valx = Physics::get_param(p_conv, px_idx);
    double valy = Physics::get_param(p_conv, py_idx);

    double hx = half_range(px_idx, valx);
    double hy = half_range(py_idx, valy);

    p0min_ = (float)(valx - hx);  p0max_ = (float)(valx + hx);  // X axis
    p1min_ = (float)(valy - hy);  p1max_ = (float)(valy + hy);  // Y axis
    p0conv_= (float)valx;
    p1conv_= (float)valy;

    // For 1-param case make Y a thin band
    if (np < 2) { p1min_ = p0min_; p1max_ = p0max_; }

    // ── Compute grid ──────────────────────────────────────────────────────
    std::cout << "Computing chi2 grid " << N << "x" << N << "...\n";
    std::cout << "  X axis: " << Physics::param_name(px_idx)
              << " [" << p0min_ << " .. " << p0max_ << "]\n";
    if (np >= 2)
        std::cout << "  Y axis: " << Physics::param_name(py_idx)
                  << " [" << p1min_ << " .. " << p1max_ << "]\n";
    std::cout.flush();

    std::vector<float> grid(N * N);
    chi2min_ =  1e30f;
    chi2max_ = -1e30f;

    for (int j = 0; j < N; ++j)       // j = row = Y axis (mu)
    for (int i = 0; i < N; ++i)       // i = col = X axis (v0, etc.)
    {
        double px_val = p0min_ + (p0max_ - p0min_) * i / (N - 1);
        double py_val = (np >= 2)
                        ? (p1min_ + (p1max_ - p1min_) * j / (N - 1))
                        : valy;

        Params p_test = p_conv;
        Physics::set_param(p_test, px_idx, px_val);
        if (np >= 2)
            Physics::set_param(p_test, py_idx, py_val);

        float c = (float)chi2(obs, p_test, free, dt);
        grid[j * N + i] = c;
        chi2min_ = std::min(chi2min_, c);
        chi2max_ = std::max(chi2max_, c);
    }

    // Normalise to [0,1]
    float range = chi2max_ - chi2min_;
    if (range < 1e-10f) range = 1.0f;
    for (auto& v : grid)
        v = (v - chi2min_) / range;

    std::cout << "chi2 range: [" << chi2min_ << ", " << chi2max_ << "]\n";

    // ── Upload to texture ─────────────────────────────────────────────────
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
                 N, N, 0, GL_RED, GL_FLOAT, grid.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ─────────────────────────────────────────────────────────────────────────
//  render
// ─────────────────────────────────────────────────────────────────────────
void HeatmapView::render() const
{
    glUseProgram(prog_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glUniform1i(glGetUniformLocation(prog_, "uHeatmap"), 0);
    glUniform1f(glGetUniformLocation(prog_, "uChi2Min"), chi2min_);
    glUniform1f(glGetUniformLocation(prog_, "uChi2Max"), chi2max_);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}