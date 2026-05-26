#pragma once

#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "physics.hpp"

// ─────────────────────────────────────────────────────────────────────────
//  HeatmapView
//
//  Computes χ²(c_i, c_j) on a grid of GRID_N × GRID_N points,
//  uploads to a GL_R32F texture, and renders it as a plasma heatmap.
//
//  For dataset1 (1 unknown): sweeps μ only → 1D plot shown as thin heatmap
//  For dataset2+ (2+ unknowns): sweeps the first two free parameters
//
//  The white crosshair marks the converged solution.
// ─────────────────────────────────────────────────────────────────────────

class HeatmapView
{
public:
    // grid_n : resolution of the parameter grid (e.g. 120)
    // range  : +/- factor around converged value for axis range
    //          (e.g. 0.5 → sweep from val*0.5 to val*1.5 relative)
    HeatmapView(int grid_n = 120, float range_factor = 0.5f);
    ~HeatmapView();

    // Compute chi2 grid and upload texture.
    // free must have at least 1 element; uses first 2 for 2-D heatmap.
    void compute(
        const std::vector<Point>&      obs,
        const Params&                  p_converged,
        const std::vector<ParamIndex>& free,
        double dt = 0.05
    );

    // Render the heatmap fullscreen (call before other overlays).
    void render() const;

    // Accessors for axis labels
    float paramMin(int axis) const { return axis==0 ? p0min_ : p1min_; }
    float paramMax(int axis) const { return axis==0 ? p0max_ : p1max_; }
    float chi2Min() const { return chi2min_; }
    float chi2Max() const { return chi2max_; }

private:
    int   gridN_;
    float rangeFactor_;

    GLuint prog_;
    GLuint vao_;          // empty VAO for gl_VertexID trick
    GLuint tex_;          // GL_R32F heatmap texture

    float p0min_, p0max_;
    float p1min_, p1max_;
    float chi2min_, chi2max_;
    float p0conv_, p1conv_;   // converged values (crosshair)

    // Compute chi2 for one parameter point
    static double chi2(
        const std::vector<Point>& obs,
        Params p,
        const std::vector<ParamIndex>& free,
        double dt
    );
};