#ifndef VBAM_WX_CHEATDATABASE_H_
#define VBAM_WX_CHEATDATABASE_H_

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>

// One entry loaded from cheatdb.txt
struct CheatDBEntry {
    wxString game_code;   // GBA 4-char code, or GB title, used to match loaded ROM
    wxString game_title;  // human-readable game name (for display/search)
    wxString cheat_name;  // description of what the cheat does
    wxString cheat_type;  // "GS", "GG", or "CB"
    wxString cheat_code;  // the raw code string
};

// Manages the CheatDatabase dialog: loads a bundled/user-editable cheatdb.txt,
// filters entries to the currently loaded game, and injects selected codes
// into the active cheat list via the existing core cheat APIs.
class CheatDatabase_t : public wxEvtHandler {
public:
    wxDialog* dlg = nullptr;
    wxListCtrl* list = nullptr;
    wxTextCtrl* search = nullptr;
    wxStaticText* status = nullptr;

    std::vector<CheatDBEntry> all_entries;
    std::vector<CheatDBEntry> visible_entries; // currently filtered/displayed

    // Loads cheatdb.txt from the same directory as the running executable.
    // Format (pipe-delimited, one entry per line, '#' starts a comment):
    //   GAMECODE|Game Title|Cheat Name|TYPE|CODE
    // TYPE is one of: GS (GameShark/GSA), GG (Game Genie), CB (CodeBreaker)
    void LoadDatabase();

    // Repopulates `list` based on the loaded ROM's game code/title and the
    // current search box text.
    void Refresh();

    // wx event handlers
    void OnSearchChanged(wxCommandEvent& ev);
    void OnAdd(wxCommandEvent& ev);
    void OnClose(wxCommandEvent& ev);

private:
    wxString CurrentGameCode();
};

extern CheatDatabase_t cheat_database_handler;

#endif // VBAM_WX_CHEATDATABASE_H_
