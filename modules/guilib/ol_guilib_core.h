//
// Created by Orion Letizi on 1/10/24.
//

#ifndef OL_DSP_OL_GUILIB_CORE_H
#define OL_DSP_OL_GUILIB_CORE_H

#include <string>
#include <vector>
#include "corelib/ol_corelib.h"
#include "ctllib/ol_ctllib.h"

namespace ol::gui {

    enum Color {
        Black,
        White
    };

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

        virtual void DrawCircle(int x, int y, int radius) = 0;

        virtual void DrawPixel(int x, int y) = 0;

        virtual void Print(std::string text, Rectangle area) = 0;
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

        void DrawCircle(int x, int y, int radius) override {
            g_.DrawCircle(x + offset_.x, y + offset_.y, radius);
        }

        void DrawLine(int startX, int startY, int endX, int endY, int line_width) const override {
            g_.DrawLine(startX + offset_.x, startY + offset_.y, endX + offset_.x, endY + offset_.y, line_width);
        }

        void DrawPixel(int x, int y) override {
            g_.DrawPixel(x + offset_.x, y + offset_.y);
        }

        void Print(const std::string text, const Rectangle area) override {
            Rectangle offset_area{
                    Point{
                            area.point.x + offset_.x,
                            area.point.y + offset_.y
                    },
                    area.dimension
            };
            g_.Print(text, offset_area);
        }


