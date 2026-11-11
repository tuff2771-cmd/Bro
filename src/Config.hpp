#pragma once
#include <Geode/Geode.hpp>
#include <unordered_map>
#include <string>
#include <vector>
using namespace geode::prelude;

namespace cfg {
    enum class FT { Bool, Float, Choice };
    struct Field {
        std::string key, label;
        FT type;
        float def = 0;
        std::vector<std::string> choices;
    };
    struct Cheat {
        std::string id, label;
        std::vector<Field> fields;
    };

    inline std::unordered_map<std::string, float>& cache() {
        static std::unordered_map<std::string, float> c;
        return c;
    }
    inline float get(std::string const& key, float def) {
        auto& c = cache();
        auto it = c.find(key);
        if (it != c.end()) return it->second;
        float v = (float)Mod::get()->getSavedValue<double>(key, (double)def);
        c[key] = v;
        return v;
    }
    inline void set(std::string const& key, float v) {
        cache()[key] = v;
        Mod::get()->setSavedValue<double>(key, (double)v);
    }
    inline bool on(std::string const& id) { return get(id, 0.f) > 0.5f; }
    inline float opt(std::string const& id, std::string const& f, float def) {
        return get(id + "." + f, def);
    }
    // players: 0 = P1, 1 = P2, 2 = both
    inline bool forPlayer(std::string const& id, bool p1) {
        int m = (int)opt(id, "players", 2);
        return m == 2 || ((m == 0) == p1);
    }

    inline std::vector<Cheat> const& cheats() {
        static std::vector<std::string> pl = {"Player 1", "Player 2", "Both"};
        static Field players{"players", "Players", FT::Choice, 2, pl};
        static Field thr{"threshold", "Threshold", FT::Float, 3.f, {}};
        static std::vector<Cheat> list = {
            {"greenDash", "Click Green Dash Orbs", {players}},
            {"noOrbFx", "No Orb Effects", {}},
            {"blackOrb", "Click Black Orbs", {}},
            {"easyUfoBlack", "Easy Click Black Orb UFO", {}},
            {"jumpPads", "Click Jump Pads", {}},
            {"gravPads", "Click Gravity Pads", {}},
            {"maintainGravity", "Maintain Gravity", {players}},
            {"autoStraightFly", "Auto Straight Fly", {players, thr}},
            {"autoStraightUfo", "Auto Straight UFO", {players, thr,
                {"yTarget", "Y Position Target", FT::Float, 200.f, {}}}},
            {"autoClick1", "AutoClicker Player1", {
                {"interval", "Click every (frames)", FT::Float, 2.f, {}},
                {"hold", "Hold for (frames)", FT::Float, 1.f, {}},
                {"swift", "Swift", FT::Bool, 0.f, {}}}},
            {"autoClick2", "AutoClicker Player2", {
                {"interval", "Click every (frames)", FT::Float, 2.f, {}},
                {"hold", "Hold for (frames)", FT::Float, 1.f, {}},
                {"swift", "Swift", FT::Bool, 0.f, {}}}},
            {"noclip", "Noclip", {players}},
            {"autoSwift", "Auto Swift", {}},
            {"link2P", "2P Control Link", {
                {"mode", "Mode", FT::Choice, 0.f, {"Invert", "Copy"}},
                {"driver", "Driving side", FT::Choice, 0.f, {"Player 1", "Player 2"}}}},
            {"gapSpam", "Moving Wave Gap Spam", {
                {"p1En", "Player 1", FT::Bool, 1.f, {}},
                {"p1Threshold", "P1 Threshold", FT::Float, 3.f, {}},
                {"p1Recenter", "P1 Recenter (below)", FT::Float, 0.f, {}},
                {"p1BlueOrb", "P1 Blue Orb", FT::Bool, 0.f, {}},
                {"p2En", "Player 2", FT::Bool, 1.f, {}},
                {"p2Threshold", "P2 Threshold", FT::Float, 3.f, {}},
                {"p2Recenter", "P2 Recenter (below)", FT::Float, 0.f, {}},
                {"p2BlueOrb", "P2 Blue Orb", FT::Bool, 0.f, {}}}},
        };
        return list;
    }
}
