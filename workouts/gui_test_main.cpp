//
// Created by Orion Letizi on 2/4/24.
//
#include "SfmlHelpers.h"

int main() {
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

    ol::ctl::Control control1(1, 0.5f);
    ol::ctl::Control control2(1, 0.25f);
    ol::app::synth::Fader fader_fixed_1(text_factory.NewText("Fixed 1"), control1);
    fader_fixed_1.SetFixedSize(ol::gui::Dimension{30, 45});
    ol::app::synth::Fader fader_fixed_2(text_factory.NewText("Fixed 2 with extra stuff"), control2);
    fader_fixed_2.SetFixedSize(ol::gui::Dimension{30, 45});
    ol::app::synth::Fader fader_dynamic_1(text_factory.NewText("Dynamic 1"), control1);
    ol::app::synth::Fader fader_dynamic_2(text_factory.NewText("Dynamic 2"), control2);
    ol::app::synth::Dial dial_fixed_1(text_factory.NewText("Fixed 1"), control1);
    dial_fixed_1.SetFixedSize(ol::gui::Dimension{30, 45});
    ol::app::synth::Dial dial_dynamic_1(text_factory.NewText("Dynamic 1"), control2);

    ol::gui::Layout layout{};
//    layout.SetVertical();
    layout.SetHorizontal();
    layout.SetHalign(ol::gui::LayoutProperties::CENTER);
//    layout.SetHalign(ol::gui::LayoutProperties::RIGHT);
    layout.SetValign(ol::gui::LayoutProperties::MIDDLE);
    layout.SetSpacing(10);

    layout.Add(&fader_fixed_1);
//    layout.Add(&fader_fixed_2);
//    layout.Add(&fader_dynamic_1);
//    layout.Add(&fader_dynamic_2);
    layout.Add(&dial_fixed_1);
    layout.Add(&dial_dynamic_1);

    layout.SetSize(width, height);
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
                fprintf(stderr, "Key! %d\n", event.key.code);
            }
        }
        window.clear(sf::Color::White);
        layout.Paint(g);
        window.display();
    }

    return 0;

}