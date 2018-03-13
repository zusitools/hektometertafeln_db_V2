// Copyright 2018 Zusitools

#include "gui.hpp"

#include "config.hpp"
#include "resource.hpp"

#include <windows.h>

static const char* kPropBauparameter = "__HEKTO_BAUPARAMETER";
HINSTANCE kHinstDll;

BOOL CALLBACK ConfigDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
  switch (Message) {
    case WM_INITDIALOG: {
      auto* config = reinterpret_cast<HektoDllConfig*>(lParam);
      SetProp(hwnd, kPropBauparameter, config);

      CheckDlgButton(hwnd, IDC_BEIDSEITIG, config->beidseitig == Beidseitig::kBeidseitig);
      CheckDlgButton(hwnd, IDC_KLEIN, config->groesse == Groesse::kKlein);
      CheckDlgButton(hwnd, IDC_RUECKSTRAHLEND, config->rueckstrahlend == Rueckstrahlend::kYes);
      CheckDlgButton(hwnd, IDC_ANKERPUNKT, config->ankerpunkt == Ankerpunkt::kYes);
      CheckDlgButton(hwnd, IDC_IMMER_OHNE_MAST, config->immer_ohne_mast);
      return TRUE;
    }

    case WM_COMMAND: {
      auto* config = static_cast<HektoDllConfig*>(GetProp(hwnd, kPropBauparameter));
      switch (LOWORD(wParam)) {
        case IDOK:
          EndDialog(hwnd, IDOK);
          break;
        case IDCANCEL:
          EndDialog(hwnd, IDCANCEL);
          break;
        case IDC_BEIDSEITIG:
          config->beidseitig = IsDlgButtonChecked(hwnd, IDC_BEIDSEITIG) ? Beidseitig::kBeidseitig : Beidseitig::kEinseitig;
          break;
        case IDC_KLEIN:
          config->groesse = IsDlgButtonChecked(hwnd, IDC_KLEIN) ? Groesse::kKlein : Groesse::kGross;
          break;
        case IDC_RUECKSTRAHLEND:
          config->rueckstrahlend = IsDlgButtonChecked(hwnd, IDC_RUECKSTRAHLEND) ? Rueckstrahlend::kYes : Rueckstrahlend::kNo;
          break;
        case IDC_ANKERPUNKT:
          config->ankerpunkt = IsDlgButtonChecked(hwnd, IDC_ANKERPUNKT) ? Ankerpunkt::kYes : Ankerpunkt::kNo;
          break;
        case IDC_IMMER_OHNE_MAST:
          config->immer_ohne_mast = IsDlgButtonChecked(hwnd, IDC_IMMER_OHNE_MAST);
          break;
      }
      break;
    }

    default:
      return FALSE;
  }
  return TRUE;
}

void ShowGui(HWND parentHandle, HektoDllConfig* config) {
  DialogBoxParam(kHinstDll, MAKEINTRESOURCE(IDD_HEKTO_CONFIG), parentHandle, ConfigDlgProc, (LPARAM)config);
}

int WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID /*lpReserved*/) {
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      kHinstDll = hInstDLL;
      break;

    default:
      break;
  }

  return TRUE;
}
