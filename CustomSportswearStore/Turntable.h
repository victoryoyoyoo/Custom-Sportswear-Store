#pragma once
#include <wx/wx.h>
#include <functional>
#include <memory>
#include "Showcase.h"
#include "Widgets.h"

// The product page's 360° view. Drag sideways to turn the product; it keeps
// turning a little after you let go. Arrow keys turn it in steps and a
// double-click brings it back to the front.
//
// While it moves, frames are drawn at half resolution so it keeps up with
// the mouse; once it stops, the last frame is redrawn at full quality.
class Turntable : public wxWindow {
public:
    using Builder = std::function<std::unique_ptr<Showcase::Model>(const wxSize& pixels)>;

    Turntable(wxWindow* parent, const wxSize& minDipSize, Builder builder);

    // What the builder makes has changed (colourway, print). With
    // crossfade the old picture dissolves into the new one.
    void Rebuild(bool crossfade);

    void TurnTo(double angle);    // shortest way round, animated
    void Spin(int delayMs);       // one full turn, e.g. when the page opens

    // Which side faces the viewer now (0 = the side the page opens on),
    // reported whenever it changes, for the 正面/背面 switch.
    void OnSideChanged(std::function<void(int side)> handler) { m_onSide = std::move(handler); }

private:
    void OnPaint(wxPaintEvent& event);
    void SetAngle(double angle, bool moving);
    void Settle();                // stopped: redraw at full quality
    void EnsureModel();
    void StartInertia();

    Builder m_builder;
    std::unique_ptr<Showcase::Model> m_model;
    wxSize m_modelSize;
    bool m_modelStale = true;

    double m_angle = 0;
    bool m_moving = false;
    Showcase::Frame m_frame;
    bool m_frameStale = true;
    int m_side = 0;
    std::function<void(int)> m_onSide;

    // dragging and coasting
    bool m_dragging = false;
    int m_lastX = 0;
    wxLongLong m_lastMoveMs;
    double m_velocity = 0;        // radians per second
    wxTimer m_coast;
    wxLongLong m_coastMs;

    Widgets::Tween m_turn, m_fade;
    wxTimer m_spinDelay;
    wxBitmap m_fadeFrom;          // previous picture during a crossfade
    double m_fadeLeft = 0;        // 1 -> 0
    double m_hint = 1;            // "drag to turn" label opacity; fades once used
    Widgets::Tween m_hintTween;
    bool m_rebuildPending = false;
    bool m_rebuildFade = false;
};
