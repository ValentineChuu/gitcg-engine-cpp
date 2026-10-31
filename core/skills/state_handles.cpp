#include "core/skills/state_handles.h"

#include <stdexcept>

#include "core/mutations/state_mutations.h"
#include "core/rules/character_traits.h"
#include "core/rules/costs.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include "core/values/engine_errors.h"

namespace gitcg {

namespace {

Name variableName(const char* text) {
  return intern(text);
}

}  // namespace

CharacterPointer CharacterHandle::latest() const {
  return getEntityById(context_->state(), id_).character();
}

EntityArea CharacterHandle::area() const {
  return context_->entityArea(id_);
}

bool CharacterHandle::isMine() const {
  return who() == context_->selfWho();
}

int CharacterHandle::positionIndex() const {
  const PlayerState& player = *context_->state().players[who()];
  for (size_t index = 0; index < player.characters.size(); ++index) {
    if (player.characters[index]->id == id_) return static_cast<int>(index);
  }
  throw GameInternalError("Invalid character index");
}

bool CharacterHandle::satisfyPosition(CharacterPosition position) const {
  const PlayerState& player = *context_->state().players[who()];
  bool isActiveCharacter = player.activeCharacterId == id_;
  int step = 0;
  switch (position) {
    case CharacterPosition::Active: return isActiveCharacter;
    case CharacterPosition::Standby: return !isActiveCharacter;
    case CharacterPosition::Next:
      if (isActiveCharacter) return false;
      step = 1;
      break;
    case CharacterPosition::Previous:
      if (isActiveCharacter) return false;
      step = -1;
      break;
  }
  int length = static_cast<int>(player.characters.size());
  int current = activeCharacterIndex(player);
  do {
    current = (current + step + length) % length;
  } while (!isCharacterAlive(*player.characters[current]));
  return player.characters[current]->id == id_;
}

double CharacterHandle::health() const { return getVariable(variableName("health")); }
double CharacterHandle::energy() const { return getVariable(variableName("energy")); }
Aura CharacterHandle::aura() const { return static_cast<Aura>(getVariable(variableName("aura"))); }
double CharacterHandle::maxHealth() const { return getVariable(variableName("maxHealth")); }
double CharacterHandle::maxEnergy() const { return getVariable(variableName("maxEnergy")); }
bool CharacterHandle::alive() const { return getVariable(variableName("alive")) != 0; }
DiceType CharacterHandle::element() const { return elementOfCharacter(definition()); }

Name CharacterHandle::weaponTag() const {
  static const Name WEAPONS[] = {intern("sword"), intern("claymore"), intern("pole"), intern("catalyst"), intern("bow"), intern("otherWeapon")};
  for (Name tag : definition().tags) {
    for (Name weapon : WEAPONS) {
      if (tag == weapon) return weapon;
    }
  }
  return intern("otherWeapon");
}

std::vector<Name> CharacterHandle::nationTags() const {
  static const Name NATIONS[] = {intern("mondstadt"), intern("liyue"),     intern("inazuma"), intern("sumeru"), intern("fontaine"),
                                 intern("natlan"),    intern("nodkrai"),   intern("fatui"),   intern("eremite"), intern("monster"),
                                 intern("hilichurl"), intern("sacread"),   intern("calamity")};
  std::vector<Name> result;
  for (Name tag : definition().tags) {
    for (Name nation : NATIONS) {
      if (tag == nation) result.push_back(tag);
    }
  }
  return result;
}

std::vector<EntityHandle> CharacterHandle::entities() const {
  std::vector<EntityHandle> result;
  for (const EntityPointer& entity : latest()->entities) result.emplace_back(context_, entity->id);
  return result;
}

std::optional<EntityHandle> CharacterHandle::findEntity(bool (*matches)(const EntityDefinition&, int), int parameter) const {
  for (const EntityPointer& entity : latest()->entities) {
    if (matches(*entity->definition, parameter)) return EntityHandle(context_, entity->id);
  }
  return std::nullopt;
}

std::optional<EntityHandle> CharacterHandle::hasArtifact() const {
  return findEntity([](const EntityDefinition& definition, int) {
    return definition.type == EntityType::Equipment && definition.hasTag(intern("artifact"));
  }, 0);
}

std::optional<EntityHandle> CharacterHandle::hasWeapon() const {
  return findEntity([](const EntityDefinition& definition, int) {
    return definition.type == EntityType::Equipment && definition.hasTag(intern("weapon"));
  }, 0);
}

std::optional<EntityHandle> CharacterHandle::hasTechnique() const {
  return findEntity([](const EntityDefinition& definition, int) {
    return definition.type == EntityType::Equipment && definition.hasTag(intern("technique"));
  }, 0);
}

std::optional<EntityHandle> CharacterHandle::hasEquipment(int definitionId) const {
  return findEntity([](const EntityDefinition& definition, int id) { return definition.type == EntityType::Equipment && definition.id == id; },
                    definitionId);
}

std::optional<EntityHandle> CharacterHandle::hasStatus(int definitionId) const {
  return findEntity([](const EntityDefinition& definition, int id) { return definition.type == EntityType::Status && definition.id == id; },
                    definitionId);
}

std::optional<EntityHandle> CharacterHandle::hasNightsoulsBlessing() const {
  return findEntity([](const EntityDefinition& definition, int) {
    return definition.type == EntityType::Status && definition.hasTag(intern("nightsoulsBlessing"));
  }, 0);
}

bool CharacterHandle::isSkillDisabled() const {
  static const Name DISABLE_SKILL = intern("disableSkill");
  for (const EntityPointer& entity : latest()->entities) {
    if (entity->definition->hasTag(DISABLE_SKILL)) return true;
  }
  return false;
}

void CharacterHandle::gainEnergy(double value) const { context_->gainEnergy(value, {AnyHandle(context_, id_, true)}); }
void CharacterHandle::heal(double value) const { context_->heal(value, {AnyHandle(context_, id_, true)}); }
void CharacterHandle::damage(DamageType type, double value) const { context_->damage(type, value, {AnyHandle(context_, id_, true)}); }
void CharacterHandle::apply(DamageType type) const { context_->apply(type, {AnyHandle(context_, id_, true)}); }
void CharacterHandle::addStatus(int statusId) const { context_->createEntity(EntityType::Status, statusId, area()); }
void CharacterHandle::addStatus(int statusId, const CreateEntityOptions& options) const {
  context_->createEntity(EntityType::Status, statusId, area(), options);
}
void CharacterHandle::equip(int equipmentId) const { context_->equip(equipmentId, {AnyHandle(context_, id_, true)}); }

void CharacterHandle::unequipArtifact() const {
  for (const EntityPointer& entity : latest()->entities) {
    if (entity->definition->hasTag(intern("artifact"))) {
      context_->unequip(EntityHandle(context_, entity->id));
      return;
    }
  }
}

void CharacterHandle::unequipWeapon() const {
  for (const EntityPointer& entity : latest()->entities) {
    if (entity->definition->hasTag(intern("weapon"))) {
      context_->unequip(EntityHandle(context_, entity->id));
      return;
    }
  }
}

double CharacterHandle::loseEnergy(double count) const {
  double original = energy();
  double remaining = std::max(0.0, original - count);
  context_->setVariable(variableName("energy"), remaining, AnyHandle(context_, id_, true));
  return original - remaining;
}

void CharacterHandle::setVariable(Name name, double value) const { context_->setVariable(name, value, AnyHandle(context_, id_, true)); }
void CharacterHandle::addVariable(Name name, double value) const { context_->addVariable(name, value, AnyHandle(context_, id_, true)); }

EntityPointer EntityHandle::latest() const {
  return getEntityById(context_->state(), id_).entity();
}

EntityArea EntityHandle::area() const {
  return context_->entityArea(id_);
}

bool EntityHandle::isMine() const {
  return who() == context_->selfWho();
}

int EntityHandle::diceCost() const {
  return diceCostSizeOfCard(context_->rawState(), *latest());
}

bool EntityHandle::withAttachment(int attachmentId) const {
  for (const EntityPointer& attachment : latest()->attachments) {
    if (attachment->definition->id == attachmentId) return true;
  }
  return false;
}

CharacterHandle EntityHandle::master() const {
  EntityArea entityArea = area();
  if (entityArea.type != AreaType::Characters) throw GameDataError("master expect a character area");
  return CharacterHandle(context_, entityArea.characterId);
}

EntityHandle EntityHandle::masterCard() const {
  EntityArea entityArea = area();
  if (entityArea.type != AreaType::Hands && entityArea.type != AreaType::Pile) throw GameDataError("master expect a hands/pile area");
  return EntityHandle(context_, entityArea.cardId);
}

void EntityHandle::setVariable(Name name, double value) const { context_->setVariable(name, value, AnyHandle(context_, id_, false)); }
void EntityHandle::addVariable(Name name, double value) const { context_->addVariable(name, value, AnyHandle(context_, id_, false)); }
void EntityHandle::consumeUsage(double count) const { context_->consumeUsage(count, AnyHandle(context_, id_, false)); }

void EntityHandle::resetUsagePerRound() const {
  resetVariables(context_->mutator_.mutableState(), id_, true);
}

void EntityHandle::dispose() const {
  context_->dispose(std::vector<AnyHandle>{AnyHandle(context_, id_, false)});
}

AnyState AnyHandle::latest() const {
  return getEntityById(context_->state(), id_);
}

}  // namespace gitcg
