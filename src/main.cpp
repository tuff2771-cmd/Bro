#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/RingObject.hpp>
#include "Config.hpp"
#include "CheatsPopup.hpp"
#include <cmath>
using namespace geode::prelude;

static bool g_gravLocked[2] = {false, false};
static bool g_gravVal[2] = {false, false};

// object ids (verify): green dash 1704, black 1330, blue orb 84
// pads: yellow 35, pink 140, red 1332, blue(gravity) 67

static std::vector<GameObject*> fresh(PlayerObject* p, bool p1, std::initializer_list<int> ids) {
    std::vector<GameObject*> out;
    if (!p->m_touchingRings) return out;
    for (auto o : CCArrayExt<GameObject*>(p->m_touchingRings)) {
        if (p1 ? o->m_hasBeenActivated : o->m_hasBeenActivatedP2) continue;
        for (int id : ids) if (o->m_objectID == id) { out.push_back(o); break; }
    }
    return out;
}

class $modify(CUPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto spr = ButtonSprite::create("Utils");
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCObject*) {
            CheatsPopup::create()->show();
        });
        auto menu = CCMenu::create();
        menu->addChild(btn);
        auto ws = CCDirector::get()->getWinSize();
        menu->setPosition({ws.width - 50.f, 30.f});
        this->addChild(menu);
    }
};

class $modify(CUPlay, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        g_gravLocked[0] = g_gravLocked[1] = false;
    }
    void destroyPlayer(PlayerObject* p, GameObject* o) {
        if (o != m_anticheatSpike && cfg::on("noclip") && cfg::forPlayer("noclip", p == m_player1)) return;
        PlayLayer::destroyPlayer(p, o);
    }
};

class $modify(CURing, RingObject) {
    void spawnCircle() {
        if (cfg::on("noOrbFx")) return;
        RingObject::spawnCircle();
    }
};

class $modify(CUPlayer, PlayerObject) {
    void bumpPlayer(float a, int b, bool c, GameObject* o) {
        PlayerObject::bumpPlayer(a, b, c, o);
        auto pl = PlayLayer::get();
        if (!pl || !o) return;
        int id = o->m_objectID;
        bool jump = id == 35 || id == 140 || id == 1332;
        bool grav = id == 67;
        if ((jump && cfg::on("jumpPads")) || (grav && cfg::on("gravPads"))) {
            bool p1 = this == pl->m_player1;
            pl->handleButton(true, 1, p1);
            pl->handleButton(false, 1, p1);
        }
    }
};

class $modify(CUBase, GJBaseGameLayer) {
    void swift(bool p1) {
        this->handleButton(true, 1, p1);
        this->handleButton(false, 1, p1);
    }
    void hold(PlayerObject* p, bool p1, bool down) {
        if (p->m_holdingButtons[1] != down) this->handleButton(down, 1, p1);
    }

    void gap(PlayerObject* p, bool p1) {
        std::string k = p1 ? "p1" : "p2";
        if (cfg::opt("gapSpam", k + "En", 1) < 0.5f) return;
        float thr = cfg::opt("gapSpam", k + "Threshold", 3.f);
        float rec = cfg::opt("gapSpam", k + "Recenter", 0.f);
        float px = p->getPositionX(), py = p->getPositionY();
        float above = 1e9f, below = -1e9f;
        bool hA = false, hB = false;
        // scans every object each tick, ok for now, optimize later with sections
        for (auto o : CCArrayExt<GameObject*>(m_objects)) {
            if (!o || o->m_objectType != GameObjectType::Solid) continue;
            if (std::abs(o->getPositionX() - px) > 60.f) continue;
            auto r = o->getObjectRect();
            if (r.getMinY() >= py) { if (r.getMinY() < above) { above = r.getMinY(); hA = true; } }
            else if (r.getMaxY() <= py) { if (r.getMaxY() > below) { below = r.getMaxY(); hB = true; } }
        }
        float mid;
        if (hA && hB) mid = (above + below) / 2.f;
        else if (rec != 0.f) mid = rec;
        else return;
        if (py < mid - thr) hold(p, p1, true);
        else if (py > mid + thr) hold(p, p1, false);
        else hold(p, p1, !p->m_holdingButtons[1]); // spam inside band
        if (cfg::opt("gapSpam", k + "BlueOrb", 0.f) > 0.5f) {
            for (auto o : fresh(p, p1, {84})) { (void)o; swift(p1); }
        }
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);
        auto pl = PlayLayer::get();
        if (!pl || static_cast<GJBaseGameLayer*>(pl) != this) return;
        if (!m_player1 || m_player1->m_isDead) return;

