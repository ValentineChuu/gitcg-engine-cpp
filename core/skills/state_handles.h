// Live views of characters and entities for card code (core/src/runtime/reactive): a handle is (context, id) and
// every read looks up the latest state by id, so card code sees its own changes immediately.
#pragma once

#include <optional>
#include <vector>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/state/any_state.h"
#include "core/state/entity_area.h"

namespace gitcg {

class SkillContext;
class EntityHandle;
struct CreateEntityOptions;

enum class CharacterPosition { Active, Next, Previous, Standby };

class CharacterHandle {
 public:
  CharacterHandle(SkillContext* context, int characterId) : context_(context), id_(characterId) {}

  int id() const { return id_; }
  CharacterPointer latest() const;
  const CharacterDefinition& definition() const { return *latest()->definition; }
  const Variables& variables() const { return latest()->variables; }
  double getVariable(Name name) const { return latest()->variables.valueOr(name, 0); }
  EntityArea area() const;
  int who() const { return area().who; }
  bool isMine() const;
  int positionIndex() const;
  bool satisfyPosition(CharacterPosition position) const;
  bool isActive() const { return satisfyPosition(CharacterPosition::Active); }
  double health() const;
  double energy() const;
  Aura aura() const;
  double maxHealth() const;
  double maxEnergy() const;
  bool fullEnergy() const { return energy() == maxEnergy(); }
  bool alive() const;
  DiceType element() const;
  Name weaponTag() const;
  std::vector<Name> nationTags() const;
  std::vector<EntityHandle> entities() const;
  std::optional<EntityHandle> hasArtifact() const;
  std::optional<EntityHandle> hasWeapon() const;
  std::optional<EntityHandle> hasTechnique() const;
  std::optional<EntityHandle> hasEquipment(int definitionId) const;
  std::optional<EntityHandle> hasStatus(int definitionId) const;
  std::optional<EntityHandle> hasNightsoulsBlessing() const;
  bool isSkillDisabled() const;

  // Writes (through the context).
  void gainEnergy(double value = 1) const;
  void heal(double value) const;
  void damage(DamageType type, double value) const;
  void apply(DamageType type) const;
  void addStatus(int statusId) const;
  void addStatus(int statusId, const CreateEntityOptions& options) const;
  void equip(int equipmentId) const;
  void unequipArtifact() const;
  void unequipWeapon() const;
  double loseEnergy(double count = 1) const;
  void setVariable(Name name, double value) const;
  void addVariable(Name name, double value) const;

  bool operator==(const CharacterHandle& other) const { return id_ == other.id_; }

 private:
  std::optional<EntityHandle> findEntity(bool (*matches)(const EntityDefinition&, int), int parameter) const;
  SkillContext* context_;
  int id_;
};

class EntityHandle {
 public:
  EntityHandle(SkillContext* context, int entityId) : context_(context), id_(entityId) {}

  int id() const { return id_; }
  EntityPointer latest() const;
  const EntityDefinition& definition() const { return *latest()->definition; }
  const Variables& variables() const { return latest()->variables; }
  double getVariable(Name name) const { return latest()->variables.valueOr(name, 0); }
  const std::vector<EntityPointer>& attachments() const { return latest()->attachments; }
  EntityArea area() const;
  int who() const { return area().who; }
  bool isMine() const;
  int diceCost() const;
  bool withAttachment(int attachmentId) const;
  bool empowered() const { return withAttachment(206); }
  // The character carrying this status or equipment; for an attachment, the card carrying it.
  CharacterHandle master() const;
  EntityHandle masterCard() const;

  // Writes (through the context).
  void setVariable(Name name, double value) const;
  void addVariable(Name name, double value) const;
  void consumeUsage(double count = 1) const;
  void resetUsagePerRound() const;
  void dispose() const;

  bool operator==(const EntityHandle& other) const { return id_ == other.id_; }

 private:
  SkillContext* context_;
  int id_;
};

// A character or an entity, as queries return them.
class AnyHandle {
 public:
  AnyHandle(SkillContext* context, int handleId, bool character) : context_(context), id_(handleId), character_(character) {}
  int id() const { return id_; }
  bool isCharacter() const { return character_; }
  CharacterHandle asCharacter() const { return CharacterHandle(context_, id_); }
  EntityHandle asEntity() const { return EntityHandle(context_, id_); }
  AnyState latest() const;

 private:
  SkillContext* context_;
  int id_;
  bool character_;
};

}  // namespace gitcg
