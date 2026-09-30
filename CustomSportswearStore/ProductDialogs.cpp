#include "ProductDialogs.h"
#include "Theme.h"
#include <wx/clipbrd.h>
#include <wx/statline.h>
#include <wx/tokenzr.h>
#include <algorithm>
#include <cmath>
#include <map>

// ===========================================================================
// SizeAdvisorDialog
// ===========================================================================
SizeAdvisorDialog::SizeAdvisorDialog(wxWindow* parent, const Product& product)
    : wxDialog(parent, wxID_ANY, wxT("尺寸建議｜運動用品客製購物系統")), m_product(product) {
    SetBackgroundColour(Theme::kPage);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    const bool shoes = product.sizeAdvice == SizeAdvice::Shoes;
    root->Add(Theme::MakeHeader(this, wxT("尺寸建議"),
                                shoes ? wxString(wxT("量一下腳長（腳跟到最長的腳趾）"))
                                      : wxString(wxT("輸入身高與體重，推薦最接近的尺寸"))),
              0, wxEXPAND);

    Widgets::Card* card = Theme::MakeCard(this);
    wxBoxSizer* body = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* form = new wxFlexGridSizer(2, FromDIP(10), FromDIP(14));
    auto addField = [&](const wxString& text, wxWindow* field) {
        form->Add(Theme::MakeLabel(card, text, 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
        field->SetFont(Theme::Font(12));
        form->Add(field);
    };
    if (shoes) {
        m_footLength = new wxSpinCtrlDouble(card, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(120, -1)),
                                            wxSP_ARROW_KEYS, 22.0, 30.0, 26.0, 0.5);
        addField(wxT("腳長（cm）"), m_footLength);
        m_footLength->Bind(wxEVT_SPINCTRLDOUBLE, [this](wxSpinDoubleEvent&) { Recalculate(); });
        m_footLength->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Recalculate(); });
    } else {
        m_height = new wxSpinCtrl(card, wxID_ANY, wxT("172"), wxDefaultPosition, FromDIP(wxSize(120, -1)),
                                  wxSP_ARROW_KEYS, 120, 210, 172);
        m_weight = new wxSpinCtrl(card, wxID_ANY, wxT("65"), wxDefaultPosition, FromDIP(wxSize(120, -1)),
                                  wxSP_ARROW_KEYS, 30, 150, 65);
        addField(wxT("身高（cm）"), m_height);
        addField(wxT("體重（kg）"), m_weight);
        for (wxSpinCtrl* spin : { m_height, m_weight }) {
            spin->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { Recalculate(); });
            spin->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { Recalculate(); });
        }
    }
    body->Add(form, 0, wxALIGN_CENTER);
    body->Add(new wxStaticLine(card), 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(18));
    body->Add(Theme::MakeLabel(card, wxT("建議尺寸"), 10, false, Theme::kMuted), 0, wxALIGN_CENTER);
    m_result = Theme::MakeLabel(card, wxEmptyString, 26, true, Theme::kOrange);
    body->Add(m_result, 0, wxALIGN_CENTER | wxTOP, FromDIP(4));
    m_detail = Theme::MakeLabel(card, wxEmptyString, 10, false, Theme::kMuted);
    body->Add(m_detail, 0, wxALIGN_CENTER | wxTOP, FromDIP(6));
    body->Add(Theme::MakeLabel(card, shoes ? wxString(wxT("兩個尺寸之間時選大一號，穿厚襪也比較舒服"))
                                           : wxString(wxT("喜歡寬鬆一點的穿法，可以再選大一號")),
                               9, false, Theme::kMuted),
              0, wxALIGN_CENTER | wxTOP, FromDIP(12));
    wxBoxSizer* pad = new wxBoxSizer(wxVERTICAL);
    pad->Add(body, 1, wxEXPAND | wxALL, FromDIP(28));
    card->SetSizer(pad);
    root->Add(card, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    auto* cancel = Theme::MakeSecondaryButton(this, wxT("取消"), 11);
    auto* apply = Theme::MakePrimaryButton(this, wxT("套用這個尺寸"), 12);
    footer->AddStretchSpacer();
    footer->Add(cancel, 0, wxALIGN_CENTER_VERTICAL);
    footer->Add(apply, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    root->Add(footer, 0, wxEXPAND | wxALL, FromDIP(20));
    cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    apply->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });

    SetSizer(root);
    SetMinClientSize(wxSize(FromDIP(420), -1));
    Recalculate();
    Fit();
    CentreOnParent();
}

