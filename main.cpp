#include <SFML/Graphics.hpp>
#include <string.h>
#include <time.h>

const int I_MAX          = 256;
const int X_RES          = 800;
const int Y_RES          = 600;
const int MAX_NUM_LEN    = 5;
const float RNG_MX       = 10.0f;
const float X_OFF        = 0.5f;
const float Y_OFF        = 0.5f;
const float X_SCL        = 4.0f;
const float Y_SCL        = 3.0f;

void    drawSet();
double  evalTime();
int     evalPoint(float x_arg, float y_arg);
void    processArgs(int argc, const char* argv[], int* doDraw, int* testIter);

void drawSet(){
    sf::RenderWindow window(sf::VideoMode(X_RES, Y_RES), "Mandelbrot Set");

    sf::Image image;
    image.create(X_RES, Y_RES, sf::Color::Black);

    sf::Texture texture;
    texture.create(X_RES, Y_RES);
    sf::Sprite sprite(texture);

    float x_offset = 0, y_offset = 0, scale = 0.3;
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

        for (int y = 0; y < Y_RES; y++){
            for (int x = 0; x < X_RES; x++){
                float x_arg = ((float)x / X_RES - X_OFF + x_offset) * X_SCL * scale;
                float y_arg = ((float)y / Y_RES - Y_OFF + y_offset) * Y_SCL * scale;

                int iter = evalPoint(x_arg, y_arg);

                sf::Color sfColor((255 - (iter * 16)) % 255, (255 - (iter * 16)) % 255, (255 - (iter * 16)) % 255);

                image.setPixel(x, y, sfColor);
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

int evalPoint(float x_arg, float y_arg){
    float x_2 = 0, y_2 = 0;
    float x_y = 0 , x_next = 0, y_next = 0;
    int i = 0;

    for (; i < I_MAX && x_2 + y_2 < RNG_MX; i++){
        x_2 = x_next * x_next;
        y_2 = y_next * y_next;
        x_y = x_next * y_next;

        y_next = 2 * x_y + y_arg;
        x_next = x_2 - y_2 + x_arg;
    }

    if (i == I_MAX) return 0;

    return i;
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

double evalTime(int testIter){
    clock_t start = 0, finish = 0;
    double time = 0;
    start = clock();

    for (int i = 0; i < testIter; i++){
        for (int y = 0; y < Y_RES; y++){
            for (int x = 0; x < X_RES; x++){
                float x_arg = ((float)x / X_RES - X_OFF) * X_SCL;
                float y_arg = ((float)y / Y_RES - Y_OFF) * Y_SCL;

                int iter = evalPoint(x_arg, y_arg);
            }
        }
    }

    finish = clock();
    time = (double)(finish - start)/CLOCKS_PER_SEC;

    return time;
}



int main(int argc, const char* argv[])
{
    int  doDraw = 0, testIter = 0;
    double timePassed = 0, timeMainPassed = 0;
    clock_t startMain = 0, finishMain = 0;

    startMain = clock();

    if (argc >= 2) processArgs(argc, argv, &doDraw, &testIter);

    if (doDraw) drawSet();
    if (testIter) timePassed = evalTime(testIter);

    finishMain = clock();

    timeMainPassed = (double)(finishMain - startMain)/CLOCKS_PER_SEC;

    printf("time passed from main:%lf seconds\n", timeMainPassed);
    printf("time passed:%lf seconds\n", timePassed);
    return 0;
}
