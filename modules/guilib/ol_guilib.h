//
// Created by Orion Letizi on 1/9/24.
//

#ifndef OL_DSP_OL_GUILIB_H
#define OL_DSP_OL_GUILIB_H

#define TEENSY_DEBUG

#ifdef TEENSY_DEBUG
#define DPRINTLN(X) Serial.println(X);
#define DPRINTF(...) Serial.printf(__VA_ARGS__);
#endif

#include <vector>
#include "corelib/ol_corelib.h"

namespace ol::gui {

    struct Point {
        int x = 0;
        int y = 0;
    };

    struct Dimension {
        int width = 0;
        int height = 0;
    };

    struct Rectangle {
        Point point{};
        Dimension dimension{};
    };

    class Graphics {
    public:
        virtual void drawRect(int x, int y, int width, int height, int line_width) = 0;

        virtual void fillRect(int x, int y, int width, int height) = 0;
    };

    class OffsetGraphics : public Graphics {
    public:
        OffsetGraphics(Graphics &g, Point offset) : g_(g), offset_(offset){}

        void drawRect(int x, int y, int width, int height, int line_width) override {
            g_.drawRect(x + offset_.x, y + offset_.y, width, height, line_width);
        }

        void fillRect(int x, int y, int width, int height) override {
            g_.fillRect(x + offset_.x, y + offset_.y, width, height);
        }

    private:
        Point offset_{};
        Graphics &g_;
    };


    class Component {
    public:
        virtual void setSize(int width, int height) {
            width_ = width;
            height_ = height;
        };

        virtual void setSize(Dimension dimension) {
            setSize(dimension.width, dimension.height);
        }

        int getWidth() { return width_; }

        int getHeight() { return height_; }

        virtual void paint(Graphics &g) = 0;

    private:
        int width_ = 0;
        int height_ = 0;
    };

    enum LayoutDirection {
        Horizontal,
        Vertical
    };

    class Layout : public Component {
    public:
        Layout() : Layout(Vertical) {}

        explicit Layout(LayoutDirection direction) : Layout(0, 0, direction) {}

        explicit Layout(int width, int height, LayoutDirection direction = Vertical) : direction_(direction) {
            Component::setSize(width, height);
        }

        explicit Layout(Dimension viewport, LayoutDirection direction = Vertical) : Layout(viewport.width,
                                                                                           viewport.height,
                                                                                           direction) {}

        void setSize(int width, int height) override {
            Component::setSize(width, height);
            updateSize();
        }

        void add(Component *child) {
            children_.push_back(child);
        }

        void paint(Graphics &g) override {
            Point offset{};

            Dimension child_size{};
            childSize(child_size);
            updateSize();
            for (auto c: children_) {
                OffsetGraphics og(g, offset);
                c->paint(og);
                offset.x += direction_ == Horizontal ? child_size.width : 0;
                offset.y += direction_ == Vertical ? child_size.height : 0;
            }
        }

    private:
        Dimension &childSize(Dimension &dimension) {
            dimension.width = direction_ == Vertical ? getWidth() : getWidth() / int(children_.size());
            dimension.height = direction_ == Vertical ? getHeight() / int(children_.size()) : getHeight();
            return dimension;
        }

        void updateSize() {
            Dimension child_size{};
            childSize(child_size);
            for (auto c: children_) {
                c->setSize(child_size);
            }
        }

        std::vector<Component *> children_;
        LayoutDirection direction_;
    };

    class Meter : public Component {
    public:
        explicit Meter(t_sample level = 0) : level_(level) {}

        void setLevel(t_sample level) { level_ = level; }

        void paint(Graphics &g) override {
            g.drawRect(0, 0, getWidth(), getHeight(), 1);
            g.fillRect(0, 0, int(core::scale(level_, 0, 1, 0, float(getWidth()), 1)), getHeight());
        }

    private:
        t_sample level_;
    };
}
#endif //OL_DSP_OL_GUILIB_H