    private:
        Graphics &g_;
        Point offset_{};
    };


    class Component {
    public:
        virtual void SetSize(int width, int height) {
            width_ = width;
            height_ = height;
            Resized();
        };

        virtual void SetSize(Dimension dimension) {
            SetSize(dimension.width, dimension.height);
        }

        [[nodiscard]] virtual int GetWidth() const { return width_; }

        [[nodiscard]] virtual int GetHeight() const { return height_; }

        virtual Component *SetFixedSize(Dimension dimension) {
            SetFixedWidth(dimension.width)->SetFixedHeight(dimension.height);
            return this;
        }

        [[nodiscard]] virtual int GetFixedWidth() const { return fixed_width_; }

        virtual Component *SetFixedWidth(int w) {
            fixed_width_ = w;
            return this;
        }

        [[nodiscard]] virtual int GetFixedHeight() const { return fixed_height_; }

        virtual Component *SetFixedHeight(int h) {
            fixed_height_ = h;
            return this;
        }

        virtual void Resized() = 0;

        virtual void Paint(Graphics &g) = 0;

    private:
        int width_ = 0;
        int height_ = 0;
        int fixed_width_ = 0;
        int fixed_height_ = 0;
    };

    struct Border {
        int width_left = 0;
        int width_top = 0;
        int width_right = 0;
        int width_bottom = 0;
    };

    class Box : public Component {

    public:
        explicit Box(Component *child) : child_(child) {}

        [[nodiscard]] int GetFixedWidth() const override {
            return child_->GetFixedWidth() ? child_->GetFixedWidth() + offset_right_ + offset_left_ : 0;
        }

        [[nodiscard]] int GetFixedHeight() const override {
            return child_->GetFixedHeight() ? child_->GetFixedHeight() + offset_top_ + offset_bottom_ : 0;
        }

        void Resized() override {
            offset_left_ = margin_left_ + padding_left_;
            offset_top_ = margin_top_ + padding_top_;
            offset_right_ = margin_right_ + padding_right_;
            offset_bottom_ = margin_bottom_ + padding_bottom_;
            child_->SetSize(GetWidth() - (offset_left_ + offset_right_), GetHeight() - (offset_top_ + offset_bottom_));
            child_->Resized();
        }

        void Paint(Graphics &g) override {
            auto og = OffsetGraphics(g, Point{offset_left_, offset_top_});
            if (border_.width_left > 0) {
                g.DrawLine(margin_left_, margin_top_, margin_left_, GetHeight() - margin_bottom_, border_.width_left);
            }
            if (border_.width_top > 0) {
                g.DrawLine(margin_left_, margin_top_, GetWidth() - margin_right_, margin_top_, border_.width_top);
            }
            if (border_.width_right > 0) {
                g.DrawLine(GetWidth() - margin_right_, margin_top_, GetWidth() - margin_right_,
                           GetHeight() - margin_bottom_, border_.width_right);
            }
            if (border_.width_bottom > 0) {
                g.DrawLine(margin_left_, GetHeight() - margin_bottom_, GetWidth() - margin_right_,
                           GetHeight() - margin_bottom_,
                           border_.width_bottom);
            }
            child_->Paint(og);
        }

        Box *SetBorder(Border border) {
            border_ = border;
            return this;
        }

        [[nodiscard]] int GetOffsetLeft() const { return offset_left_; }

        [[nodiscard]] int GetOffsetTop() const { return offset_top_; }

        [[nodiscard]] int GetOffsetRight() const { return offset_right_; }

        [[nodiscard]] int GetOffsetBottom() const { return offset_bottom_; }

        [[nodiscard]] int GetMarginLeft() const { return margin_left_; }

        [[nodiscard]] int GetOffsetVertical() const {
            return GetOffsetTop() + GetOffsetBottom();
        }

        [[nodiscard]] int GetOffsetHorizontal() const {
            return GetOffsetLeft() + GetOffsetRight();
        }

        Box *SetMargin(int m) {
            return SetMarginLeft(m)->SetMarginTop(m)->SetMarginRight(m)->SetMarginBottom(m);
        }

        Box *SetMarginLeft(int m) {
            margin_left_ = m;
            Resized();
            return this;
        }

        [[nodiscard]] int GetMarginTop() const { return margin_top_; }

        Box *SetMarginTop(int m) {
            margin_top_ = m;
            Resized();
            return this;
        }

        [[nodiscard]] int GetMarginRight() const { return margin_right_; }

        Box *SetMarginRight(int m) {
            margin_right_ = m;
            Resized();
            return this;
        }

        [[nodiscard]] int GetMarginBottom() const { return margin_bottom_; }

        Box *SetMarginBottom(int m) {
            margin_bottom_ = m;
            Resized();
            return this;
        }

        Box *SetPadding(int p) {
            return SetPaddingLeft(p)->SetPaddingTop(p)->SetPaddingRight(p)->SetPaddingBottom(p);
        }

        [[nodiscard]] int GetPaddingLeft() const { return padding_left_; }

        Box *SetPaddingLeft(int p) {
            padding_left_ = p;
            Resized();
            return this;
        }

        [[nodiscard]] int GetPaddingTop() const { return padding_top_; }

        Box *SetPaddingTop(int p) {
            padding_top_ = p;
            Resized();
            return this;
        }

        [[nodiscard]] int GetPaddingRight() const { return padding_right_; }

        Box *SetPaddingRight(int p) {
            padding_right_ = p;
            Resized();
            return this;
        }

        [[nodiscard]] int GetPaddingBottom() const { return padding_bottom_; }

        Box *SetPaddingBottom(int p) {
            padding_bottom_ = p;
            Resized();
            return this;
        }

        void SetOutsideOffset(Point offset) {
            outside_offset_ = offset;
        }

        Point GetOutsideOffset() const {
            return outside_offset_;
        }

    private:
        int margin_left_ = 0;
        int margin_top_ = 0;
        int margin_right_ = 0;
        int margin_bottom_ = 0;

        int padding_left_ = 0;
        int padding_top_ = 0;
        int padding_right_ = 0;
        int padding_bottom_ = 0;

        int offset_left_ = 0;
        int offset_top_ = 0;
        int offset_right_ = 0;
        int offset_bottom_ = 0;

        Border border_{};
        Component *child_;
        Point outside_offset_{};
    };

    class Font {

    public:
        explicit Font(const int size) : size_(size) {}

        [[nodiscard]] int GetSize() const { return size_; }

        [[nodiscard]] int GetLineHeight() const {
            return size_;
        }

    private:
        int size_;
    };

    class Text : public Component {
    public:
        explicit Text(Font &font, std::string text) : font_(font), text_(text) {}

        void Resized() override {
            area_.dimension.width = GetWidth();
            area_.dimension.height = GetHeight();
        }

        void Paint(Graphics &g) override {
            g.Print(text_, area_);
        }

        virtual void SetText(std::string text) {
            text_ = text;
        }

        [[nodiscard]] int GetHeight() const override {
            return GetFixedHeight();
        }

        [[nodiscard]] int GetFixedHeight() const override {
            return font_.GetLineHeight();
        }

    private:
        Font &font_;
        std::string text_;
        Rectangle area_{Point{0, 0}, Dimension{0, 0}};
    };


    class TextFactory {
    public:
        virtual Text *NewText(std::string string) = 0;
    };

    struct LayoutProperties {
        enum Direction {
            HORIZONTAL, VERTICAL
        };
        enum Halign {
            LEFT, CENTER, RIGHT
        };
        enum Valign {
            TOP, MIDDLE, BOTTOM
        };

        Direction direction = Direction::HORIZONTAL;
        Halign halign = Halign::LEFT;
        Valign valign = Valign::TOP;
        int spacing = 0;

    };

    class Layout : public Component {
    public:

        explicit Layout(LayoutProperties properties = LayoutProperties{}) : Layout(0, 0, properties) {}

        explicit Layout(Dimension viewport, LayoutProperties properties = LayoutProperties{}) : Layout(viewport.width,
                                                                                                       viewport.height,
                                                                                                       properties) {}

        explicit Layout(int width, int height, LayoutProperties properties = LayoutProperties{}) : properties_(
                properties) {
            Component::SetSize(width, height);
        }


        Layout *Add(Component *child) {
            if (child != nullptr) {
                children_.push_back(child);
                boxes_.push_back(new Box(child));
                Resized();
            }
            return this;
        }

        void Clear() {
            children_.clear();
            Resized(); // XXX: probably don't need this
        }

        void Paint(Graphics &g) override {
            for (auto b: boxes_) {
                OffsetGraphics og(g, b->GetOutsideOffset());
                b->Paint(og);
            }
        }

        void resizedVertical() {

            int fixed_width = 0;
            int fixed_height_sum = 0;
            int fixed_count = 0;
            int dynamic_count = 0;
            for (auto c: children_) {
                fixed_width = std::max(fixed_width, c->GetFixedWidth());
                fixed_height_sum += c->GetFixedHeight();
                if (c->GetFixedHeight()) { fixed_count++; } else { dynamic_count++; }
            }
            int fixed_height_spacing_sum = int(
                    properties_.spacing * (fixed_count * (fixed_count > 0 ? fixed_count - 1 : 0)));
            fixed_height_sum += fixed_height_spacing_sum;

            int dynamic_height_spacing_sum = properties_.spacing * (dynamic_count * (dynamic_count > 0 ? dynamic_count -1 : 0));
            Dimension dynamic_size{
                    std::max(fixed_width, GetWidth()),
                    (GetHeight() - fixed_height_sum - dynamic_height_spacing_sum) / (dynamic_count > 0 ? dynamic_count : 1)
            };
            DPRINTF("layout resized     : dynamic w: %d, h: %d\n", dynamic_size.width, dynamic_size.height);
            DPRINTF("  spacing sum      : %d\n", fixed_height_spacing_sum);
            DPRINTF("  fixed width      : %d\n", fixed_width);
            DPRINTF("  fixed height sum : %d\n", fixed_height_sum);
            DPRINTF("  fixed_count      : %d\n", fixed_count);
            DPRINTF("  dynamic_count    : %d\n", dynamic_count);
            Point offset{};
            for (auto b: boxes_) {
                // calculate horizontal offset based on halign
                int box_width = b->GetFixedWidth() ? b->GetFixedWidth() : GetWidth();

                if (properties_.halign == LayoutProperties::CENTER) {
                    // center halign, set the X offset to half the difference between the horizontal size of the container
                    // and the horizontal size of the component.
                    offset.x = (GetWidth() - box_width) / 2;
                } else if (properties_.halign == LayoutProperties::RIGHT) {
                    // right halign, set the X offset to the entire difference betweent the horizontal size of the container
                    // and the horizontal size of the component.
                    offset.x = GetWidth() - box_width;
                }
                DPRINTF("  offset: x: %d, y: %d\n", offset.x, offset.y);
                b->SetOutsideOffset(offset);
                b->SetSize(dynamic_size);
                b->Resized();
                // set vertical offset for the next component in the column
                offset.y += properties_.spacing + (b->GetFixedHeight() ? b->GetFixedHeight() : dynamic_size.height);
            }
        }

        void resizedHorizontal() {}

        void Resized() override {
            if (properties_.direction == LayoutProperties::VERTICAL) {
                resizedVertical();
            } else {
                resizedHorizontal();
            }
        }

        Layout *SetVertical() {
            properties_.direction = LayoutProperties::VERTICAL;
            return this;
        }

        Layout *SetHorizontal() {
            properties_.direction = LayoutProperties::HORIZONTAL;
            return this;
        }

        Layout *SetHalign(LayoutProperties::Halign h) {
            properties_.halign = h;
            return this;
        }

        Layout *SetValign(LayoutProperties::Valign v) {
            properties_.valign = v;
            return this;
        }

        Layout *SetSpacing(int spacing) {
            DPRINTF("SetSpacing: %d\n", spacing);
            properties_.spacing = spacing;
            Resized();
        }

    private:
        std::vector<Component *> children_;
        std::vector<Box *> boxes_;
        LayoutProperties properties_;
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
            meter_.SetLevel(control_.GetFloatValue());
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
