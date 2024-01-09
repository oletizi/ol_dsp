//
// Created by Orion Letizi on 1/9/24.
//
#include "gtest/gtest.h"
#include "gtest/fakeit.hpp"
#include "guilib/guilib.h"

using namespace ol::gui;
using namespace fakeit;
TEST(guilib, basics) {
    Rectangle viewport{
            Point{0, 0},
            128,
            64
    };

    Layout layout;
    layout.setSize(viewport);
    EXPECT_EQ(layout.getWidth(), viewport.width);
    EXPECT_EQ(layout.getHeight(), viewport.height);
}


TEST(guilib, Meter) {
    Meter meter;
    Mock<Graphics> mock_g;
    Fake(Method(mock_g, drawRect));
    Fake(Method(mock_g, fillRect));
    Graphics *g = &mock_g.get();
    meter.setSize(128, 64);
    meter.setLevel(0.5f);
    meter.paint(*g);

    Verify(Method(mock_g, drawRect).Using(0, 0, 128, 64, 1));
    Verify(Method(mock_g, fillRect).Using(0, 0, 128 / 2, 64));
}

TEST(guilib, Layout) {
    int width = 128;
    int height = 64;
    Rectangle viewport{Point{}, width, height};
    Mock<Component> mock_c;
    When(Method(mock_c, paint)).AlwaysDo([](Graphics &g) { g.fillRect(0, 0, 10, 10); });
    Component *component1 = &mock_c.get();


    Mock<Graphics> mock_g;
    Fake(Method(mock_g, drawRect));
    Fake(Method(mock_g, fillRect));
    Graphics *graphics = &mock_g.get();

    Layout layout(viewport);
    layout.add(component1);
    layout.add(component1);
    layout.paint(*graphics);

    // Make sure paint() was called on the component we added to the layout
    Verify(Method(mock_c, paint));
    // Make sure the paint() method made it to the underlying Graphics object
    Verify(Method(mock_g, fillRect));
    // Make sure the offset paint() was called
    Verify(Method(mock_g, fillRect).Using(0, height/2, 10, 10));

}