        static bool dashHeld[2] = {false, false};
        static int counter[2] = {0, 0};
        bool dual = m_gameState.m_isDualMode;

        for (int i = 0; i < 2; i++) {
            bool p1 = i == 0;
            auto p = p1 ? m_player1 : m_player2;
            if (!p || (!p1 && !dual)) continue;

            // green dash orbs: press and hold while in orb
            if (cfg::on("greenDash") && cfg::forPlayer("greenDash", p1)) {
                auto r = fresh(p, p1, {1704});
                if (!r.empty() && !dashHeld[i]) { this->handleButton(true, 1, p1); dashHeld[i] = true; }
                else if (r.empty() && dashHeld[i]) { this->handleButton(false, 1, p1); dashHeld[i] = false; }
            }

            // black orbs: one swift click per orb, so 3 orbs = 3 clicks
            if (cfg::on("blackOrb") || cfg::on("easyUfoBlack")) {
                bool skip = false;
                if (cfg::on("easyUfoBlack") && p->m_isBird) {
                    float v = p->m_yVelocity;
                    skip = p->m_isUpsideDown ? v > 0 : v < 0;
                } else if (!cfg::on("blackOrb")) skip = true;
                if (!skip) for (auto o : fresh(p, p1, {1330})) { (void)o; swift(p1); }
            }

            // maintain gravity (no input blocking)
            if (cfg::on("maintainGravity") && cfg::forPlayer("maintainGravity", p1)) {
                if (!g_gravLocked[i]) { g_gravLocked[i] = true; g_gravVal[i] = p->m_isUpsideDown; }
                else if (p->m_isUpsideDown != g_gravVal[i]) p->flipGravity(g_gravVal[i], true);
            }

            // auto straight fly
            if (cfg::on("autoStraightFly") && p->m_isShip && cfg::forPlayer("autoStraightFly", p1)) {
                float thr = cfg::opt("autoStraightFly", "threshold", 3.f) * 0.1f;
                float v = p->m_yVelocity;
                if (v < -thr) hold(p, p1, true);
                else if (v > thr) hold(p, p1, false);
            }

            // auto straight ufo
            if (cfg::on("autoStraightUfo") && p->m_isBird && cfg::forPlayer("autoStraightUfo", p1)) {
                float thr = cfg::opt("autoStraightUfo", "threshold", 3.f);
                float err = cfg::opt("autoStraightUfo", "yTarget", 200.f) - p->getPositionY();
                if (p->m_isUpsideDown) err = -err;
                if (err > thr) swift(p1);
            }

            // autoclickers
            std::string ac = p1 ? "autoClick1" : "autoClick2";
            if (cfg::on(ac)) {
                int iv = std::max(1, (int)cfg::opt(ac, "interval", 2.f));
                int hd = std::max(1, (int)cfg::opt(ac, "hold", 1.f));
                bool sw = cfg::opt(ac, "swift", 0.f) > 0.5f;
                int c = counter[i]++ % iv;
                if (c == 0) { if (sw) swift(p1); else hold(p, p1, true); }
                else if (!sw && c == hd) hold(p, p1, false);
            }

            if (cfg::on("autoSwift")) swift(p1);
            if (cfg::on("gapSpam") && p->m_isDart) gap(p, p1);
        }

        // 2P control link
        if (cfg::on("link2P") && dual && m_player2) {
            bool drvP1 = cfg::opt("link2P", "driver", 0.f) < 0.5f;
            bool invert = cfg::opt("link2P", "mode", 0.f) < 0.5f;
            auto drv = drvP1 ? m_player1 : m_player2;
            auto fol = drvP1 ? m_player2 : m_player1;
            bool want = drv->m_holdingButtons[1];
            if (invert) want = !want;
            hold(fol, !drvP1, want);
        }
    }
};
