#pragma once

#include "esphome.h"

struct TextDesc {
    esphome::display::Font* font;
    std::string text;

    TextDesc(esphome::display::Font* font, std::string text): font(font), text(std::move(text)) {}
};

struct Dimensions {
    int width, height;
    Dimensions(int w, int h): width(w), height(h) {}
};

struct Rect {
    int x, y, width, height;

    Rect(int x, int y, int w, int h): x(x), y(y), width(w), height(h) {}
};

enum Align {
    // Alignment along main axis (vertical for vertical panel, horizontal for horizontal panel)
    SPACE_BETWEEN =     0,
    START =             1 << 0,
    CENTER =            1 << 1,
    END =               1 << 2,
    
    // Alignment perpendicular to main axis (horizontal for vertical panel, vertical for horizontal panel)
    TOP =               0,
    MIDDLE =            1 << 3,
    BOTTOM =            1 << 4,
    STRETCH =           1 << 5,

    // Aliases
    LEFT = TOP,
    RIGHT = BOTTOM
};

inline Align operator|(Align a, Align b)
{
    return static_cast<Align>(static_cast<int>(a) | static_cast<int>(b));
}

class Block {
protected:
    Dimensions _measured = Dimensions(0, 0);

    virtual Dimensions do_measure() =0;
    virtual void do_render(esphome::display::DisplayBuffer& it, Rect& rect) =0;

public:
    virtual bool can_expand() { return false; }
    Dimensions measure() { return _measured = do_measure(); }
    void render(esphome::display::DisplayBuffer& it, Rect& rect) { do_render(it, rect); }
};

class TextBlock : public Block {
private:
    TextDesc _desc;

    Dimensions do_measure() override {
        int width,height,x_offset,baseline;
        _desc.font->measure(_desc.text.data(), &width, &x_offset, &baseline, &height);

        return Dimensions(width, height);
    }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override {
        // Usually container will give measured size back.
        // If container decided to stretch us, render in center.
        int x = rect.x, y = rect.y;
        x += (rect.width - _measured.width) / 2;
        y += (rect.height - _measured.height) / 2;

        it.print(x, y, _desc.font, _desc.text.data());
    }

public:
    TextDesc& get_desc() { return _desc; }

    TextBlock(TextDesc textDesc): _desc(std::move(textDesc)) {}
};

class TextRowBlock final: public Block {
private:
    std::vector<std::unique_ptr<TextBlock>> _blocks;
    std::vector<Dimensions> _blockDimensions;
    int _measureMaxBaseline = 0;

    Dimensions do_measure() override {
        int width = 0, max_height_above = 0, max_height_below = 0, max_baseline =0;

        for (size_t i = 0; i < _blocks.size(); i++) {
            auto& desc = _blocks[i]->get_desc();

            int w,h,x_offset,baseline;
            desc.font->measure(desc.text.data(), &w, &x_offset, &baseline, &h);

            width += w;
            max_height_above = std::max(max_height_above, baseline);
            max_height_below = std::max(max_height_below, h - baseline);
            max_baseline = std::max(max_baseline, baseline);

            _blockDimensions[i] = Dimensions(w, h);
        }

        _measureMaxBaseline = max_baseline;

        return Dimensions(width, max_height_above + max_height_below);
    }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override {
        // Usually container will give measured size back.
        // If container decided to stretch us, render in center.
        int x = rect.x, y = rect.y;
        x += (rect.width - _measured.width) / 2;
        y += (rect.height - _measured.height) / 2;

        // At this point x,y point to top left.
        // Shift y by max baseline, so that y always point at baseline
        y += _measureMaxBaseline;

        for (size_t i = 0; i < _blocks.size(); i++) {
            auto& desc = _blocks[i]->get_desc();

            it.print(x, y, desc.font, esphome::display::TextAlign::BASELINE_LEFT, desc.text.data());
            x += _blockDimensions[i].width;
        }
    }

public:
    TextRowBlock(std::vector<std::unique_ptr<TextBlock>> textBlocks) {
        _blockDimensions = std::vector<Dimensions>(textBlocks.size(), Dimensions(0, 0));
        _blocks = std::move(textBlocks);
    }
};

class PaddingBlock final : public Block {
private:
    int _width;
    int _height;

    Dimensions do_measure() override {
        return Dimensions(_width, _height);
    }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override { }

public:
    PaddingBlock(int width, int height): _width(width), _height(height) {}
};

class HorizontalLineBlock final : public Block {
private:
    int _thickness;
    int _h_padding;
    int _v_padding;

