// Armored Core Recomp hub launcher: picks a game, its discs, and settings/controls
// shared by every game, then starts that game's own executable.

#include <SDL3/SDL.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_opengl3_loader.h"
#include "imgui_impl_sdl3.h"
#include "tinyfiledialogs.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#include "stb_image.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

struct GameDef {
    const char* id;
    const char* title;
    const char* exe;
    const char* config_rel;  // folder (relative to the exe dir) holding the staged game.toml
};

static const GameDef kGames[] = {
    {"armored-core", "Armored Core", "AC1", "games/armored-core"},
    {"project-phantasma", "Project Phantasma", "ArmoredCoreProjectPhantasma", "games/project-phantasma"},
    {"master-of-arena", "Master of Arena", "ArmoredCoreMasterOfArena", "games/master-of-arena"},
};
static const int kGameCount = (int)(sizeof(kGames) / sizeof(kGames[0]));
static const int kPathCap = 1024;

// Settings shared by every game; written into each game's own settings.toml at launch.
// Everything is an int so one table can load and save it.
struct CommonSettings {
    int window = 1;  // index into kWindowW
    int renderer = 1;  // 0 software, 1 opengl, 2 vulkan
    int res = 1;       // 0..7 = Nx scale, 8.. = kResPresets
    int dynres = 0;
    int dynres_min = 0;  // same index scale as res
    int fullscreen = 0;  // 0 off, 1 borderless, 2 exclusive
    int aspect = 1;      // 0 4:3, 1 16:9, 2 21:9
    int texfilter = 1;   // 0 nearest, 1 bilinear, 2 xbr
    int aa = 0;
    int fmv = 0;  // 0 nearest, 1 bilinear, 2 sharp, 3 bicubic
    int geometry = 0;
    int persp = 0;
    int screen = 0;  // 0 raw, 1 crt, 2 composite, 3 trinitron
    int scanlines = 0;
    int scan_strength = 50;
    int vsync = 1;  // 0 immediate, 1 on, 2 adaptive
    int interp = 0;
    int fov = 100;  // percent
    int lowlat = 1;
    int render_thread = 0;
    int present_thread = 0;
    int frame_gen = 0;
    int rate = 2;  // index into kRates
    int audio = 1;
    int volume = 80;
    int spuhq = 0;
    int slot = 1;
    int fastboot = 0;
    int bioshle = 0;
    int skipfmv = 0;
    int rewind = 0;
    int rewind_depth = 0;     // index into kRewindDepth
    int rewind_interval = 4;  // index into kRewindInterval
    int ff_speed = 4;
    int dual = 0;
    int dual_dup = 0;
    int deadzone = 10;  // percent
    int card1 = 1;
    int card2 = 1;
    char bios[1024] = {0};
    char memcard_dir[1024] = {0};
};

struct GameState {
    std::vector<std::string> default_roster;       // from game.toml, absolute
    std::vector<std::array<char, kPathCap>> edit;  // chosen image per disc slot
    int boot = 1;                                  // 1-based disc inserted at start
    bool multi() const { return edit.size() > 1; }
};

struct KeyDef {
    const char* key;
    const char* label;
    SDL_Scancode def;
};

// Same keys and defaults as the runtime's keybinds.ini (player 1).
static const KeyDef kKeys[] = {
    {"up", "D-pad Up", SDL_SCANCODE_UP},
    {"down", "D-pad Down", SDL_SCANCODE_DOWN},
    {"left", "D-pad Left", SDL_SCANCODE_LEFT},
    {"right", "D-pad Right", SDL_SCANCODE_RIGHT},
    {"triangle", "Triangle", SDL_SCANCODE_A},
    {"circle", "Circle", SDL_SCANCODE_S},
    {"cross", "Cross", SDL_SCANCODE_X},
    {"square", "Square", SDL_SCANCODE_Z},
    {"l1", "L1", SDL_SCANCODE_Q},
    {"l2", "L2", SDL_SCANCODE_E},
    {"r1", "R1", SDL_SCANCODE_W},
    {"r2", "R2", SDL_SCANCODE_R},
    {"l3", "L3 (stick click)", SDL_SCANCODE_T},
    {"r3", "R3 (stick click)", SDL_SCANCODE_Y},
    {"start", "Start", SDL_SCANCODE_RETURN},
    {"select", "Select", SDL_SCANCODE_RSHIFT},
    {"ls_up", "Left stick Up", SDL_SCANCODE_UNKNOWN},
    {"ls_down", "Left stick Down", SDL_SCANCODE_UNKNOWN},
    {"ls_left", "Left stick Left", SDL_SCANCODE_UNKNOWN},
    {"ls_right", "Left stick Right", SDL_SCANCODE_UNKNOWN},
    {"rs_up", "Right stick Up", SDL_SCANCODE_UNKNOWN},
    {"rs_down", "Right stick Down", SDL_SCANCODE_UNKNOWN},
    {"rs_left", "Right stick Left", SDL_SCANCODE_UNKNOWN},
    {"rs_right", "Right stick Right", SDL_SCANCODE_UNKNOWN},
};
static const int kKeyCount = (int)(sizeof(kKeys) / sizeof(kKeys[0]));

struct HotDef {
    const char* key;  // config.ini [KeyMap] name
    const char* label;
    const char* def;
};

static const HotDef kHot[] = {
    {"Menu", "In-game menu", "Escape"},
    {"Fullscreen", "Toggle fullscreen", "Alt+Return, Ctrl+F"},
    {"Turbo", "Fast forward (hold)", "Tab"},
    {"TurboToggle", "Fast forward (toggle)", "F9"},
    {"SaveStateMenu", "Save state menu", "F7"},
    {"Rewind", "Rewind", "F8"},
    {"Scanlines", "Toggle scanlines", "F6"},
    {"DisplayPerf", "Show FPS", "F"},
    {"VolumeUp", "Volume up", "Keypad +"},
    {"VolumeDown", "Volume down", "Keypad -"},
    {"Reset", "Restart game", "Ctrl+R"},
};
static const int kHotCount = (int)(sizeof(kHot) / sizeof(kHot[0]));

// Controller twin of each hotkey: settings.toml [hotkeys] key and default (button-combo encoding).
static const char* const kHotPadKey[] = {"menu_pad", "fullscreen_pad", "fast_forward_pad",
                                         "fast_forward_toggle_pad", "save_state_menu_pad", "rewind_pad",
                                         "scanlines_pad", "fps_pad", "volume_up_pad", "volume_down_pad",
                                         "restart_pad"};
static const int kHotPadDef[] = {1032, 0, 1528, 0, 2040, 1272, 0, 0, 0, 0, 0};

static const int kWindowW[] = {960, 1280, 1600, 1920, 2560};
static const int kRates[] = {22050, 32000, 44100, 48000};
static const int kRewindDepth[] = {50, 100, 150, 200};
static const int kRewindInterval[] = {1, 4, 8, 12, 15};
static const char* const kResPresets[] = {"native", "720p", "1080p", "1440p", "4k", "5k", "8k", "display"};
static const char* const kResNames[] = {
    "1x (240 lines)", "2x (480 lines)", "3x (720 lines)", "4x (960 lines)", "5x (1200 lines)",
    "6x (1440 lines)", "7x (1680 lines)", "8x (1920 lines)", "Native",
    "720p", "1080p", "1440p", "4K", "5K", "8K", "Match display"};

static fs::path g_exe_dir;
static fs::path g_root;
static CommonSettings g_common;
static GameState g_state[3];
static SDL_Scancode g_binds[24];
static std::string g_hot[16];

struct PadInfo {
    SDL_JoystickID id;
    SDL_Gamepad* pad;
    std::string name, guid;
};
static std::vector<PadInfo> g_pads;
static std::string g_device = "keyboard";  // "keyboard" or an SDL controller GUID
static std::string g_padmap[24];
static int g_pad_capture = -1;
// Default controller sources, in kKeys order (same names input.ini uses).
static const char* const kPadDef[24] = {
    "dpup", "dpdown", "dpleft", "dpright", "y", "b", "a", "x", "leftshoulder", "lefttrigger",
    "rightshoulder", "righttrigger", "leftstick", "rightstick", "start", "back", "lefty-", "lefty+",
    "leftx-", "leftx+", "righty-", "righty+", "rightx-", "rightx+"};
static int g_capture = -1;      // index into kKeys while waiting for a key press
static int g_hot_capture = -1;  // index into kHot while waiting for a key combo
static int g_hot_col = 0;       // 0 = keyboard column, 1 = controller column
static double g_hot_t0 = 0;     // when the capture window opened
static uint32_t g_hot_held = 0, g_hot_best = 0;
static int g_hot_axis = 0;
static int g_hotpad[16];
static const double kCaptureSecs = 3.0;
static double g_synced_until = 0;  // time until which the "synced" notice shows
static float g_ui = 1.0f;       // display scale
static float g_row_x = 0;       // x where a row's control starts

