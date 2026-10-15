#include "AudioLibraryLayer.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <functional>
#include <vector>

using namespace geode::prelude;

namespace {

constexpr float kPi = 3.14159265f;

std::string toStd(gd::string const& value) {
    return std::string(value.c_str());
}

bool isSpanish() {
    return Mod::get()->getSavedValue<std::string>("language", "en") == "es";
}

char const* tr(char const* english, char const* spanish) {
    return isSpanish() ? spanish : english;
}

std::string formatBytes(std::uint64_t bytes) {
    double const value = static_cast<double>(bytes);
    if (value >= 1024.0 * 1024.0 * 1024.0) {
        return fmt::format("{:.1f} GB", value / (1024.0 * 1024.0 * 1024.0));
    }
    if (value >= 1024.0 * 1024.0) {
        return fmt::format("{:.1f} MB", value / (1024.0 * 1024.0));
    }
    if (value >= 1024.0) {
        return fmt::format("{:.0f} KB", value / 1024.0);
    }
    return fmt::format("{} B", bytes);
}

struct FileFacts {
    std::uint64_t bytes = 0;
    std::optional<int> ageDays;
    std::string date;
};

FileFacts readFileFacts(std::string const& path) {
    FileFacts facts;
    if (path.empty()) return facts;

    std::error_code error;
    auto const file = std::filesystem::path(path);
    if (!std::filesystem::is_regular_file(file, error)) return facts;

    facts.bytes = std::filesystem::file_size(file, error);
    if (error) facts.bytes = 0;

    auto const modified = std::filesystem::last_write_time(file, error);
    if (error) return facts;

    // clock_cast no existe en el libc++ de macOS y Android.
    auto const writtenAt = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        modified - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    auto const age = std::chrono::duration_cast<std::chrono::hours>(
        std::chrono::system_clock::now() - writtenAt
    );
    facts.ageDays = static_cast<int>(age.count() / 24);
    if (*facts.ageDays < 0) facts.ageDays = 0;

    std::time_t const stamp = std::chrono::system_clock::to_time_t(writtenAt);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &stamp);
#else
    localtime_r(&stamp, &local);
#endif
    char buffer[16] = {};
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local)) {
        facts.date = buffer;
    }
    return facts;
}

void drawSlice(CCDrawNode* node, CCPoint const& center, float radius, float start, float sweep, ccColor4F const& color) {
    if (sweep <= 0.001f) return;
    int const steps = std::max(4, static_cast<int>(sweep / 0.12f));
    std::vector<CCPoint> verts;
    verts.reserve(static_cast<size_t>(steps) + 2);
    verts.push_back(center);
    for (int step = 0; step <= steps; ++step) {
        float const angle = start + sweep * (static_cast<float>(step) / static_cast<float>(steps));
        verts.push_back(center + CCPoint { std::cos(angle) * radius, std::sin(angle) * radius });
    }
    node->drawPolygon(verts.data(), static_cast<unsigned int>(verts.size()), color, 0.f, color, BorderAlignment::Center);
}

CCMenuItemSpriteExtra* makeButton(char const* text, char const* background, float scale, std::function<void(CCObject*)> callback) {
    return CCMenuItemExt::createSpriteExtra(
        ButtonSprite::create(text, "bigFont.fnt", background, scale),
        std::move(callback)
    );
}

void setCaption(CCMenuItemSpriteExtra* item, char const* text, char const* background, float scale) {
    item->setNormalImage(ButtonSprite::create(text, "bigFont.fnt", background, scale));
    item->setSelectedImage(ButtonSprite::create(text, "bigFont.fnt", background, scale));
}