void SizeAdvisorDialog::Recalculate() {
    const int count = (int)m_product.sizes.size();
    if (m_footLength) {
        // EU 38 fits a 24.0 cm foot and every size adds 0.5 cm; round up.
        const double length = m_footLength->GetValue();
        m_recommended = std::clamp((int)std::ceil((length - 24.0) / 0.5 - 1e-9), 0, count - 1);
    } else {
        // Height picks the base size, build nudges it one either way.
        const int height = m_height->GetValue();
        const double metres = height / 100.0;
        const double bmi = m_weight->GetValue() / (metres * metres);
        int size = height < 160 ? 0 : height < 170 ? 1 : height < 178 ? 2 : height < 186 ? 3 : 4;
        if (bmi >= 26) ++size;
        else if (bmi < 19) --size;
        m_recommended = std::clamp(size, 0, count - 1);
    }
    const SizeOption& opt = m_product.sizes[m_recommended];
    m_result->SetLabel(m_footLength ? wxT("EU ") + opt.label : opt.label);
    m_detail->SetLabel(opt.hint);
    Layout();
}

// ===========================================================================
// TeamOrderDialog
// ===========================================================================
TeamOrderDialog::TeamOrderDialog(wxWindow* parent, const Product& product, const Colorway& colorway,
                                 const wxString& team)
    : wxDialog(parent, wxID_ANY, wxT("團體訂購｜運動用品客製購物系統"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_product(product) {
    SetBackgroundColour(Theme::kPage);
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    root->Add(Theme::MakeHeader(this, wxT("團體訂購"),
                                wxString::Format(wxT("%s・%s，每件 %s，5 件以上可用 TEAM10 打 9 折"),
                                                 product.name, colorway.name, Theme::FormatPrice(product.price))),
              0, wxEXPAND);

    Widgets::Card* card = Theme::MakeCard(this);
    wxBoxSizer* body = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* teamRow = new wxBoxSizer(wxHORIZONTAL);
    teamRow->Add(Theme::MakeLabel(card, wxT("隊名（印在正面）"), 11, false, Theme::kMuted), 0, wxALIGN_CENTER_VERTICAL);
    m_team = new wxTextCtrl(card, wxID_ANY, team);
    m_team->SetFont(Theme::Font(11));
    m_team->SetMaxLength(14);
    m_team->SetHint(wxT("選填，例如：TIGERS"));
    teamRow->Add(m_team, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    body->Add(teamRow, 0, wxEXPAND);

    // column titles, same widths as the rows below
    wxBoxSizer* head = new wxBoxSizer(wxHORIZONTAL);
    auto title = [&](const wxString& text, int widthDip, int proportion) {
        wxStaticText* t = Theme::MakeLabel(card, text, 10, true, Theme::kMuted);
        t->SetMinSize(FromDIP(wxSize(widthDip, -1)));
        head->Add(t, proportion, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    };
    title(wxT("#"), 26, 0);
    title(wxT("印製姓名"), 160, 1);
    title(wxT("背號"), 90, 0);
    title(wxT("尺寸"), 80, 0);
    title(wxEmptyString, 36, 0);
    body->Add(head, 0, wxEXPAND | wxTOP, FromDIP(18));
    body->Add(new wxStaticLine(card), 0, wxEXPAND | wxTOP | wxBOTTOM, FromDIP(6));

    m_list = new wxScrolledWindow(card, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(520, 300)), wxVSCROLL);
    m_list->SetBackgroundColour(Theme::kCard);
    m_list->SetScrollRate(0, FromDIP(12));
    m_listSizer = new wxBoxSizer(wxVERTICAL);
    m_list->SetSizer(m_listSizer);
    body->Add(m_list, 1, wxEXPAND);

    wxBoxSizer* tools = new wxBoxSizer(wxHORIZONTAL);
    auto* addRow = Theme::MakeSecondaryButton(card, wxT("＋ 新增球員"), 10);
    auto* paste = Theme::MakeSecondaryButton(card, wxT("貼上名單"), 10);
    paste->SetToolTip(wxT("從 Excel 或記事本複製「姓名、背號、尺寸」，一行一位"));
    tools->Add(addRow);
    tools->Add(paste, 0, wxLEFT, FromDIP(8));
    tools->AddStretchSpacer();
    m_warning = Theme::MakeLabel(card, wxEmptyString, 10, false, Theme::kOrangeDark);
    tools->Add(m_warning, 0, wxALIGN_CENTER_VERTICAL);
    body->Add(tools, 0, wxEXPAND | wxTOP, FromDIP(12));

    wxBoxSizer* pad = new wxBoxSizer(wxVERTICAL);
    pad->Add(body, 1, wxEXPAND | wxALL, FromDIP(24));
    card->SetSizer(pad);
    root->Add(card, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

    wxBoxSizer* footer = new wxBoxSizer(wxHORIZONTAL);
    m_summary = Theme::MakeLabel(this, wxEmptyString, 12, true);
    footer->Add(m_summary, 0, wxALIGN_CENTER_VERTICAL);
    footer->AddStretchSpacer();
    auto* cancel = Theme::MakeSecondaryButton(this, wxT("取消"), 11);
    m_confirm = Theme::MakePrimaryButton(this, wxT("全部加入購物車（0 件）"), 12);
    m_confirm->ShowArrow();
    footer->Add(cancel, 0, wxALIGN_CENTER_VERTICAL);
    footer->Add(m_confirm, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(10));
    root->Add(footer, 0, wxEXPAND | wxALL, FromDIP(20));

    addRow->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        int next = 0;
        for (const Row& r : m_rows) next = std::max(next, r.number->GetValue() + 1);
        AddRow(wxEmptyString, std::min(next, 99), m_product.defaultSize);
        if (!m_rows.empty()) m_rows.back().name->SetFocus();
    });
    paste->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { PasteRoster(); });
    cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_CANCEL); });
    m_confirm->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (!Players().empty()) EndModal(wxID_OK);
    });

    for (int i = 0; i < 5; ++i) AddRow(wxEmptyString, i + 1, product.defaultSize);
    SetSizer(root);
    Fit();
    SetMinClientSize(GetClientSize());
    CentreOnParent();
    m_rows.front().name->SetFocus();
}

