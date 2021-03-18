#include <stdio.h>
#include <unistd.h>
#include <random>
#include <ctime>
#include <vector>
#include <math.h>
#include <string>

// Class Definitions
class WorldSpace;
class Particle;
class ParticleSystem;

void renderWorldSpace(WorldSpace* ws, int num);
void resetRender(WorldSpace* ws);
void clearRender(int rows);
char getCharacter(int value);

// CLASSES --------------------------------------------------------------------

class WorldSpace {
    private:
        int rows;
        int cols;
        double width;
        double height;

        int** renderData;

        std::vector<ParticleSystem*> particleSystems;

        int buffer;
        int buffer_counter;
        char* charBuffer;
        int charBuffer_index;

        const bool FAKE_MODE = false;
        const char* FAKE_TEXT = "you can make the program print fake text!";
    public:
        WorldSpace(int,int);
        ~WorldSpace();

        // GETTERS
        double getWidth()  {   return width;   }
        double getHeight() {   return height;  }
        int getRows()   {   return rows;    }
        int getCols()   {   return cols;    }
        int getBuffer() {   return buffer;  }
        char* getCharBuffer() { return charBuffer; }

        void addParticleSystem(ParticleSystem*);
        int*** getRenderData();
        void resetRenderData();

        void addToBuffer(int);
        void clearBuffer();
        void processBuffer();
};

class Particle {
    private:
        WorldSpace* ws;
        double x; // pos
        double y;
        double vx; // vel
        double vy;
        double ax; // accel
        double ay;

        int value;
        const double COLLISION_DAMPENING = 0.85;
    public:
        Particle(double, double, int);
        void update();
        void applyForce(double,double);
        void setWorldSpace(WorldSpace* worldspace) {ws = worldspace;};

        double getX() {return x;}
        double getY() {return y;}
        int getValue() {return value;}
};

class ParticleSystem {
    public:
        ParticleSystem();

        std::vector<Particle> *getParticles();
        Particle* getParticleAt(int pos);

        void updateParticles();
        void applyForce(double,double);
        void applyRandomForce(double,double);

        void addParticle(double,double,int);
        void addParticles(int,double,double,int);        

        void setWorldSpace(WorldSpace*);
    private:
        std::vector<Particle> particles;
        WorldSpace* ws;
};

// WORLDSPACE -----------------------------------------------------------------

WorldSpace::WorldSpace(int num_rows, int num_cols) {
    // Constructor
    cols = num_cols;
    width = (double)num_cols;
    rows = num_rows;
    height = (double)num_rows * 2.222;

    // Initalize Render Data Size and State
    renderData = new int*[rows];
    for(int i = 0; i < rows; i++) {
        renderData[i] = new int[cols];
        for(int j = 0; j < cols; j++) {
            renderData[i][j] = 0;
        }
    }

    buffer = 0;
    buffer_counter = 0;

    // allocate char buffer size
    charBuffer = new char[128];
    charBuffer_index = 0;
}

WorldSpace::~WorldSpace() {
    // Destructor
    for(int i = 0; i < rows; i++) {
        delete[] renderData[i];
    }
    delete[] renderData;
}

void WorldSpace::addParticleSystem(ParticleSystem* ps) {
    ps->setWorldSpace(this);
    particleSystems.push_back(ps);
}

int*** WorldSpace::getRenderData() {
    // for each particle, calculate which tile it should appear on
    for(int i = 0; i < particleSystems.size(); i++) {
        ParticleSystem* ps = particleSystems.at(i);
        std::vector<Particle>* pv = ps->getParticles();
        int x, y, v;
        for(std::vector<Particle>::iterator p = (*pv).begin(); p != (*pv).end(); p++) {
            x = (int)p->getX();
            y = (int)(p->getY() / 2.222);
            v = p->getValue();
            if (y > rows-1) y = rows-1;
            if (x > cols-1) x = cols-1;
            renderData[y][x] = v;

        }
    }
    return &renderData;
}

void WorldSpace::resetRenderData() {
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            renderData[i][j] = 0;
        }
    }
}

void WorldSpace::addToBuffer(int input) {
    // shift the buffer left by one
    buffer = buffer << 1;

    // add the last bit of input to the buffer
    buffer = buffer | (input & 0x01);
    //printf("Adding %d to buffer %d\n", (input & 0x01), buffer);

    // increment buffer counter
    buffer_counter++;

    // if buffer counter at max
    // - process buffer
    // - clear buffer
    if(buffer_counter >= 7) {
        processBuffer();
        clearBuffer();
    }
}

void WorldSpace::clearBuffer() {
    buffer = 0b0;
    buffer_counter = 0;
}

void WorldSpace::processBuffer() {
    char out = (char)buffer;
    if(out < 32) out = 32;
    if(charBuffer_index < 127){
        if(FAKE_MODE) {
            charBuffer[charBuffer_index] = FAKE_TEXT[charBuffer_index];
        } else {
            charBuffer[charBuffer_index] = out;
        }
        charBuffer_index += 1;
    }
}

// PARTICLE -------------------------------------------------------------------

Particle::Particle(double start_x, double start_y, int in_value) {
    x = start_x;
    y = start_y;
    value = in_value;
    vx = 0;
    vy = 0;
    ax = 0;
    ay = 0;
}