std::vector<AudioLibraryLayer::Entry> loadSongs(int minimumAge) {
    std::vector<AudioLibraryLayer::Entry> entries;
    auto* manager = MusicDownloadManager::sharedState();
    auto* songs = manager ? manager->getDownloadedSongs() : nullptr;
    if (!songs) return entries;

    for (auto* song : CCArrayExt<SongInfoObject*>(songs)) {
        if (!song) continue;
        int const id = song->m_songID;
        if (manager->isResourceSong(id) || !manager->isSongDownloaded(id)) continue;

        auto const facts = readFileFacts(toStd(manager->pathForSong(id)));
        if (minimumAge > 0 && (!facts.ageDays || *facts.ageDays < minimumAge)) continue;

        auto name = toStd(song->m_songName);
        if (name.empty()) name = tr("Untitled", "Sin nombre");
        std::uint64_t bytes = facts.bytes;
        if (bytes == 0 && song->m_fileSize > 0.f) {
            bytes = static_cast<std::uint64_t>(song->m_fileSize * 1024.f * 1024.f);
        }
        auto const date = facts.date.empty() ? tr("no date", "sin fecha") : facts.date;
        entries.push_back({
            id,
            name,
            fmt::format("#{}   {}   {}", id, formatBytes(bytes), date),
            bytes,
            facts.ageDays,
        });
    }
    return entries;
}

std::vector<AudioLibraryLayer::Entry> loadSfx(int minimumAge) {
    std::vector<AudioLibraryLayer::Entry> entries;
    auto* manager = MusicDownloadManager::sharedState();
    auto* effects = manager ? manager->getAllSFXObjects(false) : nullptr;
    if (!effects) return entries;

    for (auto* effect : CCArrayExt<SFXInfoObject*>(effects)) {
        if (!effect || effect->m_folder) continue;
        int const id = effect->m_sfxID;
        if (manager->isResourceSFX(id) || !manager->isSFXDownloaded(id)) continue;

        auto const facts = readFileFacts(toStd(manager->pathForSFX(id)));
        if (minimumAge > 0 && (!facts.ageDays || *facts.ageDays < minimumAge)) continue;

        auto name = toStd(effect->m_name);
        if (name.empty()) name = tr("Untitled", "Sin nombre");
        std::uint64_t bytes = facts.bytes;
        if (bytes == 0 && effect->m_size > 0) {
            bytes = static_cast<std::uint64_t>(effect->m_size);
        }
        auto const date = facts.date.empty() ? tr("no date", "sin fecha") : facts.date;
        entries.push_back({
            id,
            name,
            fmt::format("#{}   {}   {}", id, formatBytes(bytes), date),
            bytes,
            facts.ageDays,
        });
    }
    return entries;
}

bool isSongStillAudible(MusicDownloadManager* manager, FMODAudioEngine* audio, int id) {
    if (!manager || !audio) return false;
    auto const path = manager->pathForSong(id);
    if (toStd(path).empty()) return false;
    if (audio->isMusicPlaying(path, 0)) return true;
    return audio->getActiveMusic(0) == path || audio->getActiveMusic(1) == path;
}

struct DiskUsage {
    std::uint64_t songs = 0;
    std::uint64_t sfx = 0;
    std::uint64_t capacity = 0;
    std::uint64_t free = 0;
};

DiskUsage measureDisk() {
    DiskUsage usage;
    for (auto const& entry : loadSongs(0)) usage.songs += entry.bytes;
    for (auto const& entry : loadSfx(0)) usage.sfx += entry.bytes;

    std::filesystem::path probe;
    if (auto* manager = MusicDownloadManager::sharedState()) {
        auto const folder = toStd(manager->pathForSongFolder(1));
        if (!folder.empty()) probe = folder;
    }
    if (probe.empty()) {
        if (auto* utils = CCFileUtils::sharedFileUtils()) probe = utils->getWritablePath();
    }

    std::error_code error;
    if (!probe.empty()) {
        auto const space = std::filesystem::space(probe, error);
        if (!error) {
            usage.capacity = space.capacity;
            usage.free = space.available;
        }
    }
    return usage;
}

} // namespace

AudioLibraryLayer* AudioLibraryLayer::create() {
    auto* layer = new AudioLibraryLayer();
    if (layer && layer->init()) {
        layer->autorelease();
        return layer;
    }
    CC_SAFE_DELETE(layer);
    return nullptr;
}

