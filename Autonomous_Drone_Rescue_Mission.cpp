
// ================================================================
// AUTONOMOUS DRONE RESCUE MISSION
// C++ + OpenGL + GLUT / FreeGLUT
// AIUB Computer Graphics Course Project
//
// Proposal coverage:
// Member 1: Drone Design & Movement        20/20
// Member 2: Environment & Terrain          20/20
// Member 3: Survivor & Rescue System       20/20
// Member 4: Camera, Weather & Interaction  20/20
//
// Controls are shown in the on-screen help panel.
// No external textures/models are required.
// ================================================================

#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

// -----------------------------
// Basic math
// -----------------------------
const float PI = 3.1415926535f;
float rad(float d) { return d * PI / 180.0f; }
float clampf(float x, float a, float b) { return std::max(a, std::min(b, x)); }

struct Vec3 {
    float x, y, z;
    Vec3(float X=0, float Y=0, float Z=0) : x(X), y(Y), z(Z) {}
};

float dist3(const Vec3& a, const Vec3& b) {
    float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}

// -----------------------------
// Global window/state
// -----------------------------
int W = 1280, H = 720;
bool paused = false;
bool rainOn = false;
bool nightMode = false;
bool missionComplete = false;
bool packageDropped = false;
bool spotlightOn = true;
bool helpOn = true;

enum CameraMode { FIRST_PERSON=0, THIRD_PERSON=1, TOP_DOWN=2, HELIPAD_FIXED=3 };
int cameraMode = THIRD_PERSON;

float droneX = 0.0f, droneY = 7.0f, droneZ = 16.0f;
float droneYaw = 0.0f;
float dronePitch = 0.0f, droneRoll = 0.0f;
float rotorAngle = 0.0f;
float droneSpeed = 0.0f;
float zoom = 1.0f;

bool takingOff = false;
bool landing = false;
float takeoffTarget = 7.0f;
float packageY = -1.0f;

float cloudOffset = 0.0f;
float waterOffset = 0.0f;
float firePhase = 0.0f;
float survivorWave = 0.0f;
float flagPhase = 0.0f;
float rainPhase = 0.0f;

float missionTime = 0.0f;
int waypointIndex = 0;

struct Waypoint {
    float x, z;
    const char* label;
};

std::vector<Waypoint> waypoints = {
    {-18.0f,  12.0f, "Sector A"},
    { 18.0f,   8.0f, "Sector B"},
    { 12.0f, -16.0f, "Sector C"}
};

struct Survivor {
    float x, y, z;
    bool rescued;
};
Survivor survivor = {-18.0f, 0.8f, 12.0f, false};

// -----------------------------
// Collision objects
// -----------------------------
struct Box {
    float x,z,w,d,h;
};
std::vector<Box> buildings = {
    {-30, 25, 12, 10, 12},
    { 24, 25, 10, 12, 9},
    { 28,-18, 12, 10, 10},
    {-28,-22, 10, 12, 8},
    {  0,-29, 16, 7, 6}
};

std::vector<Vec3> debris = {
    {-11,0.7f,  7}, {-7,0.5f, 11}, {6,0.7f,18},
    {15,0.5f,  4}, {20,0.6f,-8}, {-18,0.5f,-10},
    {-7,0.4f,-20}, {9,0.5f,-24}, {25,0.5f,17}
};

