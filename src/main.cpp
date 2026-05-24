#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "physics.hpp"
#include "renderer.hpp"
#include "text_renderer.hpp"

static const int WIN_W = 1280;
static const int WIN_H = 720;

static void separator()
{
    std::cout << "─────────────────────────────────────────────────────────\n";
}

static std::string fmt(double v, int prec = 5)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────
//  Grid builder – returns interleaved { x0,y0,type, x1,y1,type } pairs
//  for GL_LINES (one segment = 2 vertices, no zigzag).
// ─────────────────────────────────────────────────────────────────────────
static std::vector<float> buildGrid(float xMin, float xMax,
                                    float yMin, float yMax,
                                    float step)
{
    std::vector<float> v;

    // Snap start to grid
    float xs = std::floor(xMin / step) * step;
    float ys = std::floor(yMin / step) * step;

    // Vertical lines
    for (float x = xs; x <= xMax + step * 0.5f; x += step)
    {
        float t = (std::abs(x) < step * 0.01f) ? 1.0f : 0.0f;
        // Segment: bottom vertex, top vertex
        v.insert(v.end(), { x, yMin, t,
                             x, yMax, t });
    }
    // Horizontal lines
    for (float y = ys; y <= yMax + step * 0.5f; y += step)
    {
        float t = (std::abs(y) < step * 0.01f) ? 1.0f : 0.0f;
        v.insert(v.end(), { xMin, y, t,
                             xMax, y, t });
    }
    return v;
}

