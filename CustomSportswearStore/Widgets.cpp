#include "Widgets.h"
#include "Theme.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <algorithm>
#include <cmath>
#include <memory>

namespace Widgets {

namespace {
    std::unique_ptr<wxGraphicsContext> BeginPaint(wxWindow* w, wxAutoBufferedPaintDC& dc) {
        dc.SetBackground(wxBrush(w->GetParent()->GetBackgroundColour()));
        dc.Clear();
        return std::unique_ptr<wxGraphicsContext>(wxGraphicsContext::Create(dc));
    }

    wxColour WithAlpha(const wxColour& c, int alpha) { return wxColour(c.Red(), c.Green(), c.Blue(), alpha); }
}

// ---------------------------------------------------------------------------
// Tween
// ---------------------------------------------------------------------------
void Tween::Start(int durationMs, std::function<void(double)> onStep, std::function<void()> onDone) {
    m_durationMs = std::max(1, durationMs);
    m_onStep = std::move(onStep);
    m_onDone = std::move(onDone);
    m_startMs = wxGetLocalTimeMillis();
    wxTimer::Start(15);
    if (m_onStep) m_onStep(0.0);
}

void Tween::Notify() {
    const double t = std::min(1.0, (wxGetLocalTimeMillis() - m_startMs).ToDouble() / m_durationMs);
    if (m_onStep) m_onStep(EaseOut(t));
    if (t >= 1.0) {
        Stop();
        if (m_onDone) {
            auto done = std::move(m_onDone);  // may start a new tween
            m_onDone = nullptr;
            done();
        }
    }
}

double EaseOut(double t) {
    const double u = 1.0 - t;
    return 1.0 - u * u * u * u;
}

wxColour Mix(const wxColour& from, const wxColour& to, double t) {
    auto lerp = [t](int a, int b) { return (unsigned char)std::clamp((int)std::lround(a + (b - a) * t), 0, 255); };
    return wxColour(lerp(from.Red(), to.Red()), lerp(from.Green(), to.Green()), lerp(from.Blue(), to.Blue()));
}

// ---------------------------------------------------------------------------
// FlatButton
// ---------------------------------------------------------------------------
FlatButton::FlatButton(wxWindow* parent, const wxString& label, Style style, int pointSize, int heightDip)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
      m_style(style), m_heightDip(heightDip) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetFont(Theme::Font(pointSize, true));
    SetCursor(wxCursor(wxCURSOR_HAND));
    switch (style) {
    case Style::Primary:
        m_base = Theme::kOrange; m_hoverColour = Theme::kOrangeDark; m_text = Theme::kOnDark; break;
    case Style::Secondary:
        m_base = wxColour(234, 237, 244); m_hoverColour = wxColour(218, 224, 235); m_text = Theme::kText; break;
    case Style::OnDark:
        m_base = Theme::kNavyLight; m_hoverColour = wxColour(50, 68, 106); m_text = Theme::kOnDark; break;
    }
    wxControl::SetLabel(label);
    SetInitialSize();

    Bind(wxEVT_PAINT, &FlatButton::OnPaint, this);
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { AnimateHoverTo(1.0); });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        m_pressed = false;
        AnimateHoverTo(0.0);
    });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) {
        m_pressed = true;
        if (!HasCapture()) CaptureMouse();
        Refresh();
    });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& event) {
        if (HasCapture()) ReleaseMouse();
        const bool fire = m_pressed && GetClientRect().Contains(event.GetPosition());
        m_pressed = false;
        Refresh();
        if (fire) Click();
    });
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) {
        m_pressed = false;
        Refresh();
    });
    Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
    Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
    Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
        const int key = event.GetKeyCode();
        if (key == WXK_RETURN || key == WXK_NUMPAD_ENTER || key == WXK_SPACE) Click();
        else event.Skip();
    });
}

void FlatButton::SetLabel(const wxString& label) {
    if (label == GetLabel()) return;
    wxControl::SetLabel(label);
    InvalidateBestSize();
    SetMinSize(wxSize(std::max(GetMinSize().x, GetBestSize().x), GetMinSize().y));
    Refresh();
}

bool FlatButton::Enable(bool enable) {
    const bool changed = wxControl::Enable(enable);
    SetCursor(wxCursor(enable ? wxCURSOR_HAND : wxCURSOR_ARROW));
    if (!enable) m_hover = 0.0;
    Refresh();
    return changed;
}

