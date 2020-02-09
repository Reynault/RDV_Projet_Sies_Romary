#include <iostream>
#include "model.cpp"
/*
 * Doc used for the development of this project: https://github.com/ssloy/tinyrenderer/wiki/Lesson-0-getting-started
 */

void render(){
    const int width = 1024;
    const int height = 768;
    const int fov = 90;
    std::vector<Vec3f> framebuffer(width*height);

    #pragma omp parallel for
    for(size_t i = 0; i < width; i++){
        for(size_t j = 0; j < height; j++){
            framebuffer[i+j*width] = Vec3f (1 ,1 , 0);
        }
    }

    // writing a ppm image, code from: https://github.com/ssloy/tinyraytracer
    std::ofstream ofs;
    ofs.open("./out1.ppm", std::ios::binary);
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

int main() {
    std::cout << "Rendering images" << std::endl;
    render();
}