void TeamOrderDialog::AddRow(const wxString& name, int number, int sizeIndex) {
    if (m_rows.size() >= 40) return;
    wxPanel* panel = new wxPanel(m_list);
    panel->SetBackgroundColour(Theme::kCard);
    wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* index = Theme::MakeLabel(panel, wxEmptyString, 10, false, Theme::kMuted);
    index->SetMinSize(FromDIP(wxSize(26, -1)));
    wxTextCtrl* nameCtrl = new wxTextCtrl(panel, wxID_ANY, name);
    nameCtrl->SetFont(Theme::Font(11));
    nameCtrl->SetMaxLength(12);
    nameCtrl->SetHint(wxT("例如：WANG"));
    wxSpinCtrl* numberCtrl = new wxSpinCtrl(panel, wxID_ANY, wxString::Format(wxT("%d"), number), wxDefaultPosition,
                                            FromDIP(wxSize(90, -1)), wxSP_ARROW_KEYS, 0, 99, number);
    numberCtrl->SetFont(Theme::Font(11));
    wxArrayString sizes;
    for (const SizeOption& s : m_product.sizes) sizes.Add(s.label);
    wxChoice* sizeCtrl = new wxChoice(panel, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(80, -1)), sizes);
    sizeCtrl->SetFont(Theme::Font(11));
    sizeCtrl->SetSelection(std::clamp(sizeIndex, 0, (int)sizes.size() - 1));
    auto* remove = Theme::MakeSecondaryButton(panel, wxT("✕"), 10);
    remove->SetMinSize(FromDIP(wxSize(36, 32)));
    remove->SetToolTip(wxT("移除這位球員"));

    row->Add(index, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    row->Add(nameCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    row->Add(numberCtrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    row->Add(sizeCtrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    row->Add(remove, 0, wxALIGN_CENTER_VERTICAL);
    panel->SetSizer(row);
    m_listSizer->Add(panel, 0, wxEXPAND | wxBOTTOM, FromDIP(6));
    m_rows.push_back({ panel, index, nameCtrl, numberCtrl, sizeCtrl });

    nameCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { RefreshSummary(); });
    numberCtrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { RefreshSummary(); });
    numberCtrl->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { RefreshSummary(); });
    // The button can't destroy its own row while its click is still being handled.
    remove->Bind(wxEVT_BUTTON, [this, panel](wxCommandEvent&) { CallAfter([this, panel] { RemoveRow(panel); }); });

    m_list->FitInside();
    m_list->Layout();
    m_list->Scroll(0, m_list->GetVirtualSize().y);
    RefreshSummary();
}

