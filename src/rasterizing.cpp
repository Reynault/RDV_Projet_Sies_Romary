#include <iostream>
#include "model.cpp"
/*
 * Doc used for the development of this project: https://github.com/ssloy/tinyrenderer/wiki/Lesson-0-getting-started
 */

/*
 * checking if the point is in a certain range
 */
bool inRange(int x, int y, int width, int height){
    return x >= 0 && y >= 0 && x < width && y < height;
}

/*
 *  drawing a line using bresenham algorithm
 */
void line(int x1, int y1, int x2, int y2, std::vector<Vec3f> &framebuffer, int width, int height, Vec3f color){
    if(inRange(x1, y1, width, height) && inRange(x2, y2, width, height)) {
        int dx, dy, error, derror;
        bool steep = false;
        if (std::abs(x2 - x1) < std::abs(y2 - y1)) {
            std::swap(x1, y1);
            std::swap(x2, y2);
            steep = true;
        }
        if (x1 > x2) {
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        dx = x2 - x1;
        dy = y2 - y1;
        derror = std::abs(dy) * 2;
        error = 0;
        int y = y1;
        for (int x = x1; x <= x2; x++) {
            if (steep) {
                framebuffer[y + x * width] = color;
            } else {
                framebuffer[x + y * width] = color;
            }
            if ((error = error + derror) > dx) {
                y += (y1 > y2) ? -1 : 1;
                error -= dx * 2;
            }
        }
    }
}

/*
 * drawing a triangle as a mesh
 */
void polygonMesh(int x[], int y[], int nbLines, std::vector<Vec3f> &framebuffer, int width, int height, Vec3f color){
    for(int i = 0; i < nbLines-1; i++){
        line(x[i], y[i], x[i+1], y[i+1],
                framebuffer, width, height, color);
    }
    if(nbLines > 1){
        line(x[0], y[0], x[nbLines-1], y[nbLines-1], framebuffer, width, height, color);
    }
}


/*
 * writing a ppm image, code from: https://github.com/ssloy/tinyraytracer
 */
void writeInFile(std::vector<Vec3f> &framebuffer, int width, int height){
    std::ofstream ofs;
    ofs.open("../images/out3.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for(size_t i = 0; i < height*width; i++){
        Vec3f &c = framebuffer[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max>1) c = c*(1./max);
        for (size_t j = 0; j<3; j++) {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}

void render(){
    const Vec3f white = Vec3f(1, 1, 1);
    const int fov = 90;
    const int width = 1024;
    const int height = 768;
    Model diablo = Model("../res/diablo3_pose.obj");
    std::vector<Vec3f> framebuffer(width*height);
    Vec3f camera = Vec3f(0, 0, 0);
    Vec3f orient = Vec3f(0, 0, 0);

    // drawing things
    int nbPoints = 3;
    int x[nbPoints], y[nbPoints];
    Vec3f point;
    for(int i = 0; i < diablo.nfaces(); i++){
        for(int j = 0; j < nbPoints; j++){
            point = diablo.point(diablo.vert(i, j));
            x[j] = width - (int)((point.x+1) * width)/2;
            y[j] = height - (int)((point.y+1) * height)/2;
        }
        polygonMesh(x, y, nbPoints, framebuffer, width, height, white);
    }

    // generating file
    writeInFile(framebuffer, width, height);
}

int main() {
    render();
}
