// A character or an entity (AnyState in the core): what skills are called from and what queries return.
#pragma once

#include "core/state/character_state.h"
#include "core/state/entity_state.h"

namespace gitcg {

class AnyState {
 public:
  AnyState() = default;
  AnyState(CharacterPointer character) : character_(std::move(character)) {}
  AnyState(EntityPointer entity) : entity_(std::move(entity)) {}

  bool isCharacter() const { return static_cast<bool>(character_); }
  bool isEntity() const { return static_cast<bool>(entity_); }
  bool empty() const { return !character_ && !entity_; }
  int id() const { return character_ ? character_->id : entity_->id; }
  const Variables& variables() const { return character_ ? character_->variables : entity_->variables; }

  const CharacterPointer& character() const { return character_; }
  const EntityPointer& entity() const { return entity_; }

 private:
  CharacterPointer character_;
  EntityPointer entity_;
};

}  // namespace gitcg