bool AudioLibraryLayer::init() {
    auto const win = CCDirector::get()->getWinSize();
    float const width = std::min(460.f, win.width - 20.f);
    float const height = std::min(292.f, win.height - 10.f);
    if (!Popup::init(width, height)) return false;

    float const top = height / 2.f;
    float const tabY = top - 48.f;
    float const infoY = top - 104.f;
    float const filterY = top - 154.f;
    float const listBottom = 42.f;
    float const listTop = top - 172.f;
    float const listHeight = std::max(52.f, listTop - (-top + listBottom));
    float const listWidth = width - 20.f;
    float const tabGap = std::min(96.f, width * 0.22f);
    float const buttonGap = std::min(108.f, (width - 30.f) / 3.2f);

    m_langItem = makeButton("ES", "GJ_button_05.png", .38f, [this](CCObject* sender) {
        this->onLanguage(sender);
    });
    m_buttonMenu->addChildAtPosition(m_langItem, Anchor::TopRight, { -28.f, -16.f });

    m_songsItem = makeButton("SONGS", "GJ_button_01.png", .4f, [this](CCObject*) {
        m_tab = Tab::Songs;
        this->rebuild();
    });
    m_sfxItem = makeButton("EFFECTS", "GJ_button_01.png", .4f, [this](CCObject*) {
        m_tab = Tab::Sfx;
        this->rebuild();
    });
    m_buttonMenu->addChildAtPosition(m_songsItem, Anchor::Center, { -tabGap, tabY });
    m_buttonMenu->addChildAtPosition(m_sfxItem, Anchor::Center, { tabGap, tabY });

    m_pie = CCDrawNode::create();
    m_pie->setContentSize({ 64.f, 64.f });
    m_pie->setAnchorPoint({ .5f, .5f });
    m_mainLayer->addChildAtPosition(m_pie, Anchor::Center, { -width / 2.f + 46.f, infoY });

    auto addLegend = [&](CCLabelBMFont*& label, float y) {
        label = CCLabelBMFont::create("", "chatFont.fnt");
        label->setAnchorPoint({ 0.f, .5f });
        label->setScale(.45f);
        m_mainLayer->addChildAtPosition(label, Anchor::Center, { -width / 2.f + 86.f, y });
    };
    addLegend(m_legendSongs, infoY + 16.f);
    addLegend(m_legendSfx, infoY);
    addLegend(m_legendDisk, infoY - 16.f);
    m_legendSongs->setColor({ 70, 220, 95 });
    m_legendSfx->setColor({ 255, 196, 50 });

    m_ageLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_ageLabel->setAnchorPoint({ 0.f, .5f });
    m_ageLabel->setScale(.5f);
    m_mainLayer->addChildAtPosition(m_ageLabel, Anchor::Center, { -width / 2.f + 14.f, filterY });

    m_daysInput = TextInput::create(46.f, "0", "chatFont.fnt");
    m_daysInput->setCommonFilter(CommonFilter::Uint);
    m_daysInput->setMaxCharCount(4);
    m_daysInput->setString("0");
    m_mainLayer->addChildAtPosition(m_daysInput, Anchor::Center, { -width / 2.f + 128.f, filterY });

    m_daysLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_daysLabel->setAnchorPoint({ 0.f, .5f });
    m_daysLabel->setScale(.5f);
    m_mainLayer->addChildAtPosition(m_daysLabel, Anchor::Center, { -width / 2.f + 158.f, filterY });

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setAnchorPoint({ 1.f, .5f });
    m_status->setScale(.45f);
    m_mainLayer->addChildAtPosition(m_status, Anchor::Center, { width / 2.f - 12.f, filterY });

    auto* background = CCLayerColor::create({ 0, 0, 0, 75 }, listWidth, listHeight);
    background->setAnchorPoint({ 0.f, 0.f });
    m_mainLayer->addChildAtPosition(background, Anchor::Center, { -width / 2.f + 10.f, -top + listBottom });

    m_list = ScrollLayer::create({ listWidth, listHeight });
    m_list->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(3.f));
    m_mainLayer->addChildAtPosition(m_list, Anchor::Center, { -width / 2.f + 10.f, -top + listBottom });

    m_emptyLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_emptyLabel->setScale(.5f);
    m_emptyLabel->setZOrder(5);
    m_mainLayer->addChildAtPosition(m_emptyLabel, Anchor::Center, { 0.f, -top + listBottom + listHeight / 2.f });

    m_markItem = makeButton("SELECT", "GJ_button_04.png", .42f, [this](CCObject* sender) {
        this->onSelectVisible(sender);
    });
    m_clearItem = makeButton("CLEAR", "GJ_button_05.png", .42f, [this](CCObject* sender) {
        this->onClearSelection(sender);
    });
    m_deleteItem = makeButton("DELETE", "GJ_button_06.png", .42f, [this](CCObject* sender) {
        this->onDelete(sender);
    });
    m_buttonMenu->addChildAtPosition(m_markItem, Anchor::Center, { -buttonGap, -top + 22.f });
    m_buttonMenu->addChildAtPosition(m_clearItem, Anchor::Center, { 0.f, -top + 22.f });
    m_buttonMenu->addChildAtPosition(m_deleteItem, Anchor::Center, { buttonGap, -top + 22.f });

    m_daysInput->setCallback([this](std::string const&) {
        this->rebuild();
    });
    this->applyLanguage();
    return true;
}