void FlatButton::ShowArrow(bool show) {
    m_arrow = show;
    InvalidateBestSize();
    SetMinSize(wxSize(std::max(GetMinSize().x, GetBestSize().x), GetMinSize().y));
    Refresh();
}

void FlatButton::Flash(const wxColour& colour) {
    m_flashColour = colour;
    m_flashTween.Start(1100, [this](double t) {
        m_flash = 1.0 - t;
        Refresh();
    });
}

wxSize FlatButton::DoGetBestClientSize() const {
    wxClientDC dc(const_cast<FlatButton*>(this));
    dc.SetFont(GetFont());
    const wxSize text = dc.GetTextExtent(GetLabel());
    const int arrow = m_arrow ? FromDIP(m_heightDip) - FromDIP(8) + FromDIP(10) : 0;
    return wxSize(text.x + FromDIP(48) + arrow, FromDIP(m_heightDip));
}

void FlatButton::AnimateHoverTo(double target) {
    if (!IsEnabled()) return;
    const double from = m_hover;
    m_hoverTween.Start(220, [this, from, target](double t) {
        m_hover = from + (target - from) * t;
        Refresh();
    });
}

void FlatButton::Click() {
    if (!IsEnabled()) return;
    wxCommandEvent event(wxEVT_BUTTON, GetId());
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}

void FlatButton::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto gc = BeginPaint(this, dc);
    if (!gc) return;

    const wxColour parentBg = GetParent()->GetBackgroundColour();
    wxColour fill = Mix(m_base, m_hoverColour, m_hover);
    if (m_pressed) fill = Mix(fill, *wxBLACK, 0.10);
    if (m_flash > 0) fill = Mix(fill, m_flashColour, m_flash);
    wxColour text = m_text;
    if (!IsEnabled()) {
        fill = Mix(fill, parentBg, 0.55);
        text = Mix(text, fill, 0.45);
    }

    const wxSize size = GetClientSize();
    // Pressing shrinks the button by a pixel or two: reads as a physical press.
    const double inset = m_pressed ? FromDIP(1.5) : 0.0;
    const double w = size.x - 2 * inset, h = size.y - 2 * inset;
    const double radius = m_style == Style::Primary ? h / 2 : FromDIP(10);

    // Primary buttons sit on a soft coloured glow that grows on hover.
    if (m_style == Style::Primary && IsEnabled()) {
        for (int i = 3; i >= 1; --i) {
            const double grow = i * (1.0 + m_hover);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(WithAlpha(fill, (int)(10 + 8 * m_hover) / i)));
            gc->DrawRoundedRectangle(inset - grow + 1, inset - grow + 2, w + 2 * grow - 2, h + 2 * grow - 2, radius + grow);
        }
    }
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(fill));
    gc->DrawRoundedRectangle(inset, inset, w, h, radius);
    // top highlight for a hint of volume
    gc->SetBrush(gc->CreateLinearGradientBrush(0, inset, 0, inset + h / 2, WithAlpha(*wxWHITE, 26), WithAlpha(*wxWHITE, 0)));
    gc->DrawRoundedRectangle(inset, inset, w, h / 2, radius);

    if (HasFocus() && IsEnabled()) {
        const double f = FromDIP(3);
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(wxPen(WithAlpha(*wxWHITE, 150), FromDIP(1)));
        gc->DrawRoundedRectangle(inset + f, inset + f, w - 2 * f, h - 2 * f, std::max(2.0, radius - f));
    }

    wxDouble tw, th;
    gc->SetFont(GetFont(), text);
    gc->GetTextExtent(GetLabel(), &tw, &th);
    double textX = (size.x - tw) / 2;
    if (m_arrow) {
        const double d = h - FromDIP(10);                  // arrow bubble diameter
        const double bx = inset + w - d - FromDIP(5) + m_hover * FromDIP(3);
        const double by = inset + (h - d) / 2;
        textX = inset + (w - d - FromDIP(10) - tw) / 2 + FromDIP(4);
        gc->SetBrush(wxBrush(WithAlpha(*wxBLACK, 34)));
        gc->DrawEllipse(bx, by, d, d);
        wxGraphicsPath arrow = gc->CreatePath();
        const double cx = bx + d / 2 + m_hover * FromDIP(1), cy = by + d / 2, a = d * 0.2;
        arrow.MoveToPoint(cx - a, cy);
        arrow.AddLineToPoint(cx + a, cy);
        arrow.MoveToPoint(cx + a * 0.25, cy - a * 0.75);
        arrow.AddLineToPoint(cx + a, cy);
        arrow.AddLineToPoint(cx + a * 0.25, cy + a * 0.75);
        gc->SetPen(wxPen(text, FromDIP(2)));
        gc->StrokePath(arrow);
    }
    gc->DrawText(GetLabel(), textX, (size.y - th) / 2);
}

