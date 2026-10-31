// SkillContext: operations on characters (switching, energy, healing, damage, elements, nightsoul).

#include <algorithm>

#include "core/definitions/character_definition.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include "core/values/engine_errors.h"
#include "query/builder.h"

namespace gitcg {

namespace {

const Name& nameEnergy() { static const Name name = intern("energy"); return name; }
const Name& nameMaxEnergy() { static const Name name = intern("maxEnergy"); return name; }
const Name& nameMaxHealth() { static const Name name = intern("maxHealth"); return name; }
const Name& nameAura() { static const Name name = intern("aura"); return name; }
const Name& nameNightsoul() { static const Name name = intern("nightsoul"); return name; }

}  // namespace

std::vector<CharacterHandle> SkillContext::charactersOf(const std::vector<AnyHandle>& targets) {
  std::vector<CharacterHandle> characters;
  for (const AnyHandle& target : targets) {
    if (!target.isCharacter()) throw GameDataError("Expected character target, but found a noncharacter entity");
    characters.push_back(target.asCharacter());
  }
  return characters;
}

std::vector<Reaction> SkillContext::enabledLunarReactions(int targetWho) {
  std::vector<Reaction> result;
  for (const CharacterPointer& character : state().players[1 - targetWho]->characters) {
    const auto& enabled = character->definition->enabledLunarReactions;
    result.insert(result.end(), enabled.begin(), enabled.end());
  }
  return result;
}

void SkillContext::switchActive(const std::vector<AnyHandle>& targets) {
  std::vector<CharacterHandle> characters = charactersOf(targets);
  if (characters.empty()) return;
  if (characters.size() > 1) throw GameDataError("Expected exactly one target when switching active");
  SwitchActiveOptions options;
  options.via = sharedSkillInfo();
  options.fromReaction = fromReaction;
  callAndEmit(mutator_.switchActive(characters[0].who(), characters[0].latest(), options));
}

void SkillContext::gainEnergy(double value, const std::vector<AnyHandle>& targets) {
  for (const CharacterHandle& character : charactersOf(targets)) {
    CharacterPointer target = character.latest();
    double energy = target->variables.valueOr(nameEnergy(), 0);
    double maxEnergy = target->variables.valueOr(nameMaxEnergy(), 0);
    double gained = std::min(value, maxEnergy - energy);
    modifyEntityVariable(mutator_.mutableState(), target->id, nameEnergy(), energy + gained, 1);
  }
}

void SkillContext::heal(double value, const std::vector<AnyHandle>& targets, HealKind kind) {
  for (const CharacterHandle& character : charactersOf(targets)) {
    EventsAndDefeat result = mutator_.heal(value, character.latest(), sharedSkillInfo(), kind);
    callAndEmit(result.events);
    causeDefeated_ = causeDefeated_ || result.causeDefeated;
  }
}

void SkillContext::immune(double newHealth) {
  auto* zeroHealth = dynamic_cast<ZeroHealthEventArg*>(eventArgument_);
  if (!zeroHealth) throw GameDataError("The .immune() must be called in .on(\"beforeDefeated\")");
  CharacterPointer target = CharacterHandle(this, zeroHealth->damageInfo().target->id).latest();
  EventsAndDefeat result = mutator_.heal(newHealth, target, sharedSkillInfo(), HealKind::ImmuneDefeated);
  callAndEmit(result.events);
  zeroHealth->markImmune();
}

void SkillContext::increaseMaxHealth(double value, const std::vector<AnyHandle>& targets, bool alsoHeal) {
  for (const CharacterHandle& character : charactersOf(targets)) {
    CharacterPointer target = character.latest();
    modifyEntityVariable(mutator_.mutableState(), target->id, nameMaxHealth(), target->variables.valueOr(nameMaxHealth(), 0) + value);
    if (alsoHeal) {
      EventsAndDefeat result = mutator_.heal(value, character.latest(), sharedSkillInfo(), HealKind::IncreaseMaxHealth);
      callAndEmit(result.events);
    }
  }
}

void SkillContext::damage(DamageType type, double value) {
  // The JS default target $.opp.active: empty while the opponent's active character is defeated.
  damage(type, value, queryAll(query::dollar().opp().active()));
}

void SkillContext::damage(DamageType type, double value, const std::vector<AnyHandle>& targets) {
  if (type == damage::Heal) {
    heal(value, targets);
    return;
  }
  for (const CharacterHandle& character : charactersOf(targets)) {
    bool isSkillMainDamage = isCharacterInitiativeSkill(skillInfo_, true) && fromReaction == reaction::None && !mainDamage_ &&
                             type != damage::Piercing;
    CharacterPointer target = character.latest();
    DamageInfo info;
    info.source = skillInfo_.caller;
    info.target = target;
    info.targetAura = character.aura();
    info.type = type;
    info.value = value;
    info.via = sharedSkillInfo();
    info.isSkillMainDamage = isSkillMainDamage;
    info.causeDefeated = character.alive() && character.health() <= value;
    info.fromReaction = fromReaction;
    DamageOption option;
    option.via = sharedSkillInfo();
    option.callerWho = selfWho();
    option.targetWho = character.who();
    option.targetIsActive = character.isActive();
    option.enabledLunarReactions = enabledLunarReactions(character.who());
    DamageResult result = mutator_.damage(info, option);
    callAndEmit(result.events);
    if (isSkillMainDamage) mainDamage_ = std::make_shared<DamageInfo>(result.damageInfo);
    causeDefeated_ = causeDefeated_ || result.causeDefeated;
  }
}

void SkillContext::apply(DamageType type, const std::vector<AnyHandle>& targets) {
  for (const CharacterHandle& character : charactersOf(targets)) {
    DamageOption option;
    option.via = sharedSkillInfo();
    option.callerWho = selfWho();
    option.targetWho = character.who();
    option.targetIsActive = character.isActive();
    option.enabledLunarReactions = enabledLunarReactions(character.who());
    EventsAndDefeat result = mutator_.apply(character.latest(), type, option, nullptr);
    callAndEmit(result.events);
    causeDefeated_ = causeDefeated_ || result.causeDefeated;
  }
}

void SkillContext::cleanAura(Aura what, const std::vector<AnyHandle>& targets) {
  if (what == aura::None) throw GameDataError("Invalid: cleaning Aura.None");
  for (const CharacterHandle& character : charactersOf(targets)) {
    Aura current = character.aura();
    Aura newAura = current;
    if (what < 0 || current == what) {
      newAura = aura::None;
    } else if (current == aura::CryoDendro) {
      if (what == aura::Cryo) newAura = aura::Dendro;
      if (what == aura::Dendro) newAura = aura::Cryo;
    }
    modifyEntityVariable(mutator_.mutableState(), character.id(), nameAura(), newAura);
  }
}

void SkillContext::swapCharacterPosition(const std::vector<AnyHandle>& first, const std::vector<AnyHandle>& second) {
  std::vector<CharacterHandle> firstCharacters = charactersOf(first);
  std::vector<CharacterHandle> secondCharacters = charactersOf(second);
  if (firstCharacters.size() != 1 || secondCharacters.size() != 1) throw GameDataError("Expected exactly one target for swapping character");
  if (firstCharacters[0].who() != secondCharacters[0].who()) throw GameDataError("Cannot swap characters of different players");
  gitcg::swapCharacterPosition(mutator_.mutableState(), firstCharacters[0].who(), firstCharacters[0].id(), secondCharacters[0].id());
}

void SkillContext::consumeNightsoul(const std::vector<AnyHandle>& targets, double count) {
  for (const CharacterHandle& character : charactersOf(targets)) {
    std::optional<EntityHandle> status = character.hasNightsoulsBlessing();
    if (!status) continue;
    AnyHandle statusHandle(this, status->id(), false);
    double oldValue = getVariable(nameNightsoul(), statusHandle);
    setVariable(nameNightsoul(), std::max(0.0, oldValue - count), statusHandle);
  }
}

void SkillContext::gainNightsoul(const std::vector<AnyHandle>& targets, double count) {
  for (const CharacterHandle& character : charactersOf(targets)) {
    const EntityDefinition* blessing = character.definition().associatedNightsoulsBlessing;
    if (!blessing) continue;
    std::optional<EntityHandle> status = character.hasNightsoulsBlessing();
    if (!status) {
      CreateEntityOptions options;
      options.modifyOverriddenVariablesOnly = true;
      options.overrideVariables = {{nameNightsoul(), 0}};
      status = createEntity(EntityType::Status, blessing->id, character.area(), options);
      if (!status) continue;
    }
    addVariable(nameNightsoul(), count, AnyHandle(this, status->id(), false));
  }
}

}  // namespace gitcg