void Particle::update() {
    // height/width
    double height = (double)ws->getHeight();
    double width = (double)ws->getWidth();

    // x
    vx += ax;
    ax = 0.0;
    if(x + vx > width) {
        x = width + ((width - x) - vx);
        vx = -vx;
        vx *= COLLISION_DAMPENING;
    } else if(x + vx < 0) {
        x = (-x - vx);
        vx = -vx;
        vx *= COLLISION_DAMPENING;
    } else {
        x += vx;
    }
    if (x > width) x = width;
    if (x < 0.0) x = 0.0;

    // y
    vy += ay;
    ay = 0.0;
    if(y + vy > height) {
        y = height + ((height - y) - vy);
        vy = -vy;
        vy *= COLLISION_DAMPENING;
        ws->addToBuffer(value - 1);
    } else if(y + vy < 0) {
        y = (-y - vy);
        vy = -vy;
        vy *= COLLISION_DAMPENING;
    } else {
        y += vy;
    }
    y += vy;
    if (y > height) y = height;
    if (y < 0.0) y = 0.0;
    //printf("%2.2f, %2.2f, v %2.2f, %2.2f\n", x, y, vx, vy);
}

void Particle::applyForce(double fx, double fy) {
    ax += fx;
    ay += fy;
}

// PARTICLESYSTEM -------------------------------------------------------------

ParticleSystem::ParticleSystem() {
    ws = NULL;
}

void ParticleSystem::setWorldSpace(WorldSpace* worldSpace) {
    ws = worldSpace;
}

std::vector<Particle> *ParticleSystem::getParticles() {
    return &particles;
}

Particle* ParticleSystem::getParticleAt(int pos) {
    if(pos >= particles.size() || pos < 0) {
        return NULL;
    } else {
        return &particles.at(pos);
    }
}

void ParticleSystem::updateParticles() {
    for(std::vector<Particle>::iterator i = particles.begin(); i != particles.end(); i++) {
        i->update();
    }
}

void ParticleSystem::applyForce(double fx, double fy) {
    for(std::vector<Particle>::iterator i = particles.begin(); i != particles.end(); i++) {
        i->applyForce(fx, fy);
    }
}

void ParticleSystem::applyRandomForce(double maxMagnitude, double minMagnitude) {
    // seed randomness
    srand((int)time(NULL));

    // get random angle radian between 0 and 2pi
    double fx, fy, theta, mag;

    for(std::vector<Particle>::iterator i = particles.begin(); i != particles.end(); i++) {
        theta = ((double)rand() / (double)RAND_MAX) * 2 * M_PI;
        mag = (((double)rand() / (double)RAND_MAX) * (maxMagnitude - minMagnitude)) + minMagnitude;
        fx = cos(theta) * mag;
        fy = cos(theta) * mag;
        i->applyForce(fx, fy);
    }


}

void ParticleSystem::addParticle(double x, double y, int value) {
    Particle p(x, y, value);
    p.setWorldSpace(ws);
    particles.push_back(p);
}

void ParticleSystem::addParticles(int count, double x, double y, int value) {
    for(int i = 0; i < count; i++) {
        this->addParticle(x, y, value);
    }
}

// CONSTANTS ------------------------------------------------------------------

const int ROWS = 15;
const int COLS = 33;

// MAIN LOOP ------------------------------------------------------------------

int main(int argc, char** argv) {

    // Initialize Data

    WorldSpace ws(ROWS, COLS);
    ParticleSystem ps;
    ws.addParticleSystem(&ps);

    /*
    ps.addParticle(3, 3, 1);
    ps.addParticle(3, 4, 2);
    ps.addParticle(4, 3, 2);
    */

    ps.addParticles(10, 1, 1, 2);
    ps.addParticles(10, ws.getCols()-1, 1, 1);

    // Seed random

    // Render initial frame
    renderWorldSpace(&ws, 0);
    ps.applyRandomForce(1.0, 1.0);

    int s_clk, e_clk;
    long d_clk;
    int elapsed_time;
    long CLOCKS_PER_MICROSECOND = CLOCKS_PER_SEC / 1000000;

    // Run Frame Loop
    for(int frame = 0; frame < 1200; frame++) {
        // start timing
        s_clk = clock();

        // Reset Render Data & Clear Console
        resetRender(&ws);

        // Update Particles
        ps.applyForce(0, 0.09);
        ps.updateParticles();

        //printf("%p: %3.3lf, %3.3lf\n", &p1, p1.getX(), p1.getY());

        // Render WorldSpace
        renderWorldSpace(&ws, frame);

        // get end time
        e_clk = clock();

        d_clk = e_clk - s_clk;
        elapsed_time = d_clk / CLOCKS_PER_MICROSECOND;

        // Wait
        usleep(16666 - elapsed_time); // 60fps: 16666, 30fps: 33333
    }


}

// STATIC METHODS -------------------------------------------------------------

void renderWorldSpace(WorldSpace* ws, int num) {
    int cols = ws->getCols();
    int rows = ws->getRows();

    int*** data = ws->getRenderData();
    
    // HEADER
    printf("*");

    printf("%4d", num);

    for(int i = 0; i < cols-4; i++) {
        printf("-");
    }
    printf("*\n");

    // BODY & MARGINS
    for(int i = 0; i < rows; i++) {
        printf("|");
        for(int j = 0; j < cols; j++) {
            char c = getCharacter((*data)[i][j]);
            printf("%c",c);
        }
        printf("|\n");
    }
    
    // FOOTER
    printf("*");
    for(int i = 0; i < cols; i++) {
        printf("-");
    }
    printf("*\n");
    // For printing buffer
    printf("%x\n", ws->getBuffer());
    // For printing message
    printf("%s\n", ws->getCharBuffer());
}

void resetRender(WorldSpace* ws) {
    ws->resetRenderData();
    clearRender(ws->getRows());
}

void clearRender(int rows) {
    for(int i = 0; i < rows + 4; i++) {
        printf("\033[A\33[2k\r");
    }
}

char getCharacter(int value) {
    int v = value;
    if (v == 2) return '1';
    if (v == 1) return '0';
    else return ' ';
}