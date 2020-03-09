#include "rasterizing.h"
/*
 * Doc used for the development of this project: https://github.com/ssloy/tinyrenderer/wiki/Lesson-0-getting-started
 */

Vec3f lamp = Vec3f(0, 0, 1);
Model diablo = Model("../res/diablo3_pose.obj");
TGAImage texture = TGAImage();

const int fov = 90;
const int width = 2048;
const int height = 1536;
const int depth = 255;
const int nbPoints = 3;

/*
 * checking if the point is in a certain range
 */
bool inRange(int x, int y){
    return x >= 0 && y >= 0 && x < width && y < height;
}

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye-center).normalize();
    Vec3f x = (up^z).normalize();
    Vec3f y = (z^x).normalize();
    Matrix ret = Matrix::identity(4);
    for (int i=0; i<3; i++) {
        ret[0][i] = x[i];
        ret[1][i] = y[i];
        ret[2][i] = z[i];
        ret[i][3] = -center[i];
    }
    return ret;
}

Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;

    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
    return m;
}

Vec3f getBarycentricCoordinates(const Vec3f &AB, const Vec3f &AC, const Vec3f &PA){
    Vec3f vecX, vecY, coor;
    vecX = Vec3f(AB.x, AC.x, PA.x);
    vecY = Vec3f(AB.y, AC.y, PA.y);
    coor = vecX^vecY;
    return Vec3f((1.f - (coor.x + coor.y)/coor.z), (coor.x / coor.z), (coor.y / coor.z));
}

/*
 * method that will update the z-buffer by comparing the current value of z
 * with the stocked one.
 */
void updateZBuffer(int x, int y, Vec3f points[], Vec2f text[], const Vec3f norms[], const Vec3f &barycenter,
                   std::vector<Vec3f> &framebuffer, std::vector<float> &zBuffer) {
    // bi-linear interpolation to retrieve Z
    float z = points[0].z * barycenter.x + points[1].z * barycenter.y + points[2].z * barycenter.z;

    // getting norm of the current point using bi-linear interpolation between norms of vertices
    Vec3f interNorm = (norms[0] * barycenter.x + norms[1] * barycenter.y + norms[2] * barycenter.z).normalize();
    float illuminate = interNorm*lamp;

    // retrieve position in the texture

    float xText, yText;
    xText = text[0].x * barycenter.x + text[1].x * barycenter.y + text[2].x * barycenter.z;
    yText = text[0].y * barycenter.x + text[1].y * barycenter.y + text[2].y * barycenter.z;
    xText *= (float)(texture.get_width());
    yText = (yText) * (float)(texture.get_height());
    TGAColor textureForThePoint = texture.get((int)xText, (int)(yText));

    //    std::cout << (int) textureForThePoint.bgra[2] << std::endl;
    //    std::cout << (int) textureForThePoint.bgra[1] << std::endl;
    //    std::cout << (int) textureForThePoint.bgra[0] << std::endl;
//        std::cout << illuminate << std::endl;

    // checking z-buffer
    if(z >= zBuffer[y*width + x]){
        zBuffer[y*width + x] = z;
        framebuffer[(height - 1 - y)*width + x] = Vec3f(
                ((float)textureForThePoint.bgra[2]/255)*illuminate,
                ((float)textureForThePoint.bgra[1]/255)*illuminate,
                ((float)textureForThePoint.bgra[0]/255)*illuminate
                );
    }
}

/*
 * drawing a triangle using barycentric coordinates  algorithm (using bounding box)
 */
void rasterizeTriangle(Vec3f points[], const Vec3f norms[], Vec2f text[], std::vector<Vec3f> &framebuffer,
                       std::vector<float> &zBuffer) {
    Vec3f A, B, C, P, AB, AC, PA, barycenter;
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
            barycenter = getBarycentricCoordinates(AB, AC, PA);
            // checking if inside triangle and update z-buffer
            if(barycenter.x > 0 && barycenter.y > 0 && barycenter.z > 0){
                updateZBuffer(x, y, points, text, norms, barycenter, framebuffer, zBuffer);
            }
        }
    }
}

/*
 * writing a ppm image, code from: https://github.com/ssloy/tinyraytracer
 */
void writeInFile(std::vector<Vec3f> &image, int w, int h, std::string name){
    std::ofstream ofs;
    ofs.open("../images/out" + name + ".ppm", std::ios::binary);
    ofs << "P6\n" << w << " " << h << "\n255\n";
    for(size_t i = 0; i < h * w; i++){
        Vec3f &c = image[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max>1) c = c*(1.f/max);
        for (size_t j = 0; j<3; j++) {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, image[i][j])));
        }
    }
    ofs.close();
}

