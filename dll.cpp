// Copyright 2018 Zusitools

#include "dll.hpp"

#include "config.hpp"
#include "gui.hpp"
#include "hekto_builder.hpp"

#include <shlwapi.h>
#include <windows.h>

#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

// Globale Variablen
DWORD g_zusi_datenpfad_laenge;
char g_zielverzeichnis[MAX_PATH];  // wird von Zusi beim Initialisieren gesetzt; ohne abschliessenden Slash/Backslash
char g_outDatei[MAX_PATH];  // zwecks Rueckgabe an Zusi

HektoDllConfig g_config = {
  Beidseitig::kEinseitig,
  Groesse::kGross,
  Rueckstrahlend::kNo,
  Ankerpunkt::kNo,
  TexturDatei::kStandard,
  /* immer_ohne_mast */ false,
  /* hat_ueberlaenge */ false,
  /* basis_km */ 0,
  /* basis_hm */ 0,
};

enum class Standort : std::uint8_t {
  kEigenerStandort = 0,
  kMontageAmAnkerpunkt = 1,
};

void Fehlermeldung(const char* format, ...) {
    char buffer[1024];
    std::va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    MessageBoxA(NULL, buffer, "Error", MB_OK | MB_ICONERROR);
}

DLL_EXPORT uint32_t Init(const char* zielverzeichnis) {
  HKEY key;
  g_zusi_datenpfad_laenge = MAX_PATH;
  if (!SUCCEEDED(RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Zusi3", 0, KEY_READ, &key))) {
    Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
    return 0;
  }

  const auto hatDatenverzeichnisRegulaer = (RegQueryValueEx(key, "DatenVerzeichnis", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);
  const auto hatDatenverzeichnisSteam = (RegQueryValueEx(key, "DatenVerzeichnisSteam", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);

  const auto liesDatenverzeichnis = [&](const char* wertName) -> bool {
    DWORD type;
    g_zusi_datenpfad_laenge = MAX_PATH;
    // Kann RegGetValue nicht nutzen, da auf Windows XP nicht unterstuetzt.
    if (!SUCCEEDED(RegQueryValueEx(key, wertName, nullptr, &type, (LPBYTE)g_zielverzeichnis, &g_zusi_datenpfad_laenge))) {
      return false;
    }
    if (type != REG_SZ) {
      return false;
    }
    // "If the data has the REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ type, the string may not have been stored with the proper terminating null characters.
    // Therefore, even if the function returns ERROR_SUCCESS, the application should ensure that the string is properly terminated before using it"
    if (g_zielverzeichnis[g_zusi_datenpfad_laenge - 1] != '\0') {
      g_zusi_datenpfad_laenge = std::min(g_zusi_datenpfad_laenge + 1, static_cast<DWORD>(MAX_PATH));
      g_zielverzeichnis[g_zusi_datenpfad_laenge - 1] = '\0';
    }
    g_zusi_datenpfad_laenge = std::strlen(g_zielverzeichnis);
    return true;
  };

  if (hatDatenverzeichnisRegulaer && hatDatenverzeichnisSteam) {
    // Wenn _InstSetup/usb.dat relativ zum Zusi-Verzeichnis existiert, sind wir eine regulaere Version
    std::array<char, MAX_PATH> buf;
    GetModuleFileName(nullptr, buf.data(), buf.size());
    PathRemoveFileSpec(buf.data());
    if (PathFileExists((std::string(buf.data()) + "\\_InstSetup\\usb.dat").c_str())) {
      if (!liesDatenverzeichnis("DatenVerzeichnis")) {
        Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
        return 0;
      }
    } else {
      if (!liesDatenverzeichnis("DatenVerzeichnisSteam")) {
        Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
        return 0;
      }
    }
  } else if (hatDatenverzeichnisRegulaer) {
    if (!liesDatenverzeichnis("DatenVerzeichnis")) {
      Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
      return 0;
    }
  } else if (hatDatenverzeichnisSteam) {
    if (!liesDatenverzeichnis("DatenVerzeichnisSteam")) {
      Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
      return 0;
    }
  } else {
    Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
    return 0;
  }

  if (!PathAppend(g_zielverzeichnis, zielverzeichnis)) {
    Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
    return 0;
  }

  if (!PathAppend(g_zielverzeichnis, "Hektometertafeln")) {
    Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
    return 0;
  }

  // Der an Zusi zurueckgegebene Pfad soll keinen fuehrenden Backslash enthalten
  if (g_zielverzeichnis[g_zusi_datenpfad_laenge - 1] != '\\') {
    g_zusi_datenpfad_laenge += 1;
  }

  return 1;
}

DLL_EXPORT const char* dllVersion() {
  return "0.0.15";
}

DLL_EXPORT const char* Autor() {
  return "Zusitools";
}

DLL_EXPORT const char* Bezeichnung() {
  return "Hektometertafeln (DB) V2";
}

DLL_EXPORT float AbstandTafeln() {
  return 200.0;
}

DLL_EXPORT float AbstandGleis(uint8_t modus) {
  if (static_cast<Standort>(modus) == Standort::kEigenerStandort) {
    return 3.0;
  } else {
    return 4.0;
  }
}

DLL_EXPORT const char* Gruppe() {
  return "Hektometertafeln";
}

DLL_EXPORT void Config(HWND appHandle) {
  ShowGui(appHandle, &g_config);
}

const char* GetDateiname(const BauParameter& bau_parameter, Kilometrierung kilometrierung, std::optional<int> ueberlaenge_hm) {
  snprintf(g_outDatei, sizeof(g_outDatei)/sizeof(g_outDatei[0]),
      "%s\\Hekto%s%s%s%s_%s%d_%d%s.ls3", g_zielverzeichnis,
      (bau_parameter.mast == Mast::kMitMast ? "_Mast" : ""),
      (bau_parameter.beidseitig == Beidseitig::kBeidseitig ? "_beids" : ""),
      (bau_parameter.groesse == Groesse::kKlein ? "_klein" : ""),
      (bau_parameter.rueckstrahlend == Rueckstrahlend::kYes ? "_rueckstrahlend" : ""),
      (kilometrierung.istNegativ() ? "-" : ""),
      std::abs(kilometrierung.km),
      std::abs(kilometrierung.hm),
      (ueberlaenge_hm.has_value() ? (std::string("_") + std::to_string(*ueberlaenge_hm)).c_str() : ""));

  return g_outDatei;
}

// "name": kein abschliessender Slash/Backslash
bool CreateDirectoryWithParents(std::string name) {
  const auto attrib = GetFileAttributes(name.c_str());
  if (attrib == INVALID_FILE_ATTRIBUTES) {
    static const std::string separators { "/\\" };
    const auto lastSlashPos = name.find_last_of(separators);
    if (lastSlashPos == std::string::npos) {
      return false;
    }
    if (!CreateDirectoryWithParents(name.substr(0, lastSlashPos))) {
      return false;
    }
    return CreateDirectory(name.c_str(), nullptr);
  } else {
    return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 || (attrib & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
  }
}

DLL_EXPORT uint8_t Erzeugen(float wert_m, uint8_t modus, const char** datei) {
  *datei = nullptr;

  Kilometrierung km_basis = g_config.hat_ueberlaenge ?
    Kilometrierung { g_config.basis_km, g_config.basis_hm } : Kilometrierung::fromMeter(wert_m);
  const auto ueberlaenge_hm = g_config.hat_ueberlaenge ?
    std::optional { Kilometrierung::fromMeter(wert_m).toHektometer() - km_basis.toHektometer() } : std::nullopt;

  if (ueberlaenge_hm.has_value() && ((ueberlaenge_hm < 0) || (ueberlaenge_hm > kMaxUeberlaenge))) {
    Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
    return 0;
  }

  auto standort = static_cast<Standort>(modus);
  BauParameter bauparameter = {
    standort == Standort::kEigenerStandort ? Hoehe::kHoch : Hoehe::kNiedrig,
    (standort == Standort::kMontageAmAnkerpunkt || g_config.immer_ohne_mast) ? Mast::kOhneMast : Mast::kMitMast,
    g_config.beidseitig,
    g_config.groesse,
    g_config.rueckstrahlend,
    g_config.ankerpunkt,
    g_config.textur
  };

  *datei = GetDateiname(bauparameter, km_basis, ueberlaenge_hm) + g_zusi_datenpfad_laenge;
  if (!CreateDirectoryWithParents(g_zielverzeichnis)) {
    Fehlermeldung("failed at %s:%d\n", __FILE__, __LINE__);
    return 0;
  }

  FILE* fd = fopen(g_outDatei, "w");
  assert(fd != nullptr);
  HektoBuilder::Build(fd, bauparameter, km_basis, ueberlaenge_hm);
  fclose(fd);

  return 1;
}