// ---------------------------------------------------------------------------
// Card
// ---------------------------------------------------------------------------
Card::Card(wxWindow* parent, bool hoverable)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(Theme::kCard);  // children (labels, pictures) sit on white
    Bind(wxEVT_PAINT, &Card::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
        Refresh();
        e.Skip();
    });
    if (!hoverable) return;

    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) { TrackHover(); e.Skip(); });
    m_leaveCheck.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { TrackHover(); });
    CallAfter([this] {
        for (wxWindow* child : GetChildren())
            child->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) { TrackHover(); e.Skip(); });
    });
}

void Card::TrackHover() {
    const bool inside = IsShownOnScreen() && GetScreenRect().Contains(wxGetMousePosition());
    if (inside) m_leaveCheck.Start(80);
    else m_leaveCheck.Stop();
    if (inside == m_hovered) return;
    m_hovered = inside;
    const double from = m_hover, target = inside ? 1.0 : 0.0;
    m_tween.Start(260, [this, from, target](double t) {
        m_hover = from + (target - from) * t;
        Refresh(false);
    });
}

void Card::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto gc = BeginPaint(this, dc);
    if (!gc) return;

    const wxSize size = GetClientSize();
    const double m = FromDIP(InsetDip());
    const double shellPad = FromDIP(3);
    const double radius = FromDIP(18);
    const wxRect2DDouble shell(m - shellPad, m - shellPad, size.x - 2 * (m - shellPad), size.y - 2 * (m - shellPad));

    // Ambient shadow: stacked, very faint rounded rects, a little lower than
    // the card, spreading further when the card is lifted by hover.
    gc->SetPen(*wxTRANSPARENT_PEN);
    const int layers = 6;
    for (int i = layers; i >= 1; --i) {
        const double spread = i * (0.9 + 0.35 * m_hover);
        const int alpha = (int)((5 + 4 * m_hover) * (layers + 1 - i) / layers);
        gc->SetBrush(wxBrush(wxColour(20, 30, 60, alpha)));
        gc->DrawRoundedRectangle(shell.m_x - spread, shell.m_y - spread + FromDIP(2) + m_hover * FromDIP(2),
                                 shell.m_width + 2 * spread, shell.m_height + 2 * spread, radius + spread);
    }

    // Shell: a thin tinted rim around the white core (concentric corners).
    gc->SetBrush(wxBrush(Mix(wxColour(236, 240, 246), wxColour(255, 236, 224), m_hover)));
    gc->SetPen(wxPen(Mix(wxColour(222, 228, 237), Theme::kOrange, m_hover * 0.8), 1));
    gc->DrawRoundedRectangle(shell.m_x, shell.m_y, shell.m_width, shell.m_height, radius);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(Theme::kCard));
    gc->DrawRoundedRectangle(m, m, size.x - 2 * m, size.y - 2 * m, radius - shellPad);
}

// ---------------------------------------------------------------------------
// ChipPicker
// ---------------------------------------------------------------------------
ChipPicker::ChipPicker(wxWindow* parent, const std::vector<wxString>& labels, int selection)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS),
      m_labels(labels), m_selected(selection) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetFont(Theme::Font(11, true));
    SetCursor(wxCursor(wxCURSOR_HAND));
    wxClientDC dc(this);
    dc.SetFont(GetFont());
    int widest = 0;
    for (const wxString& l : m_labels) widest = std::max(widest, dc.GetTextExtent(l).x);
    m_chipWidth = std::max(FromDIP(46), widest + FromDIP(26));
    const int n = (int)m_labels.size();
    SetMinSize(wxSize(n * m_chipWidth + (n - 1) * FromDIP(8) + 2, FromDIP(38)));

    Bind(wxEVT_PAINT, &ChipPicker::OnPaint, this);
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int hit = HitTest(e.GetPosition());
        if (hit != m_hovered) { m_hovered = hit; Refresh(); }
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hovered = -1; Refresh(); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
        SetFocus();
        Select(HitTest(e.GetPosition()));
    });
    Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == WXK_LEFT) Select(m_selected - 1);
        else if (e.GetKeyCode() == WXK_RIGHT) Select(m_selected + 1);
        else e.Skip();
    });
    Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
    Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) { Refresh(); e.Skip(); });
}

