// Presets pour la voix
#pragma once

namespace velvet
{
struct Preset
{
    const char* name;
    float mix;        // %
    float predelay;   // ms
    float size;       // %
    float decay;      // s
    float damping;    // %
    float lowCut;     // Hz
    float highCut;    // Hz
    float width;      // %
    float duck;       // %
};

//                       nom                     mix  pre  size  decay damp lowcut highcut width duck
static const Preset kPresets[] =
{
    { "Vocal Plate",             25,  20,  55,  1.8,  35,  200,  9000, 100,   0 },
    { "Pop Shine",               22,  35,  60,  2.2,  20,  250, 12000, 110,  30 },
    { "R&B Silk",                28,  40,  70,  2.8,  45,  180,  8000, 115,  25 },
    { "Rap Ambiance",            12,  10,  30,  0.8,  50,  300,  7000,  80,  40 },
    { "Small Room",              18,   8,  25,  0.6,  40,  150, 10000,  90,   0 },
    { "Studio Booth",            10,   3,  15, 0.35,  55,  200,  8000,  70,   0 },
    { "Big Hall",                30,  45,  85,  3.8,  40,  150,  8500, 120,  20 },
    { "Ballade Long Tail",       35,  60,  90,  6.0,  50,  200,  7500, 125,  35 },
    { "Cathedral",               40,  30, 100, 10.0,  55,  120,  6500, 130,  15 },
    { "Ducked Hall (Clean Vox)", 35,  50,  80,  3.5,  35,  220,  9500, 115,  70 },
    { "Lo-Fi Dream",             45,  25,  75,  4.5,  75,  400,  3500, 100,  10 },
    { "Slapback Space",          15, 110,  20,  0.5,  45,  300,  6000,  60,  20 },
};

static constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
} // namespace velvet
