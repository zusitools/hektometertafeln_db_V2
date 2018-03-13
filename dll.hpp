// Copyright 2018 Zusitools

#ifndef DLL_HPP_
#define DLL_HPP_

#include <cstdint>

#include <windows.h>

#ifdef DLL_EXPORTS
  #define DLL_EXPORT \
    __declspec(dllexport) \
    __attribute__((stdcall))
#else
  #define DLL_EXPORT \
    __declspec(dllimport) \
    __attribute__((stdcall))
#endif

#define DLL_LOCAL

extern "C" {

DLL_EXPORT uint32_t Init(const char* zielverzeichnis);
DLL_EXPORT const char* dllVersion();
DLL_EXPORT const char* Autor();
DLL_EXPORT const char* Bezeichnung();
DLL_EXPORT float AbstandTafeln();

/**
 * @return Im Baumodus "0 - Montage am Ankerpunkt": den standardmaessigen Maximalabstand zum Gleis fuer die Ankerpunktsuche.
 *   Im Baumodus "1 - Eigener Standort mit Mast": den standardmaessigen Bauabstand zum Gleis.
 */
DLL_EXPORT float AbstandGleis(uint8_t modus);

/**
 * @return die Objektgruppe, in die die erzeugten Tafeln eingeordnet werden.
 */
DLL_EXPORT const char* Gruppe();

DLL_EXPORT void Config(HWND appHandle);

/**
 * @param wert_m Die Kilometrierung in Metern
 * @param modus 0: Montage am Ankerpunkt. 1: Eigener Standort mit Mast
 * @return 1 bei Erfolg, 0 bei Fehlschlag
 */
DLL_EXPORT uint8_t Erzeugen(float wert_m, uint8_t modus, const char** datei);

}

#endif  // DLL_HPP_
