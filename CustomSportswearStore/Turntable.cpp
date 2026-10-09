#include "Turntable.h"
#include "Lang.h"
#include "Theme.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <algorithm>
#include <cmath>

namespace {
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kFriction = 3.2;
    constexpr double kKeyStep = kPi / 8;
}

Turntable::Turntable(wxWindow* parent, const wxSize& minDipSize, Builder builder)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_NONE),
      m_builder(std::move(builder)) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(FromDIP(minDipSize));
    SetCursor(wxCursor(wxCURSOR_SIZEWE));
    SetToolTip(L(wxT("左右拖曳可 360° 旋轉，雙擊回到正面"), wxT("Drag sideways to turn it round; double-click to face front")));

    Bind(wxEVT_PAINT, &Turntable::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        m_modelStale = m_frameStale = true;
        Refresh(false);
        event.Skip();
    });

    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
        SetFocus();
        m_turn.Stop();
        m_coast.Stop();
        m_dragging = true;
        m_lastX = event.GetX();
        m_lastMoveMs = wxGetLocalTimeMillis();
        m_velocity = 0;
        if (!HasCapture()) CaptureMouse();
    });
    Bind(wxEVT_MOTION, [this](wxMouseEvent& event) {
        if (!m_dragging) return;
        if (!event.LeftIsDown()) {
            m_dragging = false;
            if (HasCapture()) ReleaseMouse();
            Settle();
            return;
        }
        const double perPixel = kPi / std::max(1, (int)(GetClientSize().x * 0.6));
        const double delta = (event.GetX() - m_lastX) * perPixel;
        const wxLongLong now = wxGetLocalTimeMillis();
        const double dt = std::max(1.0, (now - m_lastMoveMs).ToDouble()) / 1000.0;
        m_velocity = 0.6 * m_velocity + 0.4 * (delta / dt);
        m_lastX = event.GetX();
        m_lastMoveMs = now;
        SetAngle(m_angle + delta, true);
        if (m_hint > 0 && !m_hintTween.IsRunning())
            m_hintTween.Start(400, [this](double t) { m_hint = 1 - t; Refresh(false); });
    });
    auto release = [this] {
        if (!m_dragging) return;
        m_dragging = false;
        if (HasCapture()) ReleaseMouse();
        if ((wxGetLocalTimeMillis() - m_lastMoveMs).ToDouble() > 80) m_velocity = 0;
        StartInertia();
    };
    Bind(wxEVT_LEFT_UP, [release](wxMouseEvent&) { release(); });
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { m_dragging = false; Settle(); });
    Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent&) { TurnTo(0); });
    Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
        switch (event.GetKeyCode()) {
        case WXK_LEFT:  TurnTo(std::round(m_angle / kKeyStep) * kKeyStep - kKeyStep); break;
        case WXK_RIGHT: TurnTo(std::round(m_angle / kKeyStep) * kKeyStep + kKeyStep); break;
        case WXK_HOME:  TurnTo(0); break;
        default:        event.Skip();
        }
    });

    m_coast.Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        const wxLongLong now = wxGetLocalTimeMillis();
        const double dt = std::min(0.05, (now - m_coastMs).ToDouble() / 1000.0);
        m_coastMs = now;
        m_velocity *= std::exp(-kFriction * dt);
        if (std::abs(m_velocity) < 0.15) {
            m_coast.Stop();
            Settle();
            return;
        }
        SetAngle(m_angle + m_velocity * dt, true);
    });
    m_spinDelay.Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        const double from = m_angle - 2 * kPi, to = m_angle;
        m_turn.Start(1700, [this, from, to](double t) { SetAngle(from + (to - from) * t, t < 1); },
                     [this] { Settle(); });
    });
}

void Turntable::StartInertia() {
    if (std::abs(m_velocity) < 0.3) {
        Settle();
        return;
    }
    m_velocity = std::clamp(m_velocity, -4 * kPi, 4 * kPi);
    m_coastMs = wxGetLocalTimeMillis();
    m_coast.Start(15);
}