void AudioLibraryLayer::applyLanguage() {
    this->setTitle(tr("AUDIO LIBRARY", "BIBLIOTECA DE AUDIO"), "goldFont.fnt", .55f, 15.f);
    setCaption(m_langItem, isSpanish() ? "EN" : "ES", "GJ_button_05.png", .38f);
    setCaption(m_songsItem, tr("SONGS", "CANCIONES"), "GJ_button_01.png", .4f);
    setCaption(m_sfxItem, tr("EFFECTS", "EFECTOS"), "GJ_button_01.png", .4f);
    setCaption(m_markItem, tr("SELECT", "MARCAR"), "GJ_button_04.png", .42f);
    setCaption(m_clearItem, tr("CLEAR", "LIMPIAR"), "GJ_button_05.png", .42f);
    setCaption(m_deleteItem, tr("DELETE", "BORRAR"), "GJ_button_06.png", .42f);
    m_ageLabel->setString(tr("Older than", "Mas de"));
    m_daysLabel->setString(tr("days", "dias"));
    m_emptyLabel->setString(tr("Nothing downloaded for this filter", "Nada descargado con este filtro"));
    m_emptyLabel->limitLabelWidth(m_size.width - 48.f, .5f, .25f);
    this->rebuild();
}

void AudioLibraryLayer::onLanguage(CCObject*) {
    Mod::get()->setSavedValue<std::string>("language", isSpanish() ? "en" : "es");
    this->applyLanguage();
}

int AudioLibraryLayer::ageLimitDays() const {
    if (!m_daysInput) return 0;
    auto const text = toStd(m_daysInput->getString());
    if (text.empty()) return 0;
    return std::max(0, std::atoi(text.c_str()));
}

std::unordered_set<int>& AudioLibraryLayer::selection() {
    return m_tab == Tab::Songs ? m_selectedSongs : m_selectedSfx;
}

