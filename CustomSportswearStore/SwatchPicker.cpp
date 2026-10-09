#include "SwatchPicker.h"
#include "Theme.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <memory>

namespace {
    constexpr int kCellDip = 44;
    constexpr int kDotDip = 30;

    void DrawDot(wxGraphicsContext* gc, const Colorway& c, double cx, double cy, double diameter) {
        const double r = diameter / 2;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(c.trim));
        gc->DrawEllipse(cx - r, cy - r, diameter, diameter);
        const double inner = diameter * 0.72;
        gc->SetBrush(wxBrush(c.fabric));
        gc->DrawEllipse(cx - inner / 2, cy - inner / 2, inner, inner);
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(wxPen(wxColour(0, 0, 0, 40), 1));
        gc->DrawEllipse(cx - r, cy - r, diameter, diameter);
    }
}

SwatchPicker::SwatchPicker(wxWindow* parent, int columns)
    : wxPanel(parent, wxID_ANY), m_columns(columns) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    const int count = (int)Catalog::Colorways().size();
    const int rows = (count + columns - 1) / columns;
    SetMinSize(FromDIP(wxSize(columns * kCellDip, rows * kCellDip)));
    SetCursor(wxCursor(wxCURSOR_HAND));

    Bind(wxEVT_PAINT, &SwatchPicker::OnPaint, this);
    Bind(wxEVT_MOTION, &SwatchPicker::OnMouseMove, this);
    Bind(wxEVT_LEAVE_WINDOW, &SwatchPicker::OnMouseLeave, this);
    Bind(wxEVT_LEFT_UP, &SwatchPicker::OnLeftUp, this);
}

void SwatchPicker::SetSelection(int index) {
    if (index < 0 || index >= (int)Catalog::Colorways().size() || index == m_selected) return;
    m_previous = m_selected;
    m_selected = index;
    m_ringTween.Start(260, [this](double t) {
        m_ring = t;
        Refresh(false);
    });
    if (m_onChange) m_onChange(m_selected);
}

wxRect SwatchPicker::CellRect(int index) const {
    const int cell = FromDIP(kCellDip);
    return wxRect((index % m_columns) * cell, (index / m_columns) * cell, cell, cell);
}

int SwatchPicker::HitTest(const wxPoint& pos) const {
    const int count = (int)Catalog::Colorways().size();
    for (int i = 0; i < count; ++i)
        if (CellRect(i).Contains(pos)) return i;
    return -1;
}

void SwatchPicker::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
    dc.Clear();

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    const auto& colorways = Catalog::Colorways();
    const double dot = FromDIP(kDotDip);
    for (int i = 0; i < (int)colorways.size(); ++i) {
        wxRect cell = CellRect(i);
        const double cx = cell.x + cell.width / 2.0;
        const double cy = cell.y + cell.height / 2.0;

        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        if (i == m_selected) {
            const double ring = dot + FromDIP(10) * m_ring;
            gc->SetPen(wxPen(Widgets::Mix(GetParent()->GetBackgroundColour(), Theme::kOrange, m_ring), FromDIP(3)));
            gc->DrawEllipse(cx - ring / 2, cy - ring / 2, ring, ring);
        } else if (i == m_previous && m_ring < 1.0) {
            const double ring = dot + FromDIP(10);
            gc->SetPen(wxPen(Widgets::Mix(Theme::kOrange, GetParent()->GetBackgroundColour(), m_ring), FromDIP(3)));
            gc->DrawEllipse(cx - ring / 2, cy - ring / 2, ring, ring);
        } else if (i == m_hovered) {
            const double ring = dot + FromDIP(8);
            gc->SetPen(wxPen(Theme::kBorder, FromDIP(2)));
            gc->DrawEllipse(cx - ring / 2, cy - ring / 2, ring, ring);
        }
        DrawDot(gc.get(), colorways[i], cx, cy, dot);
    }
}

void SwatchPicker::OnMouseMove(wxMouseEvent& event) {
    int hit = HitTest(event.GetPosition());
    if (hit != m_hovered) {
        m_hovered = hit;
        SetToolTip(hit >= 0 ? Catalog::Colorways()[hit].name : wxString());
        Refresh();
    }
}

void SwatchPicker::OnMouseLeave(wxMouseEvent&) {
    m_hovered = -1;
    Refresh();
}

void SwatchPicker::OnLeftUp(wxMouseEvent& event) {
    SetSelection(HitTest(event.GetPosition()));
}
