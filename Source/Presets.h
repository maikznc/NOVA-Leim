// Automatisch erzeugt aus dem Web-Prototyp – 8 Genres à 12 Presets + 7 Basis-Presets.
#pragma once

#include <vector>
#include "dsp/LeimDSP.h"

namespace leim
{

struct Preset
{
    const char* name;
    double threshold, ratio, attackMs, releaseS, makeup, range, scHpf, compMix, xover;
    int loStyle; double loDrive, loLevel;
    int hiStyle; double hiDrive, hiLevel, satMix;
    int order; double outGain; bool softClip;
};

struct Genre
{
    const char* name;
    std::vector<Preset> presets;
};

// Spalten: Name, Threshold, Ratio, Attack ms, Release s (−1 = Auto), Makeup, Range, SC-HPF, Komp-Mix, Crossover,
//          Low-Stil, Drive, Level, High-Stil, Drive, Level, Sat-Mix, Reihenfolge (1 = Sat→Komp), Output, Soft Clip
inline const std::vector<Genre>& presetGenres()
{
    static const std::vector<Genre> genres
    {
        { "Basis", {
            { "Sanfter Kleber", -16, 2, 10, -1, 2, 12, 60, 100, 120, Tape, 4, 0, Tube, 8, 0, 70, 0, 0, true },
            { "Kick-Punch", -20, 4, 30, 0.1, 4, 10, 0, 100, 90, Trafo, 3, 0, Diode, 10, 1, 60, 0, 0, true },
            { "Rumble verdichten", -24, 10, 0.3, 0.4, 7, 9, 30, 75, 150, Tape, 9, -1, Tape, 12, 0, 80, 0, 0, true },
            { "Dreckiger Bass", -18, 4, 1, -1, 3, 12, 90, 100, 140, Tube, 8, 0, Clip, 18, -2, 85, 1, 0, true },
            { "Parallel-Smash", -32, 10, 0.1, 0.2, 12, 24, 45, 40, 110, Tape, 2, 0, Tube, 5, 0, 40, 0, 0, true },
            { "Fold-Growl", -20, 4, 3, 0.2, 3, 12, 60, 100, 180, Tape, 3, 0, Fold, 14, -3, 65, 1, 3, true },
            { "Neutral", 0, 2, 10, -1, 0, 12, 0, 100, 120, Tape, 0, 0, Tape, 0, 0, 0, 0, 0, false },
        } },
        { "Techno", {
            { "Rolling Glue", -18, 2, 10, -1, 2.5, 12, 60, 100, 110, Tape, 4, 0, Tube, 7, 0, 60, 0, 0, true },
            { "Warehouse Punch", -20, 4, 30, 0.1, 4, 10, 90, 100, 100, Trafo, 3, 0, Diode, 9, 0, 55, 0, 0, true },
            { "Peak-Time Druck", -22, 4, 3, 0.2, 5, 12, 60, 100, 120, Tape, 6, 0, Tube, 10, 0, 70, 0, 0, true },
            { "Rumble Kontrolle", -24, 10, 1, 0.4, 6, 8, 30, 80, 140, Tape, 8, -1, Tape, 10, 0, 75, 0, 0, true },
            { "Berlin Dub", -16, 2, 10, 0.8, 2, 10, 90, 100, 90, Tape, 5, 0, Tape, 6, -1, 50, 0, 0, true },
            { "Acid Bass Biss", -18, 4, 1, 0.2, 3, 12, 120, 100, 160, Trafo, 3, 0, Diode, 14, -1, 75, 1, 0, true },
            { "Hypnotisch Pumpen", -26, 4, 10, 0.4, 6, 14, 0, 100, 110, Tape, 3, 0, Tube, 5, 0, 45, 0, 0, true },
            { "Industrial Grit", -20, 4, 0.3, 0.2, 4, 12, 60, 100, 150, Tube, 9, 0, Clip, 18, -2, 80, 1, 0, true },
            { "Parallel Maschine", -32, 10, 0.1, 0.2, 12, 24, 45, 40, 110, Tape, 3, 0, Tube, 6, 0, 45, 0, 0, true },
            { "Minimal Tight", -14, 2, 30, 0.1, 1.5, 6, 90, 100, 100, Trafo, 2, 0, Tape, 4, 0, 40, 0, 0, true },
            { "Detroit Wärme", -16, 2, 10, -1, 2, 10, 60, 100, 120, Tube, 5, 0, Tube, 6, 0, 65, 0, 0, true },
            { "Sub Fokus", -18, 4, 10, 0.6, 3, 10, 30, 100, 80, Tape, 2, 1, Trafo, 8, -1, 60, 0, 0, true },
        } },
        { "Hard Techno", {
            { "Rumble Wand", -26, 10, 0.3, 0.2, 8, 12, 30, 100, 130, Tube, 12, 0, Clip, 16, -1, 85, 0, -1, true },
            { "Schranz Druck", -24, 10, 0.1, 0.1, 8, 14, 60, 100, 160, Trafo, 10, 0, Diode, 18, -2, 90, 1, 0, true },
            { "Distorted Kick Glue", -20, 4, 30, 0.2, 5, 10, 90, 100, 120, Tape, 8, 0, Clip, 14, -1, 80, 0, 0, true },
            { "Industrial Hammer", -22, 10, 1, 0.1, 7, 12, 60, 100, 180, Diode, 10, 0, Clip, 22, -3, 90, 1, 0, true },
            { "Rave Pumpe", -28, 10, 3, 0.4, 9, 16, 0, 100, 110, Tape, 6, 0, Tube, 10, 0, 70, 0, 0, true },
            { "Gabber Biss", -18, 10, 0.1, 0.1, 6, 12, 45, 100, 200, Clip, 14, -1, Clip, 24, -4, 95, 1, -2, true },
            { "Tunnel Rumble", -24, 4, 3, 0.6, 6, 12, 30, 90, 100, Tube, 14, -1, Tape, 12, 0, 85, 0, 0, true },
            { "Hardgroove Swing", -20, 4, 10, 0.2, 4, 10, 90, 100, 120, Trafo, 6, 0, Tube, 12, 0, 70, 0, 0, true },
            { "Fold Monster", -20, 4, 3, 0.2, 4, 12, 60, 100, 160, Tape, 8, 0, Fold, 16, -4, 75, 1, 2, true },
            { "Parallel Zerstörung", -36, 10, 0.01, 0.1, 16, 24, 60, 45, 130, Tube, 10, 0, Clip, 20, -2, 70, 0, 0, true },
            { "Warehouse 150", -22, 4, 1, -1, 5, 12, 60, 100, 140, Tape, 10, 0, Diode, 14, -1, 80, 0, 0, true },
            { "Kick Clip Limit", -14, 10, 0.01, 0.1, 3, 6, 0, 100, 120, Clip, 10, 0, Clip, 12, 0, 60, 0, 0, true },
        } },
        { "Soft & Clean", {
            { "Unsichtbarer Kleber", -14, 2, 30, -1, 1, 4, 90, 100, 120, Tape, 1, 0, Tape, 2, 0, 30, 0, 0, true },
            { "Sanfter Bus", -16, 2, 10, -1, 1.5, 6, 60, 100, 110, Tape, 2, 0, Tube, 3, 0, 35, 0, 0, true },
            { "Transienten frei", -18, 2, 30, 0.4, 2, 6, 120, 100, 100, Trafo, 1, 0, Tape, 2, 0, 25, 0, 0, true },
            { "Nur Wärme", 0, 2, 10, -1, 0, 12, 0, 100, 120, Tape, 4, 0, Tube, 5, 0, 50, 0, 0, true },
            { "Leichte Parallele", -28, 4, 3, 0.4, 8, 12, 60, 25, 110, Tape, 1, 0, Tape, 2, 0, 20, 0, 0, true },
            { "Sub sauber", -16, 2, 10, 0.8, 1, 5, 90, 100, 90, Tape, 0, 0, Tube, 6, 0, 40, 0, 0, true },
            { "Clean Lift", -12, 2, 10, 0.6, 1, 4, 60, 100, 140, Trafo, 1, 0, Trafo, 3, 0, 30, 0, 0, true },
            { "Mastering-Bus Low", -10, 2, 30, -1, 0.5, 3, 150, 100, 120, Tape, 1, 0, Tape, 1, 0, 25, 0, 0, false },
            { "Akustik-Bass", -18, 2, 3, 0.4, 2, 8, 45, 100, 150, Tube, 2, 0, Tube, 3, 0, 35, 0, 0, false },
            { "Lo-Fi Samt", -16, 2, 10, 0.8, 2, 8, 60, 100, 100, Tape, 6, -1, Tape, 8, -2, 60, 0, 0, true },
            { "Pop Kick Polish", -18, 4, 30, 0.2, 2.5, 6, 90, 100, 110, Trafo, 2, 0, Tube, 4, 0, 35, 0, 0, true },
            { "Neutral Plus", -12, 2, 30, -1, 0.5, 2, 90, 100, 120, Tape, 0.5, 0, Tape, 1, 0, 15, 0, 0, false },
        } },
        { "House", {
            { "Classic Groove", -18, 2, 10, -1, 2, 10, 60, 100, 120, Tape, 4, 0, Tube, 6, 0, 55, 0, 0, true },
            { "Deep Wärme", -16, 2, 10, 0.8, 2, 8, 90, 100, 100, Tube, 4, 0, Tape, 5, 0, 60, 0, 0, true },
            { "Offbeat Pumpen", -24, 4, 10, 0.4, 5, 12, 0, 100, 110, Tape, 3, 0, Tube, 5, 0, 45, 0, 0, true },
            { "Disco Glue", -16, 2, 30, 0.4, 2, 8, 90, 100, 130, Trafo, 3, 0, Tube, 5, 0, 50, 0, 0, true },
            { "Chicago Grit", -18, 4, 3, 0.2, 3, 10, 60, 100, 140, Tube, 6, 0, Diode, 10, 0, 65, 0, 0, true },
            { "Garage Swing", -20, 4, 10, 0.2, 3, 10, 90, 100, 120, Tape, 3, 0, Trafo, 7, 0, 50, 0, 0, true },
            { "Orgel-Bass", -16, 2, 3, 0.6, 2, 8, 60, 100, 160, Tube, 5, 0, Tube, 7, -1, 60, 0, 0, true },
            { "Lo-Fi House", -20, 4, 10, 0.6, 4, 12, 45, 100, 110, Tape, 9, -1, Tape, 12, -2, 75, 0, 0, true },
            { "Piano Bounce", -14, 2, 30, 0.2, 1.5, 6, 120, 100, 120, Tape, 2, 0, Tube, 4, 0, 40, 0, 0, true },
            { "French Filter", -22, 4, 1, 0.4, 5, 12, 0, 100, 150, Tube, 6, 0, Clip, 10, -1, 60, 1, 0, true },
            { "Afro Perkussiv", -18, 2, 30, 0.2, 2, 8, 90, 100, 100, Trafo, 3, 0, Tape, 5, 0, 45, 0, 0, true },
            { "Sonnenuntergang", -20, 2, 10, -1, 3, 10, 60, 100, 110, Tape, 5, 0, Tube, 8, 0, 60, 0, 0, true },
        } },
        { "Tech House", {
            { "Rolling Groove", -20, 4, 10, 0.2, 4, 10, 60, 100, 120, Tape, 5, 0, Tube, 9, 0, 65, 0, 0, true },
            { "Bass Wobble Druck", -22, 4, 3, 0.4, 5, 12, 90, 100, 140, Tube, 8, 0, Diode, 12, -1, 75, 1, 0, true },
            { "Kick-Bass Lock", -18, 4, 30, 0.1, 3, 8, 0, 100, 100, Trafo, 4, 0, Tube, 8, 0, 60, 0, 0, true },
            { "Minimal Snap", -16, 2, 30, 0.1, 2, 6, 90, 100, 110, Tape, 2, 0, Trafo, 6, 0, 45, 0, 0, true },
            { "Funky Mitten", -20, 4, 10, 0.2, 4, 10, 60, 100, 180, Tape, 3, 0, Tube, 12, 0, 70, 0, 0, true },
            { "Club Dreck", -22, 4, 1, -1, 4, 12, 60, 100, 150, Tube, 8, 0, Clip, 14, -2, 75, 1, 0, true },
            { "Parallel Groove", -30, 10, 0.3, 0.2, 10, 20, 60, 45, 120, Tape, 4, 0, Tube, 8, 0, 55, 0, 0, true },
            { "Swing Pumpe", -26, 4, 10, 0.4, 6, 14, 0, 100, 110, Tape, 3, 0, Tube, 6, 0, 50, 0, 0, true },
            { "Growl Bass", -18, 4, 3, 0.2, 3, 12, 90, 100, 160, Tape, 4, 0, Fold, 10, -3, 60, 1, 1, true },
            { "Latin Perc Bus", -18, 2, 30, 0.2, 2, 8, 120, 100, 120, Trafo, 3, 0, Tape, 6, 0, 45, 0, 0, true },
            { "Big Room Light", -22, 4, 3, 0.4, 5, 12, 45, 100, 130, Tape, 6, 0, Diode, 10, -1, 70, 0, 0, true },
            { "Sub Saturiert", -18, 4, 10, 0.6, 3, 10, 30, 100, 90, Tube, 6, 0, Tube, 10, -1, 70, 0, 0, true },
        } },
        { "Melodic & Progressive", {
            { "Weiter Raum", -16, 2, 10, -1, 2, 8, 90, 100, 110, Tape, 2, 0, Tube, 4, 0, 40, 0, 0, true },
            { "Sanfter Druck", -18, 2, 3, 0.8, 3, 10, 60, 100, 120, Tape, 3, 0, Tube, 5, 0, 45, 0, 0, true },
            { "Tiefer Sub", -20, 4, 10, 1.2, 4, 10, 45, 100, 90, Tape, 3, 1, Trafo, 7, -1, 55, 0, 0, true },
            { "Arpeggio Glanz", -18, 2, 10, 0.4, 2, 8, 90, 100, 150, Tape, 2, 0, Tube, 8, 0, 55, 0, 0, true },
            { "Progressive Atmen", -24, 4, 30, -1, 5, 12, 0, 100, 110, Tape, 2, 0, Tube, 4, 0, 35, 0, 0, true },
            { "Cinematic Weich", -14, 2, 30, 1.2, 1.5, 6, 120, 100, 100, Tube, 3, 0, Tape, 3, 0, 40, 0, 0, false },
            { "Breaks Druck", -22, 4, 3, 0.2, 5, 12, 60, 100, 130, Tape, 5, 0, Tube, 9, 0, 60, 0, 0, true },
            { "Dunkle Wärme", -18, 2, 10, 0.8, 3, 10, 60, 100, 100, Tube, 6, -1, Tape, 6, -1, 65, 0, 0, true },
            { "Parallel Tiefe", -30, 10, 1, 0.4, 10, 20, 60, 35, 110, Tape, 2, 0, Tube, 5, 0, 40, 0, 0, true },
            { "Trance Rolling", -20, 4, 10, 0.2, 4, 10, 90, 100, 120, Tape, 4, 0, Diode, 8, -1, 55, 0, 0, true },
            { "Indie Dance Grit", -18, 4, 3, 0.4, 3, 10, 60, 100, 150, Tube, 6, 0, Clip, 10, -1, 60, 1, 0, true },
            { "Organic Puls", -16, 2, 30, 0.6, 2, 8, 90, 100, 120, Trafo, 3, 0, Tape, 4, 0, 40, 0, 0, true },
        } },
        { "Drum & Bass", {
            { "Reese Biss", -20, 4, 1, 0.1, 4, 12, 90, 100, 150, Tape, 3, 0, Diode, 14, -2, 75, 1, 0, true },
            { "Liquid Glue", -16, 2, 10, 0.4, 2, 8, 90, 100, 120, Tape, 2, 0, Tube, 5, 0, 45, 0, 0, true },
            { "Neuro Dreck", -22, 4, 0.3, 0.1, 5, 12, 120, 100, 180, Tube, 6, 0, Clip, 22, -3, 85, 1, -1, true },
            { "Sub Kontrolle", -18, 10, 3, 0.2, 4, 8, 30, 100, 90, Tape, 2, 0, Tube, 8, -2, 55, 0, 0, true },
            { "Jump Up Wobble", -20, 4, 1, 0.1, 4, 12, 90, 100, 160, Trafo, 6, 0, Fold, 14, -4, 75, 1, 2, true },
            { "Jungle Punch", -22, 4, 30, 0.1, 5, 12, 60, 100, 110, Tape, 5, 0, Tube, 10, 0, 60, 0, 0, true },
            { "Halftime Druck", -24, 4, 3, 0.4, 6, 12, 60, 100, 120, Tape, 6, 0, Diode, 12, -1, 70, 0, 0, true },
            { "Rollers", -18, 4, 10, 0.2, 3, 10, 90, 100, 130, Tape, 3, 0, Tube, 8, 0, 55, 0, 0, true },
            { "Parallel Amen", -34, 10, 0.01, 0.1, 14, 24, 60, 40, 120, Tape, 3, 0, Tube, 6, 0, 45, 0, 0, true },
            { "Dancefloor Druck", -20, 4, 3, 0.2, 5, 12, 60, 100, 140, Tube, 5, 0, Diode, 10, -1, 70, 0, 0, true },
            { "Minimal DnB", -16, 2, 30, 0.1, 2, 6, 90, 100, 120, Trafo, 2, 0, Trafo, 6, 0, 45, 0, 0, true },
            { "Techstep Kälte", -20, 10, 0.3, 0.1, 5, 10, 120, 100, 160, Trafo, 4, 0, Clip, 16, -2, 70, 1, 0, true },
        } },
        { "Hip-Hop & Trap", {
            { "808 Glue", -18, 4, 10, 0.4, 3, 10, 30, 100, 90, Tape, 3, 0, Tube, 8, 0, 60, 0, 0, true },
            { "808 hörbar machen", -16, 2, 10, -1, 2, 8, 45, 100, 70, Tape, 2, 0, Tube, 14, 0, 75, 0, 0, true },
            { "Drill Slide", -20, 4, 3, 0.4, 4, 12, 30, 100, 80, Tube, 6, 0, Diode, 14, -2, 80, 1, 0, true },
            { "Boom Bap Wärme", -18, 2, 30, 0.4, 3, 10, 90, 100, 120, Tape, 6, 0, Tube, 6, 0, 60, 0, 0, true },
            { "Rage Distortion", -18, 4, 0.3, 0.2, 3, 12, 60, 100, 100, Clip, 12, -1, Clip, 20, -3, 90, 1, -1, true },
            { "Kick duckt 808", -24, 4, 0.1, 0.2, 5, 14, 0, 100, 80, Tape, 2, 0, Tube, 6, 0, 50, 0, 0, true },
            { "Lo-Fi Beat", -20, 4, 10, 0.8, 4, 12, 60, 100, 100, Tape, 10, -1, Tape, 12, -2, 75, 0, 0, true },
            { "Phonk Bus", -18, 4, 1, 0.2, 3, 12, 60, 100, 110, Tube, 10, 0, Fold, 12, -3, 75, 1, 1, true },
            { "Clean 808", -14, 2, 30, 0.6, 1.5, 6, 30, 100, 80, Tape, 1, 0, Tube, 4, 0, 35, 0, 0, true },
            { "Parallel Knock", -30, 10, 0.3, 0.2, 10, 20, 45, 40, 90, Tape, 3, 0, Tube, 6, 0, 45, 0, 0, true },
            { "West Coast Sub", -16, 2, 10, 1.2, 2, 8, 30, 100, 70, Tube, 3, 1, Tape, 6, -1, 55, 0, 0, true },
            { "Grime Druck", -20, 4, 1, 0.2, 4, 12, 60, 100, 130, Trafo, 6, 0, Diode, 14, -2, 75, 1, 0, true },
        } },
    };
    return genres;
}

inline int totalPresetCount()
{
    int n = 0;
    for (auto& g : presetGenres()) n += (int) g.presets.size();
    return n;
}

// Globale Nummer → (Genre, Preset)
inline bool locatePreset (int index, int& genre, int& preset)
{
    const auto& gs = presetGenres();
    for (int g = 0; g < (int) gs.size(); ++g)
    {
        const int n = (int) gs[(size_t) g].presets.size();
        if (index < n) { genre = g; preset = index; return true; }
        index -= n;
    }
    return false;
}

inline int globalPresetIndex (int genre, int preset)
{
    int idx = 0;
    for (int g = 0; g < genre; ++g) idx += (int) presetGenres()[(size_t) g].presets.size();
    return idx + preset;
}

} // namespace leim