void AudioLibraryLayer::rebuild() {
    m_visible = m_tab == Tab::Songs ? loadSongs(this->ageLimitDays()) : loadSfx(this->ageLimitDays());
    if (m_songsItem) m_songsItem->setOpacity(m_tab == Tab::Songs ? 255 : 145);
    if (m_sfxItem) m_sfxItem->setOpacity(m_tab == Tab::Sfx ? 255 : 145);

    auto& selected = this->selection();
    std::unordered_set<int> visibleIds;
    for (auto const& entry : m_visible) visibleIds.insert(entry.id);
    for (auto it = selected.begin(); it != selected.end();) {
        if (!visibleIds.contains(*it)) it = selected.erase(it);
        else ++it;
    }

    auto* content = m_list->m_contentLayer;
    content->removeAllChildren();
    float const rowWidth = m_list->getContentWidth();
    m_emptyLabel->setVisible(m_visible.empty());

    for (auto const& entry : m_visible) {
        auto* row = CCNode::create();
        row->setContentSize({ rowWidth, 42.f });

        auto* toggle = CCMenuItemToggler::create(
            CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png"),
            CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png"),
            this,
            menu_selector(AudioLibraryLayer::onToggle)
        );
        toggle->setTag(entry.id);
        toggle->setScale(.62f);
        m_syncingToggles = true;
        toggle->toggle(selected.contains(entry.id));
        m_syncingToggles = false;

        auto* checkMenu = CCMenu::create();
        checkMenu->setContentSize({ 34.f, 42.f });
        checkMenu->ignoreAnchorPointForPosition(false);
        checkMenu->setAnchorPoint({ 0.f, .5f });
        checkMenu->setPosition({ 0.f, 21.f });
        toggle->setPosition({ 18.f, 21.f });
        checkMenu->addChild(toggle);
        row->addChild(checkMenu);

        bool const playing = m_previewing && m_previewSong == entry.id && m_tab == Tab::Songs;
        char const* frame = playing ? "GJ_pauseBtn_001.png" : "GJ_playBtn2_001.png";
        auto* icon = CCSprite::createWithSpriteFrameName(frame);
        if (!icon) icon = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
        if (icon) icon->setScale(.28f);
        int const id = entry.id;
        auto* play = CCMenuItemExt::createSpriteExtra(icon, [this, id](CCObject*) {
            this->togglePlay(id);
        });
        auto* playMenu = CCMenu::create();
        playMenu->setContentSize({ 36.f, 42.f });
        playMenu->ignoreAnchorPointForPosition(false);
        playMenu->setAnchorPoint({ 1.f, .5f });
        playMenu->setPosition({ rowWidth - 2.f, 21.f });
        play->setPosition({ 18.f, 21.f });
        playMenu->addChild(play);
        row->addChild(playMenu);

        auto* name = CCLabelBMFont::create(entry.name.c_str(), "goldFont.fnt");
        name->setAnchorPoint({ 0.f, .5f });
        name->setPosition({ 36.f, 27.f });
        name->limitLabelWidth(rowWidth - 80.f, .4f, .16f);
        row->addChild(name);

        auto* detail = CCLabelBMFont::create(entry.detail.c_str(), "chatFont.fnt");
        detail->setAnchorPoint({ 0.f, .5f });
        detail->setPosition({ 36.f, 12.f });
        detail->limitLabelWidth(rowWidth - 80.f, .4f, .18f);
        row->addChild(detail);

        content->addChild(row);
    }

    content->updateLayout();
    m_list->scrollToTop();
    this->updateStatus();
    this->refreshPie();
}

void AudioLibraryLayer::updateStatus() {
    auto const& selected = this->selection();
    std::uint64_t bytes = 0;
    for (auto const& entry : m_visible) {
        if (selected.contains(entry.id)) bytes += entry.bytes;
    }
    m_status->setString(fmt::format(
        fmt::runtime(tr("{} selected, {}", "{} marcados, {}")),
        selected.size(),
        formatBytes(bytes)
    ).c_str());
    m_status->limitLabelWidth(120.f, .45f, .2f);
}