struct IntField {
    const char* name;
    int* v;
    int lo, hi;
};
static std::vector<IntField> g_fields;

static void init_fields() {
    CommonSettings& c = g_common;
    g_fields = {
        {"window", &c.window, 0, 4}, {"renderer", &c.renderer, 0, 2}, {"res", &c.res, 0, 15},
        {"dynres", &c.dynres, 0, 1}, {"dynres_min", &c.dynres_min, 0, 15}, {"fullscreen", &c.fullscreen, 0, 2}, {"aspect", &c.aspect, 0, 2},
        {"texfilter", &c.texfilter, 0, 2}, {"aa", &c.aa, 0, 1}, {"fmv", &c.fmv, 0, 3},
        {"geometry", &c.geometry, 0, 1}, {"persp", &c.persp, 0, 1}, {"screen", &c.screen, 0, 3},
        {"scanlines", &c.scanlines, 0, 1}, {"scan_strength", &c.scan_strength, 0, 100},
        {"vsync", &c.vsync, 0, 2}, {"interp", &c.interp, 0, 1}, {"fov", &c.fov, 50, 200},
        {"lowlat", &c.lowlat, 0, 1}, {"render_thread", &c.render_thread, 0, 1},
        {"present_thread", &c.present_thread, 0, 1}, {"frame_gen", &c.frame_gen, 0, 1},
        {"rate", &c.rate, 0, 3}, {"audio", &c.audio, 0, 1}, {"volume", &c.volume, 0, 100},
        {"spuhq", &c.spuhq, 0, 1}, {"slot", &c.slot, 1, 12}, {"fastboot", &c.fastboot, 0, 1},
        {"bioshle", &c.bioshle, 0, 1}, {"skipfmv", &c.skipfmv, 0, 1}, {"rewind", &c.rewind, 0, 1},
        {"rewind_depth", &c.rewind_depth, 0, 3}, {"rewind_interval", &c.rewind_interval, 0, 4},
        {"ff_speed", &c.ff_speed, 2, 16}, {"dual", &c.dual, 0, 1}, {"dual_dup", &c.dual_dup, 0, 1},
        {"deadzone", &c.deadzone, 0, 50}, {"card1", &c.card1, 0, 1}, {"card2", &c.card2, 0, 1},
    };
}

// Armored Core palette from recomp-ui's launcher theme.
static ImVec4 rgb(int hex, float a = 1.0f) {
    return ImVec4(((hex >> 16) & 255) / 255.0f, ((hex >> 8) & 255) / 255.0f, (hex & 255) / 255.0f, a);
}
static const ImVec4 kBg = rgb(0x000000), kBg2 = rgb(0x060708), kPanel = rgb(0x0B0D0F),
                    kPanelHot = rgb(0x15181B), kControl = rgb(0x121518), kControlHot = rgb(0x322506),
                    kBorder = rgb(0x38424D), kAmber = rgb(0xFF9E00), kAmberDim = rgb(0xBF7300),
                    kCyan = rgb(0x00D1F2), kText = rgb(0xE0E7ED), kMuted = rgb(0x707D8A),
                    kGood = rgb(0x00EB94), kBad = rgb(0xFF3814);
static ImU32 u32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

struct TomlDoc {
    std::vector<std::string> lines;

    void load(const fs::path& p) {
        std::ifstream f(p);
        std::string l;
        while (std::getline(f, l)) lines.push_back(l);
    }
    bool save(const fs::path& p) const {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
        std::ofstream f(p, std::ios::trunc);
        if (!f) return false;
        for (const auto& l : lines) f << l << "\n";
        return true;
    }
    static bool is_key(const std::string& t, const std::string& key) {
        if (t.rfind(key, 0) != 0) return false;
        const std::string rest = trim(t.substr(key.size()));
        return !rest.empty() && rest[0] == '=';
    }
    void set(const std::string& sec, const std::string& key, const std::string& val) {
        size_t hdr = lines.size();
        for (size_t i = 0; i < lines.size(); ++i)
            if (trim(lines[i]) == "[" + sec + "]") { hdr = i; break; }
        if (hdr == lines.size()) {
            if (!lines.empty() && !trim(lines.back()).empty()) lines.push_back("");
            lines.push_back("[" + sec + "]");
            lines.push_back(key + " = " + val);
            return;
        }
        size_t end = lines.size();
        for (size_t i = hdr + 1; i < lines.size(); ++i) {
            const std::string t = trim(lines[i]);
            if (!t.empty() && t[0] == '[') { end = i; break; }
            if (is_key(t, key)) { lines[i] = key + " = " + val; return; }
        }
        size_t ins = end;
        while (ins > hdr + 1 && trim(lines[ins - 1]).empty()) --ins;
        lines.insert(lines.begin() + (long)ins, key + " = " + val);
    }
    void remove(const std::string& sec, const std::string& key) {
        bool in = false;
        for (size_t i = 0; i < lines.size(); ++i) {
            const std::string t = trim(lines[i]);
            if (!t.empty() && t[0] == '[') in = t == "[" + sec + "]";
            else if (in && is_key(t, key)) { lines.erase(lines.begin() + (long)i); return; }
        }
    }
};

static std::string quote(const std::string& s) {
    std::string o = "\"";
    for (char c : s) { if (c == '\\' || c == '"') o += '\\'; o += c; }
    return o + "\"";
}
static const char* tf(int b) { return b ? "true" : "false"; }

static fs::path config_dir(int g) { return g_exe_dir / kGames[g].config_rel; }
static fs::path settings_path(int g) { return config_dir(g) / "settings.toml"; }
static fs::path hub_ini() { return g_exe_dir / "launcher.ini"; }

static fs::path find_root() {
    fs::path p = g_exe_dir;
    for (int i = 0; i < 6; ++i) {
        std::error_code ec;
        if (fs::exists(p / "CMakeLists.txt", ec) && fs::exists(p / "games", ec)) return p;
        if (!p.has_parent_path() || p.parent_path() == p) break;
        p = p.parent_path();
    }
    return g_exe_dir;
}

static std::vector<std::string> read_roster(int g) {
    std::vector<std::string> out;
    std::ifstream f(config_dir(g) / "game.toml");
    std::string l;
    bool in_list = false;
    while (std::getline(f, l)) {
        const std::string t = trim(l);
        if (!in_list && t.rfind("discs", 0) == 0 && t.find('[') != std::string::npos) {
            in_list = true;
        } else if (!in_list && t.rfind("disc", 0) == 0 && t.find('"') != std::string::npos) {
            const size_t a = t.find('"'), b = t.rfind('"');
            if (b > a) out.push_back(t.substr(a + 1, b - a - 1));
            break;
        }
        if (in_list) {
            const size_t a = t.find('"'), b = t.rfind('"');
            if (a != std::string::npos && b > a) out.push_back(t.substr(a + 1, b - a - 1));
            if (t.find(']') != std::string::npos) break;
        }
    }
    for (auto& d : out) {
        fs::path p(d);
        if (p.is_relative()) p = g_root / p;
        d = p.string();
    }
    return out;
}

static void set_buf(char* dst, const std::string& s) { std::snprintf(dst, kPathCap, "%s", s.c_str()); }

static void reset_binds() {
    for (int i = 0; i < kKeyCount; ++i) g_binds[i] = kKeys[i].def;
}
static void reset_hotkeys() {
    for (int i = 0; i < kHotCount; ++i) {
        g_hot[i] = kHot[i].def;
        g_hotpad[i] = kHotPadDef[i];
    }
}
static void reset_padmap() {
    for (int i = 0; i < kKeyCount; ++i) g_padmap[i] = kPadDef[i];
}

static void refresh_pads() {
    for (auto& p : g_pads)
        if (p.pad) SDL_CloseGamepad(p.pad);
    g_pads.clear();
    int n = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&n);
    for (int i = 0; ids && i < n; ++i) {
        PadInfo p;
        p.id = ids[i];
        p.pad = SDL_OpenGamepad(ids[i]);
        const char* nm = SDL_GetGamepadNameForID(ids[i]);
        p.name = nm ? nm : "Controller";
        char guid[64] = {0};
        SDL_GUIDToString(SDL_GetJoystickGUIDForID(ids[i]), guid, sizeof(guid));
        p.guid = guid;
        g_pads.push_back(p);
    }
    SDL_free(ids);
}

