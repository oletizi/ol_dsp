//
// Created by Orion Letizi on 1/10/24.
//

#ifndef OL_DSP_OL_GUILIB_CORE_H
#define OL_DSP_OL_GUILIB_CORE_H

#include <vector>
#include "corelib/ol_corelib.h"
#include "ctllib/ol_ctllib.h"

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
        virtual void drawLine(int startX, int startY, int endX, int endY, int line_width) const = 0;

        virtual void drawRect(int x, int y, int width, int height, int line_width) = 0;

        void drawRect(Rectangle rect) {
            drawRect(rect.point.x, rect.point.y, rect.dimension.width, rect.dimension.height, 1);
        }

        virtual void fillRect(int x, int y, int width, int height) = 0;

        void fillRect(Rectangle rect) {
            fillRect(rect.point.x, rect.point.y, rect.dimension.width, rect.dimension.height);
        }
    };

    class OffsetGraphics : public Graphics {
    public:
        OffsetGraphics(Graphics &g, Point offset) : g_(g), offset_(offset) {}

        void drawRect(int x, int y, int width, int height, int line_width) override {
            g_.drawRect(x + offset_.x, y + offset_.y, width, height, line_width);
        }

        void fillRect(int x, int y, int width, int height) override {
            g_.fillRect(x + offset_.x, y + offset_.y, width, height);
        }

        void drawLine(int startX, int startY, int endX, int endY, int line_width) const override {
            g_.drawLine(startX + offset_.x, startY + offset_.y, endX + offset_.x, endY + offset_.y, line_width);
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

        virtual void resized() = 0;

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

        void add(Component *child) {
            children_.push_back(child);
        }

        void paint(Graphics &g) override {
            Point offset{};
            for (auto c: children_) {
                OffsetGraphics og(g, offset);
                c->paint(og);
                offset.x += direction_ == Horizontal ? child_size.width : 0;
                offset.y += direction_ == Vertical ? child_size.height : 0;
            }
        }

        void resized() override {
            child_size.width =
                    direction_ == Vertical ? getWidth() : getWidth() / int(children_.empty() ? 1 : children_.size());
            child_size.height =
                    direction_ == Vertical ? getHeight() / int(children_.empty() ? 1 : children_.size()) : getHeight();
            for (auto c: children_) {
                c->setSize(child_size);
                c->resized();
            }
        }

        void setDirection(LayoutDirection direction) {
            direction_ = direction;
        }

    private:
        Dimension child_size{};
        std::vector<Component *> children_;
        LayoutDirection direction_;
    };

    class Meter : public Component {
    public:
        explicit Meter(t_sample level = 0) : level_(level) {}

        void setLevel(const t_sample level) {
            level_ = level;
            updateDimensions();
        }

        void paint(Graphics &g) override {
            g.drawRect(rect_max_);
            g.fillRect(rect_level_);
        }

        void resized() override {
            updateDimensions();
        }

    private:
        void updateDimensions() {
            rect_level_.dimension.width = int(core::scale(level_, 0, 1, 0, float(getWidth()), 1));
            rect_level_.dimension.height = getHeight();
            rect_max_.dimension.width = getWidth();
            rect_max_.dimension.height = getHeight();
        }

        t_sample level_;
        Rectangle rect_max_;
        Rectangle rect_level_;
    };

    /**
     * Meter component backed by the value of a Control.
     */
    class ControlMeter : public Component {
    public:
        explicit ControlMeter(ol::ctl::Control &control) : control_(control) {}

        void paint(Graphics &g) override {
            meter_.setLevel(control_.scaledValue());
            meter_.paint(g);
        }

        void resized() override {
            meter_.setSize(getWidth(), getHeight());
            meter_.resized();
        }

    private:
        ol::ctl::Control &control_;
        Meter meter_;
    };
}

#endif //OL_DSP_OL_GUILIB_CORE_H
