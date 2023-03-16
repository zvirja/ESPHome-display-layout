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
- Use macros/functions to build root layout component (usually horizontal or vertical panel). Render it at the end:
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

Alignment:

```cpp
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

    // Aliases for better readability
    LEFT = TOP,
    RIGHT = BOTTOM
```

Play with them to feel how they work.

Example:
```yaml
_COL(
  _T(id(font_text_40), "Hello"),

  _ROW_A(CENTER | STRETCH,
    _T(id(font_text_40), "My"),
    _T(id(font_text_40), "World")
  )
);

Valid alignments: `CENTER | MIDDLE`, `START | STRETCH`
```

### EXPAND to put in Stack Panel

To understand why you need expander, you have to first understand how panel layouting works. Panel consists of the following steps (if there are nested panels, happens recursively):
1. Measure. Ask each child block how much space it needs
2. Position. Use the measured dimenions to position blocks depending on the specified panel alignment (e.g. `END` - put all the available space at the beginning; `SPACE_BETWEEN` - distribute free space between the blocks evenly)
3. Render. Render each component one by one after positioning them.

Expander is a dummy components which wraps another component. It tells parent Panel to give all the available space to it upon rendering. If there is more than one EXPAND block next to each to each other, free space is distributed evently before them.

Notice, when EXPAND is used, the main-axis alignment (`START/CENTER/END/SPACE_BETWEEN`) is effectively ignored, as there is no free space to distribute.

Functions:
- `_EXPAND(x)` - Wrap child component giving it all the available space.

Example:
```yaml
```yaml
_COL(
  _T(id(font_text_20), "Header"),
  
  _EXPAND(
    _COL_A(CENTER | STRETCH
      _VSPACE(25),
      _T(id(font_text_40), "HELLO"),
      _VSPACE(10),
      _T(id(font_text_40), "WORLD"),      
      _VSPACE(25)
    )
  ),

  _T(id(font_text_20), "Footer")
);
```

Notice that here Header will be at the top, Footer at the bottom and content in the middle will occupy the remaining space. It was build for exactly such purposes.

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

Text Row renders a few text blocks one by one in a row. The difference between TextRow and usage of horizontal panel (`_ROW()`) is that Text Row is aligning all the text blocks by baseline.
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