wxRect ChipPicker::ChipRect(int i) const {
    return wxRect(1 + i * (m_chipWidth + FromDIP(8)), 1, m_chipWidth, GetClientSize().y - 2);
}

int ChipPicker::HitTest(const wxPoint& p) const {
    for (int i = 0; i < (int)m_labels.size(); ++i)
        if (ChipRect(i).Contains(p)) return i;
    return -1;
}

void ChipPicker::Select(int index) {
    if (index < 0 || index >= (int)m_labels.size() || index == m_selected) return;
    m_selected = index;
    Refresh();
    if (m_onChange) m_onChange(m_selected);
}

void ChipPicker::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto gc = BeginPaint(this, dc);
    if (!gc) return;
    for (int i = 0; i < (int)m_labels.size(); ++i) {
        const wxRect r = ChipRect(i);
        const bool sel = i == m_selected, hov = i == m_hovered;
        const double radius = r.height / 2.0;
        gc->SetBrush(wxBrush(sel ? wxColour(255, 241, 232) : hov ? wxColour(244, 246, 250) : Theme::kCard));
        gc->SetPen(wxPen(sel ? Theme::kOrange : hov ? wxColour(190, 199, 214) : Theme::kBorder,
                         sel ? FromDIP(2) : 1));
        gc->DrawRoundedRectangle(r.x + 0.5, r.y + 0.5, r.width - 1, r.height - 1, radius);
        if (sel && HasFocus()) {
            gc->SetPen(wxPen(wxColour(242, 106, 33, 70), FromDIP(3)));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawRoundedRectangle(r.x - 1, r.y - 1, r.width + 2, r.height + 2, radius + 1);
        }
        gc->SetFont(GetFont(), sel ? Theme::kOrangeDark : Theme::kText);
        wxDouble tw, th;
        gc->GetTextExtent(m_labels[i], &tw, &th);
        gc->DrawText(m_labels[i], r.x + (r.width - tw) / 2, r.y + (r.height - th) / 2);
    }
}

// ---------------------------------------------------------------------------
// HeartToggle
// ---------------------------------------------------------------------------
HeartToggle::HeartToggle(wxWindow* parent, bool on, int sizeDip)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE), m_on(on), m_sizeDip(sizeDip) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetCursor(wxCursor(wxCURSOR_HAND));
    SetInitialSize();
    Bind(wxEVT_PAINT, &HeartToggle::OnPaint, this);
    auto hoverTo = [this](double target) {
        const double from = m_hover;
        m_hoverTween.Start(160, [this, from, target](double t) { m_hover = from + (target - from) * t; Refresh(); });
    };
    Bind(wxEVT_ENTER_WINDOW, [hoverTo](wxMouseEvent&) { hoverTo(1.0); });
    Bind(wxEVT_LEAVE_WINDOW, [hoverTo](wxMouseEvent&) { hoverTo(0.0); });
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
        m_on = !m_on;
        m_popTween.Start(320, [this](double t) {
            m_pop = std::sin(t * 3.14159265358979);  // up and back down
            Refresh();
        });
        if (m_onToggle) m_onToggle(m_on);
    });
    // Clicks on the heart shouldn't also count as clicks on whatever it sits on.
    Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent&) {});
}

void HeartToggle::SetOn(bool on) {
    if (on == m_on) return;
    m_on = on;
    Refresh();
}