static void load_hub() {
    std::ifstream f(hub_ini());
    std::string l;
    while (std::getline(f, l)) {
        const size_t eq = l.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = trim(l.substr(0, eq)), v = trim(l.substr(eq + 1));
        const int n = std::atoi(v.c_str());
        bool done = false;
        for (auto& fld : g_fields)
            if (k == fld.name) { *fld.v = std::clamp(n, fld.lo, fld.hi); done = true; break; }
        if (done) continue;
        if (k == "bios") set_buf(g_common.bios, v);
        else if (k == "device") g_device = v.empty() ? "keyboard" : v;
        else if (k.rfind("pad.", 0) == 0) {
            for (int i = 0; i < kKeyCount; ++i)
                if (k == std::string("pad.") + kKeys[i].key) g_padmap[i] = v;
        }
        else if (k == "memcard_dir") set_buf(g_common.memcard_dir, v);
        else if (k.rfind("bind.", 0) == 0) {
            for (int i = 0; i < kKeyCount; ++i)
                if (k == std::string("bind.") + kKeys[i].key)
                    g_binds[i] = v == "None" ? SDL_SCANCODE_UNKNOWN : SDL_GetScancodeFromName(v.c_str());
        } else if (k.rfind("hotpad.", 0) == 0) {
            for (int i = 0; i < kHotCount; ++i)
                if (k == std::string("hotpad.") + kHot[i].key) g_hotpad[i] = n;
        } else if (k.rfind("hot.", 0) == 0) {
            for (int i = 0; i < kHotCount; ++i)
                if (k == std::string("hot.") + kHot[i].key) g_hot[i] = v;
        } else
            for (int g = 0; g < kGameCount; ++g) {
                const std::string id = kGames[g].id;
                if (k == "boot." + id) g_state[g].boot = std::max(1, n);
                for (size_t i = 0; i < g_state[g].edit.size(); ++i)
                    if (k == "path." + id + "." + std::to_string(i + 1)) set_buf(g_state[g].edit[i].data(), v);
            }
    }
}

static const char* bind_name(SDL_Scancode sc) {
    if (sc == SDL_SCANCODE_UNKNOWN) return "None";
    const char* n = SDL_GetScancodeName(sc);
    return n && n[0] ? n : "None";
}

static void save_hub() {
    std::ofstream f(hub_ini(), std::ios::trunc);
    for (auto& fld : g_fields) f << fld.name << "=" << *fld.v << "\n";
    f << "bios=" << g_common.bios << "\nmemcard_dir=" << g_common.memcard_dir << "\ndevice=" << g_device << "\n";
    for (int i = 0; i < kKeyCount; ++i) f << "pad." << kKeys[i].key << "=" << g_padmap[i] << "\n";
    for (int i = 0; i < kKeyCount; ++i) f << "bind." << kKeys[i].key << "=" << bind_name(g_binds[i]) << "\n";
    for (int i = 0; i < kHotCount; ++i) f << "hot." << kHot[i].key << "=" << g_hot[i] << "\n";
    for (int i = 0; i < kHotCount; ++i) f << "hotpad." << kHot[i].key << "=" << g_hotpad[i] << "\n";
    for (int g = 0; g < kGameCount; ++g) {
        f << "boot." << kGames[g].id << "=" << g_state[g].boot << "\n";
        for (size_t i = 0; i < g_state[g].edit.size(); ++i)
            if (g_state[g].edit[i][0])
                f << "path." << kGames[g].id << "." << (i + 1) << "=" << g_state[g].edit[i].data() << "\n";
    }
}

// Every game reads keybinds.ini from its own folder, so one shared mapping is written to each.
static void write_keybinds(int g) {
    std::ofstream f(config_dir(g) / "keybinds.ini", std::ios::trunc);
    if (!f) return;
    f << "# Written by the Armored Core launcher; the same mapping is used by every game.\n\n[player1]\n";
    for (int i = 0; i < kKeyCount; ++i) {
        char line[96];
        std::snprintf(line, sizeof(line), "%-10s= %s\n", kKeys[i].key, bind_name(g_binds[i]));
        f << line;
    }
    f << "\n[player2]\n";
    for (int i = 0; i < kKeyCount; ++i) {
        char line[96];
        std::snprintf(line, sizeof(line), "%-10s= None\n", kKeys[i].key);
        f << line;
    }
}

static void write_inputini(int g) {
    std::ofstream f(config_dir(g) / "input.ini", std::ios::trunc);
    if (!f) return;
    f << "; Written by the Armored Core launcher; the same controller mapping is used by every game.\n\n"
      << "[controller]\nenabled = true\ndevice = 0\ndeadzone = " << g_common.deadzone * 32767 / 100
      << "\n\n[mapping]\n";
    for (int i = 0; i < kKeyCount; ++i) f << kKeys[i].key << " = " << g_padmap[i] << "\n";
}

// Hotkeys are read from config.ini next to the executables, so one file serves every game.
static void write_hotkeys() {
    const fs::path path = g_exe_dir / "config.ini";
    TomlDoc d;
    d.load(path);
    for (int i = 0; i < kHotCount; ++i) d.set("KeyMap", kHot[i].key, g_hot[i].empty() ? "None" : g_hot[i]);
    d.save(path);
}

// The runtime builds its disc roster from the staged game.toml, so the chosen images go there.
static void rewrite_roster(int g) {
    const fs::path path = config_dir(g) / "game.toml";
    TomlDoc d;
    d.load(path);
    size_t a = d.lines.size(), b = a;
    for (size_t i = 0; i < d.lines.size(); ++i) {
        const std::string t = trim(d.lines[i]);
        if (t.rfind("discs", 0) == 0 && t.find('[') != std::string::npos) {
            a = i;
            for (b = i; b < d.lines.size(); ++b)
                if (d.lines[b].find(']') != std::string::npos) break;
            break;
        }
    }
    if (a >= d.lines.size() || b >= d.lines.size()) return;
    std::vector<std::string> block{"discs = ["};
    for (const auto& e : g_state[g].edit) block.push_back("    " + quote(e.data()) + ",");
    block.push_back("]");
    d.lines.erase(d.lines.begin() + (long)a, d.lines.begin() + (long)b + 1);
    d.lines.insert(d.lines.begin() + (long)a, block.begin(), block.end());
    d.save(path);
}

static void apply_to_game(int g) {
    const fs::path path = settings_path(g);
    TomlDoc d;
    d.load(path);
    const CommonSettings& c = g_common;
    char buf[32];

    static const char* const kRenderer[] = {"\"software\"", "\"opengl\"", "\"vulkan\""};
    d.set("video", "renderer", kRenderer[c.renderer]);
    static const char* const kAspect[] = {"\"4:3\"", "\"16:9\"", "\"21:9\""};
    d.set("video", "aspect_ratio", kAspect[c.aspect]);
    d.set("video", "window_width", std::to_string(kWindowW[c.window]));
    d.set("video", "fullscreen", std::to_string(c.fullscreen));
    if (c.res < 8) {
        d.set("video", "supersampling", std::to_string(c.res + 1));
        d.set("video", "internal_resolution", std::to_string((c.res + 1) * 240));
    } else {
        d.set("video", "internal_resolution", quote(kResPresets[c.res - 8 == 7 ? 7 : c.res - 8]));
    }
    d.set("video", "dynamic_resolution", tf(c.dynres));
    if (c.dynres_min < 8) d.set("video", "dynamic_resolution_min", std::to_string((c.dynres_min + 1) * 240));
    else d.set("video", "dynamic_resolution_min", quote(kResPresets[c.dynres_min - 8]));
    d.set("video", "vsync", c.vsync == 0 ? "\"immediate\"" : c.vsync == 2 ? "\"adaptive\"" : "\"on\"");
    d.set("video", "texture_filtering",
          c.texfilter == 2 ? "\"xbr\"" : c.texfilter == 1 ? "\"bilinear\"" : "\"nearest\"");
    d.set("video", "antialiasing", tf(c.aa));
    static const char* const kFmv[] = {"\"nearest\"", "\"bilinear\"", "\"sharp\"", "\"bicubic\""};
    d.set("video", "fmv_filter", kFmv[c.fmv]);
    static const char* const kScreen[] = {"\"raw\"", "\"crt\"", "\"composite\"", "\"trinitron\""};
    d.set("video", "crt_filter", kScreen[c.screen]);
    d.set("video", "scanlines", tf(c.scanlines));
    std::snprintf(buf, sizeof(buf), "%.2f", c.scan_strength / 100.0);
    d.set("video", "scanline_strength", buf);
    d.set("video", "frame_interpolation", tf(c.interp));
    std::snprintf(buf, sizeof(buf), "%.2f", c.fov / 100.0);
    d.set("video", "fov_scale", buf);
    d.set("video", "geometry_correction", tf(c.geometry));
    d.set("video", "perspective_texturing", tf(c.persp));
    d.set("video", "low_latency_input", tf(c.lowlat));
    d.set("video", "render_thread", tf(c.render_thread));
    d.set("video", "present_thread", tf(c.present_thread));
    d.set("video", "frame_generation", tf(c.frame_gen));
    d.set("video", "fast_boot", tf(c.fastboot));
    d.set("video", "auto_skip_fmv", tf(c.skipfmv));
    d.set("video", "bios_hle", tf(c.bioshle));
    d.set("video", "rewind", tf(c.rewind));
    d.set("video", "rewind_depth", std::to_string(kRewindDepth[c.rewind_depth]));
    d.set("video", "rewind_interval", std::to_string(kRewindInterval[c.rewind_interval]));
    d.set("audio", "enabled", tf(c.audio));
    d.set("audio", "volume", std::to_string(c.volume));
    d.set("audio", "spu_hq", tf(c.spuhq));
    d.set("audio", "frequency", std::to_string(kRates[c.rate]));
    d.set("savestate", "slot", std::to_string(c.slot));
    d.set("hotkeys", "fast_forward_speed", std::to_string(c.ff_speed));
    for (int i = 0; i < kHotCount; ++i) d.set("hotkeys", kHotPadKey[i], std::to_string(g_hotpad[i]));
    d.set("controller", "p1_device", quote(g_device));
    d.set("controller", "dual_analog_buttons", tf(c.dual));
    d.set("controller", "dual_analog_duplicate_face", tf(c.dual && c.dual_dup));
    const int dz = c.deadzone * 32767 / 100;
    d.set("controller", "p1_deadzone", std::to_string(dz));
    d.set("controller", "p2_deadzone", std::to_string(dz));
    d.set("bios", "path", quote(c.bios));
    d.set("memcard", "dir", quote(c.memcard_dir[0] ? std::string(c.memcard_dir) : (g_root / "saves").string()));
    d.set("memcard", "enable1", tf(c.card1));
    d.set("memcard", "enable2", tf(c.card2));

    GameState& s = g_state[g];
    if (s.multi()) {
        rewrite_roster(g);
        const int idx = std::clamp(s.boot, 1, (int)s.edit.size());
        d.set("disc", "selected", std::to_string(idx));
        d.set("disc", "path", quote(s.edit[(size_t)idx - 1].data()));
    } else if (!s.edit.empty() && s.edit[0][0]) {
        d.set("disc", "path", quote(s.edit[0].data()));
    }
    if (!d.save(path)) std::fprintf(stderr, "launcher: could not write %s\n", path.string().c_str());
    write_keybinds(g);
    write_inputini(g);
    write_hotkeys();
}

