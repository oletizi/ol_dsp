#include "SfmlHelpers.h"

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

    ol::app::synth::SynthMediumGui gui(config, text_factory);
    ol::app::synth::SynthApp app(config, gui);

    BasicApp basic_app(app, gui);

    ol::gui::Box box(&gui);
    box.SetMargin(5);
    box.SetSize(width, height);
    box.Resized();

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
                basic_app.handleKeyPressed(event);
            }
        }

        window.clear(sf::Color::White);
        box.Paint(g);
        window.display();
    }

    return 0;
}