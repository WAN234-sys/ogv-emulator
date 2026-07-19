# Applying the Cheat Database feature to your OGV fork

These files add an **offline cheat database** dialog ("Cheats > Cheat Database...")
that reads a `cheatdb.txt` file placed next to your `ogv.exe`, matches entries to
the currently loaded ROM, and lets the user inject a code into the existing
cheat system with one click.

⚠️ **Not compile-tested.** I don't have wxWidgets/MSYS2 available in this
sandbox to build against, so treat this as a strong first draft. If the CI
build fails, paste me the error from the Actions log and I'll fix it fast —
this kind of thing is usually a missing include or a slightly-off function
signature, not a design problem.

## 1. Copy new files into your fork

Copy these into your cloned `visualboyadvance-m` fork, preserving the paths:
```
src/wx/cheatdatabase.h
src/wx/cheatdatabase.cpp
src/wx/xrc/CheatDatabase.xrc
```

## 2. Register the new files in `src/wx/CMakeLists.txt`

Find this block (it lists the other cheat XRC files):
```
    xrc/CheatAdd.xrc
    xrc/CheatCreate.xrc
    xrc/CheatEdit.xrc
    xrc/CheatList.xrc
```
Add a line right after it:
```
    xrc/CheatDatabase.xrc
```

Then find where `.cpp` sources are listed (look for an existing entry like
`viewers.cpp` or `panel.cpp`) and add:
```
    cheatdatabase.cpp
```
And wherever the matching `.h` files are listed, add:
```
    cheatdatabase.h
```

## 3. Add the menu entry — `src/wx/xrc/MainMenu.xrc`

Find the `Cheats` submenu block:
```xml
<object class="wxMenu">
  <label>_Cheats</label>
  <object class="wxMenuItem" name="CheatsList">
    <label>List _cheats...</label>
  </object>
  <object class="wxMenuItem" name="CheatsSearch">
    <label>Find c_heat...</label>
  </object>
```
Add a new item right after `CheatsSearch`:
```xml
  <object class="wxMenuItem" name="CheatsDatabase">
    <label>Cheat _Database...</label>
  </object>
```

## 4. Add the command handler — `src/wx/cmdevents.cpp`

Near the top, add the include:
```cpp
#include "wx/cheatdatabase.h"
```

Find this block:
```cpp
EVT_HANDLER_MASK(CheatsSearch, "Create cheat...", CMDEN_GB | CMDEN_GBA)
{
    wxDialog* dlg = GetXRCDialog("CheatCreate");
    ShowModal(dlg);
}
```
Add right after it:
```cpp
EVT_HANDLER_MASK(CheatsDatabase, "Cheat Database...", CMDEN_GB | CMDEN_GBA)
{
    wxDialog* dlg = GetXRCDialog("CheatDatabase");
    cheat_database_handler.LoadDatabase();
    cheat_database_handler.Refresh();
    ShowModal(dlg);
}
```

## 5. Wire the dialog controls — `src/wx/guiinit.cpp`

Add the include near the top:
```cpp
#include "wx/cheatdatabase.h"
```

Find the `else if (name == "CheatList") { ... }` block (search for it — it's
a big block that ends with `LoadDialog("CheatEdit");`). Add a new `else if`
right after that whole block closes:

```cpp
else if (name == "CheatDatabase") {
    d = LoadXRCDialog("CheatDatabase");
    cheat_database_handler.dlg = d;
    d->SetEscapeId(wxID_CLOSE);

    wxListCtrl* lc = SafeXRCCTRL<wxListCtrl>(d, "CheatDBList");
    lc->InsertColumn(0, _("Game"));
    lc->InsertColumn(1, _("Cheat"));
    lc->InsertColumn(2, _("Type"));
    lc->SetColumnWidth(0, 150);
    lc->SetColumnWidth(1, 220);
    lc->SetColumnWidth(2, 60);
    cheat_database_handler.list = lc;

    cheat_database_handler.search = SafeXRCCTRL<wxTextCtrl>(d, "CheatDBSearch");
    cheat_database_handler.status = SafeXRCCTRL<wxStaticText>(d, "CheatDBStatus");

    d->Bind(wxEVT_TEXT, &CheatDatabase_t::OnSearchChanged, &cheat_database_handler,
        XRCID("CheatDBSearch"));
    d->Bind(wxEVT_BUTTON, &CheatDatabase_t::OnAdd, &cheat_database_handler,
        XRCID("CheatDBAdd"));
    d->Bind(wxEVT_BUTTON, &CheatDatabase_t::OnClose, &cheat_database_handler,
        wxID_CLOSE);

    d->Fit();
}
```

## 6. Ship `cheatdb.txt` alongside `ogv.exe`

Put `cheatdb.txt` (sample included in this bundle) in your repo root, and add
one line to your release workflow so it's copied next to the exe before the
release step:

```yaml
      - name: Copy cheat database next to exe
        run: cp cheatdb.txt "$(dirname '${{ steps.rename.outputs.exe_path }}')/"
```
and include it in the release files list too:
```yaml
      - name: Create GitHub Release with exe attached
        uses: softprops/action-gh-release@v2
        with:
          files: |
            ${{ steps.rename.outputs.exe_path }}
            cheatdb.txt
```

Users can then edit `cheatdb.txt` themselves to add more codes without
needing a rebuild — and you can grow it over time as your own curated
database.
