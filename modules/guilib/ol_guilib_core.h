//
// Created by Orion Letizi on 1/10/24.
//

#ifndef OL_DSP_OL_GUILIB_CORE_H
#define OL_DSP_OL_GUILIB_CORE_H

#include <string>
#include <utility>
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
            fixed_width_ = dimension.width;
            fixed_height_ = dimension.height;
            Resized();
            return this;
        }

        [[nodiscard]] virtual int GetFixedWidth() const { return fixed_width_; }

        virtual Component *SetFixedWidth(int w) {
            return SetFixedSize(Dimension{w, GetFixedHeight()});
        }

        [[nodiscard]] virtual int GetFixedHeight() const { return fixed_height_; }

        virtual Component *SetFixedHeight(int h) {
            return SetFixedSize(Dimension{GetFixedWidth(), h});
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
            int rv = 0;
            if (Component::GetFixedWidth()) {
                rv = Component::GetFixedWidth();
            } else {
                rv = child_->GetFixedWidth() ? child_->GetFixedWidth() + offset_right_ + offset_left_ : 0;
            }
            return rv;
        }

        [[nodiscard]] int GetFixedHeight() const override {
            int rv = 0;
            if (Component::GetFixedWidth()) {
                rv = Component::GetFixedWidth();
            } else {
                rv = child_->GetFixedHeight() ? child_->GetFixedHeight() + offset_top_ + offset_bottom_ : 0;
            }
            return rv;
        }

        void Resized() override {
            offset_left_ = margin_left_ + padding_left_;
            offset_top_ = margin_top_ + padding_top_;
            offset_right_ = margin_right_ + padding_right_;
            offset_bottom_ = margin_bottom_ + padding_bottom_;
            DPRINTF("Box resized: w: %d, h: %d, fixed: h: %d, w: %d\n", GetWidth(), GetHeight(), GetFixedWidth(),
                    GetFixedHeight());
            DPRINTF("  offset : %d, t: %d, r: %d, b:%d\n", offset_left_,
                    offset_top_, offset_right_, offset_bottom_);
            if (GetFixedWidth()) {
                child_->SetFixedWidth(GetFixedWidth() - GetOffsetHorizontal());
            }
            if (GetFixedHeight()) {
                child_->SetFixedHeight(GetFixedHeight() - GetOffsetVertical());
            }
            child_->SetSize(GetWidth() - GetOffsetHorizontal(), GetHeight() - GetOffsetVertical());
        }

        void Paint(Graphics &g) override {
            int width = GetFixedWidth() ? GetFixedWidth() : GetWidth();
            int height = GetFixedHeight() ? GetFixedHeight() : GetHeight();
            auto og = OffsetGraphics(g, Point{offset_left_, offset_top_});
            if (border_.width_left > 0) {
                g.DrawLine(margin_left_, margin_top_, margin_left_, height - margin_bottom_, border_.width_left);
            }
            if (border_.width_top > 0) {
                g.DrawLine(margin_left_, margin_top_, width - margin_right_, margin_top_, border_.width_top);
            }
            if (border_.width_right > 0) {
                g.DrawLine(width - margin_right_, margin_top_, width - margin_right_,
                           height - margin_bottom_, border_.width_right);
            }
            if (border_.width_bottom > 0) {
                g.DrawLine(margin_left_, height - margin_bottom_, width - margin_right_,
                           height - margin_bottom_,
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

        [[nodiscard]] Point GetOutsideOffset() const {
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
        explicit Text(Font &font, std::string text) : font_(font), text_(std::move(text)) {
            Component::SetFixedHeight(font.GetLineHeight());
        }

        void Resized() override {
            area_.dimension.width = GetWidth();
            area_.dimension.height = GetHeight();
        }

        void Paint(Graphics &g) override {
            g.Print(text_, area_);
        }

        virtual void SetText(std::string text) {
            text_ = std::move(text);
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


        void Resized() override {
            DPRINTF("Layout resized: w: %d, h: %d, fixed w: %d, h: %d", GetWidth(), GetHeight(), GetFixedWidth(),
                    GetFixedHeight());
            bool vert = properties_.direction == LayoutProperties::VERTICAL;
            int fixedLengthSum = 0;
            int fixedCount = 0;
            int dynamicCount = 0;
            int width = GetFixedWidth() ? GetFixedWidth() : GetWidth();
            int height = GetFixedHeight() ? GetFixedHeight() : GetHeight();
            for (auto c: children_) {
                fixedLengthSum += vert ? c->GetFixedHeight() : c->GetFixedWidth();
                if ((vert ? c->GetFixedHeight() : c->GetFixedWidth())) { fixedCount++; } else { dynamicCount++; }
            }
            int fixedSpacingSum = int(
                    properties_.spacing * (fixedCount * (fixedCount > 0 ? fixedCount - 1 : 0)));
            fixedLengthSum += fixedSpacingSum;

            int dynamicSpacingSpacingSum =
                    properties_.spacing * (dynamicCount * (dynamicCount > 0 ? dynamicCount - 1 : 0));
            Dimension dynamicSize{
                    vert ? width : (width - fixedLengthSum - dynamicSpacingSpacingSum) /
                                   (dynamicCount > 0 ? dynamicCount : 1),
                    vert ? (height - fixedLengthSum - dynamicSpacingSpacingSum) /
                           (dynamicCount > 0 ? dynamicCount : 1) : height
            };
            Point offset{};
            for (auto b: boxes_) {
                // calculate  offset based on alignment
                int boxWidth = b->GetFixedWidth() ? b->GetFixedWidth() : width;
                int boxHeight = b->GetFixedHeight() ? b->GetFixedHeight() : height;
                if (vert && properties_.halign == LayoutProperties::CENTER) {
                    // center halign, set the X offset to half the difference between the horizontal size of the container
                    // and the horizontal size of the component.
                    offset.x = (width - boxWidth) / 2;
                } else if (vert && properties_.halign == LayoutProperties::RIGHT) {
                    // right halign, set the X offset to the entire difference betweent the horizontal size of the container
                    // and the horizontal size of the component.
                    offset.x = width - boxWidth;
                } else if ((!vert) && properties_.valign == LayoutProperties::MIDDLE) {
                    offset.y = (height - boxHeight) / 2;
                } else if ((!vert) && properties_.valign == LayoutProperties::BOTTOM) {
                    offset.y = height - boxHeight;
                }
                b->SetOutsideOffset(offset);
                b->SetSize(dynamicSize);
                // set vertical offset for the next component in the column
                if (vert) {
                    offset.y += properties_.spacing + (b->GetFixedHeight() ? b->GetFixedHeight() : dynamicSize.height);
                } else {
                    offset.x += properties_.spacing + (b->GetFixedWidth() ? b->GetFixedWidth() : dynamicSize.width);
                }
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