/*
 * creating viewport matrix, code from https://github.com/ssloy/tinyrenderer
 */
Matrix viewport(float x, float y, float w, float h) {
    Matrix Viewport = Matrix::identity(4);
    Viewport[0][3] = x+w/2.f;
    Viewport[1][3] = y+h/2.f;
    Viewport[2][3] = depth/2.f;
    Viewport[0][0] = w/2.f;
    Viewport[1][1] = h/2.f;
    Viewport[2][2] = depth/2.f;
    return Viewport;
}

Matrix projection(const float c){
    Matrix Projection = Matrix::identity(4);
    Projection[3][2] = -1.f/c;
    return Projection;
}

/*
 * code from https://github.com/ssloy/tinyrenderer
 */
Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}

/*
 * code from https://github.com/ssloy/tinyrenderer
 */
Matrix v2m(const Vec3f &v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

void render(){
    //std::vector<float> zBuffer(width*height);
    //Vec3f camera = Vec3f(0, 0, 2); // camera.z used in the projection matrix, != 0

    texture.read_tga_file("../res/diablo3_pose_diffuse.tga"); // getting texture
    texture.flip_vertically();
    //std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::lowest()); // filling z-buffer

    std::vector<Vec3f> framebuffer(width*height); //used for the fusion
    std::vector<Vec3f> framebufferLeftEye(width*height); //image used for left eye
    std::vector<Vec3f> framebufferRightEye(width*height);//image used for right eye

    //Matrix VP = viewport((float)width/8, (float)height/8, (float)width*3/4, (float)height*3/4); // getting viewport
    //Matrix P  = projection(camera.z);// getting perspective matrix


    std::vector<float> zBufferLeft(width*height);
    std::fill(zBufferLeft.begin(), zBufferLeft.end(), std::numeric_limits<float>::lowest());
    std::vector<float> zBufferRight(width*height);
    std::fill(zBufferRight.begin(), zBufferRight.end(), std::numeric_limits<float>::lowest());

    Vec3f center(0,0,0);

    Vec3f leftEye(-0.1, 0, 2.5);
    Vec3f rightEye(0.1, 0, 2.5);
    Matrix ModelViewLeft = lookat(leftEye, center, Vec3f(0,1,0));
    Matrix ModelViewRight = lookat(rightEye, center, Vec3f(0,1,0));
    Matrix ViewPort = viewport(width/8, height/8, width*3/4, height*3/4);

    Matrix ProjectionLeft = projection((leftEye - center).norm());
    Matrix ProjectionRight = projection((rightEye - center).norm());

    Matrix TotalTransformLeft = ViewPort * ProjectionLeft * ModelViewLeft;
    Matrix TotalTransformRight = ViewPort * ProjectionRight * ModelViewRight;


    // drawing things here
    Vec3f pointsLeft[nbPoints];
    Vec3f pointsRight[nbPoints];
    Vec3f pointLeft;
    Vec3f pointRight;

    #pragma omp parallel for
    for(int i = 0; i < diablo.nfaces(); i++){   // fetching diablo's mesh
        for(int j = 0; j < nbPoints; j++){
            pointLeft = diablo.point(diablo.vert(i, j));
            pointRight = diablo.point(diablo.vert(i, j));

            pointsLeft[j] = m2v(TotalTransformLeft * v2m(pointLeft));
            pointsRight[j] = m2v(TotalTransformRight * v2m(pointRight));
        }

        Vec3f norms[nbPoints];
        Vec2f text[nbPoints];
        // retrieving normal for each vertices, and textures position
        for(int k = 0 ; k < nbPoints; k++){
            norms[k] = diablo.normal(i, k);
            text[k] = diablo.uv(i, k);
        }
        rasterizeTriangle(pointsLeft, norms, text, framebufferLeftEye, zBufferLeft);
        rasterizeTriangle(pointsRight, norms, text, framebufferRightEye, zBufferRight);
    }

    // generating file
    writeInFile(framebufferLeftEye, width, height, "Left");
    writeInFile(framebufferRightEye, width, height, "Right");

    for (size_t i = 0; i < height * width; ++i) {
        Vec3f &c = framebufferLeftEye[i];
        Vec3f &cbis = framebufferRightEye[i];
        Vec3f res;
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max > 1) c = c * (1. / max);
        float maxbis = std::max(cbis[0], std::max(cbis[1], cbis[2]));
        if (maxbis > 1) cbis = cbis * (1. / maxbis);

        res = Vec3f((c.x + c.z) / 2., (c.y + cbis.y)/2., (cbis.x + cbis.z) / 2.);

        framebuffer[i] = res ;

    }


    writeInFile(framebuffer, width, height, "17");



}

int main() {
    render();
}