    Dimensions do_measure() override {
        return Dimensions(_h_padding * 2, _thickness + _v_padding * 2);
    }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override {
        // It uses all the available horizontal space
        auto line_len = rect.width - _h_padding * 2;

        it.filled_rectangle(rect.x + _h_padding, rect.y + _v_padding, line_len, _thickness);
    }

public:
    HorizontalLineBlock(int thickness, int h_padding, int v_padding): _thickness(thickness), _h_padding(h_padding), _v_padding(v_padding) {}
};

class ExpandBlock final : public Block {
private:
    std::unique_ptr<Block> _inner;

    Dimensions do_measure() override {
        return _inner->measure();
    }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override {
        _inner->render(it, rect);
    }

public:
    ExpandBlock(std::unique_ptr<Block> inner): _inner(std::move(inner)) {}

    bool can_expand() override { return true; }
};

class PanelBlock final : public Block {
private:
    bool _vertical;
    Align _align;
    std::vector<std::unique_ptr<Block>> _blocks;
    std::vector<Dimensions> _blockDimensions;

    Dimensions do_measure() override {
        int width = 0, height = 0;

        for (size_t i = 0; i < _blocks.size(); i++) {
            auto dm = _blockDimensions[i] = _blocks[i]->measure();
            
            if (_vertical) {
                height += dm.height;
                width = std::max(width, dm.width);
            } else {
                height = std::max(height, dm.height);
                width += dm.width;
            }
        }

        return Dimensions(width, height);
    }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override {
        auto freeFlowSpace = main_axis(rect.width, rect.height) - main_axis(_measured.width, _measured.height);

        int startOffset = 0;
        int flowPadding = 0;
        int spacePerExpandableBlock = 0;

        // STEP 1
        // If we have expandable blocks, give them all the free space we have (distribute across all)
        // Otherwise use that space for even spacing between the components
        auto expandableCount = std::count_if(_blocks.begin(), _blocks.end(), [] (std::unique_ptr<Block>& b) { return b->can_expand(); });
        if (expandableCount > 0) {
            spacePerExpandableBlock = freeFlowSpace / expandableCount;
            freeFlowSpace = 0;
        }
        
        // Evaluate align strategy and calculate offset parameters
        if (freeFlowSpace > 0) {
            if ((_align & START) == START) {
                // Leave all the free space for the end
                startOffset = 0;
                flowPadding = 0;
            } else if ((_align & CENTER) == CENTER) {
                startOffset = freeFlowSpace / 2;
                flowPadding = 0;
            } else if ((_align & END) == END) {
                startOffset = freeFlowSpace;
                flowPadding = 0;
            } else {
                // SPACE_BETWEEN
                startOffset = 0;
                flowPadding = _blocks.size() > 1
                                ? freeFlowSpace / (_blocks.size() - 1)
                                // If 1 - place in center. If 0 - doesn't matter.
                                : freeFlowSpace / 2;
            }
        }

        // STEP 2
        // Iterate over each component and patch their size.
        //// We always inherit non-directional dimension (i.e. width for vertical, heigth for horizontal)
        // For directional dimension we use measured or grow if expandable
        for (size_t i = 0; i < _blocks.size(); i++) {
            if (_vertical) {
                // _blockDimensions[i].width = rect.width;
                _blockDimensions[i].height += _blocks[i]->can_expand() ? spacePerExpandableBlock : 0;
            } else {
                // _blockDimensions[i].height = rect.width;
                _blockDimensions[i].width += _blocks[i]->can_expand() ? spacePerExpandableBlock : 0;
            }
        }

        // STEP 3
        // Render components one by one.
        // Add padding in between.
        int renderX = rect.x, renderY = rect.y;

        // Add initial offset
        main_axis(renderX, renderY) = main_axis(renderX, renderY) + startOffset;

        for (size_t i = 0; i < _blocks.size(); i++) {
            auto& bDim = _blockDimensions[i];

            int x = renderX, y = renderY;
            int w = bDim.width, h = bDim.height;

            // Align block perpendicullary to flow
            if ((_align & STRETCH) == STRETCH) {
                cross_axis(w, h) = cross_axis(rect.width, rect.height);
            } else if ((_align & RIGHT) == RIGHT) {
                // RIGHT/BOTTOM
                cross_axis(x, y) += cross_axis(rect.width, rect.height) - cross_axis(w, h);
            } else if ((_align & MIDDLE) == MIDDLE) {
                // MIDDLE
                cross_axis(x, y) += (cross_axis(rect.width, rect.height) - cross_axis(w, h)) / 2;
            } else {
                // TOP/LEFT
                // don't offset, it's already top/left
            }

            auto bRect = Rect(x, y, w, h);
            _blocks[i]->render(it, bRect);

            main_axis(renderX, renderY) += main_axis(w, h) + flowPadding;
        }
    }

