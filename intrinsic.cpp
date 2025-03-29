#include <SFML/Graphics.hpp>
#include <string.h>
#include <math.h>
#include <immintrin.h>

#define XMM_FLOAT_SIZE 8
#define _CMP_LT_OQ    0x11 /* Less-than (ordered, signaling)  */

const int I_MAX          = 256;
const int X_RES          = 800;
const int Y_RES          = 600;
const int MAX_NUM_LEN    = 5;
const int SCALE_TIME     = 9; //10**9 cycles
const __m256 mRNG_MX     = _mm256_set1_ps(100.0f);
const float RNG_MX       = 100.0f;
const float X_OFF        = 0.5f;
const float Y_OFF        = 0.5f;
const float X_SCL        = 4.0f;
const float Y_SCL        = 3.0f;

void                drawSet();
unsigned long long  evalTime();
inline void         evalPoint(__m256 x_arg, __m256 y_arg, volatile __m256i* iter);
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


        __m256 iter_const = _mm256_set_ps(7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f);
        __m256 x_res      = _mm256_set1_ps(X_RES);
        __m256 x_off      = _mm256_set1_ps(X_OFF);
        __m256 x_scl      = _mm256_set1_ps(X_SCL);

        __m256 y_res      = _mm256_set1_ps(Y_RES);
        __m256 y_off      = _mm256_set1_ps(Y_OFF);
        __m256 y_scl      = _mm256_set1_ps(Y_SCL);

        for (int y = 0; y < Y_RES; y++){
            __m256  y_arg = _mm256_set1_ps((((float)y)/ Y_RES - Y_OFF) * Y_SCL);

            for (int x = 0; x < X_RES; x += XMM_FLOAT_SIZE){
                __m256  x_arg = _mm256_set1_ps(x);

                if ((x == 0 || x == 8)&& y == 0){
                    for (int i = 0; i < 8; i++){
                        printf("%d:%f ", i, ((float*)&x_arg)[i]);
                    }
                    printf("\n");
                }
                        x_arg = _mm256_add_ps(x_arg, iter_const);


                if ((x == 0 || x == 8) && y == 0){
                    for (int i = 0; i < 8; i++){
                        printf("%d:%f ", i, ((float*)&x_arg)[i]);
                    }
                    printf("\n");
                }
                        x_arg = _mm256_div_ps(x_arg, x_res);

                if ((x == 0 || x == 8) && y == 0){
                    for (int i = 0; i < 8; i++){
                        printf("%d:%f ", i, ((float*)&x_arg)[i]);
                    }
                    printf("\n");
                }
                        x_arg = _mm256_sub_ps(x_arg, x_off);

                if ((x == 0 || x == 8) && y == 0){
                    for (int i = 0; i < 8; i++){
                        printf("%d:%d ", i, ((int*)&x_arg)[i]);
                    }
                    printf("\n");
                }
                        x_arg = _mm256_mul_ps(x_arg, x_scl);

                if ((x == 0 || x == 8)&& y == 0){
                    for (int i = 0; i < 8; i++){
                        printf("%d:%d ", i, ((int*)&x_arg)[i]);
                    }
                    printf("\n");
                }
                __m256 iter = _mm256_setzero_ps();
                evalPoint(x_arg, y_arg, (__m256i*)&iter);

                if ((x == 0 || x == 8) && y == 0){
                    for (int i = 0; i < 8; i++){
                        printf("%d:%d ", i, ((int*)&iter)[i]);
                    }
                    printf("\n\n\n\n");
                }


                int* iter_int = (int*)&iter;

                //printf("1:%d, 2:%d, 3:%d, 4:%d, 5:%d, 6:%d, 7:%d, 8:%d\n", iter_int[0], iter_int[1], iter_int[2], iter_int[3], iter_int[4], iter_int[5], iter_int[6], iter_int[7]);

                for (int k = 0; k < XMM_FLOAT_SIZE; k++){
                    sf::Color sfColor((255 - (iter_int[k] * 16)) % 255, (255 - (iter_int[k] * 16)) % 255, (255 - (iter_int[k] * 16)) % 255);
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

inline void evalPoint(__m256 x_arg, __m256 y_arg, volatile __m256i* iter){
    __m256 x = _mm256_set1_ps(0);
    __m256 y = _mm256_set1_ps(0);

    _mm256_storeu_si256((__m256i*)&x, (__m256i)x_arg);
    _mm256_storeu_si256((__m256i*)&y, (__m256i)y_arg);

    for (int i_glbl = 0; i_glbl < I_MAX; i_glbl++){

        __m256 x_2 = _mm256_mul_ps(x, x);
        __m256 y_2 = _mm256_mul_ps(y, y);
        __m256 x_y = _mm256_mul_ps(x, y);
        __m256 rad = _mm256_add_ps(x_2, y_2);

        __m256 cmp = _mm256_cmp_ps(rad, mRNG_MX, _CMP_LT_OQ);

        int mask = _mm256_movemask_ps(cmp);

        if (!mask) break;

        *iter = _mm256_sub_epi32(*iter, (__m256i)cmp);

        _mm256_storeu_si256((__m256i*)&x, (__m256i)_mm256_add_ps(_mm256_sub_ps(x_2, y_2), x_arg));
        _mm256_storeu_si256((__m256i*)&y, (__m256i)_mm256_add_ps(_mm256_add_ps(x_y, x_y), y_arg));
    }
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

    __m256 iter_const = (__m256)_mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
    __m256 x_res      = _mm256_set1_ps(X_RES);
    __m256 x_off      = _mm256_set1_ps(X_OFF);
    __m256 x_scl      = _mm256_set1_ps(X_SCL);

    __m256 y_res      = _mm256_set1_ps(Y_RES);
    __m256 y_off      = _mm256_set1_ps(Y_OFF);
    __m256 y_scl      = _mm256_set1_ps(Y_SCL);

    unsigned long long total = 0;
    unsigned long long start_rdtsc = __rdtsc();

    for (int i = 0; i < testIter; i++){
        for (int y = 0; y < Y_RES; y++){

            __m256  y_arg = _mm256_set1_ps(y);
                    y_arg = _mm256_div_ps(y_arg, y_res);
                    y_arg = _mm256_sub_ps(y_arg, y_off);
                    y_arg = _mm256_mul_ps(y_arg, y_scl);

            __m256 iter = _mm256_setzero_ps();

            for (int x = 0; x < X_RES; x += XMM_FLOAT_SIZE){

                __m256  x_arg = _mm256_set1_ps(x);
                        x_arg = _mm256_add_ps(x_arg, iter_const);
                        x_arg = _mm256_div_ps(x_arg, x_res);
                        x_arg = _mm256_sub_ps(x_arg, x_off);
                        x_arg = _mm256_mul_ps(x_arg, x_scl);

                evalPoint(x_arg, y_arg, (__m256i*)&iter);
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
