#include <Geode/modify/MenuLayer.hpp>

#include "AudioLibraryLayer.hpp"

using namespace geode::prelude;

class $modify(AudioLibraryMenu, MenuLayer) {
    static void onModify(auto& self) {
        (void)self.setHookPriorityAfter("MenuLayer::init", "geode.node-ids");
    }

    bool init() {
        if (!MenuLayer::init()) return false;

        auto* menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto* button = CCMenuItemExt::createSpriteExtra(
            CircleButtonSprite::createWithSpriteFrameName(
                "GJ_musicOnBtn_001.png",
                .8f,
                CircleBaseColor::Blue,
                CircleBaseSize::MediumAlt
            ),
            [](CCObject*) {
                if (auto* layer = AudioLibraryLayer::create()) layer->show();
            }
        );
        button->setID("audio-library-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }
};
