#include <SFML/Graphics.hpp>
#include "guilib/ol_guilib.h"

namespace ol::gui {
    class SfmlGraphics : public Graphics {
    public:

        SfmlGraphics(sf::RenderWindow &window, sf::Font &font, int font_size) : window_(window), font_(font),
                                                                                font_size_(font_size) {}

        void DrawLine(int startX, int startY, int endX, int endY, int line_width) const override {
            sf::VertexArray line(sf::Lines, 2);
            line[0].position = sf::Vector2f(startX, startY);
            line[0].color = color_;
            line[1].position = sf::Vector2f(endX, endY);
            line[1].color = color_;
            window_.draw(line);
        }

        void DrawRect(int x, int y, int width, int height, int line_width) override {
            sf::VertexArray rect(sf::LineStrip, 5);
            plotRect(rect, x, y, width, height);
        }

        void FillRect(int x, int y, int width, int height) override {
            sf::VertexArray rect(sf::TriangleStrip, 5);
            plotRect(rect, x, y, width, height);
        }

        void plotRect(sf::VertexArray &rect, float x, float y, float width, float height) {
            rect[0].position = sf::Vector2f(float(x), float(y));
            rect[1].position = sf::Vector2f(float(x + width), float(y));
            rect[2].position = sf::Vector2f(float(x + width), float(y + height));
            rect[3].position = sf::Vector2f(float(x), float(y + height));
            rect[4].position = sf::Vector2f(float(x), float(y));
            for (int i = 0; i < 4; i++) {
                rect[i].color = color_;
            }
            window_.draw(rect);
        }

        void DrawPixel(int x, int y) override {
            sf::VertexArray pixel(sf::Points, 1);
            pixel[0].position = sf::Vector2f(x, y);
            pixel[0].color = color_;
            window_.draw(pixel);
        }

        void Print(std::string text, Rectangle area) override {
            sf::Text t(text, font_, font_size_);
            t.setFillColor(color_);
//            t.setOutlineColor(color_);
            t.setPosition(area.point.x, area.point.y);
            window_.draw(t);
        }

    private:
        sf::Color color_ = sf::Color::Black;
        sf::RenderWindow &window_;
        sf::Font &font_;
        int font_size_;
    };
}

int main() {
    // create the window
    const int width = 320;
    const int height = 240;
    sf::RenderWindow window(sf::VideoMode(width + 10, height + 10), "My window");
    sf::Font font = sf::Font();
//    auto font_path = "/System/Library/Fonts/HelveticaNeue.ttc";
    auto font_path = "/Users/orion/Library/Fonts/Inconsolata Nerd Font Complete.otf";
//    auto font_path = "/Users/orion/Library/Fonts/ArchitectRegular-D0XR.ttf";
//    auto font_path = "/Users/orion/Library/Fonts/Woolkarth-Bold Bold.ttf";
//    auto font_path = "/Users/orion/Library/Fonts/Flux Architect Regular.ttf";
    if (!font.loadFromFile(font_path)) {
        return 3;
    }
    font.setSmooth(true);
    auto g = ol::gui::SfmlGraphics(window, font, 16);

    ol::app::synth::SynthConfig config{};
    ol::app::synth::SynthMediumGui gui(config);
    ol::gui::Box box(&gui);
    box.SetMargin(5);
    box.SetSize(width, height);
    box.Resized();
    // run the program as long as the window is open
    while (window.isOpen()) {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event)) {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::White);

        // draw everything here...
        // window.draw(...);

        box.Paint(g);
        // end the current frame
        window.display();
    }

    return 0;
}