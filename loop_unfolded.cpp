#include <SFML/Graphics.hpp>
#include <string.h>
#include <math.h>
#include <immintrin.h>

#define XMM_FLOAT_SIZE 8

const int I_MAX          = 256;
const int X_RES          = 800;
const int Y_RES          = 600;
const int MAX_NUM_LEN    = 5;
const int SCALE_TIME     = 9; //10**9 cycles
const float RNG_MX       = 100.0f;
const float X_OFF        = 0.5f;
const float Y_OFF        = 0.5f;
const float X_SCL        = 4.0f;
const float Y_SCL        = 3.0f;

void                drawSet();
unsigned long long  evalTime();
inline int          evalPoint(float* x_arg, float* y_arg, volatile int* iter);
void                processArgs(int argc, const char* argv[], int* doDraw, int* testIter);

void drawSet(){
    sf::RenderWindow window(sf::VideoMode(X_RES, Y_RES), "Mandelbrot Set");

    sf::Image image;
    image.create(X_RES, Y_RES, sf::Color::Black);

    sf::Texture texture;
    texture.create(X_RES, Y_RES);
    sf::Sprite sprite(texture);

    float x_offset = 0, y_offset = 0, scale = 1.0;
    float timeElapsed = 0, fps = 0;
    clock_t start = 0, finish = 0;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))       y_offset -= 0.05;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))     y_offset += 0.05;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))     x_offset -= 0.05;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))    x_offset += 0.05;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))        scale /= 1.5;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))        scale *= 1.5;
        }

        start = clock();

        float x_arg[XMM_FLOAT_SIZE] = {}, y_arg[XMM_FLOAT_SIZE] = {};
        float y_ar = 0;

        for (int y = 0; y < Y_RES; y++){
            y_ar = (((float)y)/ Y_RES - Y_OFF + y_offset) * Y_SCL * scale;
            for (int i = 0; i < XMM_FLOAT_SIZE; i++) y_arg[i] = y_ar;

            for (int x = 0; x < X_RES; x += XMM_FLOAT_SIZE){
                for (int j = 0; j < XMM_FLOAT_SIZE; j++) x_arg[j] = (((float)x + j)/ X_RES - X_OFF + x_offset) * X_SCL * scale;
                int iter[XMM_FLOAT_SIZE] = {};
                evalPoint(x_arg, y_arg, iter);

                for (int k = 0; k < XMM_FLOAT_SIZE; k++){
                    sf::Color sfColor((255 - (iter[k] * 16)) % 255, (255 - (iter[k] * 16)) % 255, (255 - (iter[k] * 16)) % 255);
                    image.setPixel(x + k, y, sfColor);
                }
            }
        }

        finish = clock();
        timeElapsed = (double)(finish - start)/CLOCKS_PER_SEC;
        fps = 1.0f / timeElapsed;

        printf("fps:%2.1lf\n", fps);

        texture.update(image);

        window.clear();
        window.draw(sprite);
        window.display();
    }
}

inline int evalPoint(float* x_arg, float* y_arg, volatile int* iter){
    float x_2[XMM_FLOAT_SIZE] = {}, y_2[XMM_FLOAT_SIZE] = {};
    float x_y[XMM_FLOAT_SIZE] = {}, x_next[XMM_FLOAT_SIZE] = {}, y_next[XMM_FLOAT_SIZE] = {};

    for (int i_glbl = 0; i_glbl < I_MAX; i_glbl++){

        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            x_2[i] = x_next[i] * x_next[i];
            y_2[i] = y_next[i] * y_next[i];
            x_y[i] = x_next[i] * y_next[i];
        }

        int cmp[XMM_FLOAT_SIZE] = {};

        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            if (x_2[i] + y_2[i] < RNG_MX) cmp[i] = 1;
        }

        int mask = 0;
        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            mask = mask | (cmp[i] << i);
        }

        if (!mask) break;

        for (int i = 0; i < XMM_FLOAT_SIZE; i++){
            iter[i] += cmp[i];
            y_next[i] = 2 * x_y[i] + y_arg[i];
            x_next[i] = x_2[i] - y_2[i] + x_arg[i];
        }
    }

    for (int i = 0; i < XMM_FLOAT_SIZE; i++){
        if (iter[i] == I_MAX) iter[i] = 0;
    }

    return 0;
}


void processArgs(int argc, const char* argv[], int* doDraw, int* testIter){
    for (int i = argc - 1; i > 0; i--){
        if (!strncmp(argv[i], "--graphics", strlen("--graphics"))){
            *doDraw = 1;
        }

        else if (!strncmp(argv[i], "--test", strlen("--test"))){
            *testIter = atoi(argv[i] + strlen("--test"));
            printf("%d\n", *testIter);
        }

        else {
            fprintf(stderr, "unknown arg:%s\n", argv[i]);
        }
    }
}

unsigned long long evalTime(int testIter){
    float x_arg[XMM_FLOAT_SIZE] = {};
    float y_arg[XMM_FLOAT_SIZE] = {};
    float y_ar = 0;

    unsigned long long total = 0;
    unsigned long long start_rdtsc = __rdtsc();

    for (int i = 0; i < testIter; i++){
        for (int y = 0; y < Y_RES; y++){
            y_ar = (((float)y)/ Y_RES - Y_OFF) * Y_SCL;
            for (int i = 0; i < XMM_FLOAT_SIZE; i++) y_arg[i] = y_ar;
            volatile int iter[XMM_FLOAT_SIZE] = {};

            for (int x = 0; x < X_RES; x += XMM_FLOAT_SIZE){
                for (int i = 0; i < XMM_FLOAT_SIZE; i++) x_arg[i] = (((float)x + i)/ X_RES - X_OFF) * X_SCL;

                evalPoint(x_arg, y_arg, iter);
            }
        }
    }

    unsigned long long end_rdtsc = __rdtsc();
    unsigned long long time_rdtsc = end_rdtsc - start_rdtsc;

    return time_rdtsc;
}



int main(int argc, const char* argv[])
{
    int  doDraw = 0, testIter = 0;
    double cyclesPassed = 0;

    if (argc >= 2) processArgs(argc, argv, &doDraw, &testIter);

    if (doDraw) drawSet();
    if (testIter) cyclesPassed = evalTime(testIter) / pow(10, SCALE_TIME);

    printf("cycles passed:%2.3lf*10^%d\n", cyclesPassed, SCALE_TIME);
    return 0;
}