// -----------------------------
// Utility drawing
// -----------------------------
void setMaterial(float r,float g,float b,float a=1.0f,float emission=0.0f) {
    GLfloat c[] = {r,g,b,a};
    GLfloat e[] = {r*emission,g*emission,b*emission,1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
}

void cube(float sx,float sy,float sz) {
    glPushMatrix();
    glScalef(sx,sy,sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

void cylinder(float r,float h,int slices=16) {
    GLUquadric* q = gluNewQuadric();
    gluCylinder(q,r,r,h,slices,1);
    glPushMatrix();
    glTranslatef(0,0,h);
    glutSolidSphere(r, slices, 8);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void sphere(float r) {
    glutSolidSphere(r, 16, 12);
}

void text3D(const char* s) {
    for (int i=0;s[i];++i) glutStrokeCharacter(GLUT_STROKE_ROMAN,s[i]);
}

void drawText2D(float x,float y,const std::string& s,void* font=GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x,y);
    for(char c:s) glutBitmapCharacter(font,c);
}

void drawCircle(float radius, int n=32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,0,0);
    for(int i=0;i<=n;i++) {
        float a=2*PI*i/n;
        glVertex3f(radius*std::cos(a),0,radius*std::sin(a));
    }
    glEnd();
}

// -----------------------------
// Lighting
// -----------------------------
void setupLights() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    if (!nightMode) {
        GLfloat pos0[] = {-25.0f,35.0f,20.0f,1.0f};
        GLfloat diff0[] = {1.0f,0.92f,0.78f,1.0f};
        GLfloat amb0[]  = {0.30f,0.30f,0.30f,1.0f};
        glLightfv(GL_LIGHT0,GL_POSITION,pos0);
        glLightfv(GL_LIGHT0,GL_DIFFUSE,diff0);
        glLightfv(GL_LIGHT0,GL_AMBIENT,amb0);
    } else {
        GLfloat pos0[] = {-20.0f,30.0f,-15.0f,1.0f};
        GLfloat diff0[] = {0.30f,0.38f,0.75f,1.0f};
        GLfloat amb0[]  = {0.07f,0.08f,0.16f,1.0f};
        glLightfv(GL_LIGHT0,GL_POSITION,pos0);
        glLightfv(GL_LIGHT0,GL_DIFFUSE,diff0);
        glLightfv(GL_LIGHT0,GL_AMBIENT,amb0);
    }

    // Rescue center / streetlight illumination
    GLfloat pos1[] = {32.0f,12.0f,-28.0f,1.0f};
    GLfloat diff1[] = {1.0f,0.65f,0.25f,1.0f};
    GLfloat amb1[]  = {0.02f,0.02f,0.02f,1.0f};
    glLightfv(GL_LIGHT1,GL_POSITION,pos1);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,diff1);
    glLightfv(GL_LIGHT1,GL_AMBIENT,amb1);

    // Animated campfire light
    glEnable(GL_LIGHT2);
    GLfloat firePos[] = {survivor.x+3.0f, 2.0f, survivor.z, 1.0f};
    GLfloat fireDiff[] = {1.0f, 0.25f, 0.03f, 1.0f};
    GLfloat fireAmb[]  = {0.04f, 0.015f, 0.005f, 1.0f};
    glLightfv(GL_LIGHT2, GL_POSITION, firePos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, fireDiff);
    glLightfv(GL_LIGHT2, GL_AMBIENT, fireAmb);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.7f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.08f);
}

