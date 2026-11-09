#pragma once

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <cstdint>
#include <string>

struct PocketCultivatorCharacter {
  std::string name;
  std::string sectName;
  uint16_t generation = 1;
  uint16_t strength = 0;
  uint16_t constitution = 0;
  uint16_t spirit = 0;
  uint16_t comprehension = 0;
  uint16_t luck = 0;
  uint8_t constitutionRarity = 0;
};

// First milestone save: one active character, deliberately separate from
// CrossPointState. World/sect history and transactional reincarnation will
// receive their own versioned stores in later milestones.
class PocketCultivatorStore : public PersistableStore<PocketCultivatorStore> {
  PocketCultivatorStore() = default;
  friend class PersistableStore<PocketCultivatorStore>;

 public:
  static const char* getFilePath() { return "/.crosspoint/pocket_cult_character.json"; }

  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);

  bool hasCharacter() const { return characterCreated; }
  const PocketCultivatorCharacter& character() const { return activeCharacter; }

  // Creates and persists the founding character. The natural-stat and
  // constitution ranges are prototype balance values, not final game tuning.
  bool createFoundingCharacter(const std::string& name, const std::string& sectName);

 private:
  bool characterCreated = false;
  PocketCultivatorCharacter activeCharacter;
};

#define POCKET_CULTIVATOR PocketCultivatorStore::getInstance()
