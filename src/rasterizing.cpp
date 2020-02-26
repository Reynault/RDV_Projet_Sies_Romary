#include "rasterizing.h"
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
void drawLine(int x1, int y1, int x2, int y2, std::vector<Vec3f> &framebuffer, int width, int height, Vec3f color){
    if(inRange(x1, y1, width, height) && inRange(x2, y2, width, height)) {
        int dx, dy, error, offset, shift, relocate;
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
        offset = std::abs(dy) * 2;
        error = 0;
        relocate = dx * 2;
        shift = (y1 > y2) ? -1 : 1;
        int y = y1;
        for (int x = x1; x <= x2; x++) {
            (steep) ? framebuffer[y + x * width] = color : framebuffer[x + y * width] = color;
            if ((error = error + offset) > dx) {
                y += shift;
                error -= relocate;
            }
        }
    }
}

/*
 * drawing a polygon as a mesh
 */
void drawPolygonMesh(Vec3f points[], int nbLines, std::vector<Vec3f> &framebuffer, int width, int height, Vec3f color){
    for(int i = 0; i < nbLines-1; i++){ // drawing every lines
        drawLine(points[i].x, points[i].y, points[i+1].x, points[i+1].y,
                framebuffer, width, height, color);
    }
    if(nbLines > 1){    // drawing last one
        drawLine(points[0].x, points[0].y, points[nbLines-1].x, points[nbLines-1].y,
                framebuffer, width, height, color);
    }
}

/*
 * drawing a triangle using line sweeping algorithm (first approach)
 */
void sweepingTriangle(Vec3f points[], std::vector<float> &zBuffer,
        std::vector<Vec3f> &framebuffer, int width, int height, Vec3f color){
    Vec3f A, B, C, AB, BC, AC, pAC, pAB, pBC;
    A = points[0];
    B = points[1];
    C = points[2];

    // sort vertices of the triangle by their y-coordinates;
    if(A.y > B.y) std::swap(A, B);
    if(B.y > C.y) std::swap(B, C);
    if(A.y > B.y) std::swap(A, B);

    // getting vectors AB, BC, AC
    AB = Vec3f(
            std::abs(B.x - A.x),
            std::abs(B.y - A.y),
            std::abs(B.z - A.z)
    );
    BC = Vec3f(
            std::abs(B.x - C.x),
            std::abs(B.y - C.y),
            std::abs(B.z - C.z)
    );
    AC = Vec3f(
            std::abs(C.x - A.x),
            std::abs(C.y - A.y),
            std::abs(C.z - A.z)
    );

    // getting points of the current line
    pAC = A;
    pAB = A;
    pBC = B;

    // drawing upper part of the triangle
    if(AB.y > 0){
        for (float i = 0; i < AB.y; i++) {
            // linear interpolation to retrieve x position of pAC
            // (depending on the direction of the segment)
            (A.x < C.x ) ? pAC.x = A.x + (AC.x * i) / AC.y : pAC.x = A.x - (AC.x * i) / AC.y ;
            pAC.y = A.y + i;

            (A.x < B.x) ? pAB.x = A.x + (AB.x * i) / AB.y : pAB.x = A.x - (AB.x * i) / AB.y;
            pAB.y = pAC.y;

            drawLine(pAB.x, pAB.y, pAC.x, pAC.y, framebuffer, width, height, color);
        }
    }else{
        drawLine(A.x, A.y, B.x, B.y, framebuffer, width, height, color);
    }


    // drawing lower part of the triangle
    if(BC.y > 0){
        for(float i = AB.y; i < AC.y; i++){
            (A.x < C.x ) ? pAC.x = A.x + (AC.x * i) / AC.y : pAC.x = A.x - (AC.x * i) / AC.y ;
            pAC.y = A.y + i;

            (B.x < C.x ) ? pBC.x = B.x + (BC.x * (i-AB.y)) / BC.y : pBC.x = B.x - (BC.x * (i-AB.y)) / BC.y;
            pBC.y = pAC.y;

            drawLine(pBC.x, pBC.y, pAC.x, pAC.y, framebuffer, width, height, color);
        }
    }else{
        drawLine(B.x, B.y, C.x, C.y, framebuffer, width, height, color);
    }

    drawLine(A.x, A.y, B.x, B.y, framebuffer, width, height, color);
    drawLine(A.x, A.y, C.x, C.y, framebuffer, width, height, color);
    drawLine(B.x, B.y, C.x, C.y, framebuffer, width, height, color);

}