static bool exists(const char* p) {
    std::error_code ec;
    return p && p[0] && fs::exists(p, ec);
}

static std::string dialog_start(const char* cur) {
    std::string s = cur && cur[0] ? fs::path(cur).parent_path().string() : g_root.string();
    if (s.empty()) s = g_root.string();
    return s + "/";
}

static void browse_file(char* buf, const char* title, bool disc) {
    static const char* const kDisc[] = {"*.cue", "*.chd", "*.bin", "*.iso", "*.img", "*.pbp"};
    static const char* const kBios[] = {"*.bin", "*.BIN"};
    const std::string start = dialog_start(buf);
    const char* r = disc ? tinyfd_openFileDialog(title, start.c_str(), 6, kDisc, "Disc images", 0)
                         : tinyfd_openFileDialog(title, start.c_str(), 2, kBios, "BIOS images", 0);
    if (r) set_buf(buf, r);
}

static void browse_folder(char* buf, const char* title) {
    const std::string start = dialog_start(buf);
    const char* r = tinyfd_selectFolderDialog(title, start.c_str());
    if (r) set_buf(buf, r);
}

// "// TITLE //" panel heading with a tag box, as in the game launchers.
static void heading(const char* tag, const char* title) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = ImGui::GetTextLineHeight();
    const ImVec2 ts = ImGui::CalcTextSize(tag);
    const float box = 26.0f * g_ui;
    const float boxw = std::max(box, ts.x + 12 * g_ui);
    dl->AddRect(ImVec2(p.x, p.y), ImVec2(p.x + boxw, p.y + box), u32(kCyan), 0, 0, g_ui);
    dl->AddText(ImVec2(p.x + (boxw - ts.x) * 0.5f, p.y + (box - ts.y) * 0.5f), u32(kAmber), tag);
    float x = p.x + boxw + 10 * g_ui;
    const float y = p.y + (box - h) * 0.5f;
    dl->AddText(ImVec2(x, y), u32(kAmber), "// ");
    x += ImGui::CalcTextSize("// ").x;
    char ch[2] = {0, 0};
    for (const char* q = title; *q; ++q) {
        ch[0] = *q;
        dl->AddText(ImVec2(x, y), u32(kCyan), ch);
        x += ImGui::CalcTextSize(ch).x + 1.5f * g_ui;
    }
    dl->AddText(ImVec2(x, y), u32(kAmber), " //");
    const float w = ImGui::GetContentRegionAvail().x;
    dl->AddLine(ImVec2(p.x, p.y + box + 5 * g_ui), ImVec2(p.x + w, p.y + box + 5 * g_ui), u32(kBorder), g_ui);
    dl->AddRectFilled(ImVec2(p.x, p.y + box + 4 * g_ui), ImVec2(p.x + 22 * g_ui, p.y + box + 6 * g_ui), u32(kAmber));
    ImGui::Dummy(ImVec2(w, box + 14 * g_ui));
}

static bool g_tip_label_hot = false;

// Label on the left in muted text, control right-anchored (width ctrl_w), as in the game launchers.
static void row_label(const char* s, float ctrl_w = -1.0f) {
    if (ctrl_w < 0) ctrl_w = 150.0f * g_ui;
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextUnformatted(s);
    ImGui::PopStyleColor();
    g_tip_label_hot = ImGui::IsItemHovered();
    ImGui::SameLine(0, 0);
    const float shift = std::max(ImGui::GetContentRegionAvail().x - ctrl_w, 16.0f * g_ui);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + shift);
    ImGui::SetNextItemWidth(ctrl_w);
}

// Call right after a row's control: shows the help text when the label or control is hovered.
static void row_tip(const char* tip) {
    if (tip && (g_tip_label_hot || ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)))
        ImGui::SetTooltip("%s", tip);
}

static void combo_row(const char* label, int* v, const char* const* items, int n, const char* tip = nullptr) {
    ImGui::PushID(label);
    row_label(label);
    *v = std::clamp(*v, 0, n - 1);
    if (ImGui::BeginCombo("##c", items[*v])) {
        for (int i = 0; i < n; ++i)
            if (ImGui::Selectable(items[i], *v == i)) *v = i;
        ImGui::EndCombo();
    }
    row_tip(tip);
    ImGui::PopID();
}

static void check_row(const char* label, int* v, const char* tip = nullptr) {
    ImGui::PushID(label);
    row_label(label, ImGui::GetFrameHeight());
    bool b = *v != 0;
    if (ImGui::Checkbox("##k", &b)) *v = b ? 1 : 0;
    row_tip(tip);
    ImGui::PopID();
}

static void slider_row(const char* label, int* v, int lo, int hi, const char* fmt, const char* tip = nullptr) {
    ImGui::PushID(label);
    row_label(label);
    ImGui::SliderInt("##s", v, lo, hi, fmt);
    row_tip(tip);
    ImGui::PopID();
}

// "-  80%  +" stepper like the launchers' volume row.
static void stepper_row(const char* label, int* v, int lo, int hi, int step, const char* tip = nullptr) {
    ImGui::PushID(label);
    row_label(label);
    const float bw = 32 * g_ui, gap = 6 * g_ui;
    const float fw = 150 * g_ui - 2 * bw - 2 * gap;
    bool hot = ImGui::IsItemHovered();
    if (ImGui::Button("-", ImVec2(bw, 0))) *v = std::max(lo, *v - step);
    hot |= ImGui::IsItemHovered();
    ImGui::SameLine(0, gap);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d%%", *v);
    const float cx = ImGui::GetCursorPosX();
    ImGui::SetCursorPosX(cx + (fw - ImGui::CalcTextSize(buf).x) * 0.5f);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(buf);
    hot |= ImGui::IsItemHovered();
    ImGui::SameLine(0, 0);
    ImGui::SetCursorPosX(cx + fw + gap);
    if (ImGui::Button("+", ImVec2(bw, 0))) *v = std::min(hi, *v + step);
    hot |= ImGui::IsItemHovered();
    if (tip && (g_tip_label_hot || hot)) ImGui::SetTooltip("%s", tip);
    ImGui::PopID();
}

static void note(const char* s) {
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextWrapped("%s", s);
    ImGui::PopStyleColor();
}

