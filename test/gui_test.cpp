//
// Created by Orion Letizi on 1/9/24.
//
#include "gtest/gtest.h"
#include "guilib/guilib.h"

using namespace ol::gui;
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