/*
 * drawing a triangle using barycentric coordinates  algorithm (using bounding box)
 */
void rasterizeTriangle(Vec3f points[], std::vector<float> &zBuffer,
                      std::vector<Vec3f> &framebuffer, int width, int height, Vec3f color){
    Vec3f A, B, C, P, AB, AC, PA, vecX, vecY, coor;
    int minX, maxX, minY, maxY;

    A = points[0];
    B = points[1];
    C = points[2];

    AB = Vec3f(
        B.x - A.x,
        B.y - A.y,
        B.z - A.z
    );
    AC = Vec3f(
            C.x - A.x,
            C.y - A.y,
            C.z - A.z
    );

    // finding bounding box
    minX = std::min(A.x, std::min(B.x, C.x));
    maxX = std::max(A.x, std::max(B.x, C.x));
    minY = std::min(A.y, std::min(B.y, C.y));
    maxY = std::max(A.y, std::max(B.y, C.y));

    // iterating on bounding box
    for(int x = minX; x <= maxX; x++){
        for(int y = minY; y <= maxY; y++){
            P = Vec3f(x, y, 1);
            PA = Vec3f(
                    A.x - P.x,
                    A.y - P.y,
                    A.z - P.z
            );

            // computing barycentric coordinates
            vecX = Vec3f(AB.x, AC.x, PA.x);
            vecY = Vec3f(AB.y, AC.y, PA.y);
            coor = cross(vecX, vecY);

            // checking if inside triangle
            if( (1.f - (coor.x + coor.y)/coor.z) >= 0 && (coor.x / coor.z) >= 0
            && (coor.y / coor.z) >= 0 && std::abs(coor.z)>=1){
                framebuffer[y*width + x] = color;
            }
        }
    }
}

bool updateZBuffer(float x, float y, Vec3f points[], std::vector<float> &zBuffer,
        std::vector<Vec3f> &framebuffer, int width, int height){
    bool res = false;
    if(inRange(x, y, width, height)){
        // bi-linear interpolation to retrieve Z
        // checking if z in z-buffer is <
    }
    return res;
}

/*
 * writing a ppm image, code from: https://github.com/ssloy/tinyraytracer
 */
void writeInFile(std::vector<Vec3f> &framebuffer, int width, int height){
    std::ofstream ofs;
    ofs.open("../images/out5.ppm", std::ios::binary);
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
    const int fov = 90;
    const int width = 1024;
    const int height = 768;
    Model diablo = Model("../res/diablo3_pose.obj");
    std::vector<Vec3f> framebuffer(width*height);
    std::vector<float> zBuffer(width*height);
    Vec3f camera = Vec3f(0, 0, 0);
    Vec3f orient = Vec3f(0, 0, 0);

    // drawing things here
    int nbPoints = 3;
    Vec3f points[nbPoints];
    Vec3f point;
    #pragma omp parallel for
    for(int i = 0; i < diablo.nfaces(); i++){   // fetching diablo's mesh
        for(int j = 0; j < nbPoints; j++){
            point = diablo.point(diablo.vert(i, j));
            point.x = width - ((point.x+1) * width)/2;
            point.y = height - ((point.y+1) * height)/2;
            points[j] = point;
        }
        rasterizeTriangle(points, zBuffer, framebuffer, width, height, WHITE);
    }

//    Vec3f t1[] = {Vec3f(10,70,10), Vec3f(50,160,10), Vec3f(70,80,20)};
//    Vec3f t2[]  = {Vec3f(180,50,15), Vec3f(150,1,15), Vec3f(70,180,15)};
//    Vec3f t3[]  = {Vec3f(180,150,0), Vec3f(120,160,0), Vec3f(130,180,60)};
//
//    sweepingTriangle(t1, zBuffer, framebuffer, width, height, GREEN);
//    sweepingTriangle(t2, zBuffer, framebuffer, width, height, GREEN);
//    sweepingTriangle(t3, zBuffer, framebuffer, width, height, GREEN);
//
//    rasterizeTriangle(t1, zBuffer, framebuffer, width, height, RED);
//    rasterizeTriangle(t2, zBuffer, framebuffer, width, height, RED);
//    rasterizeTriangle(t3, zBuffer, framebuffer, width, height, RED);

    // generating file
    writeInFile(framebuffer, width, height);
}

int main() {
    render();
}