    inline int& main_axis(int& x, int& y) {
        return _vertical ? y : x;
    }

    inline int& cross_axis(int& x, int& y) {
        return _vertical ? x : y;
    }

public:
    PanelBlock(bool vertical, Align align, std::vector<std::unique_ptr<Block>> blocks) : _vertical(vertical), _align(align) {
        _blockDimensions = std::vector<Dimensions>(blocks.size(), Dimensions(0,0));
        _blocks = std::move(blocks);
    }
};

class DebugBlock final : public Block {
private:
    esphome::display::Font* _font;
    std::unique_ptr<Block> _inner;

    Dimensions do_measure() override { return _inner->measure(); }

    void do_render(esphome::display::DisplayBuffer& it, Rect& rect) override {
        _inner->render(it, rect);

        it.rectangle(rect.x, rect.y, rect.width, rect.height);

        if (_font != nullptr) {
            it.filled_rectangle(rect.x + 2, rect.y + 2, 50, 30, esphome::display::COLOR_OFF);
            it.printf(rect.x + 2, rect.y + 2, _font, "%d:%d", rect.x, rect.y);
            it.printf(rect.x + 2, rect.y + 20, _font, "%dx%d", rect.width, rect.height);
        }

        // ESP_LOGD("DebugBlock", "Render dimensions. X: %d, Y: %d, W: %d, H: %d", rect.x, rect.y, rect.width, rect.height);
    }

public:
    DebugBlock(Font* font, std::unique_ptr<Block> inner): _font(font), _inner(std::move(inner)) {}

    bool can_expand() override { return _inner->can_expand(); }
};

#define _D(x) std::unique_ptr<Block>(new DebugBlock(NULL, std::move(x)))

#define BLOCK std::unique_ptr<Block>

template<typename...TArgs>
std::unique_ptr<TextBlock> _T(esphome::display::Font* font, const char* format, TArgs...args) {
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), format, args...);

    return std::unique_ptr<TextBlock>(new TextBlock(TextDesc(font, std::string(buffer, len))));
}

template<typename...TArgs>
std::unique_ptr<TextRowBlock> _TROW(TArgs&&...blocks) {
    // Trick from here: https://stackoverflow.com/a/33747281/2009373
    std::unique_ptr<TextBlock> itemArr[] = {std::move(blocks)...};
    auto blocksV = std::vector<std::unique_ptr<TextBlock>> {std::make_move_iterator(std::begin(itemArr)), std::make_move_iterator(std::end(itemArr))};

    return std::unique_ptr<TextRowBlock>(new TextRowBlock(std::move(blocksV)));
}

template<typename...TBlocks>
BLOCK _COL_A(Align align, TBlocks&&...blocks) {
    // Trick from here: https://stackoverflow.com/a/33747281/2009373
    std::unique_ptr<Block> itemArr[] = {std::move(blocks)...};
    auto blocksV = std::vector<std::unique_ptr<Block>> {std::make_move_iterator(std::begin(itemArr)), std::make_move_iterator(std::end(itemArr))};

    return std::unique_ptr<Block>(new PanelBlock(true, align, std::move(blocksV)));
}

template<typename...TBlocks>
BLOCK _ROW_A(Align align, TBlocks&&...blocks) {
    // Trick from here: https://stackoverflow.com/a/33747281/2009373
    std::unique_ptr<Block> itemArr[] = {std::move(blocks)...};
    auto blocksV = std::vector<std::unique_ptr<Block>> {std::make_move_iterator(std::begin(itemArr)), std::make_move_iterator(std::end(itemArr))};

    return std::unique_ptr<Block>(new PanelBlock(false, align, std::move(blocksV)));
}

template<typename...TBlocks>
BLOCK _COL(TBlocks&&...blocks) {
    return _COL_A(SPACE_BETWEEN | STRETCH, blocks...);
}

template<typename...TBlocks>
BLOCK _ROW(TBlocks&&...blocks) {
    return _ROW_A(SPACE_BETWEEN | STRETCH, blocks...);
}

#define _HSPACE(x)   std::unique_ptr<Block>(new PaddingBlock((x), 0))
#define _VSPACE(x)   std::unique_ptr<Block>(new PaddingBlock(0, (x)))
#define _EXPAND(x) std::unique_ptr<Block>(new ExpandBlock((x)))

#define _HR(thickness, hpad, vpad) std::unique_ptr<Block>(new HorizontalLineBlock(thickness, hpad, vpad))
