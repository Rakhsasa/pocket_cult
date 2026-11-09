#include "PocketCultivatorStore.h"

#include <esp_system.h>

#include <algorithm>
#include <cstring>

namespace {
uint32_t randomBelow(const uint32_t bound) {
  uint32_t value = 0;
  if (bound == 0) return 0;
  esp_fill_random(&value, sizeof(value));
  return value % bound;
}

uint16_t rollNaturalStat() {
  // Prototype range: modest birth-to-birth variation, with long-term growth
  // supplied later by Ancestral Reincarnation.
  return static_cast<uint16_t>(8 + randomBelow(8));
}

uint8_t rollConstitutionRarity() {
  // 0 Mortal, 1 Spiritual, 2 Profound, 3 Heavenly, 4 Immortal, 5 Divine.
  // Distribution is the currently proposed 50/30/12/6/1.8/0.2 table.
  const uint32_t roll = randomBelow(10000);
  if (roll < 5000) return 0;
  if (roll < 8000) return 1;
  if (roll < 9200) return 2;
  if (roll < 9800) return 3;
  if (roll < 9980) return 4;
  return 5;
}
}  // namespace

void PocketCultivatorStore::toJson(JsonDocument& doc) const {
  doc["schemaVersion"] = 1;
  doc["characterCreated"] = characterCreated;
  if (!characterCreated) return;

  JsonObject character = doc["character"].to<JsonObject>();
  character["name"] = activeCharacter.name;
  character["sectName"] = activeCharacter.sectName;
  character["generation"] = activeCharacter.generation;
  character["strength"] = activeCharacter.strength;
  character["constitution"] = activeCharacter.constitution;
  character["spirit"] = activeCharacter.spirit;
  character["comprehension"] = activeCharacter.comprehension;
  character["luck"] = activeCharacter.luck;
  character["constitutionRarity"] = activeCharacter.constitutionRarity;
}

bool PocketCultivatorStore::fromJson(JsonVariantConst doc) {
  const uint8_t version = doc["schemaVersion"] | uint8_t{0};
  if (version != 1) return false;

  characterCreated = doc["characterCreated"] | false;
  if (!characterCreated) {
    activeCharacter = {};
    return true;
  }

  const JsonVariantConst saved = doc["character"];
  if (saved.isNull()) return false;

  activeCharacter.name = saved["name"] | "";
  activeCharacter.sectName = saved["sectName"] | "";
  activeCharacter.generation = saved["generation"] | uint16_t{1};
  activeCharacter.strength = saved["strength"] | uint16_t{0};
  activeCharacter.constitution = saved["constitution"] | uint16_t{0};
  activeCharacter.spirit = saved["spirit"] | uint16_t{0};
  activeCharacter.comprehension = saved["comprehension"] | uint16_t{0};
  activeCharacter.luck = saved["luck"] | uint16_t{0};
  activeCharacter.constitutionRarity = saved["constitutionRarity"] | uint8_t{0};

  if (activeCharacter.name.empty() || activeCharacter.sectName.empty() ||
      activeCharacter.generation == 0 || activeCharacter.constitutionRarity > 5) {
    characterCreated = false;
    activeCharacter = {};
    return false;
  }
  return true;
}

bool PocketCultivatorStore::createFoundingCharacter(const std::string& name, const std::string& sectName) {
  if (name.empty() || sectName.empty() || name.size() > 32 || sectName.size() > 32) return false;

  PocketCultivatorCharacter candidate;
  candidate.name = name;
  candidate.sectName = sectName;
  candidate.generation = 1;
  candidate.strength = rollNaturalStat();
  candidate.constitution = rollNaturalStat();
  candidate.spirit = rollNaturalStat();
  candidate.comprehension = rollNaturalStat();
  candidate.luck = rollNaturalStat();
  candidate.constitutionRarity = rollConstitutionRarity();

  activeCharacter = std::move(candidate);
  characterCreated = true;
  if (saveToFile()) return true;

  // Do not leave an unsaved character looking like a successful creation.
  activeCharacter = {};
  characterCreated = false;
  return false;
}
