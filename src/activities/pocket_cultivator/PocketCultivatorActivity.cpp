#include "PocketCultivatorActivity.h"

#include <GfxRenderer.h>
#include <Logging.h>
#include <Memory.h>

#include <cstdio>
#include <memory>
#include <utility>

#include "ActivityManager.h"
#include "MappedInputManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "activities/util/FullScreenMessageActivity.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

PocketCultivatorActivity::PocketCultivatorActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : UiListActivity("PocketCultivator", renderer, mappedInput) {}

int PocketCultivatorActivity::listCount() const {
  return POCKET_CULTIVATOR.hasCharacter() ? MAX_ROWS : 1;
}

void PocketCultivatorActivity::onEnter() {
  // Keep the first scheduled paint from observing the store before it has
  // loaded. This mirrors the data-publication pattern used by LibraryListActivity.
  RenderLock lock(*this);
  UiListActivity::onEnter();
  if (!storeLoaded) {
    POCKET_CULTIVATOR.loadFromFile();
    storeLoaded = true;
  }
  requestUpdate();
}

void PocketCultivatorActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  screen.setContentMarginFromScreen(fui::Insets{static_cast<int16_t>(metrics.topPadding + metrics.headerHeight), 0,
                                                static_cast<int16_t>(metrics.buttonHintsHeight), 0});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  const auto& character = POCKET_CULTIVATOR.character();
  if (!POCKET_CULTIVATOR.hasCharacter()) {
    rows[0] = {};
    rows[0].label = "Establish your sect";
    rows[0].value = "Begin";
    rows[0].actionValue = 0;
  } else {
    rows[0] = {};
    rows[0].label = character.name.c_str();
    rows[0].value = character.sectName.c_str();
    rows[0].actionValue = 0;

    std::snprintf(attributeSummary.data(), attributeSummary.size(), "STR %u  CON %u  SPI %u  COM %u  LCK %u",
                  static_cast<unsigned>(character.strength), static_cast<unsigned>(character.constitution),
                  static_cast<unsigned>(character.spirit), static_cast<unsigned>(character.comprehension),
                  static_cast<unsigned>(character.luck));
    rows[1] = {};
    rows[1].label = "Cultivation Realm";
    rows[1].value = "Body Tempering · 1";
    rows[1].actionValue = 1;

    rows[2] = {};
    rows[2].label = "Natural attributes";
    rows[2].value = attributeSummary.data();
    rows[2].actionValue = 2;

    static constexpr const char* CONSTITUTIONS[] = {"Mortal", "Spiritual", "Profound",
                                                      "Heavenly", "Immortal", "Divine"};
    rows[3] = {};
    rows[3].label = "Constitution";
    rows[3].value = CONSTITUTIONS[character.constitutionRarity <= 5 ? character.constitutionRarity : 0];
    rows[3].actionValue = 3;

    rows[4] = {};
    rows[4].label = "Personal Library";
    rows[4].value = "Open books";
    rows[4].actionValue = 4;

    rows[5] = {};
    rows[5].label = "Settings";
    rows[5].value = "Device settings";
    rows[5].actionValue = 5;
  }

  fui::ListProps props;
  props.items = rows.data();
  props.count = static_cast<uint16_t>(listCount());
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;
  props.valueInset = 8;
  syncListViewport(screen, props);
  screen.list(props);
}

void PocketCultivatorActivity::activateIndex(const int index) {
  nav.selected = index;
  app.clearTapFlash();

  if (!POCKET_CULTIVATOR.hasCharacter()) {
    if (index == 0) beginFoundingCharacter();
    return;
  }

  switch (index) {
    case 4:
      activityManager.goToLibrary();
      return;
    case 5:
      activityManager.goToSettings();
      return;
    default:
      // Character detail and cultivation actions are introduced in the next
      // gameplay slice; keep the first milestone deliberately read-only here.
      return;
  }
}

void PocketCultivatorActivity::beginFoundingCharacter() {
  auto activity = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "First Patriarch", "",
                                                           static_cast<size_t>(32), InputType::Text);
  if (!activity) {
    LOG_ERR("POCKET", "OOM: founder-name keyboard");
    return;
  }
  startActivityForResult(std::move(activity), [this](const ActivityResult& result) {
    if (result.isCancelled) return;
    const auto* entered = std::get_if<KeyboardResult>(&result.data);
    if (!entered || entered->text.empty()) return;
    beginSectNaming(entered->text);
  });
}

void PocketCultivatorActivity::beginSectNaming(const std::string& characterName) {
  pendingFounderName = characterName;
  auto activity = makeUniqueNoThrow<KeyboardEntryActivity>(renderer, mappedInput, "Founding Sect", "",
                                                           static_cast<size_t>(32), InputType::Text);
  if (!activity) {
    LOG_ERR("POCKET", "OOM: sect-name keyboard");
    pendingFounderName.clear();
    return;
  }
  startActivityForResult(std::move(activity), [this](const ActivityResult& result) {
    if (result.isCancelled) {
      pendingFounderName.clear();
      return;
    }
    const auto* entered = std::get_if<KeyboardResult>(&result.data);
    if (!entered || entered->text.empty()) {
      pendingFounderName.clear();
      return;
    }
    finishFoundingCharacter(entered->text);
  });
}

void PocketCultivatorActivity::finishFoundingCharacter(const std::string& sectName) {
  bool created = false;
  {
    // Publish the new character while the render task is excluded; the store
    // write is a one-time operation during first-run setup.
    RenderLock lock(*this);
    created = POCKET_CULTIVATOR.createFoundingCharacter(pendingFounderName, sectName);
  }
  pendingFounderName.clear();
  if (!created) {
    LOG_ERR("POCKET", "Failed to save founding character");
    return;
  }
  nav.reset();
  requestUpdate();
}