void TeamOrderDialog::RemoveRow(wxPanel* panel) {
    auto it = std::find_if(m_rows.begin(), m_rows.end(), [panel](const Row& r) { return r.panel == panel; });
    if (it == m_rows.end()) return;
    m_rows.erase(it);
    panel->Destroy();
    m_list->FitInside();
    m_list->Layout();
    RefreshSummary();
}

void TeamOrderDialog::PasteRoster() {
    wxString text;
    if (wxTheClipboard->Open()) {
        if (wxTheClipboard->IsSupported(wxDF_UNICODETEXT) || wxTheClipboard->IsSupported(wxDF_TEXT)) {
            wxTextDataObject data;
            wxTheClipboard->GetData(data);
            text = data.GetText();
        }
        wxTheClipboard->Close();
    }
    std::vector<Player> pasted;
    for (const wxString& line : wxStringTokenize(text, wxT("\r\n"), wxTOKEN_STRTOK)) {
        // "WANG 7 L", "WANG,7,L" or tab-separated from a spreadsheet; number and size optional.
        wxArrayString parts = wxStringTokenize(line, wxT(" \t,，"), wxTOKEN_STRTOK);
        if (parts.empty()) continue;
        Player p{ parts[0].Upper(), 0, m_product.defaultSize };
        for (size_t i = 1; i < parts.size(); ++i) {
            long number = 0;
            if (parts[i].ToLong(&number)) {
                p.number = std::clamp((int)number, 0, 99);
                continue;
            }
            for (size_t s = 0; s < m_product.sizes.size(); ++s)
                if (m_product.sizes[s].label.IsSameAs(parts[i], false)) p.sizeIndex = (int)s;
        }
        pasted.push_back(p);
    }
    if (pasted.empty()) {
        m_warning->SetLabel(wxT("剪貼簿裡沒有名單：請複製「姓名 背號 尺寸」，一行一位"));
        Layout();
        return;
    }
    // Rows the user hasn't filled in make way for the pasted roster.
    std::vector<wxPanel*> blank;
    for (const Row& r : m_rows)
        if (r.name->GetValue().Trim().IsEmpty()) blank.push_back(r.panel);
    for (wxPanel* p : blank) RemoveRow(p);
    for (const Player& p : pasted) AddRow(p.name, p.number, p.sizeIndex);
}

void TeamOrderDialog::RefreshSummary() {
    std::map<int, int> numbersUsed;
    int filled = 0;
    for (size_t i = 0; i < m_rows.size(); ++i) {
        m_rows[i].index->SetLabel(wxString::Format(wxT("%zu"), i + 1));
        wxString name = m_rows[i].name->GetValue();
        if (name.Trim().IsEmpty()) continue;
        ++filled;
        ++numbersUsed[m_rows[i].number->GetValue()];
    }
    wxString duplicates;
    for (const auto& [number, count] : numbersUsed)
        if (count > 1) duplicates += (duplicates.IsEmpty() ? wxString() : wxString(wxT("、"))) + wxString::Format(wxT("%d"), number);
    m_warning->SetLabel(duplicates.IsEmpty() ? wxString() : wxT("背號重複：") + duplicates);
    m_summary->SetLabel(wxString::Format(wxT("%d 件・%s"), filled, Theme::FormatPrice(filled * m_product.price)));
    m_confirm->Enable(filled > 0);
    m_confirm->SetLabel(wxString::Format(wxT("全部加入購物車（%d 件）"), filled));
    Layout();
}

std::vector<TeamOrderDialog::Player> TeamOrderDialog::Players() const {
    std::vector<Player> players;
    for (const Row& r : m_rows) {
        wxString name = r.name->GetValue();
        name.Trim().Trim(false);
        if (name.IsEmpty()) continue;
        players.push_back({ name.Upper(), r.number->GetValue(), r.size->GetSelection() });
    }
    return players;
}

wxString TeamOrderDialog::TeamName() const {
    wxString team = m_team->GetValue();
    team.Trim().Trim(false);
    return team.Upper();
}