// ─────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    // ── 0. Dataset ────────────────────────────────────────────────────────
    const std::string dataset_path =
        (argc > 1) ? argv[1] : "../dataset1.csv";

    std::cout << "\nLoading dataset: " << dataset_path << '\n';
    std::vector<Point> observations;
    try {
        observations = Physics::read_csv(dataset_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    std::cout << "Loaded " << observations.size() << " points.\n";

    // ── 1. Fixed parameters (dataset1) ───────────────────────────────────
    Params params;
    params.x0     = 0.0;
    params.y0     = 0.0;
    params.v0     = 200.0;
    params.theta0 = 45.0;
    params.g      = 9.80665;
    params.mu     = 0.0;

    // ── 2. Least-squares ─────────────────────────────────────────────────
    Physics physics;
    separator();
    const double sigma_mu = physics.solve(
        observations, params, 0.001, 1e-12, 200, 1.0
    );

    // ── 3. Post-processing ───────────────────────────────────────────────
    separator();
    const double t_last = observations.back().t;
    const double tmax   = 300.0;

    auto [x_impact, t_impact] = physics.find_impact       (params,0.001,t_last,tmax);
    auto [y_max,    t_ymax  ] = physics.find_max_altitude  (params,0.001,tmax);
    auto [v_min,    t_vmin  ] = physics.find_min_velocity  (params,0.001,tmax);

    separator();
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "mu       = " << params.mu  << " +/- " << sigma_mu << " m^-1\n";
    std::cout << "x_impact = " << x_impact   << " m   at t=" << t_impact << " s\n";
    std::cout << "y_max    = " << y_max       << " m   at t=" << t_ymax   << " s\n";
    std::cout << "v_min    = " << v_min       << " m/s at t=" << t_vmin   << " s\n";
    separator();

    // ── 4. Trajectory vertex data ─────────────────────────────────────────
    auto model_traj = physics.simulate(params, 0.01, t_impact + 0.5);
    const int N = (int)model_traj.size();

    // LINE_STRIP vertices: { x, y, speed }
    std::vector<float> line_verts;
    line_verts.reserve(N * 3);
    float spd_min =  1e9f, spd_max = -1e9f;

    for (int i = 0; i < N; ++i)
    {
        float spd;
        if (i + 1 < N) {
            float dx   = (float)(model_traj[i+1].x - model_traj[i].x);
            float dy   = (float)(model_traj[i+1].y - model_traj[i].y);
            float dtau = (float)(model_traj[i+1].t - model_traj[i].t);
            spd = (dtau > 0.f) ? std::sqrt(dx*dx+dy*dy)/dtau : 0.f;
        } else {
            spd = (line_verts.size() >= 3) ? line_verts[line_verts.size()-1] : 0.f;
        }
        spd_min = std::min(spd_min, spd);
        spd_max = std::max(spd_max, spd);
        line_verts.push_back((float)model_traj[i].x);
        line_verts.push_back((float)model_traj[i].y);
        line_verts.push_back(spd);
    }

    // POINTS vertices: { x, y }
    std::vector<float> obs_verts;
    for (const auto& p : observations) {
        obs_verts.push_back((float)p.x);
        obs_verts.push_back((float)p.y);
    }

    // ── 5. Bounding box + orthographic projection ─────────────────────────
    float xMin =  1e9f, xMax = -1e9f;
    float yMinF=  1e9f, yMaxF= -1e9f;
    for (int i = 0; i < N*3; i += 3) {
        xMin  = std::min(xMin,  line_verts[i]);
        xMax  = std::max(xMax,  line_verts[i]);
        yMinF = std::min(yMinF, line_verts[i+1]);
        yMaxF = std::max(yMaxF, line_verts[i+1]);
    }
    // Enforce y=0 visible (ground)
    yMinF = std::min(yMinF, 0.0f);

    const float mx = 0.06f*(xMax-xMin);
    const float my = 0.08f*(yMaxF-yMinF+1.f);
    const float L  = xMin - mx,   R = xMax + mx;
    const float B  = yMinF - my,  T = yMaxF + my;

    glm::mat4 projection = glm::ortho(L, R, B, T);

    // Grid step: round to nearest 100 or 50
    float worldSpan = xMax - xMin;
    float gridStep  = (worldSpan > 2000.f) ? 200.f :
                      (worldSpan > 1000.f) ? 100.f : 50.f;

    // ── 6. GLFW + GLAD ────────────────────────────────────────────────────
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H,
        "Projectile – Numerical Methods (IPSA)", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return -1;
    }

    glViewport(0, 0, WIN_W, WIN_H);
    glClearColor(0.04f, 0.04f, 0.10f, 1.0f);
    glEnable(GL_MULTISAMPLE);

    // ── 7. Renderers ──────────────────────────────────────────────────────

    // Grid – GL_LINES, stride=3
    Renderer rGrid(DrawMode::LINES,
                   "shaders/grid.vert", "shaders/grid.frag");
    rGrid.uploadData(buildGrid(L, R, B, T, gridStep));

    // Trajectory line – GL_LINE_STRIP, stride=3, speed ramp
    Renderer rLine(DrawMode::LINE_STRIP,
                   "shaders/line.vert", "shaders/line.frag");
    rLine.uploadData(line_verts);

    // Observation points – GL_POINTS, stride=2
    Renderer rObs(DrawMode::POINTS,
                  "shaders/point.vert", "shaders/point.frag");
    rObs.uploadData(obs_verts);

    // Comet (animated head + trail) – GL_POINTS, stride=3, additive blend
    Renderer rComet(DrawMode::COMET,
                    "shaders/comet.vert", "shaders/comet.frag");

    // Text overlay
    TextRenderer text(WIN_W, WIN_H,
                      "shaders/text.vert", "shaders/text.frag");

    // ── 8. Animation state ────────────────────────────────────────────────
    static const int TRAIL_LEN = 40;
    double simTime  = 0.0;
    double simSpeed = 1.0;
    double wallPrev = glfwGetTime();

    glfwSetWindowUserPointer(window, &simSpeed);
    glfwSetKeyCallback(window, [](GLFWwindow* w, int key, int, int action, int)
    {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        double& spd = *static_cast<double*>(glfwGetWindowUserPointer(w));
        if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
            spd = std::min(spd * 2.0, 32.0);
        if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
            spd = std::max(spd * 0.5, 0.125);
        if (key == GLFW_KEY_SPACE)
            spd = 1.0;
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(w, GLFW_TRUE);
    });

    // Pre-build static overlay strings
    const std::string sTitle  = "Projectile with quadratic drag";
    const std::string sMu     = "mu      = " + fmt(params.mu,6) +
                                 " +/- "      + fmt(sigma_mu,2) + " m^-1";
    const std::string sImpact = "x_land  = " + fmt(x_impact,1) +
                                 " m   at t=" + fmt(t_impact,2) + " s";
    const std::string sYmax   = "y_max   = " + fmt(y_max,1) +
                                 " m   at t=" + fmt(t_ymax,2)   + " s";
    const std::string sVmin   = "v_min   = " + fmt(v_min,2) +
                                 " m/s at t=" + fmt(t_vmin,2)   + " s";
    const std::string sCtrl   = "[+/-] speed   [Space] x1   [Esc] quit";

    // Colour legend strings (colour ramp)
    const std::string sLegHigh = "high speed";
    const std::string sLegLow  = "low speed";

    // ── 9. Main loop ──────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        // Time
        double wallNow = glfwGetTime();
        simTime += (wallNow - wallPrev) * simSpeed;
        wallPrev = wallNow;
        if (simTime > t_impact) simTime = 0.0;   // loop

        // Find head index (binary search)
        int lo = 0, hi = N - 1;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (model_traj[mid].t < simTime) lo = mid+1; else hi = mid;
        }
        int headIdx = std::min(lo, N-1);

        // Build comet vertices { x, y, alpha }
        std::vector<float> cometV;
        cometV.reserve(TRAIL_LEN * 3);
        for (int k = 0; k < TRAIL_LEN; ++k)
        {
            int idx = headIdx - (TRAIL_LEN-1-k);
            if (idx < 0) continue;
            float alpha = (float)(k+1) / TRAIL_LEN;
            cometV.push_back((float)model_traj[idx].x);
            cometV.push_back((float)model_traj[idx].y);
            cometV.push_back(alpha);
        }
        rComet.uploadData(cometV);

        // ── Draw ──────────────────────────────────────────────────────────
        glClear(GL_COLOR_BUFFER_BIT);

        rGrid.render(projection);                          // 1. grid
        rLine.render(projection, spd_min, spd_max);        // 2. trajectory
        rObs.render(projection);                           // 3. observations
        rComet.render(projection);                         // 4. comet

        // ── Text overlay ──────────────────────────────────────────────────
        const float S  = 2.0f;
        const float lh = S * 8.0f * 1.5f;
        float tx = 20.0f, ty = 20.0f;

        auto drawShadowed = [&](const std::string& s, float x, float y,
                                glm::vec3 col)
        {
            text.draw(s, x+1, y+1, S, {0,0,0});
            text.draw(s, x,   y,   S, col);
        };

        // Title
        drawShadowed(sTitle,  tx, ty, {1.00f, 0.82f, 0.18f}); ty += lh*1.6f;

        // Results
        drawShadowed(sMu,     tx, ty, {0.75f, 0.95f, 1.00f}); ty += lh;
        drawShadowed(sImpact, tx, ty, {0.75f, 0.95f, 1.00f}); ty += lh;
        drawShadowed(sYmax,   tx, ty, {0.75f, 0.95f, 1.00f}); ty += lh;
        drawShadowed(sVmin,   tx, ty, {0.75f, 0.95f, 1.00f}); ty += lh*1.6f;

        // Live sim time
        std::string sTime = "t_sim   = " + fmt(simTime,2) +
                            " s  (x"    + fmt(simSpeed,2) + ")";
        drawShadowed(sTime, tx, ty, {0.60f, 1.00f, 0.60f}); ty += lh*1.6f;

        // Controls
        drawShadowed(sCtrl, tx, ty, {0.50f, 0.50f, 0.55f});

        // Colour legend (bottom-right)
        float lx = WIN_W - 200.0f, ly = WIN_H - 60.0f;
        drawShadowed(sLegHigh, lx, ly,         {1.00f,0.40f,0.00f}); ly += lh;
        drawShadowed(sLegLow,  lx, ly,         {0.10f,0.10f,1.00f});

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}