void AudioLibraryLayer::refreshPie() {
    auto const usage = measureDisk();
    auto const percent = [&](std::uint64_t bytes) {
        if (usage.capacity == 0) return 0.0;
        return 100.0 * static_cast<double>(bytes) / static_cast<double>(usage.capacity);
    };
    m_legendSongs->setString(fmt::format(
        "{} {} ({:.2f}%)",
        tr("Songs", "Canciones"),
        formatBytes(usage.songs),
        percent(usage.songs)
    ).c_str());
    m_legendSfx->setString(fmt::format(
        "{} {} ({:.2f}%)",
        tr("Effects", "Efectos"),
        formatBytes(usage.sfx),
        percent(usage.sfx)
    ).c_str());
    if (usage.capacity == 0) {
        m_legendDisk->setString(tr("Disk unavailable", "Disco no disponible"));
    } else {
        m_legendDisk->setString(fmt::format(
            "{} {} {}",
            tr("Disk", "Disco"),
            formatBytes(usage.free),
            tr("free", "libres")
        ).c_str());
    }
    float const legendWidth = m_size.width - 110.f;
    m_legendSongs->limitLabelWidth(legendWidth, .45f, .2f);
    m_legendSfx->limitLabelWidth(legendWidth, .45f, .2f);
    m_legendDisk->limitLabelWidth(legendWidth, .45f, .2f);

    m_pie->clear();
    auto const center = CCPoint { m_pie->getContentWidth() / 2.f, m_pie->getContentHeight() / 2.f };
    float const radius = 28.f;
    ccColor4F const freeColor { .22f, .24f, .28f, 1.f };
    ccColor4F const otherColor { .48f, .46f, .40f, 1.f };
    ccColor4F const songColor { .30f, .86f, .38f, 1.f };
    ccColor4F const sfxColor { .98f, .78f, .22f, 1.f };

    std::uint64_t const audio = usage.songs + usage.sfx;
    std::uint64_t const used = usage.capacity > usage.free ? usage.capacity - usage.free : 0;
    std::uint64_t other = used > audio ? used - audio : 0;
    std::uint64_t free = usage.free;
    std::uint64_t total = other + free + audio;
    if (total == 0) total = 1;

    auto angleFor = [&](std::uint64_t part) {
        return kPi * 2.f * static_cast<float>(static_cast<double>(part) / static_cast<double>(total));
    };
    float songSweep = angleFor(usage.songs);
    float sfxSweep = angleFor(usage.sfx);
    float otherSweep = angleFor(other);
    float freeSweep = angleFor(free);
    constexpr float kMinSlice = 0.16f;
    float extra = 0.f;
    if (usage.songs > 0 && songSweep < kMinSlice) {
        extra += kMinSlice - songSweep;
        songSweep = kMinSlice;
    }
    if (usage.sfx > 0 && sfxSweep < kMinSlice) {
        extra += kMinSlice - sfxSweep;
        sfxSweep = kMinSlice;
    }
    if (extra > 0.f && freeSweep > extra) freeSweep -= extra;
    else if (extra > 0.f && otherSweep > extra) otherSweep -= extra;

    float cursor = -kPi / 2.f;
    drawSlice(m_pie, center, radius, cursor, songSweep, songColor);
    cursor += songSweep;
    drawSlice(m_pie, center, radius, cursor, sfxSweep, sfxColor);
    cursor += sfxSweep;
    drawSlice(m_pie, center, radius, cursor, otherSweep, otherColor);
    cursor += otherSweep;
    drawSlice(m_pie, center, radius, cursor, freeSweep, freeColor);
}

void AudioLibraryLayer::stopPreview() {
    if (!m_previewing) return;
    if (auto* audio = FMODAudioEngine::sharedEngine()) audio->stopMusic(0);
    if (auto* game = GameManager::sharedState()) game->fadeInMenuMusic();
    m_previewing = false;
    m_previewSong = 0;
}

void AudioLibraryLayer::togglePlay(int id) {
    auto* manager = MusicDownloadManager::sharedState();
    auto* audio = FMODAudioEngine::sharedEngine();
    if (!manager || !audio) return;

    if (m_tab == Tab::Sfx) {
        auto const path = manager->pathForSFX(id);
        if (!toStd(path).empty()) audio->playEffect(path);
        return;
    }

    if (m_previewing && m_previewSong == id) {
        this->stopPreview();
        this->rebuild();
        return;
    }

    this->stopPreview();
    auto const path = manager->pathForSong(id);
    if (toStd(path).empty()) return;
    audio->playMusic(path, false, .15f, 0);
    m_previewing = true;
    m_previewSong = id;
    this->rebuild();
}

