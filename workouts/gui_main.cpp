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
            DPRINTF("  SfmlText resized: %d, %d\n", GetWidth(), GetHeight());
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

class BasicApp {
public:

    BasicApp(ol::app::synth::SynthApp &app, ol::app::synth::SynthMediumGui &gui) : app_(app), gui_(gui) {}

    BasicApp *handleKeyPressed(sf::Event &event) {
        switch (event.key.code) {
            case sf::Keyboard::Key::A:
                gui_.SelectMainScreen();
                break;
            case sf::Keyboard::Key::S:
                gui_.SelectFilterScreen();
                break;
            case sf::Keyboard::Key::D:
                gui_.SelectAmpScreen();
                break;
            case sf::Keyboard::Key::F:
                gui_.SelectFxScreen();
                break;
            case sf::Keyboard::Key::G:
                gui_.SelectModScreen();
                break;
            default:
                break;

        }
        return this;
    }

private:
    ol::app::synth::SynthApp &app_;
    ol::app::synth::SynthMediumGui &gui_;
};


int main() {
    // create the window
    const int width = 320;
    const int height = 240;
    sf::RenderWindow window(sf::VideoMode(width, height), "My window");
    sf::Font font = sf::Font();
    auto font_path = "/Users/orion/Library/Fonts/Architect Bold.ttf";
    if (!font.loadFromFile(font_path)) {
        return 3;
    }
    font.setSmooth(true);
    auto g = ol::gui::SfmlGraphics(window, font, 14);

    ol::gui::SfmlTextFactory text_factory(font, 14);
    ol::app::synth::SynthConfig config{};
/*
    ol::app::synth::SynthMediumGui gui(config, text_factory);
    ol::app::synth::SynthApp app(config, gui);

    BasicApp basic_app(app, gui);

    ol::gui::Box box(&gui);
    box.SetMargin(5);
    box.SetSize(width, height);
    box.Resized();
*/
    ol::ctl::Control control1(1, 0.5f);
    ol::ctl::Control control2(1, 0.25f);
    ol::app::synth::Fader fader_fixed_1(text_factory.NewText("Fixed 1"), control1);
    fader_fixed_1.SetFixedSize(ol::gui::Dimension{30, 45});
    ol::app::synth::Fader fader_fixed_2(text_factory.NewText("Fixed 2 with extra stuff"), control2);
    fader_fixed_2.SetFixedSize(ol::gui::Dimension{30, 45});
    ol::app::synth::Fader fader_dynamic_1(text_factory.NewText("Dynamic 1"), control1);
    ol::app::synth::Fader fader_dynamic_2(text_factory.NewText("Dynamic 2"), control2);

    ol::gui::Layout layout{};
    //layout.SetHorizontal();
    layout.SetHalign(ol::gui::LayoutProperties::CENTER);
    layout.SetSpacing(10);
    ol::gui::Circle circle{};
    ol::gui::Box box(&circle);
    box.SetBorder(ol::gui::Border{1,1,1, 1});
    layout.Add(&box);

//    layout.Add(&fader_fixed_1);
//    layout.Add(&fader_fixed_2);
//    layout.Add(&fader_dynamic_1);
//    layout.Add(&fader_dynamic_2);

    DPRINTLN("");
    DPRINTLN("========= SETTING LAYOUT SIZE =========");
    DPRINTLN("");
    layout.SetVertical();
    layout.SetSize(width, height);
    layout.Resized();
    // run the program as long as the window is open
    while (window.isOpen()) {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event{};
        while (window.pollEvent(event)) {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                fprintf(stderr, "Mouse! %d, %d\n", event.mouseButton.x, event.mouseButton.y);
            }
            if (event.type == sf::Event::KeyPressed) {
//                basic_app.handleKeyPressed(event);
            }
        }

        // clear the window with background color
        window.clear(sf::Color::White);
        layout.Paint(g);
        // draw everything here...
        // window.draw(...);

//        box.Paint(g);

        // end the current frame
        window.display();
    }

    return 0;
}