#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "physics.hpp"
#include "renderer.hpp"
#include "text_renderer.hpp"

static const int WIN_W = 1280;
static const int WIN_H = 720;

static void separator(){ std::cout<<"─────────────────────────────────────────────────────────\n"; }

static std::string fmt(double v,int prec=5){
    std::ostringstream ss; ss<<std::fixed<<std::setprecision(prec)<<v; return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────
//  Detect dataset number from filename  (dataset1 → 1, etc.)
// ─────────────────────────────────────────────────────────────────────────
static int detect_dataset(const std::string& path)
{
    for (int d : {4,3,2,1})
        if (path.find("dataset"+std::to_string(d)) != std::string::npos)
            return d;
    return 1;  // default
}

// ─────────────────────────────────────────────────────────────────────────
//  Configure Params + free list from dataset number
// ─────────────────────────────────────────────────────────────────────────
static std::vector<ParamIndex> configure(int dataset, Params& p)
{
    // Common fixed values
    p.g = 9.80665;

    switch (dataset)
    {
    case 1:
        // x0,y0,v0,theta0,g fixed → mu unknown
        p.x0=0; p.y0=0; p.v0=200.0; p.theta0=45.0;
        p.mu=0.0;
        return { IDX_MU };

    case 2:
        // x0,y0,theta0,g fixed → mu, v0 unknown
        p.x0=0; p.y0=0; p.theta0=45.0;
        p.mu=0.001; p.v0=100.0;   // initial guesses
        return { IDX_MU, IDX_V0 };

    case 3:
        // x0,y0,g fixed → mu, v0, theta0 unknown
        p.x0=0; p.y0=0;
        p.mu=0.001; p.v0=200.0; p.theta0=20.0;   // initial guesses
        return { IDX_MU, IDX_V0, IDX_THETA0 };

    case 4:
        // g fixed → x0,y0,v0,mu,theta0 unknown
        p.x0=0.0; p.y0=0.0; p.v0=100.0;
        p.mu=0.001; p.theta0=60.0;   // initial guesses
        return { IDX_X0, IDX_Y0, IDX_V0, IDX_MU, IDX_THETA0 };

    default:
        return { IDX_MU };
    }
}

// ─────────────────────────────────────────────────────────────────────────
//  Grid builder
// ─────────────────────────────────────────────────────────────────────────
static std::vector<float> buildGrid(float xMin,float xMax,float yMin,float yMax,float step)
{
    std::vector<float> v;
    float xs=std::floor(xMin/step)*step, ys=std::floor(yMin/step)*step;
    for(float x=xs;x<=xMax+step*0.5f;x+=step){
        float t=(std::abs(x)<step*0.01f)?1.f:0.f;
        v.insert(v.end(),{x,yMin,t, x,yMax,t});
    }
    for(float y=ys;y<=yMax+step*0.5f;y+=step){
        float t=(std::abs(y)<step*0.01f)?1.f:0.f;
        v.insert(v.end(),{xMin,y,t, xMax,y,t});
    }
    return v;
}

// ─────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    // ── 0. Dataset ────────────────────────────────────────────────────────
    const std::string dataset_path =
        (argc > 1) ? argv[1] : "../dataset1.csv";

    const int dataset_num = detect_dataset(dataset_path);
    std::cout << "\nDataset " << dataset_num << " : " << dataset_path << '\n';

    std::vector<Point> observations;
    try {
        observations = Physics::read_csv(dataset_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n'; return 1;
    }
    std::cout << "Loaded " << observations.size() << " points.\n";

    // ── 1. Parameters ─────────────────────────────────────────────────────
    Params params;
    auto free = configure(dataset_num, params);

    // ── 2. Least-squares ──────────────────────────────────────────────────
    Physics physics;
    separator();
    auto sigmas = physics.solve(observations, params, free,
                                0.001, 1e-10, 300, 1.0);

    // ── 3. Post-processing ────────────────────────────────────────────────
    separator();
    const double t_last = observations.back().t;
    std::cout << "DEBUG: calling find_impact...\n"; std::cout.flush();
    auto [x_impact,t_impact] = physics.find_impact       (params,0.01,t_last,300.0);
    std::cout << "DEBUG: x_impact=" << x_impact << " t_impact=" << t_impact << '\n'; std::cout.flush();
    auto [y_max,   t_ymax  ] = physics.find_max_altitude  (params,0.01,300.0);
    std::cout << "DEBUG: y_max=" << y_max << " t_ymax=" << t_ymax << '\n'; std::cout.flush();
    auto [v_min,   t_vmin  ] = physics.find_min_velocity  (params,0.01,300.0);
    std::cout << "DEBUG: v_min=" << v_min << " t_vmin=" << t_vmin << '\n'; std::cout.flush();

    separator();
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "x_impact = " << x_impact << " m  at t=" << t_impact << " s\n";
    std::cout << "y_max    = " << y_max     << " m  at t=" << t_ymax   << " s\n";
    std::cout << "v_min    = " << v_min     << " m/s at t=" << t_vmin  << " s\n";
    separator();

    // ── 4. Trajectory data for rendering ─────────────────────────────────
    std::cout << "DEBUG: calling simulate, t_impact=" << t_impact << '\n'; std::cout.flush();
    auto model_traj = physics.simulate(params, 0.01, t_impact+0.5);
    const int N = (int)model_traj.size();
    std::cout << "DEBUG: simulate done, N=" << N << '\n'; std::cout.flush();

    std::vector<float> line_verts;
    line_verts.reserve(N*3);
    float spd_min=1e9f, spd_max=-1e9f;
    for(int i=0;i<N;++i){
        float spd=0.f;
        if(i+1<N){
            float dx=(float)(model_traj[i+1].x-model_traj[i].x);
            float dy=(float)(model_traj[i+1].y-model_traj[i].y);
            float dtau=(float)(model_traj[i+1].t-model_traj[i].t);
            spd=(dtau>0.f)?std::sqrt(dx*dx+dy*dy)/dtau:0.f;
        } else spd=line_verts.empty()?0.f:line_verts.back();
        spd_min=std::min(spd_min,spd); spd_max=std::max(spd_max,spd);
        line_verts.push_back((float)model_traj[i].x);
        line_verts.push_back((float)model_traj[i].y);
        line_verts.push_back(spd);
    }

    std::vector<float> obs_verts;
    for(auto& pt:observations){obs_verts.push_back((float)pt.x);obs_verts.push_back((float)pt.y);}

    // Marker vertices : y_max point (shape=0) + impact point (shape=1)
    std::vector<float> marker_verts = {
        (float)model_traj[0].x, (float)y_max,   0.0f,   // y_max  marker (cross)
        (float)x_impact,        0.0f,            1.0f    // impact marker (triangle)
    };
    // find x at y_max
    for(auto& pt:model_traj)
        if(std::abs(pt.y-y_max)<1.0){ marker_verts[0]=(float)pt.x; break; }

    // ── 5. Bounding box + projection ──────────────────────────────────────
    float xMin=1e9f,xMax=-1e9f,yMinF=1e9f,yMaxF=-1e9f;
    for(int i=0;i<N*3;i+=3){
        xMin=std::min(xMin,line_verts[i]);   xMax=std::max(xMax,line_verts[i]);
        yMinF=std::min(yMinF,line_verts[i+1]); yMaxF=std::max(yMaxF,line_verts[i+1]);
    }
    yMinF=std::min(yMinF,0.f);
    const float mx=0.06f*(xMax-xMin), my=0.08f*(yMaxF-yMinF+1.f);
    const float L=xMin-mx, R=xMax+mx, B=yMinF-my, T=yMaxF+my;
    glm::mat4 projection=glm::ortho(L,R,B,T);

    float worldSpan=xMax-xMin;
    float gridStep=(worldSpan>2000.f)?200.f:(worldSpan>1000.f)?100.f:50.f;

    // ── 6. GLFW + GLAD ────────────────────────────────────────────────────
    if(!glfwInit()){std::cerr<<"GLFW fail\n";return -1;}
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES,4);

    std::string title = "Projectile – Dataset "+std::to_string(dataset_num)+" – Numerical Methods (IPSA)";
    GLFWwindow* window=glfwCreateWindow(WIN_W,WIN_H,title.c_str(),nullptr,nullptr);
    if(!window){glfwTerminate();return -1;}
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){std::cerr<<"GLAD fail\n";return -1;}

    glViewport(0,0,WIN_W,WIN_H);
    glClearColor(0.04f,0.04f,0.10f,1.0f);
    glEnable(GL_MULTISAMPLE);

    // ── 7. Renderers ──────────────────────────────────────────────────────
    Renderer rGrid  (DrawMode::LINES,      "shaders/grid.vert",   "shaders/grid.frag");
    Renderer rLine  (DrawMode::LINE_STRIP, "shaders/line.vert",   "shaders/line.frag");
    Renderer rObs   (DrawMode::POINTS,     "shaders/point.vert",  "shaders/point.frag");
    Renderer rComet (DrawMode::COMET,      "shaders/comet.vert",  "shaders/comet.frag");
    Renderer rMarker(DrawMode::COMET,      "shaders/marker.vert", "shaders/marker.frag");

    rGrid.uploadData(buildGrid(L,R,B,T,gridStep));
    rLine.uploadData(line_verts);
    rObs.uploadData(obs_verts);
    rMarker.uploadData(marker_verts);

    TextRenderer text(WIN_W,WIN_H,"shaders/text.vert","shaders/text.frag");

    // ── 8. Animation ──────────────────────────────────────────────────────
    static const int TRAIL_LEN=40;
    double simTime=0.0, simSpeed=1.0, wallPrev=glfwGetTime();

    glfwSetWindowUserPointer(window,&simSpeed);
    glfwSetKeyCallback(window,[](GLFWwindow* w,int key,int,int action,int){
        if(action!=GLFW_PRESS&&action!=GLFW_REPEAT)return;
        double& spd=*static_cast<double*>(glfwGetWindowUserPointer(w));
        if(key==GLFW_KEY_EQUAL||key==GLFW_KEY_KP_ADD)    spd=std::min(spd*2.0,32.0);
        if(key==GLFW_KEY_MINUS||key==GLFW_KEY_KP_SUBTRACT)spd=std::max(spd*0.5,0.125);
        if(key==GLFW_KEY_SPACE) spd=1.0;
        if(key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(w,GLFW_TRUE);
    });

    // ── Pre-build overlay strings ──────────────────────────────────────────
    std::string sTitle = "Dataset "+std::to_string(dataset_num)
                         +" – quadratic drag";
    // One line per free parameter
    std::vector<std::string> sParams;
    for(int j=0;j<(int)free.size();++j){
        std::string line2 = Physics::param_name(free[j])
            +" = "+fmt(Physics::get_param(params,free[j]),6)
            +" +/- "+fmt(sigmas[j],3);
        sParams.push_back(line2);
    }
    std::string sImpact = "x_land = "+fmt(x_impact,1)+" m  t="+fmt(t_impact,2)+" s";
    std::string sYmax   = "y_max  = "+fmt(y_max,1)   +" m  t="+fmt(t_ymax,2)  +" s";
    std::string sVmin   = "v_min  = "+fmt(v_min,2)   +" m/s t="+fmt(t_vmin,2) +" s";
    std::string sCtrl   = "[+/-] speed  [Space] x1  [Esc] quit";

    // ── 9. Main loop ──────────────────────────────────────────────────────
    while(!glfwWindowShouldClose(window))
    {
        double wallNow=glfwGetTime();
        simTime+=(wallNow-wallPrev)*simSpeed;
        wallPrev=wallNow;
        if(simTime>t_impact) simTime=0.0;

        // Comet
        int lo=0,hi=N-1;
        while(lo<hi){int mid=(lo+hi)/2;if(model_traj[mid].t<simTime)lo=mid+1;else hi=mid;}
        int headIdx=std::min(lo,N-1);
        std::vector<float> cometV;
        for(int k=0;k<TRAIL_LEN;++k){
            int idx=headIdx-(TRAIL_LEN-1-k);
            if(idx<0)continue;
            float alpha=(float)(k+1)/TRAIL_LEN;
            cometV.push_back((float)model_traj[idx].x);
            cometV.push_back((float)model_traj[idx].y);
            cometV.push_back(alpha);
        }
        rComet.uploadData(cometV);

        glClear(GL_COLOR_BUFFER_BIT);
        rGrid.render(projection);
        rLine.render(projection,spd_min,spd_max);
        rObs.render(projection);
        rMarker.render(projection);
        rComet.render(projection);

        // Text
        const float S=2.0f, lh=S*8.0f*1.5f;
        float tx=20.f, ty=20.f;
        auto drawS=[&](const std::string& s,float x,float y,glm::vec3 col){
            text.draw(s,x+1,y+1,S,{0,0,0});
            text.draw(s,x,  y,  S,col);
        };

        drawS(sTitle, tx,ty,{1.f,0.82f,0.18f}); ty+=lh*1.6f;

        for(auto& sp:sParams){
            drawS(sp,tx,ty,{0.75f,0.95f,1.f}); ty+=lh;
        }
        ty+=lh*0.5f;
        drawS(sImpact,tx,ty,{1.f,0.5f,0.5f});   ty+=lh;
        drawS(sYmax,  tx,ty,{0.5f,1.f,0.5f});    ty+=lh;
        drawS(sVmin,  tx,ty,{0.75f,0.95f,1.f});  ty+=lh*1.5f;

        std::string sTime="t_sim = "+fmt(simTime,2)+" s (x"+fmt(simSpeed,1)+")";
        drawS(sTime,tx,ty,{0.6f,1.f,0.6f}); ty+=lh*1.5f;
        drawS(sCtrl,tx,ty,{0.5f,0.5f,0.55f});

        // Legend
        float lx=WIN_W-180.f, ly=WIN_H-60.f;
        drawS("high speed",lx,ly,  {1.f,0.4f,0.f});  ly+=lh;
        drawS("low speed", lx,ly,  {0.1f,0.1f,1.f});

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}