void AudioLibraryLayer::onToggle(CCObject* sender) {
    if (m_syncingToggles) return;
    auto* toggle = static_cast<CCMenuItemToggler*>(sender);
    auto& selected = this->selection();
    // Geometry Dash avisa el clic antes de cambiar isToggled().
    bool const nowOn = !toggle->isToggled();
    if (nowOn) selected.insert(toggle->getTag());
    else selected.erase(toggle->getTag());
    this->updateStatus();
}

void AudioLibraryLayer::onSelectVisible(CCObject*) {
    auto& selected = this->selection();
    for (auto const& entry : m_visible) selected.insert(entry.id);
    this->rebuild();
}

void AudioLibraryLayer::onClearSelection(CCObject*) {
    this->selection().clear();
    this->rebuild();
}

void AudioLibraryLayer::onDelete(CCObject*) {
    if (PlayLayer::get()) {
        FLAlertLayer::create(
            tr("Audio library", "Biblioteca"),
            tr("Close the level before deleting audio.", "Cierra el nivel antes de borrar audio."),
            "OK"
        )->show();
        return;
    }

    auto const& selected = this->selection();
    if (selected.empty()) {
        FLAlertLayer::create(
            tr("Audio library", "Biblioteca"),
            tr("Select at least one file.", "Marca al menos un archivo."),
            "OK"
        )->show();
        return;
    }

    std::uint64_t bytes = 0;
    int count = 0;
    for (auto const& entry : m_visible) {
        if (!selected.contains(entry.id)) continue;
        bytes += entry.bytes;
        ++count;
    }

    auto const kind = m_tab == Tab::Songs ? tr("songs", "canciones") : tr("effects", "efectos");
    auto const message = fmt::format(
        fmt::runtime(tr(
            "This will delete {} {} ({}). Original game audio is kept.",
            "Se borraran {} {} ({}). El audio original no se toca."
        )),
        count,
        kind,
        formatBytes(bytes)
    );

    Ref<AudioLibraryLayer> self = this;
    createQuickPopup(
        tr("Delete audio", "Borrar audio"),
        message,
        tr("Cancel", "Cancelar"),
        tr("Delete", "Borrar"),
        [self](FLAlertLayer*, bool confirmed) {
            if (self && confirmed) self->performDelete();
        }
    );
}

void AudioLibraryLayer::performDelete() {
    if (PlayLayer::get()) {
        FLAlertLayer::create(
            tr("Audio library", "Biblioteca"),
            tr("Close the level before deleting audio.", "Cierra el nivel antes de borrar audio."),
            "OK"
        )->show();
        return;
    }

    this->stopPreview();
    auto* manager = MusicDownloadManager::sharedState();
    auto* audio = FMODAudioEngine::sharedEngine();
    if (!manager) return;

    int removed = 0;
    int skipped = 0;
    auto const pending = this->selection();
    for (int const id : pending) {
        bool const visible = std::ranges::any_of(m_visible, [id](Entry const& entry) {
            return entry.id == id;
        });
        if (!visible) continue;

        if (m_tab == Tab::Songs) {
            if (manager->isResourceSong(id) || manager->isRunningActionForSongID(id) || isSongStillAudible(manager, audio, id)) {
                ++skipped;
                continue;
            }
            manager->deleteSong(id);
            m_selectedSongs.erase(id);
        } else {
            if (manager->isResourceSFX(id)) {
                ++skipped;
                continue;
            }
            manager->deleteSFX(id);
            m_selectedSfx.erase(id);
        }
        ++removed;
    }

    this->rebuild();
    if (skipped > 0) {
        FLAlertLayer::create(
            tr("Audio library", "Biblioteca"),
            fmt::format(fmt::runtime(tr("Skipped {} because they were playing or downloading.", "Se omitieron {} porque estaban sonando o en descarga.")), skipped).c_str(),
            "OK"
        )->show();
    } else if (removed == 0) {
        FLAlertLayer::create(
            tr("Audio library", "Biblioteca"),
            tr("Nothing was deleted.", "No se borro ningun archivo."),
            "OK"
        )->show();
    }
}

void AudioLibraryLayer::onClose(CCObject* sender) {
    this->stopPreview();
    Popup::onClose(sender);
}
