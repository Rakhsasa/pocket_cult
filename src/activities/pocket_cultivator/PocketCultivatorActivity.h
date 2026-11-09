#pragma once

#include <array>
#include <string>

#include "PocketCultivatorStore.h"
#include "activities/UiListActivity.h"

class PocketCultivatorActivity final : public UiListActivity {
 public:
  explicit PocketCultivatorActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  bool isHomeActivity() const override { return true; }

 private:
  static constexpr int MAX_ROWS = 6;
  int listCount() const override;
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  const char* headerTitle() const override { return "Pocket Cultivator"; }
  void onBackButton() override {}

  void beginFoundingCharacter();
  void beginSectNaming(const std::string& characterName);
  void finishFoundingCharacter(const std::string& sectName);

  std::array<freeink::ui::ListItem, MAX_ROWS> rows{};
  std::array<char, 64> attributeSummary{};
  std::string pendingFounderName;
  bool storeLoaded = false;
};