static void path_row(const char* label, char* buf, bool is_disc, bool folder, const char* placeholder,
                     const char* tip = nullptr) {
    ImGui::PushID(label);
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    bool hot = ImGui::IsItemHovered();
    ImGui::SetNextItemWidth(-ImGui::CalcTextSize("Browse").x - ImGui::GetStyle().FramePadding.x * 2 -
                            ImGui::GetStyle().ItemSpacing.x);
    ImGui::InputTextWithHint("##path", placeholder, buf, kPathCap);
    hot |= ImGui::IsItemHovered();
    if (tip && hot) ImGui::SetTooltip("%s", tip);
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {
        if (folder) browse_folder(buf, label);
        else browse_file(buf, label, is_disc);
    }
    if (buf[0]) {
        const bool ok = exists(buf);
        ImGui::TextColored(ok ? kGood : kBad, "%s",
                           ok ? (folder ? "Folder found" : "File found")
                              : (folder ? "Folder missing" : "File missing"));
    }
    ImGui::Spacing();
    ImGui::PopID();
}

static bool panel_begin(const char* id, float w) {
    const bool open = ImGui::BeginChild(id, ImVec2(w, 0), ImGuiChildFlags_Borders);
    g_row_x = ImGui::GetContentRegionAvail().x * 0.48f;
    return open;
}

static void two_panels(void (*left)(), void (*right)()) {
    const float w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    panel_begin("left", w);
    left();
    ImGui::EndChild();
    ImGui::SameLine();
    panel_begin("right", w);
    right();
    ImGui::EndChild();
}

static void panel_display() {
    CommonSettings& c = g_common;
    heading("DISP", "DISPLAY");
    char win[5][32];
    const char* wins[5];
    for (int i = 0; i < 5; ++i) {
        const int w = kWindowW[i];
        const int h = c.aspect == 0 ? w * 3 / 4 : c.aspect == 2 ? w * 9 / 21 : w * 9 / 16;
        std::snprintf(win[i], sizeof(win[i]), "%d x %d", w, h);
        wins[i] = win[i];
    }
    combo_row("Window size", &c.window, wins, 5, "Size of the game window when it is not fullscreen.");
    static const char* const kRend[] = {"Software", "OpenGL (Recommended)", "Vulkan"};
    combo_row("Renderer", &c.renderer, kRend, 3,
              "Graphics backend. OpenGL is recommended; Vulkan and Software are alternatives.");
    combo_row("Internal resolution", &c.res, kResNames, 16,
              "Resolution the 3D scene is drawn at. Higher is sharper but needs a faster GPU. Above 4x requires OpenGL.");
    note("Above 4x: OpenGL only.");
    check_row("Dynamic resolution", &c.dynres,
              "Lowers the render resolution automatically when the game would otherwise run slowly.");
    if (c.dynres)
        combo_row("Minimum resolution", &c.dynres_min, kResNames, 16,
                  "The lowest resolution dynamic resolution is allowed to drop to.");
    static const char* const kFull[] = {"Off", "Borderless", "Exclusive"};
    combo_row("Fullscreen", &c.fullscreen, kFull, 3,
              "Off: windowed. Borderless: a fullscreen window at desktop resolution. Exclusive: changes the display mode.");
    static const char* const kAsp[] = {"4:3 (Standard)", "16:9 (Widescreen)", "21:9 (Ultrawide)"};
    combo_row("Aspect ratio", &c.aspect, kAsp, 3,
              "Shape of the picture. 16:9 and 21:9 widen the view in games that support widescreen.");
    static const char* const kTex[] = {"Nearest", "Bilinear", "xBR"};
    combo_row("Texture filtering", &c.texfilter, kTex, 3,
              "How textures are smoothed. Nearest keeps the original blocky look, Bilinear smooths, xBR sharpens edges.");
    static const char* const kOff[] = {"Off", "On"};
    combo_row("Antialiasing", &c.aa, kOff, 2, "Smooths jagged polygon edges.");
    static const char* const kFmv[] = {"Nearest", "Bilinear", "Sharp", "Bicubic"};
    combo_row("FMV filtering", &c.fmv, kFmv, 4, "Scaling filter used for pre-rendered movies.");
    static const char* const kVs[] = {"Off", "On", "Adaptive"};
    combo_row("VSync", &c.vsync, kVs, 3,
              "Syncs frames to your display to prevent tearing. Adaptive only syncs when running at full speed.");
    check_row("Frame interpolation", &c.interp, "Blends frames for smoother motion on high refresh rate displays.");
}

static void panel_graphics() {
    CommonSettings& c = g_common;
    heading("GFX", "GRAPHICS");
    check_row("Geometry precision", &c.geometry,
              "Removes the wobbling polygon vertices of the original hardware (PGXP) for steadier 3D models.");
    check_row("Perspective textures", &c.persp, "Corrects the warped, swimming textures of the original hardware.");
    static const char* const kScr[] = {"Raw", "CRT", "Composite", "Trinitron"};
    combo_row("Screen model", &c.screen, kScr, 4,
              "Display shader preset. Raw applies nothing; the others imitate those kinds of screen.");
    check_row("Scanlines", &c.scanlines, "Draws CRT-style scanlines over the picture.");
    if (c.scanlines)
        stepper_row("Scanline strength", &c.scan_strength, 0, 100, 5, "How dark the scanlines are.");
    stepper_row("Field of view", &c.fov, 50, 200, 5,
                "Widens or narrows the 3D projection. 100% matches the original game.");
    ImGui::Spacing();
    heading("PIPE", "PIPELINE");
    check_row("Low latency input", &c.lowlat, "Reads the controller as late as possible to reduce input lag.");
    check_row("Render thread", &c.render_thread, "Renders on a separate thread. Can raise performance. Applies next launch.");
    check_row("Present thread", &c.present_thread, "Presents frames on a separate thread. Applies next launch.");
    check_row("Frame generation", &c.frame_gen, "Generates extra frames for smoother motion. Experimental.");
}

static void panel_audio() {
    CommonSettings& c = g_common;
    heading("AUD", "AUDIO");
    char rate[4][16];
    const char* rates[4];
    for (int i = 0; i < 4; ++i) {
        std::snprintf(rate[i], sizeof(rate[i]), "%d Hz", kRates[i]);
        rates[i] = rate[i];
    }
    combo_row("Sample rate", &c.rate, rates, 4, "Audio output frequency. 44100 Hz is the PlayStation's native rate.");
    check_row("Audio", &c.audio, "Turns all sound on or off.");
    stepper_row("Volume", &c.volume, 0, 100, 5, "Master volume.");
    check_row("High-quality SPU", &c.spuhq, "Uses floating-point sound processing for cleaner audio.");
}

static void panel_system() {
    CommonSettings& c = g_common;
    heading("SYS", "SYSTEM");
    slider_row("State slot", &c.slot, 1, 12, "%d", "Which of the 12 save-state slots quick save and load use.");
    check_row("Fast boot", &c.fastboot, "Skips the PlayStation boot logo.");
    check_row("HLE BIOS", &c.bioshle, "Uses faster emulated BIOS routines instead of running the real BIOS code.");
    check_row("Skip FMVs", &c.skipfmv, "Automatically skips pre-rendered movies.");
    ImGui::Spacing();
    path_row("BIOS image (empty = bundled OpenBIOS)", c.bios, false, false, "OpenBIOS",
             "Pick a retail BIOS image. Leave it empty to use the bundled OpenBIOS.");
    if (ImGui::Button("Use OpenBIOS")) c.bios[0] = 0;
}

static void panel_saves() {
    CommonSettings& c = g_common;
    heading("SAV", "MEMORY CARDS");
    path_row("Memory card folder (shared by all games)", c.memcard_dir, false, true,
             (g_root / "saves").string().c_str(), "Folder holding the memory card files. All games share it.");
    check_row("Card 1", &c.card1, "Inserts memory card 1.");
    check_row("Card 2", &c.card2, "Inserts memory card 2.");
    ImGui::Spacing();
    heading("REW", "REWIND");
    check_row("Rewind", &c.rewind, "Keeps a rolling history so you can rewind gameplay. Uses extra memory.");
    char depth[4][16], interval[5][16];
    const char *d[4], *iv[5];
    for (int i = 0; i < 4; ++i) { std::snprintf(depth[i], sizeof(depth[i]), "%d", kRewindDepth[i]); d[i] = depth[i]; }
    for (int i = 0; i < 5; ++i) { std::snprintf(interval[i], sizeof(interval[i]), "%d frames", kRewindInterval[i]); iv[i] = interval[i]; }
    combo_row("Rewind depth", &c.rewind_depth, d, 4, "How many snapshots are kept. More means a longer rewind.");
    combo_row("Rewind interval", &c.rewind_interval, iv, 5, "Frames between snapshots. Smaller is smoother but uses more memory.");
    slider_row("Fast forward speed", &c.ff_speed, 2, 16, "%dx", "Speed while the fast-forward hotkey is used.");
}