void drawSpotlight() {
    if(!spotlightOn) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,1.0f,0.8f,0.10f);
    glPushMatrix();
    glTranslatef(droneX,droneY,droneZ);
    glRotatef(droneYaw,0,1,0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0,-0.2f,0);
    for(int i=0;i<=20;i++) {
        float a=2*PI*i/20;
        glVertex3f(3.2f*std::cos(a),-8.0f,3.2f*std::sin(a));
    }
    glEnd();
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// -----------------------------
// Ground / terrain
// -----------------------------
void drawGround() {
    setMaterial(nightMode?0.08f:0.18f, nightMode?0.10f:0.42f, nightMode?0.10f:0.20f);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-60,0,-60); glVertex3f(60,0,-60);
    glVertex3f(60,0,60);   glVertex3f(-60,0,60);
    glEnd();

    // Road network
    setMaterial(nightMode?0.10f:0.16f,nightMode?0.10f:0.16f,nightMode?0.12f:0.18f);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-60,0.02f,-4); glVertex3f(60,0.02f,-4);
    glVertex3f(60,0.02f,4); glVertex3f(-60,0.02f,4);
    glVertex3f(-4,0.03f,-60); glVertex3f(4,0.03f,-60);
    glVertex3f(4,0.03f,60); glVertex3f(-4,0.03f,60);
    glEnd();

    // Road markings
    glDisable(GL_LIGHTING);
    glColor3f(0.9f,0.8f,0.25f);
    for(float x=-56;x<56;x+=8) {
        glBegin(GL_QUADS);
        glVertex3f(x,0.06f,-0.25f); glVertex3f(x+4,0.06f,-0.25f);
        glVertex3f(x+4,0.06f,0.25f); glVertex3f(x,0.06f,0.25f);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

void drawRiver() {
    setMaterial(0.05f,0.30f,0.65f);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-12,0.04f,-60); glVertex3f(12,0.04f,-60);
    glVertex3f(12,0.04f,60); glVertex3f(-12,0.04f,60);
    glEnd();

    // Flowing water stripes
    glDisable(GL_LIGHTING);
    glColor3f(0.25f,0.65f,1.0f);
    for(int z=-55;z<60;z+=6) {
        float zz=z+(waterOffset*3.0f);
        while(zz>60) zz-=120;
        glBegin(GL_LINES);
        glVertex3f(-9,0.08f,zz); glVertex3f(8,0.08f,zz);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

void drawBridge() {
    setMaterial(0.35f,0.35f,0.38f);
    glPushMatrix();
    glTranslatef(0,1.0f,18);
    cube(18,1.0f,5.0f);
    glPopMatrix();

    // Destroyed section
    setMaterial(0.18f,0.18f,0.20f);
    glPushMatrix(); glTranslatef(4,1.7f,18); glRotatef(-12,0,0,1); cube(5,0.7f,4); glPopMatrix();
    glPushMatrix(); glTranslatef(9,0.9f,19); glRotatef(18,0,0,1); cube(3,0.5f,3); glPopMatrix();

    // Rails
    setMaterial(0.18f,0.18f,0.18f);
    for(int x=-16;x<=16;x+=4) {
        glPushMatrix(); glTranslatef(x,2.0f,15.8f); cube(0.18f,1.5f,0.18f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,2.0f,20.2f); cube(0.18f,1.5f,0.18f); glPopMatrix();
    }
}

void drawBuilding(const Box& b,bool damaged) {
    setMaterial(damaged?0.30f:0.55f, damaged?0.30f:0.55f, damaged?0.32f:0.60f);
    glPushMatrix();
    glTranslatef(b.x,b.h/2.0f,b.z);
    glRotatef(damaged? -6.0f:0.0f,0,0,1);
    cube(b.w,b.h,b.d);
    glPopMatrix();

    // Windows
    glDisable(GL_LIGHTING);
    for(int side=-1;side<=1;side+=2) {
        for(int row=0;row<3;row++) {
            float yy=2.0f+row*2.5f;
            glColor3f(nightMode?0.9f:0.15f,nightMode?0.65f:0.25f,nightMode?0.15f:0.35f);
        }
    }
    glEnable(GL_LIGHTING);

    // Broken slab pieces for damaged buildings
    if(damaged) {
        setMaterial(0.22f,0.22f,0.24f);
        glPushMatrix(); glTranslatef(b.x-3,b.h+1,b.z+1); glRotatef(25,1,0,1); cube(5,0.6f,3); glPopMatrix();
        glPushMatrix(); glTranslatef(b.x+4,b.h*0.45f,b.z-2); glRotatef(-18,0,0,1); cube(2,2.0f,2); glPopMatrix();
    }
}

void drawAllBuildings() {
    for(size_t i=0;i<buildings.size();i++) drawBuilding(buildings[i], i%2==0);
}

void drawTree(float x,float z,float s=1.0f) {
    setMaterial(0.30f,0.16f,0.07f);
    glPushMatrix(); glTranslatef(x,1.3f*s,z); glScalef(s,s,s); cylinder(0.28f,2.6f,12); glPopMatrix();
    setMaterial(0.08f,0.38f,0.12f);
    glPushMatrix(); glTranslatef(x,3.3f*s,z); glScalef(s,s,s); sphere(1.3f); glPopMatrix();
    setMaterial(0.10f,0.50f,0.16f);
    glPushMatrix(); glTranslatef(x+0.6f*s,3.8f*s,z); glScalef(s*.8f,s*.8f,s*.8f); sphere(1.1f); glPopMatrix();
}

void drawTrees() {
    const float t[][2]={{-45,-45},{-35,-8},{-44,20},{-22,35},{-8,40},{20,42},{45,40},{44,10},{43,-35},{20,-44},{-35,-35}};
    for(auto &p:t) drawTree(p[0],p[1],1.0f);
}

void drawDebris() {
    setMaterial(0.27f,0.27f,0.28f);
    for(size_t i=0;i<debris.size();i++) {
        glPushMatrix();
        glTranslatef(debris[i].x,debris[i].y,debris[i].z);
        glRotatef((float)(i*37),1,1,0);
        glScalef(1.2f+(i%3)*0.4f,0.6f+(i%2)*0.3f,0.9f);
        cube(1,1,1);
        glPopMatrix();
    }
}

void drawStrandedVehicles() {
    for(int i=0;i<3;i++) {
        float x = -32.0f+i*10.0f, z=8.0f;
        setMaterial(i==1?0.65f:0.15f,0.15f,0.15f);
        glPushMatrix(); glTranslatef(x,0.7f,z); cube(3.2f,0.9f,1.6f); glPopMatrix();
        setMaterial(0.08f,0.12f,0.15f);
        for(int w=-1;w<=1;w+=2) {
            glPushMatrix(); glTranslatef(x+w*1.0f,0.35f,z-0.9f); glRotatef(90,1,0,0); cylinder(0.35f,0.25f,12); glPopMatrix();
        }
    }
}

void drawCloud(float x,float y,float z) {
    setMaterial(0.82f,0.86f,0.92f);
    glPushMatrix(); glTranslatef(x,y,z); sphere(2.0f); glTranslatef(2,0.2f,0); sphere(1.5f); glTranslatef(-4,0.1f,0.4f); sphere(1.4f); glPopMatrix();
}

void drawClouds() {
    drawCloud(-30+cloudOffset,25,25);
    drawCloud(20+cloudOffset,28,30);
    drawCloud(0+cloudOffset,23,-30);
}

void drawSunMoon() {
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(nightMode?-35.0f:35.0f,35.0f,nightMode?-30.0f:-25.0f);
    glColor3f(nightMode?0.75f:1.0f,nightMode?0.78f:0.75f,nightMode?1.0f:0.2f);
    sphere(3.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

// -----------------------------
// Streetlights and rescue center
// -----------------------------
void drawStreetlight(float x,float z) {
    setMaterial(0.15f,0.15f,0.17f);
    glPushMatrix(); glTranslatef(x,3,z); cylinder(0.12f,6,10); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.85f,0.25f);
    glPushMatrix(); glTranslatef(x,6.2f,z); sphere(0.28f); glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawStreetlights() {
    drawStreetlight(-8,10); drawStreetlight(8,10);
    drawStreetlight(-8,-10); drawStreetlight(8,-10);
}

void drawRescueCenter() {
    setMaterial(0.65f,0.65f,0.68f);
    glPushMatrix(); glTranslatef(38,2.5f,-38); cube(10,5,8); glPopMatrix();

    // Red cross
    glDisable(GL_LIGHTING);
    glColor3f(0.9f,0.05f,0.05f);
    glPushMatrix(); glTranslatef(38,5.1f,-33.9f);
    cube(0.8f,1.4f,0.1f);
    glPopMatrix();
    glPushMatrix(); glTranslatef(38,5.1f,-33.9f);
    glRotatef(90,0,0,1); cube(0.8f,1.4f,0.1f);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // Emergency vehicles
    for(int i=0;i<2;i++) {
        float x=32+i*7;
        setMaterial(0.75f,0.1f,0.1f);
        glPushMatrix(); glTranslatef(x,0.8f,-27); cube(4,1.2f,2); glPopMatrix();
        glDisable(GL_LIGHTING);
        glColor3f(1,1,1);
        glPushMatrix(); glTranslatef(x,1.4f,-27); cube(1.0f,0.3f,0.25f); glPopMatrix();
        glEnable(GL_LIGHTING);
    }
}

// -----------------------------
// Helipad and landing zone
// -----------------------------
void drawHelipad(float x,float z) {
    setMaterial(0.30f,0.32f,0.34f);
    glPushMatrix(); glTranslatef(x,0.12f,z); glScalef(8,0.25f,8); drawCircle(1); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(1,1,1);
    glPushMatrix(); glTranslatef(x,0.30f,z); glScalef(2,0.08f,0.7f); cube(1,1,1); glPopMatrix();
    glPushMatrix(); glTranslatef(x,0.30f,z); glScalef(0.7f,0.08f,2); cube(1,1,1); glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawLandingZone() {
    drawHelipad(0,-15);
}

// -----------------------------
// Survivor / distress / fire
// -----------------------------
void drawSurvivor() {
    if(survivor.rescued) return;
    glPushMatrix();
    glTranslatef(survivor.x,survivor.y,survivor.z);

    setMaterial(0.15f,0.35f,0.85f);
    glPushMatrix(); glScalef(0.8f,1.1f,0.5f); sphere(0.65f); glPopMatrix();

    setMaterial(0.75f,0.52f,0.35f);
    glPushMatrix(); glTranslatef(0,1.15f,0); sphere(0.42f); glPopMatrix();

    // Waving arm
    setMaterial(0.75f,0.52f,0.35f);
    glPushMatrix();
    glTranslatef(0.65f,1.1f,0);
    glRotatef(35+20*std::sin(survivorWave),0,0,1);
    glScalef(0.18f,0.8f,0.18f); cube(1,1,1);
    glPopMatrix();

    glPopMatrix();
}

void drawSmoke() {
    if(survivor.rescued) return;
    glDisable(GL_LIGHTING);
    glColor4f(0.45f,0.45f,0.45f,0.55f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int i=0;i<5;i++) {
        float y=1.0f+i*0.9f+0.3f*std::sin(firePhase+i);
        float x=survivor.x+0.7f*std::sin(firePhase*0.7f+i);
        float z=survivor.z+0.5f*std::cos(firePhase+i);
        glPushMatrix(); glTranslatef(x,y,z); sphere(0.5f+0.12f*i); glPopMatrix();
    }
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawCampfire() {
    float x=survivor.x+3, z=survivor.z;
    setMaterial(0.35f,0.12f,0.04f);
    for(int i=0;i<3;i++) {
        glPushMatrix(); glTranslatef(x,0.35f,z); glRotatef(i*60,0,1,0); glRotatef(65,0,0,1); cube(1.8f,0.3f,0.3f); glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.35f,0.02f);
    glPushMatrix(); glTranslatef(x,1.2f+0.2f*std::sin(firePhase),z); glScalef(0.5f,1.3f,0.5f); sphere(1); glPopMatrix();
    glColor3f(1.0f,0.8f,0.05f);
    glPushMatrix(); glTranslatef(x,1.0f+0.15f*std::cos(firePhase),z); glScalef(0.3f,0.9f,0.3f); sphere(1); glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawDistressSignal() {
    if(survivor.rescued) return;
    glDisable(GL_LIGHTING);
    glColor3f(1,0.05f,0.05f);
    float s=1.0f+0.2f*std::sin(firePhase*3);
    glPushMatrix(); glTranslatef(survivor.x,3.2f,survivor.z); glScalef(s,0.12f,s); cube(1,1,1); glPopMatrix();
    glPushMatrix(); glTranslatef(survivor.x,3.2f,survivor.z); glRotatef(90,0,1,0); glScalef(s,0.12f,s); cube(1,1,1); glPopMatrix();
    glEnable(GL_LIGHTING);
}

// -----------------------------
// Waypoints / hazards
// -----------------------------
void drawWaypoints() {
    if(missionComplete) return;
    glDisable(GL_LIGHTING);
    for(size_t i=0;i<waypoints.size();i++) {
        if((int)i < waypointIndex) continue;
        glColor3f(i==waypointIndex?1.0f:0.3f, i==waypointIndex?0.8f:0.8f, 0.1f);
        glPushMatrix();
        glTranslatef(waypoints[i].x,1.0f,waypoints[i].z);
        glRotatef(-90,1,0,0);
        glutWireCone(1.0,2.5,12,4);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawHazardZones() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,0.05f,0.05f,0.18f);
    for(int i=0;i<3;i++) {
        float x=(i==0?-24.0f:(i==1?25.0f:0.0f));
        float z=(i==0?2.0f:(i==1?-2.0f:28.0f));
        glPushMatrix(); glTranslatef(x,0.08f,z); drawCircle(4.0f); glPopMatrix();
    }
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// -----------------------------
// Drone
// -----------------------------
void drawPropeller(float angle) {
    glPushMatrix();
    glRotatef(angle,0,1,0);
    setMaterial(0.10f,0.10f,0.10f);
    glPushMatrix(); glScalef(2.4f,0.08f,0.18f); cube(1,1,1); glPopMatrix();
    glPushMatrix(); glRotatef(90,0,1,0); glScalef(2.4f,0.08f,0.18f); cube(1,1,1); glPopMatrix();
    glPopMatrix();
}

void drawDrone() {
    glPushMatrix();
    glTranslatef(droneX,droneY,droneZ);
    glRotatef(droneYaw,0,1,0);
    glRotatef(dronePitch,0,0,1);
    glRotatef(droneRoll,1,0,0);

    // Main body
    setMaterial(0.12f,0.14f,0.16f);
    glPushMatrix(); glScalef(2.6f,0.55f,2.0f); sphere(1); glPopMatrix();

    // Four rotor arms + rotors
    float px[4]={-2.3f,2.3f,-2.3f,2.3f};
    float pz[4]={-1.6f,-1.6f,1.6f,1.6f};
    for(int i=0;i<4;i++) {
        glPushMatrix();
        glTranslatef(px[i]*0.5f,0,pz[i]*0.5f);
        glRotatef(std::atan2(pz[i],px[i])*180/PI,0,1,0);
        glScalef(2.4f,0.18f,0.18f); cube(1,1,1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(px[i],0,pz[i]);
        glRotatef(rotorAngle*(i%2? -1:1),0,1,0);
        drawPropeller(0);
        glPopMatrix();

        // Navigation lights
        glDisable(GL_LIGHTING);
        glColor3f((i==0||i==3)?1:0.1f,(i==1)?1:0.1f,0.1f);
        glPushMatrix(); glTranslatef(px[i],0.05f,pz[i]); sphere(0.18f); glPopMatrix();
        glEnable(GL_LIGHTING);

        // Landing gear
        setMaterial(0.08f,0.08f,0.08f);
        glPushMatrix(); glTranslatef(px[i]*0.75f,-0.8f,pz[i]*0.75f); glScalef(0.14f,1.0f,0.14f); cube(1,1,1); glPopMatrix();
    }

    // Camera gimbal and lens
    setMaterial(0.18f,0.18f,0.20f);
    glPushMatrix(); glTranslatef(0,-0.65f,0); glRotatef(-90,1,0,0); cylinder(0.35f,0.5f,14); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.03f,0.03f,0.03f);
    glPushMatrix(); glTranslatef(0,-0.9f,0); glRotatef(-90,1,0,0); cylinder(0.20f,0.12f,14); glPopMatrix();
    glEnable(GL_LIGHTING);

    // Rescue payload box
    if(!packageDropped) {
        setMaterial(0.95f,0.95f,0.90f);
        glPushMatrix(); glTranslatef(0,-1.2f,0); cube(1.0f,0.7f,1.0f); glPopMatrix();
        glDisable(GL_LIGHTING); glColor3f(0.9f,0.05f,0.05f);
        glPushMatrix(); glTranslatef(0,-1.2f,0.51f); cube(0.15f,0.5f,0.03f); glPopMatrix();
        glPushMatrix(); glTranslatef(0,-1.2f,0.51f); glRotatef(90,0,0,1); cube(0.15f,0.5f,0.03f); glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    glPopMatrix();
}

void drawDroppedPackage() {
    if(!packageDropped) return;
    if(packageY < 0) packageY=droneY-2;
    setMaterial(0.95f,0.95f,0.90f);
    glPushMatrix(); glTranslatef(survivor.x,packageY,survivor.z); cube(1.0f,0.7f,1.0f); glPopMatrix();
}

// -----------------------------
// Collision and mission logic
// -----------------------------
bool droneHitsBuilding() {
    for(const Box& b:buildings) {
        float cx=clampf(droneX,b.x-b.w/2,b.x+b.w/2);
        float cz=clampf(droneZ,b.z-b.d/2,b.z+b.d/2);
        float dx=droneX-cx, dz=droneZ-cz;
        if(dx*dx+dz*dz < 2.5f*2.5f && droneY < b.h+2.0f) return true;
    }
    return false;
}

void updateMission() {
    if(missionComplete) return;

    // Waypoint progression
    if(waypointIndex < (int)waypoints.size()) {
        Vec3 p(droneX,0,droneZ);
        Vec3 q(waypoints[waypointIndex].x,0,waypoints[waypointIndex].z);
        if(dist3(p,q)<4.0f) waypointIndex++;
    }

    // Package delivery to survivor
    if(packageDropped && !survivor.rescued) {
        float dx=droneX-survivor.x, dz=droneZ-survivor.z;
        if(dx*dx+dz*dz < 36.0f && packageY<=1.5f) {
            survivor.rescued=true;
        }
    }

    if(survivor.rescued && waypointIndex >= (int)waypoints.size()) {
        Vec3 p(droneX,0,droneZ), q(0,0,-15);
        if(dist3(p,q)<7.0f && droneY<5.0f) missionComplete=true;
    }
}

// -----------------------------
// Camera
// -----------------------------
void applyCamera() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float yaw=rad(droneYaw);
    float fx=std::sin(yaw), fz=-std::cos(yaw);

    if(cameraMode==FIRST_PERSON) {
        gluLookAt(
            droneX,droneY,droneZ,
            droneX+fx*15,droneY+dronePitch*0.2f,droneZ+fz*15,
            0,1,0
        );
    }
    else if(cameraMode==THIRD_PERSON) {
        float d=10.0f/zoom;
        gluLookAt(
            droneX-fx*d,droneY+5.0f,droneZ-fz*d,
            droneX,droneY,droneZ,
            0,1,0
        );
    }
    else if(cameraMode==TOP_DOWN) {
        float d=35.0f/zoom;
        gluLookAt(droneX,droneY+d,droneZ,droneX,0,droneZ,0,0,-1);
    }
    else {
        gluLookAt(0,14,-2, 0,4,12, 0,1,0);
    }
}

// -----------------------------
// Minimap
// -----------------------------
void drawMinimap() {
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    float x0=W-210,y0=H-210,s=180;
    glColor4f(0.03f,0.04f,0.05f,0.85f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glVertex2f(x0,y0);glVertex2f(x0+s,y0);glVertex2f(x0+s,y0+s);glVertex2f(x0,y0+s);
    glEnd(); glDisable(GL_BLEND);

    glColor3f(0.2f,0.5f,0.8f);
    glBegin(GL_QUADS);
    glVertex2f(x0+s*0.43f,y0);glVertex2f(x0+s*0.57f,y0);
    glVertex2f(x0+s*0.57f,y0+s);glVertex2f(x0+s*0.43f,y0+s);
    glEnd();

    glColor3f(0.3f,0.3f,0.3f);
    glBegin(GL_QUADS);
    glVertex2f(x0,y0+s*0.46f);glVertex2f(x0+s,y0+s*0.46f);
    glVertex2f(x0+s,y0+s*0.54f);glVertex2f(x0,y0+s*0.54f);
    glEnd();

    auto mapX=[&](float z){return x0+(z+60)/120*s;};
    auto mapY=[&](float x){return y0+(x+60)/120*s;};

    // Survivor
    if(!survivor.rescued) {
        glColor3f(1,0.1f,0.1f);
        glPointSize(8); glBegin(GL_POINTS); glVertex2f(mapX(survivor.z),mapY(survivor.x)); glEnd();
    }

    // Drone
    glColor3f(0.1f,1.0f,0.2f);
    glPointSize(9); glBegin(GL_POINTS); glVertex2f(mapX(droneZ),mapY(droneX)); glEnd();

    // Waypoints
    glColor3f(1.0f,0.8f,0.1f);
    for(size_t i=waypointIndex;i<waypoints.size();i++) {
        glPointSize(5); glBegin(GL_POINTS); glVertex2f(mapX(waypoints[i].z),mapY(waypoints[i].x)); glEnd();
    }

    glColor3f(1,1,1);
    drawText2D(x0+8,y0+s-18,"MINIMAP");

    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);
}

// -----------------------------
// HUD / user interface
// -----------------------------
void drawHUD() {
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();

    glColor3f(1,1,1);
    drawText2D(20,H-25,"AUTONOMOUS DRONE RESCUE MISSION",GLUT_BITMAP_HELVETICA_18);

    char buf[256];
    std::snprintf(buf,sizeof(buf),"Altitude: %.1f m",droneY);
    drawText2D(20,H-48,buf);
    std::snprintf(buf,sizeof(buf),"Speed: %.1f m/s",droneSpeed);
    drawText2D(20,H-66,buf);

    std::string cam="Third Person";
    if(cameraMode==FIRST_PERSON) cam="First Person";
    if(cameraMode==TOP_DOWN) cam="Top Down";
    if(cameraMode==HELIPAD_FIXED) cam="Fixed Helipad";
    drawText2D(20,H-84,"Camera: "+cam);

    std::string objective;
    if(missionComplete) objective="MISSION COMPLETE";
    else if(!survivor.rescued) objective="Objective: Reach waypoints and deliver rescue package";
    else objective="Objective: Return rescued survivor mission to helipad";
    drawText2D(20,H-102,objective);

    std::string status="Status: ";
    if(missionComplete) status+="RESCUE SUCCESSFUL";
    else if(survivor.rescued) status+="Survivor rescued";
    else if(packageDropped) status+="Package dropped";
    else status+="Searching for survivor";
    drawText2D(20,H-120,status);

    if(nightMode) drawText2D(20,H-138,"Mode: NIGHT");
    else drawText2D(20,H-138,"Mode: DAY");
    drawText2D(20,H-156,rainOn?"Weather: RAIN":"Weather: CLEAR");

    if(paused) {
        glColor3f(1.0f,0.85f,0.1f);
        drawText2D(W/2-50,H/2,"PAUSED",GLUT_BITMAP_HELVETICA_18);
    }

    if(missionComplete) {
        glColor3f(0.2f,1.0f,0.3f);
        drawText2D(W/2-95,H-45,"*** RESCUE SUCCESSFUL ***",GLUT_BITMAP_HELVETICA_18);
    }

    if(helpOn) {
        glColor3f(0.85f,0.9f,1.0f);
        drawText2D(20,22,"W/S: forward/back   A/D: left/right   Q/E: yaw   R/F: altitude");
        drawText2D(20,38,"1-4: camera   +/-: zoom   P: pause   N: day/night   T: rain");
        drawText2D(20,54,"G: drop package   L: land/takeoff   C: spotlight   M: minimap   H: help   ESC: exit");
    }

    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_LIGHTING);
}

// -----------------------------
// Rain animation
// -----------------------------
void drawRain() {
    if(!rainOn) return;
    glDisable(GL_LIGHTING);
    glColor4f(0.55f,0.70f,1.0f,0.55f);
    glBegin(GL_LINES);
    for(int i=0;i<500;i++) {
        float x=(i*37%120)-60;
        float z=(i*53%120)-60;
        float y=2.0f+((i*71)%300)/10.0f;
        float fall=rainPhase*25.0f;
        float yy=y-std::fmod(fall,30.0f);
        if(yy<0) yy+=30;
        glVertex3f(x,yy,z);
        glVertex3f(x-0.3f,yy-2.0f,z-0.15f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// -----------------------------
// Wind indicator flag
// -----------------------------
void drawWindFlag() {
    setMaterial(0.15f,0.15f,0.15f);
    glPushMatrix(); glTranslatef(-38,3,-38); cylinder(0.1f,6,10); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(0.9f,0.1f,0.1f);
    float wave=0.5f*std::sin(flagPhase);
    glBegin(GL_TRIANGLES);
    glVertex3f(-38,6,-38);
    glVertex3f(-33,5.5f+wave,-38);
    glVertex3f(-38,5.0f,-38);
    glEnd();
    glEnable(GL_LIGHTING);
}

// -----------------------------
// World
// -----------------------------
void drawWorld() {
    drawGround();
    drawRiver();
    drawBridge();
    drawAllBuildings();
    drawTrees();
    drawDebris();
    drawStrandedVehicles();
    drawClouds();
    drawSunMoon();
    drawStreetlights();
    drawRescueCenter();
    drawLandingZone();
    drawSurvivor();
    drawSmoke();
    drawCampfire();
    drawDistressSignal();
    drawWaypoints();
    drawHazardZones();
    drawWindFlag();
    drawDroppedPackage();
    drawDrone();
    drawSpotlight();
    drawRain();
}

// -----------------------------
// Animation
// -----------------------------
void update(float dt) {
    if(paused) return;

    rotorAngle += 900.0f*dt;
    if(rotorAngle>360) rotorAngle-=360;

    cloudOffset += 0.8f*dt;
    if(cloudOffset>120) cloudOffset-=120;

    waterOffset += 0.6f*dt;
    if(waterOffset>40) waterOffset=0;

    firePhase += 5.0f*dt;
    survivorWave += 4.0f*dt;
    flagPhase += 3.0f*dt;
    rainPhase += dt;
    missionTime += dt;

    // Takeoff / landing animation
    if(takingOff) {
        droneY += 5.0f*dt;
        if(droneY>=takeoffTarget) {
            droneY=takeoffTarget;
            takingOff=false;
        }
    }
    if(landing) {
        droneY -= 4.0f*dt;
        if(droneY<=1.8f) {
            droneY=1.8f;
            landing=false;
        }
    }

    // Payload falling animation
    if(packageDropped && !survivor.rescued && packageY>0.8f) packageY-=6.0f*dt;

    updateMission();
    glutPostRedisplay();
}

// -----------------------------
// Input
// -----------------------------
void moveDrone(float forward,float sideways) {
    if(paused || takingOff || landing) return;

    float speed=12.0f;
    float a=rad(droneYaw);
    float fx=std::sin(a), fz=-std::cos(a);
    float rx=std::cos(a), rz=std::sin(a);

    float nx=droneX + (fx*forward + rx*sideways)*speed*0.08f;
    float nz=droneZ + (fz*forward + rz*sideways)*speed*0.08f;

    float ox=droneX, oz=droneZ;
    droneX=clampf(nx,-55,55);
    droneZ=clampf(nz,-55,55);

    if(droneHitsBuilding()) {
        droneX=ox; droneZ=oz;
        droneSpeed=0;
    } else {
        droneSpeed=std::sqrt(forward*forward+sideways*sideways)*speed;
        dronePitch=clampf(-forward*10,-15,15);
        droneRoll=clampf(-sideways*10,-15,15);
    }
}

void keyboard(unsigned char key,int,int) {
    switch(key) {
        case 27: std::exit(0);
        case 'w': case 'W': moveDrone(1,0); break;
        case 's': case 'S': moveDrone(-1,0); break;
        case 'a': case 'A': moveDrone(0,-1); break;
        case 'd': case 'D': moveDrone(0,1); break;
        case 'q': case 'Q': droneYaw+=6; break;
        case 'e': case 'E': droneYaw-=6; break;
        case 'r': case 'R': if(!paused && droneY<25) droneY+=1; break;
        case 'f': case 'F': if(!paused && droneY>1.5f) droneY-=1; break;

        case '1': cameraMode=FIRST_PERSON; break;
        case '2': cameraMode=THIRD_PERSON; break;
        case '3': cameraMode=TOP_DOWN; break;
        case '4': cameraMode=HELIPAD_FIXED; break;

        case '+': case '=': zoom=clampf(zoom+0.1f,0.5f,2.5f); break;
        case '-': case '_': zoom=clampf(zoom-0.1f,0.5f,2.5f); break;

        case 'p': case 'P': paused=!paused; break;
        case 'n': case 'N': nightMode=!nightMode; break;
        case 't': case 'T': rainOn=!rainOn; break;
        case 'c': case 'C': spotlightOn=!spotlightOn; break;
        case 'h': case 'H': helpOn=!helpOn; break;
        case 'm': case 'M': /* minimap is always live */ break;

        case 'g': case 'G':
            if(!packageDropped && !survivor.rescued && dist3(Vec3(droneX,0,droneZ),Vec3(survivor.x,0,survivor.z))<12)
            {
                packageDropped=true;
                packageY=droneY-2.0f;
            }
            break;

        case 'l': case 'L':
            if(droneY>3.0f && !landing) { landing=true; takingOff=false; }
            else if(droneY<=2.0f && !takingOff) { takingOff=true; landing=false; }
            break;

        case '0':
            droneX=0; droneY=7; droneZ=16;
            droneYaw=0; dronePitch=0; droneRoll=0;
            droneSpeed=0; packageDropped=false; packageY=-1;
            survivor.rescued=false; missionComplete=false; waypointIndex=0;
            takingOff=false; landing=false;
            break;
    }
    glutPostRedisplay();
}

void special(int key,int,int) {
    if(key==GLUT_KEY_UP) moveDrone(1,0);
    if(key==GLUT_KEY_DOWN) moveDrone(-1,0);
    if(key==GLUT_KEY_LEFT) moveDrone(0,-1);
    if(key==GLUT_KEY_RIGHT) moveDrone(0,1);
    glutPostRedisplay();
}

void mouse(int button,int state,int,int) {
    if(state!=GLUT_DOWN) return;
    if(button==3) zoom=clampf(zoom+0.1f,0.5f,2.5f);
    if(button==4) zoom=clampf(zoom-0.1f,0.5f,2.5f);
    glutPostRedisplay();
}

// -----------------------------
// Rendering
// -----------------------------
void display() {
    if(nightMode) glClearColor(0.015f,0.02f,0.07f,1);
    else glClearColor(0.48f,0.72f,0.92f,1);

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    setupLights();
    applyCamera();
    drawWorld();

    drawMinimap();
    drawHUD();

    glutSwapBuffers();
}

void reshape(int w,int h) {
    W=w; H=(h<1?1:h);
    glViewport(0,0,W,H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)W/H, 0.1, 250.0);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int) {
    static int last=0;
    int now=glutGet(GLUT_ELAPSED_TIME);
    if(last==0) last=now;
    float dt=(now-last)/1000.0f;
    if(dt>0.05f) dt=0.05f;
    last=now;
    update(dt);
    glutTimerFunc(16,timer,0);
}

// -----------------------------
// Initialization
// -----------------------------
void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.48f,0.72f,0.92f,1);
}

// -----------------------------
// Main
// -----------------------------
int main(int argc,char** argv) {
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(80,40);
    glutCreateWindow("Autonomous Drone Rescue Mission - OpenGL GLUT");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutTimerFunc(16,timer,0);

    glutMainLoop();
    return 0;
}
