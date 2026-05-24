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

// ═══════════════════════════════════════════════════════════════════════════
//  Window dimensions
// ═══════════════════════════════════════════════════════════════════════════
static const int WIN_W = 1280;
static const int WIN_H = 720;

// ═══════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════
static void separator()
{
    std::cout << "─────────────────────────────────────────────────────────\n";
}

// Format a double to a fixed-precision string
static std::string fmt(double v, int prec = 6)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(prec) << v;
    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Grid geometry builder
//  Returns interleaved { x, y, type }  where type=1 for axes, 0 for grid.
// ═══════════════════════════════════════════════════════════════════════════
static std::vector<float> buildGrid(float xMin, float xMax,
                                    float yMin, float yMax,
                                    float step)
{
    std::vector<float> v;

    // Snap to step
    float x0 = std::floor(xMin / step) * step;
    float y0 = std::floor(yMin / step) * step;

    // Vertical lines
    for (float x = x0; x <= xMax + step * 0.5f; x += step)
    {
        float t = (std::abs(x) < step * 0.01f) ? 1.0f : 0.0f;
        v.insert(v.end(), {x, yMin, t,  x, yMax, t});
    }
    // Horizontal lines
    for (float y = y0; y <= yMax + step * 0.5f; y += step)
    {
        float t = (std::abs(y) < step * 0.01f) ? 1.0f : 0.0f;
        v.insert(v.end(), {xMin, y, t,  xMax, y, t});
    }
    return v;
}

