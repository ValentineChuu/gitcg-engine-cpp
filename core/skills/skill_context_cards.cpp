// SkillContext: dice, hand cards, piles and discarding.

#include <algorithm>
#include <map>

#include "core/definitions/entity_definition.h"
#include "core/definitions/game_data.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/costs.h"
#include "core/rules/dice_order.h"
#include "core/rules/entity_lookup.h"
#include "core/skills/skill_context.h"
#include "core/values/engine_errors.h"

namespace gitcg {

std::vector<DiceType> SkillContext::absorbDice(bool different, int count) {
  const std::vector<DiceType>& dice = player().dice;
  std::map<DiceType, int> counts;
  for (DiceType type : dice) ++counts[type];
  // Absorb order: omni last, more numerous first, then dice index.
  std::vector<DiceType> sorted = dice;
  std::stable_sort(sorted.begin(), sorted.end(), [&](DiceType first, DiceType second) {
    auto key = [&](DiceType type) { return std::array<int, 3>{type == dice::Omni ? 1 : 0, -counts[type], diceIndex(type)}; };
    return key(first) < key(second);
  });
  int who = selfWho();
  if (!different) {
    size_t taken = std::min<size_t>(count, sorted.size());
    std::vector<DiceType> absorbed(sorted.begin(), sorted.begin() + taken);
    resetDice(mutator_.mutableState(), who, std::vector<DiceType>(sorted.begin() + taken, sorted.end()), DiceReason::Absorb);
    return absorbed;
  }
  std::vector<DiceType> collected;
  std::vector<DiceType> left = sorted;
  for (int round = 0; round < count; ++round) {
    bool found = false;
    for (size_t index = 0; index < left.size(); ++index) {
      if (left[index] == dice::Omni || std::find(collected.begin(), collected.end(), left[index]) == collected.end()) {
        collected.push_back(left[index]);
        left.erase(left.begin() + index);
        found = true;
        break;
      }
    }
    if (!found) break;
  }
  resetDice(mutator_.mutableState(), who, left, DiceReason::Absorb);
  return collected;
}

void SkillContext::convertDice(DiceType target, int count, bool opponent) {
  int who = opponent ? 1 - selfWho() : selfWho();
  std::vector<DiceType> finalDice = computeConvertDice(*state().players[who], target, count);
  resetDice(mutator_.mutableState(), who, finalDice, DiceReason::Convert, target);
}

void SkillContext::generateDice(DiceType type, int count) {
  int room = state().config.maxDiceCount - static_cast<int>(player().dice.size());
  count = std::min(count, room);
  std::vector<DiceType> inserted(std::max(count, 0), type);
  int who = selfWho();
  std::vector<DiceType> newDice = player().dice;
  newDice.insert(newDice.end(), inserted.begin(), inserted.end());
  resetDice(mutator_.mutableState(), who, sortDice(player(), newDice), DiceReason::Generate);
  for (DiceType generated : inserted) {
    emitEvent(EventName::OnGenerateDice, std::make_shared<GenerateDiceEventArg>(rawState(), who, sharedSkillInfo(), generated));
  }
}

void SkillContext::generateRandomElementDice(int count, bool includeOmni, bool allowDuplicate) {
  int room = state().config.maxDiceCount - static_cast<int>(player().dice.size());
  count = std::min(count, room);
  std::vector<DiceType> types = {dice::Anemo, dice::Cryo, dice::Dendro, dice::Electro, dice::Geo, dice::Hydro, dice::Pyro};
  if (includeOmni) types.push_back(dice::Omni);
  std::vector<DiceType> inserted;
  for (int index = 0; index < count; ++index) {
    DiceType generated = random(types);
    inserted.push_back(generated);
    if (!allowDuplicate) types.erase(std::find(types.begin(), types.end(), generated));
  }
  int who = selfWho();
  std::vector<DiceType> newDice = player().dice;
  newDice.insert(newDice.end(), inserted.begin(), inserted.end());
  resetDice(mutator_.mutableState(), who, sortDice(player(), newDice), DiceReason::Generate);
  for (DiceType generated : inserted) {
    emitEvent(EventName::OnGenerateDice, std::make_shared<GenerateDiceEventArg>(rawState(), who, sharedSkillInfo(), generated));
  }
}

std::optional<EntityHandle> SkillContext::createHandCard(int cardId, bool opponent) {
  int who = opponent ? 1 - selfWho() : selfWho();
  const EntityDefinition* definition = state().data->entities.find(cardId);
  if (!definition) throw GameDataError("Unknown card definition id " + std::to_string(cardId));
  if (static_cast<int>(state().players[who]->hands.size()) >= state().config.maxHandsCount) return std::nullopt;
  Mutator::CreatedHandCard created = mutator_.createHandCard(who, definition);
  callAndEmit(created.events);
  return EntityHandle(this, created.state->id);
}

void SkillContext::drawCards(int count, bool opponent, std::optional<Name> withTag, std::optional<int> withAttachment,
                             std::optional<int> withDefinition) {
  int who = opponent ? 1 - selfWho() : selfWho();
  if (!withTag && !withAttachment && !withDefinition) {
    callAndEmit(mutator_.drawCardsPlain(who, count));
    return;
  }
  auto matches = [&](const EntityPointer& card) {
    if (withDefinition) return card->definition->id == *withDefinition;
    if (withTag) return card->definition->hasTag(*withTag);
    return std::any_of(card->attachments.begin(), card->attachments.end(),
                       [&](const EntityPointer& attachment) { return attachment->definition->id == *withAttachment; });
  };
  for (int index = 0; index < count; ++index) {
    std::vector<EntityPointer> candidates;
    for (const EntityPointer& card : state().players[who]->pile) {
      if (matches(card)) candidates.push_back(card);
    }
    if (candidates.empty()) break;
    EntityPointer chosen = random(candidates);
    callAndEmit(mutator_.insertMovedHandCard(who, chosen, EntityArea{AreaType::Pile, who, 0, chosen->id}, MoveReason::Draw));
  }
}

void SkillContext::drawCards(const std::vector<EntityHandle>& cards) {
  for (const EntityHandle& card : cards) {
    EntityPointer cardState = card.latest();
    EntityArea area = card.area();
    if (area.type != AreaType::Pile) throw GameDataError("Cannot draw a card that is not in a pile");
    callAndEmit(mutator_.insertMovedHandCard(area.who, cardState, area, MoveReason::Draw));
  }
}

void SkillContext::createPileCards(int cardId, int count, PileInsertion strategy, bool opponent) {
  int who = opponent ? 1 - selfWho() : selfWho();
  const EntityDefinition* definition = state().data->entities.find(cardId);
  if (!definition) throw GameDataError("Unknown card definition id " + std::to_string(cardId));
  std::vector<PilePayload> payloads;
  for (int index = 0; index < count; ++index) {
    PilePayload payload;
    EntityState card;
    card.definition = definition;  // variables start empty here, as in the JS template
    payload.created = card;
    payloads.push_back(std::move(payload));
  }
  mutator_.insertPileCards(std::move(payloads), strategy, who);
}

void SkillContext::undrawCards(const std::vector<EntityHandle>& cards, PileInsertion strategy, bool opponent) {
  int who = opponent ? 1 - selfWho() : selfWho();
  std::vector<PilePayload> payloads;
  for (const EntityHandle& card : cards) {
    PilePayload payload;
    payload.moved = card.latest();
    payload.movedFrom = EntityArea{AreaType::Hands, who, 0, card.id()};
    payload.reason = MoveReason::Undraw;
    payloads.push_back(std::move(payload));
  }
  mutator_.insertPileCards(std::move(payloads), strategy, who);
}

void SkillContext::stealHandCard(const EntityHandle& card) {
  EntityPointer cardState = card.latest();
  int me = selfWho();
  gitcg::moveEntity(mutator_.mutableState(), cardState->id, EntityArea{AreaType::Hands, me, 0, cardState->id}, std::nullopt,
                    MoveReason::Steal);
  bool overflowed = false;
  if (static_cast<int>(player().hands.size()) > state().config.maxHandsCount) {
    removeEntity(mutator_.mutableState(), cardState->id, me, RemoveReason::Overflow);
    overflowed = true;
  }
  emitEvent(EventName::OnHandCardInserted, std::make_shared<HandCardInsertedEventArg>(rawState(), me, cardState, HandInsertReason::Steal, overflowed));
}

void SkillContext::swapPlayerHandCards() {
  int me = selfWho();
  std::vector<EntityPointer> myHands = state().players[me]->hands;
  std::vector<EntityPointer> opponentHands = state().players[1 - me]->hands;
  for (const EntityPointer& card : opponentHands) {
    gitcg::moveEntity(mutator_.mutableState(), card->id, EntityArea{AreaType::Hands, me, 0, card->id}, std::nullopt, MoveReason::Swap);
    emitEvent(EventName::OnHandCardInserted, std::make_shared<HandCardInsertedEventArg>(rawState(), me, card, HandInsertReason::Steal, false));
  }
  for (const EntityPointer& card : myHands) {
    gitcg::moveEntity(mutator_.mutableState(), card->id, EntityArea{AreaType::Hands, 1 - me, 0, card->id}, std::nullopt, MoveReason::Swap);
    emitEvent(EventName::OnHandCardInserted, std::make_shared<HandCardInsertedEventArg>(rawState(), 1 - me, card, HandInsertReason::Steal, false));
  }
}

void SkillContext::discard(const std::vector<EntityHandle>& cards) {
  for (const EntityHandle& card : cards) {
    EntityPointer cardState = card.latest();
    EntityArea area = card.area();
    if (area.type != AreaType::Hands && area.type != AreaType::Pile) throw GameDataError("Cannot discard a card not in hands or pile");
    emitEvent(EventName::OnDispose, std::make_shared<DisposeEventArg>(rawState(), cardState, RemoveReason::Discarded, area, sharedSkillInfo()));
    removeEntity(mutator_.mutableState(), cardState->id, area.who, RemoveReason::Discarded);
  }
}

std::vector<EntityHandle> SkillContext::discardMaxCostHands(int count, bool allowPreview) {
  if (state().versionBehavior.discardMaxCostHandsAbortPreview && !allowPreview) abortPreview();
  std::vector<EntityHandle> discarded;
  for (int round = 0; round < count; ++round) {
    std::vector<std::pair<EntityPointer, int>> hands;
    for (const EntityPointer& card : player().hands) hands.emplace_back(card, diceCostSizeOfCard(rawState(), *card));
    if (hands.empty()) break;
    int maxCost = std::max_element(hands.begin(), hands.end(), [](const auto& first, const auto& second) { return first.second < second.second; })->second;
    std::vector<EntityPointer> candidates;
    for (const auto& [card, cost] : hands) {
      if (cost == maxCost) candidates.push_back(card);
    }
    EntityPointer chosen = random(candidates);
    EntityHandle handle(this, chosen->id);
    discard({handle});
    discarded.push_back(handle);
  }
  return discarded;
}

void SkillContext::setPlayerFlag(int who, PlayerFlag flag, bool value) {
  gitcg::setPlayerFlag(mutator_.mutableState(), who, flag, value);
}

void SkillContext::continueNextTurn(bool opponent) {
  int skipWho = opponent ? selfWho() : 1 - selfWho();
  gitcg::setPlayerFlag(mutator_.mutableState(), skipWho, PlayerFlag::SkipNextTurn, true);
}

}  // namespace gitcg
