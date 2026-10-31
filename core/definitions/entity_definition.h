// What an entity or attachment is (core/src/base/entity.ts EntityDefinition, base/attachment.ts
// AttachmentDefinition). Attachments use the same struct with type Attachment and `modifications` set.
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/definitions/skill_definition.h"
#include "core/definitions/variable_config.h"
#include "core/definitions/version_info.h"
#include "core/state/game_state.h"
#include "core/values/game_enums.h"
#include "core/values/names.h"

namespace gitcg {

enum class EntityType { EventCard, Status, CombatStatus, Equipment, Support, Summon, Attachment };

// "[GCG_TOKEN_...]" -> text, computed from the state for the entity with this id.
using DescriptionEntry = std::function<std::string(const GameStatePointer& state, int id)>;

struct AttachmentModification {
  enum class Kind { IncreaseCardCost, DecreaseCardCost, ChangeCardCostType, ChangeCardTuningTarget, DisableCardTuning, MakeEffectless };
  Kind kind;
  int value = 0;           // IncreaseCardCost / DecreaseCardCost
  DiceType diceType = 0;  // ChangeCardCostType / ChangeCardTuningTarget
};
using ModificationGetter = std::function<std::vector<AttachmentModification>(const GameStatePointer& state, int id)>;

struct EntityDefinition {
  EntityType type = EntityType::Status;
  int id = 0;
  VersionInfo version;
  bool obtainable = true;
  std::optional<Name> visibleVarName;
  std::vector<Name> tags;
  std::optional<std::string> hintText;
  bool disableTuning = false;
  std::vector<std::pair<Name, VariableConfig>> varConfigs;  // insertion ordered like the JS object
  bool disposeWhenUsageIsZero = false;
  bool disposeOnMasterDefeated = false;
  std::vector<const SkillDefinition*> skills;
  std::vector<std::pair<std::string, DescriptionEntry>> descriptionDictionary;
  ModificationGetter modifications;  // attachments only

  bool hasTag(Name tag) const {
    for (Name own : tags) {
      if (own == tag) return true;
    }
    return false;
  }
  const VariableConfig* findVarConfig(Name name) const {
    for (const auto& [key, config] : varConfigs) {
      if (key == name) return &config;
    }
    return nullptr;
  }
};

}  // namespace gitcg
