#include "wx/cheatdatabase.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>

#include "core/gb/gbCheats.h"
#include "core/gb/gbGlobals.h"
#include "core/gba/gbaCheats.h"
#include "core/gba/gbaGlobals.h"
#include "wx/wxvbam.h"

CheatDatabase_t cheat_database_handler;

wxString CheatDatabase_t::CurrentGameCode()
{
    // GBA: 4-char code lives at header offset 0xAC.
    if (g_rom) {
        return wxString(reinterpret_cast<const char*>(g_rom + 0xac), wxConvLibc, 4);
    }
    // GB/GBC: no short code, fall back to the cart title at 0x134 (up to 16 bytes).
    if (gbRom) {
        char title[17] = {0};
        memcpy(title, gbRom + 0x134, 16);
        return wxString(title, wxConvLibc).Trim().Trim(false);
    }
    return wxEmptyString;
}

void CheatDatabase_t::LoadDatabase()
{
    all_entries.clear();

    // cheatdb.txt lives next to the executable so it can be updated
    // without recompiling.
    wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
    fn.SetFullName(wxT("cheatdb.txt"));

    if (!fn.FileExists()) {
        return;
    }

    wxTextFile file;
    if (!file.Open(fn.GetFullPath())) {
        return;
    }

    for (wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine()) {
        line.Trim().Trim(false);
        if (line.IsEmpty() || line.StartsWith(wxT("#")))
            continue;

        wxArrayString parts = wxSplit(line, '|');
        if (parts.GetCount() < 5)
            continue;

        CheatDBEntry entry;
        entry.game_code = parts[0].Trim().Trim(false);
        entry.game_title = parts[1].Trim().Trim(false);
        entry.cheat_name = parts[2].Trim().Trim(false);
        entry.cheat_type = parts[3].Trim().Trim(false).Upper();
        entry.cheat_code = parts[4].Trim().Trim(false);
        all_entries.push_back(entry);
    }
    file.Close();
}

void CheatDatabase_t::Refresh()
{
    if (!list)
        return;

    list->DeleteAllItems();
    visible_entries.clear();

    wxString game_code = CurrentGameCode();
    wxString filter;
    if (search)
        filter = search->GetValue().Lower();

    if (game_code.IsEmpty()) {
        if (status)
            status->SetLabel(_("No game loaded."));
    } else {
        if (status)
            status->SetLabel(wxString::Format(_("Showing cheats for: %s"), game_code));
    }

    for (const auto& e : all_entries) {
        bool game_match = game_code.IsEmpty() || e.game_code.IsSameAs(game_code, false);
        bool text_match = filter.IsEmpty()
            || e.cheat_name.Lower().Contains(filter)
            || e.game_title.Lower().Contains(filter);

        if (!(game_match && text_match))
            continue;

        long idx = list->InsertItem(list->GetItemCount(), e.game_title);
        list->SetItem(idx, 1, e.cheat_name);
        list->SetItem(idx, 2, e.cheat_type);
        // store index into visible_entries via item data
        list->SetItemData(idx, static_cast<long>(visible_entries.size()));
        visible_entries.push_back(e);
    }
}

void CheatDatabase_t::OnSearchChanged(wxCommandEvent&)
{
    Refresh();
}

void CheatDatabase_t::OnAdd(wxCommandEvent&)
{
    if (!list)
        return;

    long sel = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel == -1) {
        wxMessageBox(_("Select a cheat from the list first."), _("No cheat selected"),
            wxOK | wxICON_INFORMATION, dlg);
        return;
    }

    long data_idx = list->GetItemData(sel);
    if (data_idx < 0 || static_cast<size_t>(data_idx) >= visible_entries.size())
        return;

    const CheatDBEntry& e = visible_entries[data_idx];
    const bool is_gba = (g_rom != nullptr);

    if (is_gba) {
        if (e.cheat_type == wxT("GS")) {
            cheatsAddGSACode(UTF8(e.cheat_code), UTF8(e.cheat_name), false);
        } else if (e.cheat_type == wxT("CB")) {
            cheatsAddCBACode(UTF8(e.cheat_code), UTF8(e.cheat_name));
        } else {
            // treat anything else (e.g. GG-style raw codes) via the generic path
            cheatsAddCheatCode(UTF8(e.cheat_code), UTF8(e.cheat_name));
        }
    } else {
        if (e.cheat_type == wxT("GG")) {
            gbAddGgCheat(UTF8(e.cheat_code), UTF8(e.cheat_name));
        } else {
            gbAddGsCheat(UTF8(e.cheat_code), UTF8(e.cheat_name));
        }
    }

    wxMessageBox(wxString::Format(_("Added cheat: %s"), e.cheat_name),
        _("Cheat added"), wxOK | wxICON_INFORMATION, dlg);
}

void CheatDatabase_t::OnClose(wxCommandEvent&)
{
    if (dlg)
        dlg->EndModal(wxID_CLOSE);
}
