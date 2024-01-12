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
        virtual void DrawLine(int startX, int startY, int endX, int endY, int line_width) const = 0;

        virtual void DrawRect(int x, int y, int width, int height, int line_width) = 0;

        void DrawRect(Rectangle rect) {
            DrawRect(rect.point.x, rect.point.y, rect.dimension.width, rect.dimension.height, 1);
        }

        virtual void FillRect(int x, int y, int width, int height) = 0;

        void FillRect(Rectangle rect) {
            FillRect(rect.point.x, rect.point.y, rect.dimension.width, rect.dimension.height);
        }
    };

    class OffsetGraphics : public Graphics {
    public:
        OffsetGraphics(Graphics &g, Point offset) : g_(g), offset_(offset) {}

        void DrawRect(int x, int y, int width, int height, int line_width) override {
            g_.DrawRect(x + offset_.x, y + offset_.y, width, height, line_width);
        }

        void FillRect(int x, int y, int width, int height) override {
            g_.FillRect(x + offset_.x, y + offset_.y, width, height);
        }

        void DrawLine(int startX, int startY, int endX, int endY, int line_width) const override {
            g_.DrawLine(startX + offset_.x, startY + offset_.y, endX + offset_.x, endY + offset_.y, line_width);
        }

    private:
        Point offset_{};
        Graphics &g_;
    };


    class Component {
    public:
        virtual void SetSize(int width, int height) {
            width_ = width;
            height_ = height;
        };

        virtual void SetSize(Dimension dimension) {
            SetSize(dimension.width, dimension.height);
        }

        int GetWidth() { return width_; }

        int GetHeight() { return height_; }

        virtual void Resized() = 0;

        virtual void Paint(Graphics &g) = 0;

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
            Component::SetSize(width, height);
        }

        explicit Layout(Dimension viewport, LayoutDirection direction = Vertical) : Layout(viewport.width,
                                                                                           viewport.height,
                                                                                           direction) {}

        void Add(Component *child) {
            children_.push_back(child);
        }

        void Paint(Graphics &g) override {
            Point offset{};
            for (auto c: children_) {
                OffsetGraphics og(g, offset);
                c->Paint(og);
                offset.x += direction_ == Horizontal ? child_size.width : 0;
                offset.y += direction_ == Vertical ? child_size.height : 0;
            }
        }

        void Resized() override {
            child_size.width =
                    direction_ == Vertical ? GetWidth() : GetWidth() / int(children_.empty() ? 1 : children_.size());
            child_size.height =
                    direction_ == Vertical ? GetHeight() / int(children_.empty() ? 1 : children_.size()) : GetHeight();
            for (auto c: children_) {
                c->SetSize(child_size);
                c->Resized();
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

        void SetLevel(const t_sample level) {
            level_ = level;
            updateDimensions();
        }

        void Paint(Graphics &g) override {
            g.DrawRect(rect_max_);
            g.FillRect(rect_level_);
        }

        void Resized() override {
            updateDimensions();
        }

    private:
        void updateDimensions() {
            rect_level_.dimension.width = int(core::scale(level_, 0, 1, 0, float(GetWidth()), 1));
            rect_level_.dimension.height = GetHeight();
            rect_max_.dimension.width = GetWidth();
            rect_max_.dimension.height = GetHeight();
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

        void Paint(Graphics &g) override {
            meter_.SetLevel(control_.scaledValue());
            meter_.Paint(g);
        }

        void Resized() override {
            meter_.SetSize(GetWidth(), GetHeight());
            meter_.Resized();
        }

    private:
        ol::ctl::Control &control_;
        Meter meter_;
    };
}

#endif //OL_DSP_OL_GUILIB_CORE_H
