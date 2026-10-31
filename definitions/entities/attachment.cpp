// Attachment modifications are evaluated lazily against the current state when card costs are read.
#include "definitions/entities/builtins.h"
#include <stdexcept>

namespace gitcg {
void addAttachmentModification(EntityModel& model, const Attribute& attribute) {
  AttachmentModification modification;
  auto& name = attribute.name;
  if (name == "addCost") modification.kind = AttachmentModification::Kind::IncreaseCardCost;
  else if (name == "deductCost") modification.kind = AttachmentModification::Kind::DecreaseCardCost;
  else if (name == "changeCostType") modification.kind = AttachmentModification::Kind::ChangeCardCostType;
  else if (name == "changeTuningTarget") modification.kind = AttachmentModification::Kind::ChangeCardTuningTarget;
  else if (name == "disableTuning") modification.kind = AttachmentModification::Kind::DisableCardTuning;
  else if (name == "makeEffectless") modification.kind = AttachmentModification::Kind::MakeEffectless;
  else throw std::runtime_error("Unknown attachment attribute " + name);
  int slot = lambdaSlot(attribute.argument(0));
  if (!slot && attribute.argument(0).isNumber()) {
    if (name == "addCost" || name == "deductCost") modification.value = attribute.argument(0).asInt();
    else modification.diceType = attribute.argument(0).asInt();
  }
  auto previous = model.definition.modifications; auto slots = model.storage.slots;
  model.definition.modifications = [previous, slots, slot, modification](const GameStatePointer& state, int id) mutable {
    auto result = previous ? previous(state, id) : std::vector<AttachmentModification>{};
    auto value = modification;
    if (slot) value.value = slots->call(slot, {nullptr, state, id, nullptr, nullptr}).asInt();
    result.push_back(value);
    return result;
  };
}
}  // namespace gitcg
