#pragma once

#include <Geode/Geode.hpp>

/// Pantalla de canciones y efectos descargados.
class AudioLibraryLayer : public geode::Popup {
public:
    struct Entry {
        int id = 0;
        std::string name;
        std::string detail;
        std::uint64_t bytes = 0;
        std::optional<int> ageDays;
    };

protected:
    enum class Tab { Songs, Sfx };

    Tab m_tab = Tab::Songs;
    geode::ScrollLayer* m_list = nullptr;
    geode::TextInput* m_daysInput = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCLabelBMFont* m_ageLabel = nullptr;
    cocos2d::CCLabelBMFont* m_daysLabel = nullptr;
    cocos2d::CCLabelBMFont* m_legendSongs = nullptr;
    cocos2d::CCLabelBMFont* m_legendSfx = nullptr;
    cocos2d::CCLabelBMFont* m_legendDisk = nullptr;
    cocos2d::CCLabelBMFont* m_emptyLabel = nullptr;
    cocos2d::CCDrawNode* m_pie = nullptr;
    CCMenuItemSpriteExtra* m_songsItem = nullptr;
    CCMenuItemSpriteExtra* m_sfxItem = nullptr;
    CCMenuItemSpriteExtra* m_markItem = nullptr;
    CCMenuItemSpriteExtra* m_clearItem = nullptr;
    CCMenuItemSpriteExtra* m_deleteItem = nullptr;
    CCMenuItemSpriteExtra* m_langItem = nullptr;
    std::unordered_set<int> m_selectedSongs;
    std::unordered_set<int> m_selectedSfx;
    std::vector<Entry> m_visible;
    int m_previewSong = 0;
    bool m_previewing = false;
    bool m_syncingToggles = false;

    bool init();
    void applyLanguage();
    void rebuild();
    void updateStatus();
    void refreshPie();
    void stopPreview();
    void togglePlay(int id);
    int ageLimitDays() const;
    std::unordered_set<int>& selection();
    void onToggle(cocos2d::CCObject* sender);
    void onSelectVisible(cocos2d::CCObject* sender);
    void onClearSelection(cocos2d::CCObject* sender);
    void onDelete(cocos2d::CCObject* sender);
    void onLanguage(cocos2d::CCObject* sender);
    void performDelete();
    void onClose(cocos2d::CCObject* sender) override;

public:
    static AudioLibraryLayer* create();
};