void Turntable::Rebuild(bool crossfade) {
    m_rebuildFade = m_rebuildFade || crossfade;
    if (m_rebuildPending) return;
    m_rebuildPending = true;
    CallAfter([this] {
        m_rebuildPending = false;
        const bool fade = m_rebuildFade;
        m_rebuildFade = false;
        if (fade && m_frame.image.IsOk()) {
            m_fadeFrom = wxBitmap(m_frame.image);
            m_fadeLeft = 1;
            m_fade.Start(260, [this](double t) { m_fadeLeft = 1 - t; Refresh(false); },
                         [this] { m_fadeFrom = wxBitmap(); m_fadeLeft = 0; Refresh(false); });
        }
        m_modelStale = m_frameStale = true;
        Refresh(false);
    });
}

void Turntable::TurnTo(double angle) {
    m_coast.Stop();
    const double target = m_angle + std::remainder(angle - m_angle, 2 * kPi);
    const double from = m_angle;
    if (std::abs(target - from) < 1e-6) { Settle(); return; }
    m_turn.Start(620, [this, from, target](double t) { SetAngle(from + (target - from) * t, t < 1); },
                 [this] { Settle(); });
}

void Turntable::Spin(int delayMs) {
    m_spinDelay.StartOnce(std::max(1, delayMs));
}

void Turntable::SetAngle(double angle, bool moving) {
    m_angle = angle;
    m_moving = moving;
    m_frameStale = true;
    const int side = std::cos(angle) >= 0 ? 0 : 1;
    if (side != m_side) {
        m_side = side;
        if (m_onSide) m_onSide(side);
    }
    Refresh(false);
}

void Turntable::Settle() {
    m_angle = std::remainder(m_angle, 2 * kPi);
    m_moving = false;
    m_frameStale = true;
    Refresh(false);
}

void Turntable::EnsureModel() {
    const wxSize size = GetClientSize();
    if (!m_modelStale && m_model && size == m_modelSize) return;
    m_modelSize = size;
    m_modelStale = false;
    m_model = (size.x >= 16 && size.y >= 16 && m_builder) ? m_builder(size) : nullptr;
}

void Turntable::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    const wxColour surround = GetParent()->GetBackgroundColour();
    dc.SetBackground(wxBrush(surround));
    dc.Clear();
    const wxSize size = GetClientSize();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc || size.x < 16 || size.y < 16) return;
    gc->SetInterpolationQuality(wxINTERPOLATION_GOOD);
    Showcase::DrawStage(gc.get(), wxRect2DDouble(0, 0, size.x, size.y), surround);

    EnsureModel();
    if (m_model && m_frameStale) {
        const wxSize renderSize = m_moving ? wxSize(size.x / 2, size.y / 2) : size;
        m_frame = m_model->Render(m_angle, renderSize, !m_moving);
        if (renderSize != size) {
            const double k = (double)size.x / renderSize.x;
            wxRect& b = m_frame.bounds;
            b = wxRect((int)(b.x * k), (int)(b.y * k), (int)(b.width * k), (int)(b.height * k));
        }
        m_frameStale = false;
    }
    if (m_frame.image.IsOk()) {
        Showcase::DrawShadow(gc.get(), m_frame.bounds);
        gc->DrawBitmap(wxBitmap(m_frame.image), 0, 0, size.x, size.y);
    }
    if (m_fadeLeft > 0 && m_fadeFrom.IsOk()) {
        gc->BeginLayer(m_fadeLeft);
        Showcase::DrawStage(gc.get(), wxRect2DDouble(0, 0, size.x, size.y), surround);
        gc->DrawBitmap(m_fadeFrom, 0, 0, size.x, size.y);
        gc->EndLayer();
    }

    if (m_hint > 0.01) {
        const wxString label = L(wxT("⟲  拖曳旋轉 360°"), wxT("⟲  Drag to turn 360°"));
        gc->SetFont(Theme::Font(9, true), wxColour(90, 100, 118, (unsigned char)(255 * m_hint)));
        wxDouble w, h;
        gc->GetTextExtent(label, &w, &h);
        const double padX = FromDIP(12), padY = FromDIP(5);
        const double bw = w + padX * 2, bh = h + padY * 2;
        const double x = (size.x - bw) / 2, y = size.y - bh - FromDIP(10);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(wxColour(255, 255, 255, (unsigned char)(215 * m_hint))));
        gc->DrawRoundedRectangle(x, y, bw, bh, bh / 2);
        gc->DrawText(label, x + padX, y + padY);
    }
}