// ═══════════════════════════════════════════════════════════════════════════
//  main
// ═══════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[])
{
    // ─────────────────────────────────────────────────────────────────────
    //  0.  DATASET
    // ─────────────────────────────────────────────────────────────────────
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

    // ─────────────────────────────────────────────────────────────────────
    //  1.  FIXED PARAMETERS (dataset1)
    // ─────────────────────────────────────────────────────────────────────
    Params params;
    params.x0     = 0.0;
    params.y0     = 0.0;
    params.v0     = 200.0;
    params.theta0 = 45.0;
    params.g      = 9.80665;
    params.mu     = 0.0;

    // ─────────────────────────────────────────────────────────────────────
    //  2.  ITERATIVE LEAST-SQUARES
    // ─────────────────────────────────────────────────────────────────────
    Physics physics;
    separator();
    const double sigma_mu = physics.solve(
        observations, params, 0.001, 1e-12, 200, 1.0
    );

    // ─────────────────────────────────────────────────────────────────────
    //  3.  POST-PROCESSING
    // ─────────────────────────────────────────────────────────────────────
    separator();
    const double t_last = observations.back().t;
    const double tmax   = 300.0;

    auto [x_impact, t_impact] = physics.find_impact        (params, 0.001, t_last, tmax);
    auto [y_max,    t_ymax  ] = physics.find_max_altitude  (params, 0.001, tmax);
    auto [v_min,    t_vmin  ] = physics.find_min_velocity  (params, 0.001, tmax);

    separator();
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "mu       = " << params.mu  << " +/- " << sigma_mu << " m^-1\n";
    std::cout << "x_impact = " << x_impact   << " m  at t=" << t_impact << " s\n";
    std::cout << "y_max    = " << y_max       << " m  at t=" << t_ymax   << " s\n";
    std::cout << "v_min    = " << v_min       << " m/s at t=" << t_vmin  << " s\n";
    separator();

    // ─────────────────────────────────────────────────────────────────────
    //  4.  TRAJECTORY DATA
    // ─────────────────────────────────────────────────────────────────────
    auto model_traj = physics.simulate(params, 0.01, t_impact + 0.5);
    const int N = static_cast<int>(model_traj.size());

    // LINE vertices  { x, y, speed }
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
            spd = (dtau > 0.f) ? std::sqrt(dx*dx + dy*dy) / dtau : 0.f;
        } else {
            spd = line_verts.empty() ? 0.f : line_verts[line_verts.size()-1];
        }
        spd_min = std::min(spd_min, spd);
        spd_max = std::max(spd_max, spd);
        line_verts.push_back((float)model_traj[i].x);
        line_verts.push_back((float)model_traj[i].y);
        line_verts.push_back(spd);
    }

    // OBSERVATION vertices  { x, y }
    std::vector<float> obs_verts;
    for (const auto& p : observations) {
        obs_verts.push_back((float)p.x);
        obs_verts.push_back((float)p.y);
    }

    // ─────────────────────────────────────────────────────────────────────
    //  5.  BOUNDING BOX + PROJECTION
    // ─────────────────────────────────────────────────────────────────────
    float x_min =  1e9f, x_max = -1e9f;
    float y_min_f=  1e9f, y_max_f = -1e9f;
    for (int i = 0; i < N*3; i += 3) {
        x_min   = std::min(x_min,   line_verts[i]);
        x_max   = std::max(x_max,   line_verts[i]);
        y_min_f = std::min(y_min_f, line_verts[i+1]);
        y_max_f = std::max(y_max_f, line_verts[i+1]);
    }
    const float mx = 0.07f*(x_max-x_min);
    const float my = 0.07f*(y_max_f-y_min_f+1.f);
    const float L  = x_min - mx,  R = x_max + mx;
    const float B  = y_min_f - my, T = y_max_f + my;

    glm::mat4 projection = glm::ortho(L, R, B, T);

    // Grid step (round to nearest 50)
    float gridStep = 50.0f;

    // ─────────────────────────────────────────────────────────────────────
    //  6.  GLFW + GLAD
    // ─────────────────────────────────────────────────────────────────────
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);   // 4× MSAA

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

    // ─────────────────────────────────────────────────────────────────────
    //  7.  RENDERERS
    // ─────────────────────────────────────────────────────────────────────

    // 7a. Grid
    Renderer rGrid(DrawMode::LINE,
                   "shaders/grid.vert", "shaders/grid.frag");
    {
        auto gv = buildGrid(L, R, B, T, gridStep);
        rGrid.uploadData(gv);
    }

    // 7b. Trajectory line (speed colour ramp)
    Renderer rLine(DrawMode::LINE,
                   "shaders/line.vert", "shaders/line.frag");
    rLine.uploadData(line_verts);

    // 7c. Observation points (orange glow circles)
    Renderer rObs(DrawMode::POINTS,
                  "shaders/point.vert", "shaders/point.frag");
    rObs.uploadData(obs_verts);

    // 7d. Comet (animated projectile head + trail)
    //     We keep a ring buffer of the last TRAIL_LEN trajectory indices.
    Renderer rComet(DrawMode::POINTS,
                    "shaders/comet.vert", "shaders/comet.frag");
    static const int TRAIL_LEN = 40;

    // 7e. Text overlay
    TextRenderer text(WIN_W, WIN_H,
                      "shaders/text.vert", "shaders/text.frag");

    // ─────────────────────────────────────────────────────────────────────
    //  8.  ANIMATION STATE
    //      The projectile advances along model_traj in real time,
    //      scaled by simSpeed (press +/- to change).
    // ─────────────────────────────────────────────────────────────────────
    double simTime   = 0.0;     // current simulation time (s)
    double simSpeed  = 1.0;     // real-time multiplier
    double wallPrev  = glfwGetTime();
    bool   looping   = true;    // auto-restart when impact is reached

    // ─────────────────────────────────────────────────────────────────────
    //  9.  KEYBOARD CALLBACK  (speed control)
    // ─────────────────────────────────────────────────────────────────────
    // Store simSpeed in the window user pointer for access in lambda
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

    // Pre-build overlay strings (static, computed once)
    const std::string sTitle  = "Projectile with quadratic drag";
    const std::string sMu     = "mu      = " + fmt(params.mu,   6) + " +/- " + fmt(sigma_mu,2) + " m^-1";
    const std::string sImpact = "x_land  = " + fmt(x_impact, 1) + " m  at t=" + fmt(t_impact,2) + " s";
    const std::string sYmax   = "y_max   = " + fmt(y_max,    1) + " m  at t=" + fmt(t_ymax,  2) + " s";
    const std::string sVmin   = "v_min   = " + fmt(v_min,    2) + " m/s at t=" + fmt(t_vmin, 2) + " s";
    const std::string sCtrl   = "[+/-] speed  [Space] reset  [Esc] quit";

    // ─────────────────────────────────────────────────────────────────────
    //  10.  MAIN LOOP
    // ─────────────────────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        // -- Time update --------------------------------------------------
        double wallNow  = glfwGetTime();
        double wallDt   = wallNow - wallPrev;
        wallPrev        = wallNow;
        simTime        += wallDt * simSpeed;

        if (simTime > t_impact) {
            simTime = looping ? 0.0 : t_impact;
        }

        // -- Find trajectory index for current simTime --------------------
        // Binary search in model_traj (sorted by t)
        int lo = 0, hi = N - 1;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (model_traj[mid].t < simTime) lo = mid + 1; else hi = mid;
        }
        int headIdx = std::min(lo, N - 1);

        // -- Build comet vertex data  { x, y, alpha } --------------------
        std::vector<float> cometVerts;
        cometVerts.reserve(TRAIL_LEN * 3);
        for (int k = 0; k < TRAIL_LEN; ++k)
        {
            int idx = headIdx - (TRAIL_LEN - 1 - k);
            if (idx < 0) continue;
            float alpha = static_cast<float>(k + 1) / TRAIL_LEN;
            cometVerts.push_back((float)model_traj[idx].x);
            cometVerts.push_back((float)model_traj[idx].y);
            cometVerts.push_back(alpha);
        }
        rComet.uploadData(cometVerts);

        // -- Render -------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT);

        // 1. Grid (drawn first, behind everything)
        rGrid.render(projection);

        // 2. Full trajectory line (speed colour ramp)
        rLine.render(projection, spd_min, spd_max);

        // 3. Observation points
        rObs.render(projection);

        // 4. Animated comet
        rComet.render(projection);

        // 5. Text overlay ─────────────────────────────────────────────────
        const float S  = 2.0f;    // glyph scale  (16×16 px per char)
        const float lh = S * 8.0f * 1.4f;  // line height with spacing
        float tx = 18.0f, ty = 18.0f;

        // Panel background would be nice but requires another quad renderer.
        // Instead we draw a subtle dark shadow offset, then the bright text.
        auto drawShadowed = [&](const std::string& str, float x, float y,
                                glm::vec3 col)
        {
            text.draw(str, x+1, y+1, S, {0.0f,0.0f,0.0f});  // shadow
            text.draw(str, x,   y,   S, col);
        };

        drawShadowed(sTitle,  tx, ty,        {1.00f, 0.85f, 0.20f});  ty += lh * 1.5f;
        drawShadowed(sMu,     tx, ty,        {0.80f, 0.95f, 1.00f});  ty += lh;
        drawShadowed(sImpact, tx, ty,        {0.80f, 0.95f, 1.00f});  ty += lh;
        drawShadowed(sYmax,   tx, ty,        {0.80f, 0.95f, 1.00f});  ty += lh;
        drawShadowed(sVmin,   tx, ty,        {0.80f, 0.95f, 1.00f});  ty += lh * 1.5f;

        // Live simulation time
        std::string sTime = "t_sim   = " + fmt(simTime, 2) + " s  (x" + fmt(simSpeed,2) + ")";
        drawShadowed(sTime, tx, ty, {0.70f, 1.00f, 0.70f});           ty += lh * 1.5f;

        drawShadowed(sCtrl, tx, ty, {0.55f, 0.55f, 0.60f});

        // ─────────────────────────────────────────────────────────────────
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}