#include <SFML/Graphics.hpp>

const int I_MAX          = 1024;
const int X_RES          = 800;
const int Y_RES          = 600;
const float RNG_MX       = 10.0f;
const float X_OFF        = 0.5f;
const float Y_OFF        = 0.5f;
const float X_SCL        = 4.0f;
const float Y_SCL        = 3.0f;

void    drawSet();
int     evalPoint(float x_arg, float y_arg);

void drawSet(){
    sf::RenderWindow window(sf::VideoMode(X_RES, Y_RES), "Mandelbrot Set");

    sf::Image image;
    image.create(X_RES, Y_RES, sf::Color::Black);

    sf::Texture texture;
    texture.create(X_RES, Y_RES);
    sf::Sprite sprite(texture);

    float x_offset = 0, y_offset = 0, scale = 1.0;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))       y_offset -= 0.01;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))     y_offset += 0.01;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))     x_offset -= 0.01;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))    x_offset += 0.01;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z))        scale *= 1.5;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))        scale /= 1.5;
        }

        for (int y = 0; y < Y_RES; y++){
            for (int x = 0; x < X_RES; x++){
                float x_arg = ((float)x / X_RES - X_OFF + x_offset) * X_SCL * scale;
                float y_arg = ((float)y / Y_RES - Y_OFF + y_offset) * Y_SCL * scale;

                int iter = evalPoint(x_arg, y_arg);

                sf::Color sfColor((255 - (iter * 16)) % 255, (255 - (iter * 16)) % 255, (255 - (iter * 16)) % 255);

                image.setPixel(x, y, sfColor);
            }
        }

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

int main(int argc, const char* argv[])
{
    //if (argc > 2) processArgs(int argc, const char* argv[]);
    if (argc >= 2) printf("usage\n %s\n", argv[0]);
    drawSet();
    return 0;
}
