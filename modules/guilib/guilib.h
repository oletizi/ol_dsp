//
// Created by Orion Letizi on 1/9/24.
//

#ifndef OL_DSP_GUILIB_H
#define OL_DSP_GUILIB_H
#include <vector>

namespace ol::gui {

    struct Point {
        int x = 0;
        int y = 0;
    };

    class Graphics {
    public:
        virtual void drawRect(int x, int y, int width, int height, int line_width) = 0;
        virtual void fillRect(int x, int y, int width, int height) = 0;
    };

    class OffsetGraphics : public Graphics {
    public:
        explicit OffsetGraphics(Graphics &g, Point offset) : g_(g), offset_(offset) {}

        void drawRect(int x, int y, int width, int height, int line_width) override {
            g_.drawRect(x + offset_.x, y + offset_.y, width, height, line_width);
        }

        void fillRect(int x, int y, int width, int height) override {
            g_.fillRect(x + offset_.x, y + offset_.y, width, height);
        }

    private:
        Point offset_;
        Graphics &g_;
    };



    class Component {
    public:
        void setSize(int width, int height) {
            width_ = width;
            height_ = height;
        };

        int getWidth() { return width_; }

        int getHeight() { return height_; }

        virtual void paint(Graphics &g) = 0;

    private:
        int width_ = 0;
        int height_ = 0;
    };

    class Layout : public Component {
    public:
        void add(Component *child) {
            children_.push_back(child);
            const auto width = getWidth();
            const auto height = getHeight() / int(children_.size());
            for (auto c: children_) {
                c->setSize(width, height);
            }
        }

    public:
        void paint(Graphics &g) override {
            Point offset{};
            const auto height = getHeight() / int(children_.size());
            for (auto c: children_) {
                OffsetGraphics og(g, offset);
                c->paint(og);
                offset.y += height;
            }
        }

    private:
        std::vector<Component *> children_;
    };

    class Meter : public Component {
    public:
        explicit Meter(float level = 0) : level_(level) {}

        void setLevel(float level) { level_ = level; }

        void paint(Graphics &g) override {
            g.drawRect(0, 0, getWidth(), getHeight(), 1);
            g.fillRect(0, 0, int(core::scale(level_, 0, 1, 0, float(getWidth()), 1)), getHeight());
        }

    private:
        float level_;
    };
}
#endif //OL_DSP_GUILIB_H
