#pragma once
#include "Config.hpp"

class OptionsPopup : public geode::Popup {
protected:
    int m_idx = 0;

    bool init(int idx) {
        auto const& c = cfg::cheats()[idx];
        if (!Popup::init(320.f, 65.f + c.fields.size() * 26.f, "square02_001.png")) return false;
        m_idx = idx;
        this->setTitle(c.label.c_str());
        m_bgSprite->setColor({0, 0, 0});
        m_bgSprite->setOpacity(245);

        float W = m_size.width, H = m_size.height;
        for (size_t i = 0; i < c.fields.size(); i++) {
            auto const& f = c.fields[i];
            std::string key = c.id + "." + f.key;
            float y = H - 50.f - i * 26.f;

            auto lbl = CCLabelBMFont::create(f.label.c_str(), "bigFont.fnt");
            lbl->setColor({255, 255, 255});
            lbl->setAnchorPoint({0, 0.5f});
            lbl->setScale(0.4f);
            m_mainLayer->addChildAtPosition(lbl, Anchor::BottomLeft, {15.f, y});

            if (f.type == cfg::FT::Bool) {
                auto t = CCMenuItemToggler::createWithStandardSprites(
                    this, menu_selector(OptionsPopup::onBool), 0.6f);
                t->toggle(cfg::get(key, f.def) > 0.5f);
                t->setTag((int)i);
                m_buttonMenu->addChildAtPosition(t, Anchor::BottomLeft, {W - 40.f, y});
            } else if (f.type == cfg::FT::Float) {
                auto in = TextInput::create(70.f, "0", "bigFont.fnt");
                in->setFilter(CommonFilter::Float);
                in->setString(fmt::format("{}", cfg::get(key, f.def)));
                in->setCallback([key](std::string const& s) {
                    cfg::set(key, utils::numFromString<float>(s).unwrapOr(0.f));
                });
                m_mainLayer->addChildAtPosition(in, Anchor::BottomLeft, {W - 55.f, y});
            } else {
                int v = (int)cfg::get(key, f.def);
                auto spr = ButtonSprite::create(f.choices[v].c_str(), 80, true, "bigFont.fnt", "GJ_button_01.png", 25.f, 0.5f);
                auto b = CCMenuItemSpriteExtra::create(spr, this, menu_selector(OptionsPopup::onCycle));
                b->setTag((int)i);
                m_buttonMenu->addChildAtPosition(b, Anchor::BottomLeft, {W - 55.f, y});
            }
        }
        return true;
    }

    void onBool(CCObject* s) {
        auto t = static_cast<CCMenuItemToggler*>(s);
        auto const& c = cfg::cheats()[m_idx];
        cfg::set(c.id + "." + c.fields[t->getTag()].key, t->isToggled() ? 0.f : 1.f);
    }

    void onCycle(CCObject* s) {
        auto b = static_cast<CCMenuItemSpriteExtra*>(s);
        auto const& c = cfg::cheats()[m_idx];
        auto const& f = c.fields[b->getTag()];
        std::string key = c.id + "." + f.key;
        int v = ((int)cfg::get(key, f.def) + 1) % (int)f.choices.size();
        cfg::set(key, (float)v);
        static_cast<ButtonSprite*>(b->getNormalImage())->setString(f.choices[v].c_str());
    }

public:
    static OptionsPopup* create(int idx) {
        auto ret = new OptionsPopup();
        if (ret->init(idx)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class CheatsPopup : public geode::Popup {
protected:
    bool init() {
        if (!Popup::init(440.f, 290.f, "square02_001.png")) return false;
        this->setTitle("Gameplay");
        m_bgSprite->setColor({0, 0, 0});
        m_bgSprite->setOpacity(245);

        auto const& list = cfg::cheats();
        float H = m_size.height;
        for (size_t i = 0; i < list.size(); i++) {
            auto const& c = list[i];
            int col = i / 8, row = i % 8;
            float x = 15.f + col * 215.f;
            float y = H - 55.f - row * 28.f;

            auto t = CCMenuItemToggler::createWithStandardSprites(
                this, menu_selector(CheatsPopup::onToggle), 0.6f);
            t->setTag((int)i);
            t->toggle(cfg::on(c.id));
            m_buttonMenu->addChildAtPosition(t, Anchor::BottomLeft, {x + 12.f, y});

            auto lbl = CCLabelBMFont::create(c.label.c_str(), "bigFont.fnt");
            lbl->setColor({255, 255, 255});
            lbl->setAnchorPoint({0, 0.5f});
            lbl->limitLabelWidth(130.f, 0.4f, 0.1f);
            m_mainLayer->addChildAtPosition(lbl, Anchor::BottomLeft, {x + 30.f, y});

            if (!c.fields.empty()) {
                auto g = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
                g->setScale(0.45f);
                auto b = CCMenuItemSpriteExtra::create(g, this, menu_selector(CheatsPopup::onGear));
                b->setTag((int)i);
                m_buttonMenu->addChildAtPosition(b, Anchor::BottomLeft, {x + 185.f, y});
            }
        }
        return true;
    }

    void onToggle(CCObject* s) {
        auto t = static_cast<CCMenuItemToggler*>(s);
        cfg::set(cfg::cheats()[t->getTag()].id, t->isToggled() ? 0.f : 1.f);
    }
    void onGear(CCObject* s) {
        OptionsPopup::create(static_cast<CCNode*>(s)->getTag())->show();
    }

public:
    static CheatsPopup* create() {
        auto ret = new CheatsPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
