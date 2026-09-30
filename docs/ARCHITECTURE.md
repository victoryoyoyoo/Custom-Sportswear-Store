# How the store is built

About 6,700 lines: C++17 on wxWidgets 3.3 for the app, plus one Python script that draws the
artwork. This page walks through the pieces in the order a customer meets them.

## Windows

```
WelcomeFrame ──► LauncherFrame ──► ProductFrame ──► CartDialog ──► CheckoutDialog ──► OrderCompleteDialog
                      │                  │
                      └── OrdersDialog ◄─┘      (TeamOrderDialog, SizeAdvisorDialog from the product page)
```

| File | What it does |
| --- | --- |
| `App.cpp` | Starts the app and fades in the welcome window. |
| `WelcomeFrame` | Store name and the way in, next to a jersey and a ball turning live in 3D. |
| `LauncherFrame` | Scrolling grid of product cards, category chips, search, favourites. |
| `ProductFrame` | One product: 360° view, colour, size, customisation, quantity, add to cart. |
| `ProductDialogs` | Team order (a whole roster at once) and size advice. |
| `CartDialog` | Cart, checkout form, order confirmation and order history. |

Only one page is on screen at a time. `Theme::ShowLike` fades the next page in with the same
window state as the current one (normal, maximised or full screen) and hides the old one once
the new one is visible, so the desktop never flashes in between.

Dialogs are stack objects that belong to a page. `Theme::ShowModalDialog` counts open dialogs
and `Theme::CanClosePage` refuses to close a page while one is open; otherwise closing the page
from the taskbar would delete a dialog that is still running.

## Data: `Catalog`

Products, colourways and coupons are tables. A product row holds everything the screens need:

```cpp
struct Product {
    wxString id, category, name, englishName, tagline;
    int price;
    std::vector<SizeOption> sizes;
    Personalization personalization;   // name + number / number / text / none
    PrintArea nameArea, numberArea, textArea;
    Shape shape;                        // how the 360° view turns it
    double thickness;                   // Flat: depth of the body, as a fraction of its width
    wxString reverseArtId;              // Flat: artwork for the far side
    RoundShape round;                   // Round: the cylinder the print wraps round
    ...
};
```

No screen mentions a particular product. The launcher builds a card per row, the product page
builds itself from the row, the cart stores a row index. Adding a product means adding a row and
its artwork. The tables map directly onto database tables if the catalogue ever moves out of code.

`ShoppingCart`, `OrderHistory` and `Favorites` are single shared instances (`Get()`), so every
window sees the same cart. They live in memory while the app runs.

## Customisation: `Personalizer` (Strategy)

Each way of customising is a subclass:

| Class | Used by | Inputs |
| --- | --- | --- |
| `NameAndNumberPersonalizer` | jersey | number, name (back), team (front) |
| `NumberPersonalizer` | shorts | number on the leg |
| `TextPersonalizer` | tee, hoodie, balls, cap, bands, bottle, backpack, towel | a short text |

A personalizer adds its own inputs to the product page, describes the choice for the cart line
(`#23・WANG・正面 TIGERS`), and prints it: onto the artwork of a flat product (`Draw`), or as a
text image (`PrintText` → `TextDecal`) that the 3D view wraps round a ball, cap or bottle. Every
edit reports which side of the product it shows on, so the page can turn the product to face it.

## The 360° view: `Showcase` and `Turntable`

`Turntable` is the widget. It keeps an angle, turns it while you drag, lets it coast with some
friction after a flick, animates to a side on request, and plays one slow turn when the page
opens. While anything moves it draws at half resolution; when it stops it redraws at full quality.

`Showcase` does the drawing. There is no 3D library: for every pixel it works out which point of
the product that pixel sees, then colours and lights it. `Build` makes a `Model` for the
product's `Shape`:

| Shape | Products | How a pixel is found |
| --- | --- | --- |
| `Flat` | clothes, sneaker, backpack, towel | Each row of the artwork is an ellipse round a vertical axis. The line of sight meets the ellipse at an angle φ; the front artwork covers the front half, the back artwork the back half. Where the artwork would stretch across a side turned towards you, the side's own fabric colour is used instead. |
| `Basketball`, `SoccerBall` | balls | A point on a sphere, turned back into the ball's coordinates. The basketball's channels are planes and two curved seams; the football's 32 panels are whichever of 12 pentagon and 20 hexagon centres is nearest (allowing for their different sizes). |
| `Cap` | cap | Rays against a half-ellipsoid crown and a curved peak; six panels, stitching, eyelets, the button, the strap opening at the back. |
| `Round` | headband, wristband, bottle | The outline doesn't change as it turns; the logo and text are wrapped round the cylinder at their own angles. |

All of them use one light from the upper left, premultiplied RGBA and bilinear sampling. The
stage behind the product (a light backdrop and a soft contact shadow) is drawn by `DrawStage` and
`DrawShadow`; `Still` and `Tile` reuse the models for cart thumbnails and product-list cards.

## Look and feel: `Theme`, `Widgets`, `SwatchPicker`

- `Theme`: colours, fonts, finding the `assets` folder, headers, cards, full screen, Chinese
  confirm/notice boxes, and `ImagePanel` (a picture redrawn at the panel's real pixel size,
  with the staggered fade-in used on the product list).
- `Widgets`: `FlatButton`, `Card`, `ChipPicker`, `HeartToggle`, `ProgressBar`, `StepIndicator`,
  `Toast`, all painted with `wxGraphicsContext`, and `Tween`, the small timer behind every
  animation (eased from 0 to 1 over a set time).
- `SwatchPicker`: the row of colour dots, with an animated selection ring.

The app declares per-monitor DPI awareness in its manifest (set in the project's linker
options), so Windows doesn't blur it by scaling a bitmap; sizes are written in DIPs and turned
into pixels with `FromDIP`.

## Artwork: `tools/gen_assets.py`

Pillow draws each product at 4× and scales it down for smooth edges. Products are saved at twice
their design size as transparent cut-outs, front and back where there is one, for all 12
colourways. The app adds the backdrop, lighting and shadow itself. Balls and the cap have no
artwork; they are drawn in 3D and only need the store emblem (`emblem_<colourway>.png`).
