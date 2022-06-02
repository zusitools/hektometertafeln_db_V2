// Copyright 2018 Zusitools

#include "gui.hpp"

#include "config.hpp"
#include "resource.hpp"

#include <cassert>
#include <string>
#include <windows.h>

static const char* kPropBauparameter = "__HEKTO_BAUPARAMETER";
HINSTANCE kHinstDll;

INT_PTR CALLBACK ConfigDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
  auto SetzeUeberlaengeAktiviert = [hwnd](bool aktiviert) {
    EnableWindow(GetDlgItem(hwnd, IDC_BASIS_KM), aktiviert);
    EnableWindow(GetDlgItem(hwnd, IDC_BASIS_HM), aktiviert);
  };

  switch (Message) {
    case WM_INITDIALOG: {
      auto* config = reinterpret_cast<HektoDllConfig*>(lParam);
      SetProp(hwnd, kPropBauparameter, config);

      CheckDlgButton(hwnd, IDC_BEIDSEITIG, config->beidseitig == Beidseitig::kBeidseitig);
      CheckDlgButton(hwnd, IDC_KLEIN, config->groesse == Groesse::kKlein);
      CheckDlgButton(hwnd, IDC_RUECKSTRAHLEND, config->rueckstrahlend == Rueckstrahlend::kYes);
      CheckDlgButton(hwnd, IDC_ANKERPUNKT, config->ankerpunkt == Ankerpunkt::kYes);
      CheckDlgButton(hwnd, IDC_IMMER_OHNE_MAST, config->immer_ohne_mast);
      CheckDlgButton(hwnd, IDC_HAT_UEBERLAENGE, config->hat_ueberlaenge);
      SetzeUeberlaengeAktiviert(config->hat_ueberlaenge);

      const auto handle_basis_km = GetDlgItem(hwnd, IDC_BASIS_KM);
      SendMessage(handle_basis_km, WM_SETTEXT, 0, (LPARAM)(std::to_string(config->basis_km).c_str()));

      const auto handle_basis_hm = GetDlgItem(hwnd, IDC_BASIS_HM);
      SendMessage(handle_basis_hm, WM_SETTEXT, 0, (LPARAM)(std::to_string(config->basis_hm).c_str()));

      const auto handle_textur = GetDlgItem(hwnd, IDC_TEXTUR);
      SendMessage(handle_textur, CB_ADDSTRING, 0, (LPARAM)("Standard"));
      SendMessage(handle_textur, CB_ADDSTRING, 0, (LPARAM)("Tunnel"));
      SendMessage(handle_textur, CB_ADDSTRING, 0, (LPARAM)("Verwittert 1"));
      SendMessage(handle_textur, CB_ADDSTRING, 0, (LPARAM)("Verwittert 2"));
      SendMessage(handle_textur, CB_SETCURSEL, static_cast<WPARAM>(config->textur), 0);

      return TRUE;
    }

    case WM_COMMAND: {
      auto* config = static_cast<HektoDllConfig*>(GetProp(hwnd, kPropBauparameter));
      switch (LOWORD(wParam)) {
        case IDC_HAT_UEBERLAENGE:
          SetzeUeberlaengeAktiviert(IsDlgButtonChecked(hwnd, IDC_HAT_UEBERLAENGE));
          break;
        case IDOK: {
          config->beidseitig = IsDlgButtonChecked(hwnd, IDC_BEIDSEITIG) ? Beidseitig::kBeidseitig : Beidseitig::kEinseitig;
          config->groesse = IsDlgButtonChecked(hwnd, IDC_KLEIN) ? Groesse::kKlein : Groesse::kGross;
          config->rueckstrahlend = IsDlgButtonChecked(hwnd, IDC_RUECKSTRAHLEND) ? Rueckstrahlend::kYes : Rueckstrahlend::kNo;
          config->ankerpunkt = IsDlgButtonChecked(hwnd, IDC_ANKERPUNKT) ? Ankerpunkt::kYes : Ankerpunkt::kNo;
          config->immer_ohne_mast = IsDlgButtonChecked(hwnd, IDC_IMMER_OHNE_MAST);
          config->hat_ueberlaenge = IsDlgButtonChecked(hwnd, IDC_HAT_UEBERLAENGE);

          char buf[64];
          GetDlgItemText(hwnd, IDC_BASIS_KM, buf, sizeof(buf)/sizeof(buf[0]));
          config->basis_km = atoi(buf);
          GetDlgItemText(hwnd, IDC_BASIS_HM, buf, sizeof(buf)/sizeof(buf[0]));
          config->basis_hm = atoi(buf);

          const auto handle_textur = GetDlgItem(hwnd, IDC_TEXTUR);
          config->textur = static_cast<TexturDatei>(SendMessage(handle_textur, CB_GETCURSEL, 0, 0));

          EndDialog(hwnd, IDOK);
          break;
        }
        case IDCANCEL:
          EndDialog(hwnd, IDCANCEL);
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
