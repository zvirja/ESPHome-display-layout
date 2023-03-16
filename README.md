# ESPHome Display Layout

This repo contains a small layout utils to make designing screens simpler for ESPHome display module (e.g. famous [T5 4.7 Inch E-paper](https://www.lilygo.cc/products/t5-4-7-inch-e-paper-v2-3)).

⚠ **DISCLAIMER** Utility is very simple and misses a lot of essential features to be a proper layouting lib. It however provides an MVP to do a simple layouting.

# Installation

- Copy `layout_utils.hpp` to folder next to your ESPHome `yaml` file
- Add reference to the file:
    ```yaml
    esphome:
      includes:
        - layout_utils.hpp
    ```
- Use macros/functions to build root layout component (usually horizontal or vertical panel)
- Render it:
  ```yaml
  display:
    - platform: lilygo_t5_47_display
      id: t5_display
      rotation: 270
      update_interval: never
      lambda: |-
        #define xres 540 
        #define yres 960
        #define x_pad 10 // border padding
        #define y_pad 10 // border padding

        auto layout = _COL_A(START | STRETCH,
          _T(id(font_text_40), "Hello"),

          _EXPAND(
            _T(id(font_text_40), "World")
          )
        );

        layout->measure();
        Rect rect = Rect(x_pad, 0, xres - x_pad * 2, yres);
        layout->render(it, rect);
  ```
    

# Usage

Library provides a set of macros/functions (called BLOCKs internally to not collide with other possible layout entities) to use. Library consists of the following basic blocks.

## Layout

### Stack Panel

This is a [typical](https://wpf-tutorial.com/panels/stackpanel/) Stack Panel component which just stacks element one by one.

Properties:
- Direction: Can be either horizontal or vertical
- Main axis alignment
- Cross axis alignment

Helper Functions:
- `_COL(TBlocks&&...blocks)` - Renders a list of blocks _vertically_ with the default alignment
- `_COL_A(Align align, TBlocks&&...blocks)` - Renders a list of blocks _vertically_, allows to specify the alignment
- `_ROW(TBlocks&&...blocks)` - Renders a list of blocks _horizontally_ with the default alignment
- `_ROW_A(Align align, TBlocks&&...blocks)` - Renders a list of blocks _horizontally_, allows to specify the alignment

Example:
```yaml
_COL(
  _T(id(font_text_40), "Hello"),

  _ROW_A(CENTER | STRETCH,
    _T(id(font_text_40), "My"),
    _T(id(font_text_40), "World")
  )
);
```

### Spacers

Components to introduce space between components.

Helper functions:
- `_HSPACE(x)` - Introduces horizontal padding
- `_VSPACE(x)` - Introduces vertical padding

Example:
```yaml
_COL_A(START | STRETCH,
  _T(id(font_text_40), "Hello"),
  
  _VSPACE(50),

  _ROW_A(START | STRETCH,
    _T(id(font_text_40), "My"),
    _HSPACE(50),
    _T(id(font_text_40), "World")
  )
);
```

### Debug container

This container just renders a border around inner component. Useful for debugging.

Helper functions:
- `_D(x)` - Render a frame around inner component

## Elements

### Text

Text block renders the text using the specified font.

Helper functions:
- `_T(Font* font, const char* format, TArgs...args)` - Render text using the specified font
- ` _TROW(TArgs&&...textBlocks)` - Render a text row aligning all the inner text blocks by baseline.

Text Row is rendering a few text blocks one by one in a row. The difference between TextRow and usage of horizontal panel (`_ROW()`) is that Text Row is aligning all the text blocks by baseline.
It's often hard to achieve similar effect with pure panel, especially if you mix e.g. text and icon fonts.

Notice, `_T()` accepts format string & arguments exactly like `it.printf()` works.
Notice, `_TROW()` accepts text blocks only. If you put inside non-text blocks, compilation will fail with a cryptic error 😉

Example:
```yaml
_COL(
  _T(id(font_text_40), "Hello %d", 42),

  _TROW(
    _T(id(font_text_40), "My"),
    _T(id(font_text_40), "World")
  )
);
```

### Horizontal line

Just renders a horizontal line. Takes all the available space given by parent container if possible (e.g. if it's put inside vertical panel with `STRETCH` alignment).

Helper functions:
- `_HR(thickness, hpad, vpad)` - Renders a horizontal line

Example:
```yaml
_COL_A(START | STRETCH,
  _T(id(font_text_40), "Hello"),
  
  _VSPACE(25),
  _HR(10, 0, 0),
  _VSPACE(25),  

  _T(id(font_text_40), "World")
);
```