void HeartToggle::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto gc = BeginPaint(this, dc);
    if (!gc) return;
    const wxSize s = GetClientSize();
    const double d = std::min(s.x, s.y) - 2.0;
    const double cx = s.x / 2.0, cy = s.y / 2.0;

    gc->SetPen(wxPen(Mix(Theme::kBorder, wxColour(230, 60, 80), m_hover * 0.6), 1));
    gc->SetBrush(wxBrush(*wxWHITE));
    gc->DrawEllipse(cx - d / 2, cy - d / 2, d, d);

    // Heart made of two arcs and a point, scaled up a little while it "pops".
    const double r = d * 0.15 * (1.0 + 0.25 * m_pop);
    const double top = cy - r * 0.55;
    wxGraphicsPath heart = gc->CreatePath();
    heart.MoveToPoint(cx, cy + r * 2.0);
    heart.AddCurveToPoint(cx - r * 2.6, cy + r * 0.2, cx - r * 1.9, top - r * 1.6, cx, top - r * 0.2);
    heart.AddCurveToPoint(cx + r * 1.9, top - r * 1.6, cx + r * 2.6, cy + r * 0.2, cx, cy + r * 2.0);
    heart.CloseSubpath();
    const wxColour red(230, 60, 80);
    if (m_on) {
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(red));
    } else {
        gc->SetPen(wxPen(Mix(Theme::kMuted, red, m_hover), FromDIP(2)));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
    }
    gc->DrawPath(heart);
}

// ---------------------------------------------------------------------------
// ProgressBar
// ---------------------------------------------------------------------------
ProgressBar::ProgressBar(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(-1, FromDIP(8)));
    Bind(wxEVT_PAINT, &ProgressBar::OnPaint, this);
}

void ProgressBar::SetValue(double value) {
    value = std::clamp(value, 0.0, 1.0);
    if (value == m_target) return;
    const double from = m_shown;
    m_target = value;
    m_tween.Start(450, [this, from](double t) {
        m_shown = from + (m_target - from) * t;
        Refresh(false);
    });
}

void ProgressBar::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto gc = BeginPaint(this, dc);
    if (!gc) return;
    const wxSize s = GetClientSize();
    const double r = s.y / 2.0;
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(wxColour(233, 237, 243)));
    gc->DrawRoundedRectangle(0, 0, s.x, s.y, r);
    if (m_shown <= 0.001) return;
    const double w = std::max((double)s.y, s.x * m_shown);
    const bool full = m_shown >= 0.999;
    const wxColour a = full ? Theme::kSuccess : Theme::kOrange;
    const wxColour b = full ? wxColour(40, 170, 110) : wxColour(255, 150, 70);
    gc->SetBrush(gc->CreateLinearGradientBrush(0, 0, w, 0, a, b));
    gc->DrawRoundedRectangle(0, 0, w, s.y, r);
}

// ---------------------------------------------------------------------------
// StepIndicator
// ---------------------------------------------------------------------------
StepIndicator::StepIndicator(wxWindow* parent, const std::vector<wxString>& steps, int current)
    : wxPanel(parent, wxID_ANY), m_steps(steps), m_current(current) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetFont(Theme::Font(10, true));
    SetMinSize(wxSize(FromDIP(380), FromDIP(34)));
    Bind(wxEVT_PAINT, &StepIndicator::OnPaint, this);
}

