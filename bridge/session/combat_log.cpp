#include "bridge/session/combat_log.h"

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/events/damage_event_args.h"
#include "core/events/entity_event_args.h"

namespace gitcg {

namespace {

const char* callerTypeText(const AnyState& caller) {
  if (caller.empty()) return "unknown";
  if (caller.isCharacter()) return "character";
  switch (caller.entity()->definition->type) {
    case EntityType::EventCard: return "eventCard";
    case EntityType::Status: return "status";
    case EntityType::CombatStatus: return "combatStatus";
    case EntityType::Equipment: return "equipment";
    case EntityType::Support: return "support";
    case EntityType::Summon: return "summon";
    case EntityType::Attachment: return "attachment";
  }
  return "unknown";
}

const char* skillTypeText(SkillType type) {
  switch (type) {
    case SkillType::Normal: return "normal";
    case SkillType::Elemental: return "elemental";
    case SkillType::Burst: return "burst";
    case SkillType::Technique: return "technique";
    case SkillType::PlayCard: return "playCard";
    case SkillType::None: return "-";
  }
  return "-";
}

}  // namespace

void CombatLog::scan(const GameStatePointer& state) {
  for (int who = 0; who < 2; ++who) {
    const PlayerState& player = *state->players[who];
    for (const auto& reactionEvent : player.phaseReactionLog) {
      if (!seen_.insert(std::static_pointer_cast<const void>(reactionEvent)).second) continue;
      const ReactionInfo& info = reactionEvent->reactionInfo;
      ++reactions_[who][info.type];
      std::string cause = std::to_string(info.type) + "|" + (info.via ? callerTypeText(info.via->caller) : "unknown") + "|" +
                          (info.via && info.via->definition ? skillTypeText(info.via->definition->skillType) : "-");
      bool found = false;
      for (auto& [key, count] : reactionCauses_[who]) {
        if (key == cause) {
          ++count;
          found = true;
        }
      }
      if (!found) reactionCauses_[who].emplace_back(cause, 1);
    }
    for (const auto& damageEvent : player.phaseDamageLog) {
      if (!seen_.insert(std::static_pointer_cast<const void>(damageEvent)).second) continue;
      if (!damageEvent->isDamageTypeHeal()) damageDealt_[who] += damageEvent->value();
    }
  }
}

JsonValue CombatLog::reactionsJson() const {
  JsonValue result = JsonValue::array();
  for (const auto& counts : reactions_) {
    JsonValue player = JsonValue::object();
    for (const auto& [code, count] : counts) player[std::to_string(code)] = count;
    result.push(std::move(player));
  }
  return result;
}

JsonValue CombatLog::reactionCausesJson() const {
  JsonValue result = JsonValue::array();
  for (const auto& causes : reactionCauses_) {
    JsonValue player = JsonValue::object();
    for (const auto& [cause, count] : causes) player[cause] = count;
    result.push(std::move(player));
  }
  return result;
}

JsonValue CombatLog::damageDealtJson() const {
  JsonValue result = JsonValue::array();
  result.push(damageDealt_[0]);
  result.push(damageDealt_[1]);
  return result;
}

}  // namespace gitcg