static void panel_device() {
    CommonSettings& c = g_common;
    heading("PAD", "CONTROL SCHEME");
    note("Choose how you play. The same choice is used in every game.");
    ImGui::Spacing();
    std::vector<std::string> labels{"Keyboard"};
    std::vector<std::string> guids{"keyboard"};
    for (const auto& p : g_pads) {
        labels.push_back(p.name);
        guids.push_back(p.guid);
    }
    int cur = -1;
    for (size_t i = 0; i < guids.size(); ++i)
        if (guids[i] == g_device) cur = (int)i;
    if (cur < 0) {
        labels.push_back("Saved controller (not connected)");
        guids.push_back(g_device);
        cur = (int)labels.size() - 1;
    }
    ImGui::PushID("device");
    row_label("Control device", 360.0f * g_ui);
        if (ImGui::BeginCombo("##c", labels[(size_t)cur].c_str())) {
            for (size_t i = 0; i < labels.size(); ++i)
                if (ImGui::Selectable(labels[i].c_str(), (int)i == cur)) g_device = guids[i];
            ImGui::EndCombo();
        }
        row_tip("Choose the keyboard or one of the connected controllers. The choice is used in every game.");
    ImGui::PopID();
    check_row("Dual analog buttons", &c.dual,
              "Maps the right stick to buttons so two-stick control works with a digital pad.");
    if (c.dual)
        check_row("Duplicate face on shoulders", &c.dual_dup,
                  "Also presses L1 = Triangle, R1 = Cross, L2 = Circle, R2 = Square while dual analog is on.");
    slider_row("Stick deadzone", &c.deadzone, 0, 50, "%d%%",
               "How far a stick must move before it registers. Raise it if the stick drifts.");
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
    ImGui::TextUnformatted(g_pads.empty() ? "No controllers detected." : "Detected controllers:");
    for (const auto& p : g_pads) ImGui::BulletText("%s", p.name.c_str());
    ImGui::PopStyleColor();
    if (ImGui::Button("Rescan controllers")) refresh_pads();
}

