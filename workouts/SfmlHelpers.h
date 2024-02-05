//
// Created by Orion Letizi on 2/4/24.
//

#ifndef OL_DSP_SFMLHELPERS_H
#define OL_DSP_SFMLHELPERS_H
#include <SFML/Graphics.hpp>
#include "guilib/ol_guilib.h"
namespace ol::gui {

    class Circle : public Component {
    public:
        void Resized() override {
            radius_ = std::min(GetWidth(), GetHeight()) / 2;
        }

        void Paint(Graphics &g) override {
            g.DrawCircle(0, 0, radius_);
        }

    private:
        int radius_ = 0;
    };

    class SfmlText : public Text {
    public:
        SfmlText(sf::Font &native_font, Font &gui_font, int font_size, const std::string &text_string)
                : Text(gui_font, text_string),
                  native_font_(native_font),
                  font_size_(font_size),
                  text_string_(text_string) {}

        void Resized() override {
            Text::Resized();
            DPRINTF("  SfmlText resized: w: %d, h: %d, fixed w: %d, h: %d\n", GetWidth(), GetHeight(), GetFixedWidth(), GetFixedHeight());
        }

        int GetFixedHeight() const override {
            return int(native_text_.getLocalBounds().height);
        }

        int GetFixedWidth() const override {
            return int(native_text_.getLocalBounds().width);
        }

    private:
        sf::Font &native_font_;
        int font_size_;
        std::string text_string_;
        sf::Text native_text_ = sf::Text(text_string_, native_font_, font_size_);
    };

    class SfmlTextFactory : public TextFactory {
    public:

        SfmlTextFactory(sf::Font &font, int font_size) : native_font_(font), font_size_(font_size) {}

        Text *NewText(std::string text_string) override {
            auto rv = new SfmlText(native_font_, gui_font_, font_size_, text_string);
            created_.push_back(rv);
            return rv;
        }

    private:
        sf::Font &native_font_;
        std::vector<Text *> created_{};
        int font_size_;
        Font gui_font_ = Font(font_size_);
    };

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

        void DrawCircle(int x, int y, int radius) override {
            sf::CircleShape circ(radius);
            circ.setPosition(x, y);
            circ.setOutlineColor(color_);
            circ.setOutlineThickness(1);
            window_.draw(circ);
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
            t.setPosition(area.point.x, area.point.y);
            window_.draw(t);
        }

    private:

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

        sf::Color color_ = sf::Color::Black;
        sf::RenderWindow &window_;
        sf::Font &font_;
        int font_size_;
    };
}
#endif //OL_DSP_SFMLHELPERS_H