void StepIndicator::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    auto gc = BeginPaint(this, dc);
    if (!gc) return;
    const wxSize s = GetClientSize();
    const int n = (int)m_steps.size();
    const double d = FromDIP(24), cy = s.y / 2.0;

    // Measure each "circle + label" and lay them out evenly with connectors.
    std::vector<double> widths;
    double total = 0;
    gc->SetFont(GetFont(), Theme::kText);
    for (const wxString& label : m_steps) {
        wxDouble tw, th;
        gc->GetTextExtent(label, &tw, &th);
        widths.push_back(d + FromDIP(8) + tw);
        total += widths.back();
    }
    const double gap = n > 1 ? std::max((double)FromDIP(16), (s.x - total) / (n - 1)) : 0;
    double x = 0;
    for (int i = 0; i < n; ++i) {
        const bool done = i < m_current, now = i == m_current;
        const wxColour accent = done ? Theme::kSuccess : now ? Theme::kOrange : wxColour(196, 204, 216);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(accent));
        gc->DrawEllipse(x, cy - d / 2, d, d);
        if (done) {
            wxGraphicsPath tick = gc->CreatePath();
            tick.MoveToPoint(x + d * 0.28, cy + d * 0.02);
            tick.AddLineToPoint(x + d * 0.44, cy + d * 0.17);
            tick.AddLineToPoint(x + d * 0.72, cy - d * 0.16);
            gc->SetPen(wxPen(*wxWHITE, FromDIP(2)));
            gc->StrokePath(tick);
        } else {
            const wxString num = wxString::Format(wxT("%d"), i + 1);
            wxDouble tw, th;
            gc->SetFont(GetFont(), *wxWHITE);
            gc->GetTextExtent(num, &tw, &th);
            gc->DrawText(num, x + (d - tw) / 2, cy - th / 2);
        }
        wxDouble tw, th;
        gc->SetFont(GetFont(), now ? Theme::kText : done ? Theme::kSuccess : Theme::kMuted);
        gc->GetTextExtent(m_steps[i], &tw, &th);
        gc->DrawText(m_steps[i], x + d + FromDIP(8), cy - th / 2);
        x += widths[i];
        if (i < n - 1) {
            gc->SetPen(wxPen(done ? Theme::kSuccess : wxColour(214, 220, 230), FromDIP(2)));
            gc->StrokeLine(x + FromDIP(8), cy, x + gap - FromDIP(8), cy);
            x += gap;
        }
    }
}

// ---------------------------------------------------------------------------
// Toast
// ---------------------------------------------------------------------------
namespace {
    class Toast : public wxFrame {
    public:
        Toast(wxFrame* owner, const wxString& title, const wxString& detail, std::function<void()> onClick)
            : wxFrame(owner, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                      wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxFRAME_TOOL_WINDOW | wxFRAME_SHAPED | wxBORDER_NONE),
              m_owner(owner), m_title(title), m_detail(detail), m_onClick(std::move(onClick)) {
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            SetBackgroundColour(Theme::kNavy);
            wxClientDC dc(this);
            dc.SetFont(Theme::Font(10));
            const int textW = std::max(dc.GetTextExtent(m_detail).x, FromDIP(170));
            SetClientSize(wxSize(textW + FromDIP(96), FromDIP(64)));
            SetCursor(wxCursor(wxCURSOR_HAND));

            wxGraphicsPath shape = wxGraphicsRenderer::GetDefaultRenderer()->CreatePath();
            shape.AddRoundedRectangle(0, 0, GetClientSize().x, GetClientSize().y, FromDIP(14));
            SetShape(shape);

            Bind(wxEVT_PAINT, &Toast::OnPaint, this);
            Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) {
                auto click = m_onClick;
                Dismiss(true);
                if (click) click();
            });
            m_owner->Bind(wxEVT_MOVE, &Toast::OnOwnerMoved, this);
            m_owner->Bind(wxEVT_SIZE, &Toast::OnOwnerMoved, this);
            m_owner->Bind(wxEVT_DESTROY, &Toast::OnOwnerDestroyed, this);
        }

        ~Toast() override {
            ForgetOwner();
            if (s_current == this) s_current = nullptr;
        }

        void Present() {
            if (s_current) s_current->Dismiss(true);
            s_current = this;
            Place(-FromDIP(14));
            SetTransparent(0);
            ShowWithoutActivating();
            m_anim.Start(340, [this](double t) {
                SetTransparent((wxByte)(245 * t));
                Place(-FromDIP(14) * (1 - t));
            });
            m_life.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { Dismiss(false); });
            m_life.StartOnce(2800);
        }

    private:
        // Just under the header, below the cart button it refers to.
        void Place(double offset) {
            if (!m_owner) return;
            const wxPoint client = m_owner->ClientToScreen(wxPoint(0, 0));
            const int right = client.x + m_owner->GetClientSize().x;
            SetPosition(wxPoint(right - GetSize().x - FromDIP(24), client.y + FromDIP(92) + (int)offset));
        }

        void OnOwnerMoved(wxEvent& e) {
            if (IsShown()) Place(0);
            e.Skip();
        }

        // The page can close while the toast is still up (add to cart, then
        // straight back to the list), and the toast is deleted after its owner.
        // Drop the owner pointer here so the destructor never touches it.
        void OnOwnerDestroyed(wxWindowDestroyEvent& e) {
            if (e.GetEventObject() == m_owner) {
                ForgetOwner();
                m_life.Stop();
                m_anim.Stop();
                Hide();
            }
            e.Skip();
        }

        void ForgetOwner() {
            if (!m_owner) return;
            m_owner->Unbind(wxEVT_MOVE, &Toast::OnOwnerMoved, this);
            m_owner->Unbind(wxEVT_SIZE, &Toast::OnOwnerMoved, this);
            m_owner->Unbind(wxEVT_DESTROY, &Toast::OnOwnerDestroyed, this);
            m_owner = nullptr;
        }

        void Dismiss(bool now) {
            m_life.Stop();
            if (s_current == this) s_current = nullptr;
            if (now) {
                m_anim.Stop();
                Destroy();
                return;
            }
            m_anim.Start(260, [this](double t) { SetTransparent((wxByte)(245 * (1 - t))); },
                         [this] { Destroy(); });
        }

        void OnPaint(wxPaintEvent&) {
            wxAutoBufferedPaintDC dc(this);
            dc.SetBackground(wxBrush(Theme::kNavy));
            dc.Clear();
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            if (!gc) return;
            const wxSize s = GetClientSize();
            gc->SetPen(wxPen(wxColour(255, 255, 255, 30), 1));
            gc->SetBrush(wxBrush(Theme::kNavy));
            gc->DrawRoundedRectangle(0.5, 0.5, s.x - 1, s.y - 1, FromDIP(14));

            const double d = FromDIP(30), x = FromDIP(18), cy = s.y / 2.0;
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(Theme::kSuccess));
            gc->DrawEllipse(x, cy - d / 2, d, d);
            wxGraphicsPath tick = gc->CreatePath();
            tick.MoveToPoint(x + d * 0.28, cy + d * 0.02);
            tick.AddLineToPoint(x + d * 0.44, cy + d * 0.17);
            tick.AddLineToPoint(x + d * 0.72, cy - d * 0.16);
            gc->SetPen(wxPen(*wxWHITE, FromDIP(2)));
            gc->StrokePath(tick);

            const double tx = x + d + FromDIP(14);
            gc->SetFont(Theme::Font(11, true), *wxWHITE);
            gc->DrawText(m_title, tx, FromDIP(12));
            gc->SetFont(Theme::Font(10), Theme::kOnDarkMuted);
            gc->DrawText(m_detail, tx, FromDIP(34));
        }

        static inline Toast* s_current = nullptr;
        wxFrame* m_owner;
        wxString m_title, m_detail;
        std::function<void()> m_onClick;
        Tween m_anim;
        wxTimer m_life;
    };
}