static void panel_padmap() {
    heading("MAP", "CONTROLLER BUTTONS");
    note("Click a button, then press the control on your controller. Esc cancels, Delete unbinds.");
    ImGui::Spacing();
    for (int i = 0; i < kKeyCount; ++i) {
        ImGui::PushID(2000 + i);
        row_label(kKeys[i].label);
        const bool waiting = g_pad_capture == i;
        if (waiting) ImGui::PushStyleColor(ImGuiCol_Button, kControlHot);
        if (ImGui::Button(waiting ? "Press a control..." : g_padmap[i].c_str(), ImVec2(-FLT_MIN, 0)))
            g_pad_capture = waiting ? -1 : i;
        if (waiting) ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::Spacing();
    if (ImGui::Button("Reset controller to defaults")) { reset_padmap(); g_pad_capture = -1; }
}

static void panel_input() {
    heading("KBD", "KEYBOARD");
    note("Keyboard controls are the same in every game. Click a button, then press a key. Esc cancels, Delete or Backspace unbinds.");
    ImGui::Spacing();
    for (int i = 0; i < kKeyCount; ++i) {
        ImGui::PushID(i);
        row_label(kKeys[i].label);
        const bool waiting = g_capture == i;
        if (waiting) ImGui::PushStyleColor(ImGuiCol_Button, kControlHot);
        if (ImGui::Button(waiting ? "Press a key..." : bind_name(g_binds[i]), ImVec2(-FLT_MIN, 0)))
            g_capture = waiting ? -1 : i;
        if (waiting) ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::Spacing();
    if (ImGui::Button("Reset keyboard to defaults")) { reset_binds(); g_capture = -1; }
}

static std::string hotpad_label(int v) {
    static const char* const kBtn[] = {"Cross", "Circle", "Square", "Triangle", "Select", "Guide", "Start",
                                       "L3", "R3", "L1", "R1", "Up", "Down", "Left", "Right"};
    if (v <= 0) return "None";
    std::string s;
    if (v >= 1000) {
        const uint32_t mask = (uint32_t)(v - 1000);
        for (int b = 0; b < 15; ++b)
            if (mask & (1u << b)) s += (s.empty() ? "" : " + ") + std::string(kBtn[b]);
        return s.empty() ? "None" : s;
    }
    if (v >= 100) {
        const int ax = (v - 100) / 2;
        static const char* const kAx[] = {"Left X", "Left Y", "Right X", "Right Y", "L2", "R2"};
        s = ax >= 0 && ax < 6 ? kAx[ax] : "Axis";
        if (ax == 4 || ax == 5) return "Select + " + s;
        return "Select + " + s + (((v - 100) & 1) ? "+" : "-");
    }
    const int b = v - 1;
    return std::string("Select + ") + (b >= 0 && b < 15 ? kBtn[b] : "Button");
}

static void panel_hotkeys() {
    heading("KEY", "HOTKEYS");
    note("Each action can have a keyboard shortcut and a controller shortcut at the same time. Click, then "
         "press the keys (keyboard) or hold the buttons (controller) before the timer runs out. "
         "Right-click unbinds, Esc cancels.");
    ImGui::Spacing();
    const double now = SDL_GetTicks() / 1000.0;
    const double left = std::max(0.0, kCaptureSecs - (now - g_hot_t0));
    const int secs = (int)left + (left > (int)left ? 1 : 0);
    if (ImGui::BeginTable("hotkeys", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        const float w = ImGui::GetContentRegionAvail().x;
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Keyboard", ImGuiTableColumnFlags_WidthFixed, std::min(210 * g_ui, w * 0.3f));
        ImGui::TableSetupColumn("Controller", ImGuiTableColumnFlags_WidthFixed, std::min(260 * g_ui, w * 0.36f));
        ImGui::TableHeadersRow();
        for (int i = 0; i < kHotCount; ++i) {
            ImGui::PushID(1000 + i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
            ImGui::TextUnformatted(kHot[i].label);
            ImGui::PopStyleColor();
            for (int col = 0; col < 2; ++col) {
                ImGui::TableSetColumnIndex(col + 1);
                ImGui::PushID(col);
                const bool waiting = g_hot_capture == i && g_hot_col == col;
                char cap[48];
                std::snprintf(cap, sizeof(cap), col == 0 ? "[ press keys... %d ]" : "[ hold buttons... %d ]", secs);
                const std::string shown = col == 0 ? (g_hot[i].empty() ? "None" : g_hot[i]) : hotpad_label(g_hotpad[i]);
                if (waiting) ImGui::PushStyleColor(ImGuiCol_Button, kControlHot);
                if (ImGui::Button(waiting ? cap : shown.c_str(), ImVec2(-FLT_MIN, 0))) {
                    if (waiting) {
                        g_hot_capture = -1;
                    } else {
                        g_hot_capture = i;
                        g_hot_col = col;
                        g_hot_t0 = now;
                        g_hot_held = g_hot_best = 0;
                        g_hot_axis = 0;
                    }
                }
                if (waiting) ImGui::PopStyleColor();
                if (ImGui::IsItemHovered() && g_hot_capture < 0) {
                    ImGui::SetTooltip("Click to rebind, right-click to unbind");
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                        if (col == 0) g_hot[i].clear();
                        else g_hotpad[i] = 0;
                    }
                }
                ImGui::PopID();
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::Spacing();
    note("A single controller button or trigger only fires while Select is held; capture two or more "
         "buttons together for a shortcut that works on its own.");
    if (ImGui::Button("Reset hotkeys to defaults")) { reset_hotkeys(); g_hot_capture = -1; }
}

static bool g_own_launcher = false;
static int g_launch = -1;

static void panel_game(int g) {
    GameState& s = g_state[g];
    heading("DISC", "DISC");
    if (s.multi()) {
        s.boot = std::clamp(s.boot, 1, (int)s.edit.size());
        ImGui::PushStyleColor(ImGuiCol_Text, kText);
        ImGui::TextUnformatted("Start with");
        ImGui::PopStyleColor();
        for (int i = 1; i <= (int)s.edit.size(); ++i) {
            char label[32];
            std::snprintf(label, sizeof(label), "Disc %d", i);
            if (i > 1) ImGui::SameLine();
            const bool on = s.boot == i;
            ImGui::PushStyleColor(ImGuiCol_Button, on ? kControlHot : kControl);
            ImGui::PushStyleColor(ImGuiCol_Border, on ? kAmber : kBorder);
            if (ImGui::Button(label, ImVec2(120 * g_ui, 0))) s.boot = i;
            ImGui::PopStyleColor(2);
        }
        ImGui::Spacing();
        for (size_t i = 0; i < s.edit.size(); ++i) {
            char label[64];
            std::snprintf(label, sizeof(label), "Disc %d image", (int)i + 1);
            path_row(label, s.edit[i].data(), true, false, "Select a .cue / .chd file");
        }
        note("Switch discs while playing from the in-game menu (SYSTEM SETUP > Disc).");
    } else if (!s.edit.empty()) {
        path_row("Disc image", s.edit[0].data(), true, false, "Select a .cue / .chd file");
    }
    ImGui::Spacing();
    if (ImGui::Button("Open this game's own launcher")) {
        g_launch = g;
        g_own_launcher = true;
    }
}

static bool launch_game(int g, bool own_launcher) {
    const fs::path exe = g_exe_dir / (std::string(kGames[g].exe)
#ifdef _WIN32
                                      + ".exe"
#endif
    );
    const std::string e = exe.string();
    const char* flag = own_launcher ? "--launcher" : "--no-launcher";
#ifdef _WIN32
    const char* args[] = {e.c_str(), flag, nullptr};
    if (_spawnv(_P_NOWAIT, e.c_str(), args) < 0) return false;
    std::exit(0);
#else
    char* args[] = {const_cast<char*>(e.c_str()), const_cast<char*>(flag), nullptr};
    execv(e.c_str(), args);
    return false;
#endif
}

static void apply_style() {
    ImGuiStyle& st = ImGui::GetStyle();
    st = ImGuiStyle();
    st.WindowRounding = st.ChildRounding = st.FrameRounding = st.GrabRounding = 0;
    st.PopupRounding = st.TabRounding = st.ScrollbarRounding = 0;
    st.FrameBorderSize = 1;
    st.ChildBorderSize = 1;
    st.PopupBorderSize = 1;
    st.WindowPadding = ImVec2(24, 24);
    st.FramePadding = ImVec2(16, 8);
    st.ItemSpacing = ImVec2(16, 8);
    st.ItemInnerSpacing = ImVec2(10, 6);
    st.ScrollbarSize = 12;
    st.GrabMinSize = 14;
    st.TabBarBorderSize = 1;
    st.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    st.SelectableTextAlign = ImVec2(0.06f, 0.5f);
    st.ScaleAllSizes(g_ui);
    ImVec4* col = st.Colors;
    col[ImGuiCol_Text] = kText;
    col[ImGuiCol_TextDisabled] = kMuted;
    col[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    col[ImGuiCol_ChildBg] = kPanel;
    col[ImGuiCol_PopupBg] = kPanel;
    col[ImGuiCol_Border] = kBorder;
    col[ImGuiCol_FrameBg] = kControl;
    col[ImGuiCol_FrameBgHovered] = kControlHot;
    col[ImGuiCol_FrameBgActive] = kControlHot;
    col[ImGuiCol_Button] = kControl;
    col[ImGuiCol_ButtonHovered] = kControlHot;
    col[ImGuiCol_ButtonActive] = kAmberDim;
    col[ImGuiCol_Header] = kPanelHot;
    col[ImGuiCol_HeaderHovered] = kControlHot;
    col[ImGuiCol_HeaderActive] = kControlHot;
    col[ImGuiCol_CheckMark] = kAmber;
    col[ImGuiCol_SliderGrab] = kAmber;
    col[ImGuiCol_SliderGrabActive] = kAmberDim;
    col[ImGuiCol_Separator] = kBorder;
    col[ImGuiCol_Tab] = kControl;
    col[ImGuiCol_TabHovered] = kControlHot;
    col[ImGuiCol_TabSelected] = kControlHot;
    col[ImGuiCol_TabSelectedOverline] = kAmber;
    col[ImGuiCol_TabDimmed] = kControl;
    col[ImGuiCol_TabDimmedSelected] = kControlHot;
    col[ImGuiCol_ScrollbarBg] = kBg;
    col[ImGuiCol_ScrollbarGrab] = kBorder;
    col[ImGuiCol_NavCursor] = kAmber;
}

static GLuint load_texture(const fs::path& p, int* w_out, int* h_out) {
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load(p.string().c_str(), &w, &h, &n, 4);
    if (!px) return 0;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);
    *w_out = w;
    *h_out = h;
    return tex;
}

static void set_window_icon(SDL_Window* win) {
    const std::string path = (g_exe_dir / "assets/psxrecomp.png").string();
    int w = 0, h = 0, n = 0;
    unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 4);
    if (!px) return;
    if (SDL_Surface* s = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, px, w * 4)) {
        SDL_SetWindowIcon(win, s);
        SDL_DestroySurface(s);
    }
    stbi_image_free(px);
}

int main(int, char**) {
    const char* base = SDL_GetBasePath();
    g_exe_dir = base ? fs::path(base) : fs::current_path();
    if (g_exe_dir.filename().empty()) g_exe_dir = g_exe_dir.parent_path();
    g_root = find_root();
    for (int g = 0; g < kGameCount; ++g) {
        GameState& s = g_state[g];
        s.default_roster = read_roster(g);
        const size_t n = std::max<size_t>(1, s.default_roster.size());
        s.edit.resize(n);
        for (size_t i = 0; i < n; ++i) {
            s.edit[i].fill(0);
            if (i < s.default_roster.size()) set_buf(s.edit[i].data(), s.default_roster[i]);
        }
    }
    set_buf(g_common.memcard_dir, (g_root / "saves").string());
    init_fields();
    reset_binds();
    reset_padmap();
    reset_hotkeys();
    load_hub();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "launcher: SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* win = SDL_CreateWindow("Armored Core - Launcher", 1180, 800,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_GLContext gl = win ? SDL_GL_CreateContext(win) : nullptr;
    if (!win || !gl) {
        std::fprintf(stderr, "launcher: window/GL failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(win, gl);
    SDL_GL_SetSwapInterval(1);
    set_window_icon(win);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    g_ui = std::max(1.0f, SDL_GetWindowDisplayScale(win));
    ImFont* title = nullptr;
    const std::string regular = (g_exe_dir / "assets/fonts/LatoLatin-Regular.ttf").string();
    const std::string bold = (g_exe_dir / "assets/fonts/LatoLatin-Bold.ttf").string();
    ImFont* body = exists(regular.c_str()) ? io.Fonts->AddFontFromFileTTF(regular.c_str(), 18.0f * g_ui) : nullptr;
    if (exists(bold.c_str())) title = io.Fonts->AddFontFromFileTTF(bold.c_str(), 30.0f * g_ui);
    if (!body) {
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = g_ui * 1.25f;
    }
    apply_style();
    ImGui_ImplSDL3_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    int brand_w = 0, brand_h = 0;
    const GLuint brand = load_texture(g_exe_dir / "assets/img/brand_psx.tga", &brand_w, &brand_h);

    int selected = 0;
    bool running = true;
    refresh_pads();
    while (running && g_launch < 0) {
        SDL_Event ev;
        bool pads_changed = false;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_GAMEPAD_ADDED || ev.type == SDL_EVENT_GAMEPAD_REMOVED) pads_changed = true;
            if (g_pad_capture >= 0) {
                std::string src;
                if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                    const char* n = SDL_GetGamepadStringForButton((SDL_GamepadButton)ev.gbutton.button);
                    if (n) src = n;
                } else if (ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && std::abs((int)ev.gaxis.value) > 20000) {
                    const SDL_GamepadAxis ax = (SDL_GamepadAxis)ev.gaxis.axis;
                    const char* n = SDL_GetGamepadStringForAxis(ax);
                    if (n) {
                        src = n;
                        if (ax != SDL_GAMEPAD_AXIS_LEFT_TRIGGER && ax != SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
                            src += ev.gaxis.value > 0 ? "+" : "-";
                    }
                } else if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                    if (ev.key.key == SDLK_DELETE || ev.key.key == SDLK_BACKSPACE) src = "none";
                    else if (ev.key.key == SDLK_ESCAPE) g_pad_capture = -1;
                    if (src.empty()) continue;
                }
                if (!src.empty()) {
                    g_padmap[g_pad_capture] = src;
                    g_pad_capture = -1;
                }
                if (ev.type != SDL_EVENT_QUIT && ev.type != SDL_EVENT_WINDOW_CLOSE_REQUESTED) continue;
            }
            if (g_hot_capture >= 0) {
                if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                    const SDL_Keycode k = ev.key.key;
                    const bool modifier = k == SDLK_LCTRL || k == SDLK_RCTRL || k == SDLK_LSHIFT || k == SDLK_RSHIFT ||
                                          k == SDLK_LALT || k == SDLK_RALT || k == SDLK_LGUI || k == SDLK_RGUI;
                    if (k == SDLK_ESCAPE) {
                        g_hot_capture = -1;
                    } else if (g_hot_col == 0 && !modifier) {
                        if (k == SDLK_DELETE || k == SDLK_BACKSPACE) {
                            g_hot[g_hot_capture].clear();
                        } else {
                            std::string s;
                            if (ev.key.mod & SDL_KMOD_CTRL) s += "Ctrl+";
                            if (ev.key.mod & SDL_KMOD_ALT) s += "Alt+";
                            if (ev.key.mod & SDL_KMOD_SHIFT) s += "Shift+";
                            g_hot[g_hot_capture] = s + SDL_GetKeyName(k);
                        }
                        g_hot_capture = -1;
                    }
                    continue;
                }
                if (g_hot_col == 1) {
                    // Controller shortcuts: whatever is held together at the same time becomes the chord.
                    if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && ev.gbutton.button < 32) {
                        g_hot_held |= 1u << ev.gbutton.button;
                        if (__builtin_popcount(g_hot_held) >= __builtin_popcount(g_hot_best)) g_hot_best = g_hot_held;
                        continue;
                    }
                    if (ev.type == SDL_EVENT_GAMEPAD_BUTTON_UP && ev.gbutton.button < 32) {
                        g_hot_held &= ~(1u << ev.gbutton.button);
                        continue;
                    }
                    if (ev.type == SDL_EVENT_GAMEPAD_AXIS_MOTION && !g_hot_axis && std::abs((int)ev.gaxis.value) > 12000) {
                        g_hot_axis = 100 + ev.gaxis.axis * 2 + (ev.gaxis.value > 0 ? 1 : 0);
                        continue;
                    }
                }
            }
            if (g_capture >= 0 && ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                const SDL_Scancode sc = ev.key.scancode;
                if (sc == SDL_SCANCODE_DELETE || sc == SDL_SCANCODE_BACKSPACE) g_binds[g_capture] = SDL_SCANCODE_UNKNOWN;
                else if (sc != SDL_SCANCODE_ESCAPE) g_binds[g_capture] = sc;
                g_capture = -1;
                continue;
            }
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT || ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
        }
        if (pads_changed) refresh_pads();
        if (g_hot_capture >= 0 && SDL_GetTicks() / 1000.0 - g_hot_t0 >= kCaptureSecs) {
            if (g_hot_col == 1) {
                // The binding format has no chord with a trigger; a trigger fires with Select held.
                if (g_hot_axis) g_hotpad[g_hot_capture] = g_hot_axis;
                else if (g_hot_best) g_hotpad[g_hot_capture] = 1000 + (int)g_hot_best;
            }
            g_hot_capture = -1;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImDrawList* bgl = ImGui::GetBackgroundDrawList();
        bgl->AddRectFilledMultiColor(ImVec2(0, 0), io.DisplaySize, u32(kBg2), u32(kBg2), u32(kBg), u32(kBg));
        // Faint amber haze behind the header.
        bgl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(io.DisplaySize.x, 110 * g_ui), u32(rgb(0xFF9E00, 0.10f)),
                                     u32(rgb(0xFF9E00, 0.10f)), u32(rgb(0xFF9E00, 0.0f)), u32(rgb(0xFF9E00, 0.0f)));

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##root", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                         ImGuiWindowFlags_NoBackground);

        // Header: brand mark, title, subtitle, cyan rule.
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float bh = 44 * g_ui;
            float tx = p.x;
            if (brand) {
                const float bw = bh * (float)brand_w / (float)std::max(1, brand_h);
                ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)brand, p, ImVec2(p.x + bw, p.y + bh));
                tx += bw + 14 * g_ui;
            }
            ImGui::SetCursorScreenPos(ImVec2(tx, p.y - 2 * g_ui));
            if (title) ImGui::PushFont(title);
            ImGui::TextUnformatted(kGames[selected].title);
            if (title) ImGui::PopFont();
            ImGui::SetCursorScreenPos(ImVec2(tx, p.y + 28 * g_ui));
            ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
            ImGui::TextUnformatted("PLAYSTATION");
            ImGui::PopStyleColor();
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + bh + 10 * g_ui));
            const float w = ImGui::GetContentRegionAvail().x;
            const ImVec2 r = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(r, ImVec2(r.x + w, r.y), u32(rgb(0x00D1F2, 0.55f)), 2 * g_ui);
            ImGui::GetWindowDrawList()->AddLine(r, ImVec2(r.x + w * 0.45f, r.y), u32(kCyan), 2 * g_ui);
            ImGui::Dummy(ImVec2(0, 12 * g_ui));
        }

        const float footer = 76.0f * g_ui;
        ImGui::BeginChild("games", ImVec2(230.0f * g_ui, -footer));
        ImGui::PushStyleColor(ImGuiCol_Text, kMuted);
        ImGui::TextUnformatted("SELECT GAME");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        for (int g = 0; g < kGameCount; ++g) {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float h = ImGui::GetFrameHeight() * 1.6f;
            if (ImGui::Selectable(kGames[g].title, selected == g, 0, ImVec2(0, h))) selected = g;
            if (selected == g)
                ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 3 * g_ui, p.y + h), u32(kAmber));
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("detail", ImVec2(0, -footer), ImGuiChildFlags_None);
        if (ImGui::BeginTabBar("tabs")) {
            static int first_tab = std::getenv("AC_HUB_TAB") ? std::atoi(std::getenv("AC_HUB_TAB")) : -1;
            auto flag = [&](int i) {
                return first_tab == i ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            };
            if (ImGui::BeginTabItem("Game", nullptr, flag(0))) {
                ImGui::BeginChild("tab_game", ImVec2(0, 0), ImGuiChildFlags_Borders);
                g_row_x = ImGui::GetContentRegionAvail().x * 0.48f;
                panel_game(selected);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Video", nullptr, flag(1))) {
                two_panels(panel_display, panel_graphics);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Audio", nullptr, flag(2))) {
                two_panels(panel_audio, [] {});
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Controls", nullptr, flag(3))) {
                if (ImGui::BeginTabBar("controls")) {
                    static int first_sub = std::getenv("AC_HUB_SUB") ? std::atoi(std::getenv("AC_HUB_SUB")) : -1;
                    auto sub = [&](int i) { return first_sub == i ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None; };
                    const float fw = ImGui::GetContentRegionAvail().x;
                    if (ImGui::BeginTabItem("Scheme", nullptr, sub(0))) {
                        panel_begin("p_dev", fw);
                        panel_device();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Keyboard")) {
                        panel_begin("p_kbd", fw);
                        panel_input();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Controller")) {
                        panel_begin("p_pad", fw);
                        panel_padmap();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Hotkeys", nullptr, sub(3))) {
                        panel_begin("p_hot", fw);
                        panel_hotkeys();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                    first_sub = -1;
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("System", nullptr, flag(4))) {
                two_panels(panel_system, panel_saves);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
            first_tab = -1;
        }
        ImGui::EndChild();

        // Footer rule, Quit, and the amber PLAY button with a glow.
        {
            const ImVec2 r = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddLine(r, ImVec2(r.x + ImGui::GetContentRegionAvail().x, r.y),
                                                u32(rgb(0x00D1F2, 0.55f)), 2 * g_ui);
            ImGui::Dummy(ImVec2(0, 8 * g_ui));
            if (ImGui::Button("Quit")) running = false;
            ImGui::SameLine();
            if (ImGui::Button("Sync settings to games")) {
                save_hub();
                for (int g = 0; g < kGameCount; ++g) apply_to_game(g);
                g_synced_until = SDL_GetTicks() / 1000.0 + 3.0;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Writes these settings, controls and hotkeys into every game's own launcher settings now.");
            if (SDL_GetTicks() / 1000.0 < g_synced_until) {
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::TextColored(kGood, "Settings synced to all games");
            }
            ImGui::SameLine();
            const float bw = 260 * g_ui, bh = 48 * g_ui;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - bw - ImGui::GetStyle().WindowPadding.x);
            const ImVec2 b = ImGui::GetCursorScreenPos();
            for (int i = 1; i <= 4; ++i)
                ImGui::GetWindowDrawList()->AddRect(ImVec2(b.x - i * 2 * g_ui, b.y - i * 2 * g_ui),
                                                    ImVec2(b.x + bw + i * 2 * g_ui, b.y + bh + i * 2 * g_ui),
                                                    u32(rgb(0xFF9E00, 0.22f / (float)i)), 0, 0, 2 * g_ui);
            ImGui::PushStyleColor(ImGuiCol_Button, kAmber);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, rgb(0xFFB333));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAmberDim);
            ImGui::PushStyleColor(ImGuiCol_Text, rgb(0x000000));
            if (ImGui::Button("PLAY", ImVec2(bw, bh))) {
                g_launch = selected;
                g_own_launcher = false;
            }
            ImGui::PopStyleColor(4);
        }
        ImGui::End();

        // CRT scanlines (the Armored Core theme turns them on).
        {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            const ImU32 sl = u32(ImVec4(0, 0, 0, 0.18f));
            const float step = std::max(2.0f, 3.0f * g_ui);
            for (float y = 0; y < io.DisplaySize.y; y += step)
                fg->AddLine(ImVec2(0, y), ImVec2(io.DisplaySize.x, y), sl, std::max(1.0f, g_ui));
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    save_hub();
    for (int g = 0; g < kGameCount; ++g) apply_to_game(g);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();

    if (g_launch >= 0 && !launch_game(g_launch, g_own_launcher)) {
        std::fprintf(stderr, "launcher: could not start %s in %s\n", kGames[g_launch].exe,
                     g_exe_dir.string().c_str());
        return 1;
    }
    return 0;
}