void ShowToast(wxFrame* owner, const wxString& title, const wxString& detail, std::function<void()> onClick) {
    (new Toast(owner, title, detail, std::move(onClick)))->Present();
}

// ---------------------------------------------------------------------------
// FadeIn
// ---------------------------------------------------------------------------
namespace {
    // Owns its timer and deletes itself when done, or when the window goes
    // away mid-fade.
    class Fader : public Tween {
    public:
        Fader(wxTopLevelWindow* window, std::function<void()> onShown)
            : m_window(window), m_onShown(std::move(onShown)) {
            m_window->Bind(wxEVT_DESTROY, &Fader::OnWindowDestroyed, this);
        }

        void Run() {
            Start(240, [this](double t) { m_window->SetTransparent((wxByte)(t * 255)); }, [this] {
                m_window->SetTransparent(255);
                m_window->Unbind(wxEVT_DESTROY, &Fader::OnWindowDestroyed, this);
                auto shown = std::move(m_onShown);
                delete this;
                if (shown) shown();
            });
        }

    private:
        void OnWindowDestroyed(wxWindowDestroyEvent& event) {
            if (event.GetEventObject() == m_window) {
                Stop();
                wxTheApp->CallAfter([self = this] { delete self; });
            }
            event.Skip();
        }

        wxTopLevelWindow* m_window;
        std::function<void()> m_onShown;
    };
}

void FadeIn(wxTopLevelWindow* window, std::function<void()> show, std::function<void()> onShown) {
    if (!window->CanSetTransparent()) {
        show();
        if (onShown) onShown();
        return;
    }
    window->SetTransparent(0);
    show();
    (new Fader(window, std::move(onShown)))->Run();
}

}  // namespace Widgets
