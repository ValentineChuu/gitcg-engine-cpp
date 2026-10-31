#include "core/mutations/mutator.h"
#include "core/mutations/mutation_observer.h"

#include <algorithm>
#include <cmath>

#include "core/definitions/character_definition.h"
#include "core/definitions/entity_definition.h"
#include "core/definitions/game_data.h"
#include "core/mutations/inserted_variables.h"
#include "core/mutations/state_mutations.h"
#include "core/rules/dice_order.h"
#include "core/rules/entity_lookup.h"
#include "core/rules/reaction_table.h"
#include "core/skills/reaction_skills.h"
#include "core/skills/skill_outcome.h"
#include "core/values/engine_errors.h"

namespace gitcg {

namespace {

const Name& nameAlive() { static const Name name = intern("alive"); return name; }
const Name& nameHealth() { static const Name name = intern("health"); return name; }
const Name& nameMaxHealth() { static const Name name = intern("maxHealth"); return name; }
const Name& nameAura() { static const Name name = intern("aura"); return name; }

double variable(const CharacterPointer& character, Name name) {
  return character->variables.valueOr(name, 0);
}

std::shared_ptr<SkillInfo> inlineSkillInfo(AnyState caller, const SkillDefinition* definition, SkillEnvironment environment) {
  auto info = std::make_shared<SkillInfo>();
  info->caller = std::move(caller);
  info->definition = definition;
  info->environment = environment;
  info->finalizeMode = FinalizeMode::Simple;
  return info;
}

EntityState entityFromDefinition(const EntityDefinition* definition) {
  EntityState card;
  card.definition = definition;
  for (const auto& [name, config] : definition->varConfigs) card.variables.set(name, config.initialValue);
  return card;
}

}  // namespace

int64_t Mutator::stepRandom() {
  return gitcg::stepRandom(state_);
}

int Mutator::stepId() {
  return gitcg::stepId(state_);
}

std::vector<DiceType> Mutator::randomDice(int count, bool alwaysOmni) {
  if (alwaysOmni) return std::vector<DiceType>(count, dice::Omni);
  std::vector<DiceType> result;
  for (int index = 0; index < count; ++index) result.push_back(static_cast<DiceType>(stepRandom() % 8 + 1));
  return result;
}

EventsAndDefeat Mutator::executeInlineSkill(const SkillInfo& skill, SkillArgument& argument) {
  GameStatePointer before = state_;
  ObservedSkill observation;
  SkillOutcome outcome = skill.definition->action(state_, skill, argument);
  observation.finish(outcome.state != before);
  state_ = outcome.state;
  if (outcome.error) std::rethrow_exception(outcome.error);
  EventsAndDefeat result;
  result.events.append(outcome.emittedEvents);
  result.causeDefeated = outcome.causeDefeated;
  return result;
}

EventsAndDefeat Mutator::handleInlineEvent(const SkillInfo& parentSkill, EventName event, EventArg& argument) {
  EventsAndDefeat result;
  std::vector<std::shared_ptr<SkillInfo>> infos;
  for (const CallerAndSkill& entry : allSkills(*state_, event)) {
    infos.push_back(inlineSkillInfo(entry.caller, entry.skill, parentSkill.environment));
  }
  for (const auto& info : infos) {
    argument.currentSkillInfo = info.get();
    if (!info->definition->filter(state_, *info, argument)) continue;
    EventsAndDefeat inner = executeInlineSkill(*info, argument);
    result.events.append(inner.events);
    result.causeDefeated = result.causeDefeated || inner.causeDefeated;
  }
  return result;
}

EventAndArgument Mutator::recordReaction(const ReactionInfo& info) {
  auto reactionEvent = std::make_shared<ReactionEventArg>(state_, info);
  pushPhaseReactionLog(state_, reactionEvent->viaWho(), reactionEvent);
  return {EventName::OnReaction, reactionEvent};
}

EventsAndDefeat Mutator::apply(const CharacterPointer& target, DamageType type, const DamageOption& option,
                               std::shared_ptr<const DamageInfo> fromDamage) {
  EventsAndDefeat result;
  if (variable(target, nameAlive()) == 0) return result;
  Aura currentAura = static_cast<Aura>(variable(target, nameAura()));
  ReactionOutcome outcome = getReaction(type, currentAura, option.enabledLunarReactions);
  modifyEntityVariable(state_, target->id, nameAura(), outcome.newAura);
  if (!fromDamage) observeAura(type, target, outcome.newAura, outcome.reaction);
  if (outcome.reaction == reaction::None) return result;
  ReactionInfo reactionInfo{outcome.reaction, option.via, target, fromDamage};
  // A damage queues its reaction before the remaining damage modifiers; only pure applications record here.
  if (!fromDamage) result.events.push(recordReaction(reactionInfo));
  auto modifyReaction = std::make_shared<ModifyReactionEventArg>(state_, reactionInfo);
  EventsAndDefeat inner = handleInlineEvent(*option.via, EventName::ModifyReaction, *modifyReaction);
  result.events.append(inner.events);
  result.causeDefeated = result.causeDefeated || inner.causeDefeated;
  const SkillDefinition* description = reactionSkill(outcome.reaction);
  if (!modifyReaction->coreEffectsCancelled && description) {
    SkillInfo reactionInfoSkill;
    reactionInfoSkill.caller = option.via->caller;
    reactionInfoSkill.definition = description;
    reactionInfoSkill.environment = option.via->environment;
    reactionInfoSkill.finalizeMode = FinalizeMode::Simple;
    EventsAndDefeat effects = executeInlineSkill(reactionInfoSkill, *modifyReaction);
    result.events.append(effects.events);
    result.causeDefeated = result.causeDefeated || effects.causeDefeated;
  }
  return result;
}

EventsAndDefeat Mutator::heal(double value, const CharacterPointer& target, std::shared_ptr<const SkillInfo> via, HealKind kind) {
  EventsAndDefeat result;
  if (variable(target, nameAlive()) == 0) {
    if (kind != HealKind::Revive) return result;
    modifyEntityVariable(state_, target->id, nameAlive(), 1, 1);
    result.events.push({EventName::OnRevive, std::make_shared<CharacterEventArg>(state_, target)});
  } else if (variable(target, nameHealth()) == 0 && kind != HealKind::ImmuneDefeated) {
    // Patch 4: a character at 0 health awaiting defeat is not healed.
    return result;
  } else if ((variable(target, nameHealth()) == 0) != (kind == HealKind::ImmuneDefeated)) {
    throw GameInternalError("Cannot apply this heal kind on a character with this health");
  }
  double injury = variable(target, nameMaxHealth()) - variable(target, nameHealth());
  DamageInfo healInfo;
  healInfo.healId = stepId();
  healInfo.type = damage::Heal;
  healInfo.expectedValue = value;
  healInfo.value = std::min(value, injury);
  healInfo.healKind = kind;
  healInfo.source = via->caller;
  healInfo.via = via;
  healInfo.target = target;
  auto modifier = std::make_shared<ModifyHealEventArg>(state_, healInfo, std::nullopt);
  for (EventName event : {EventName::ModifyHeal0, EventName::ModifyHeal1}) {
    EventsAndDefeat inner = handleInlineEvent(*via, event, *modifier);
    result.events.append(inner.events);
    result.causeDefeated = result.causeDefeated || inner.causeDefeated;
  }
  if (modifier->cancelled()) return result;
  healInfo = modifier->healInfo();
  double newHealth = kind == HealKind::ImmuneDefeated ? healInfo.value : variable(target, nameHealth()) + healInfo.value;
  modifyEntityVariable(state_, target->id, nameHealth(), newHealth, 1);
  observeDamage(healInfo, static_cast<Aura>(variable(target, nameAura())), variable(target, nameHealth()),
                variable(target, nameHealth()) + healInfo.value, reaction::None);
  result.events.push({EventName::OnDamageOrHeal, std::make_shared<DamageOrHealEventArg>(state_, healInfo, std::nullopt)});
  return result;
}

DamageResult Mutator::damage(DamageInfo damageInfo, const DamageOption& option) {
  CharacterPointer target = damageInfo.target;
  DamageResult result;
  if (damageInfo.type != damage::Piercing) {
    auto modifier = std::make_shared<ModifyDamageEventArg>(state_, damageInfo, option);
    for (EventName event : {EventName::ModifyDamage0, EventName::Initiative /* reaction stage */, EventName::ModifyDamage1,
                            EventName::ModifyDamage2, EventName::ModifyDamage3}) {
      if (event == EventName::Initiative) {
        Reaction reactionType = modifier->increaseDamageByReaction();
        if (reactionType != reaction::None) {
          auto fromDamage = std::make_shared<DamageInfo>(modifier->damageInfo());
          result.events.push(recordReaction({reactionType, option.via, target, fromDamage}));
        }
        continue;
      }
      EventsAndDefeat inner = handleInlineEvent(*option.via, event, *modifier);
      result.events.append(inner.events);
      result.causeDefeated = result.causeDefeated || inner.causeDefeated;
    }
    damageInfo = modifier->damageInfo();
  }
  double finalHealth = std::max(0.0, variable(target, nameHealth()) - damageInfo.value);
  modifyEntityVariable(state_, target->id, nameHealth(), finalHealth, 2);
  if (variable(target, nameAlive()) != 0) {
    ReactionOutcome predicted = getReaction(damageInfo.type, damageInfo.targetAura, option.enabledLunarReactions);
    observeDamage(damageInfo, predicted.newAura, variable(target, nameHealth()), finalHealth, predicted.reaction);
  }
  auto damageEvent = std::make_shared<DamageOrHealEventArg>(state_, damageInfo, option);
  pushPhaseDamageLog(state_, damageEvent->sourceWho, damageEvent);
  result.events.push({EventName::OnDamageOrHeal, damageEvent});
  if (damageInfo.type != damage::Physical && damageInfo.type != damage::Piercing) {
    EventsAndDefeat inner = apply(target, damageInfo.type, option, std::make_shared<DamageInfo>(damageInfo));
    result.events.append(inner.events);
    result.causeDefeated = result.causeDefeated || inner.causeDefeated;
  }
  // causeDefeated stays as the inner events say: a defeat may still be prevented later (immune).
  result.damageInfo = damageInfo;
  return result;
}

EventList Mutator::insertCreatedHandCard(int who, EntityState card, bool noOverflow) {
  EntityArea hands{AreaType::Hands, who, 0, 0};
  card.id = createEntity(state_, hands, card);
  EntityPointer created = makeRef<EntityState>(card);
  bool overflowed = false;
  if (!noOverflow && static_cast<int>(state_->players[who]->hands.size()) > state_->config.maxHandsCount) {
    removeEntity(state_, created->id, who, RemoveReason::Overflow);
    overflowed = true;
  }
  EventList events;
  events.push({EventName::OnHandCardInserted,
               std::make_shared<HandCardInsertedEventArg>(state_, who, created, HandInsertReason::Create, overflowed)});
  return events;
}

EventList Mutator::insertMovedHandCard(int who, const EntityPointer& card, const EntityArea& from, MoveReason reason, bool noOverflow) {
  moveEntity(state_, card->id, EntityArea{AreaType::Hands, who, 0, 0}, std::nullopt, reason);
  bool overflowed = false;
  if (!noOverflow && static_cast<int>(state_->players[who]->hands.size()) > state_->config.maxHandsCount) {
    removeEntity(state_, card->id, who, RemoveReason::Overflow);
    overflowed = true;
  }
  EventList events;
  events.push({EventName::OnHandCardInserted, std::make_shared<HandCardInsertedEventArg>(
                                                  state_, who, card, static_cast<HandInsertReason>(reason), overflowed)});
  return events;
}

EventList Mutator::drawCardsPlain(int who, int count) {
  EventList events;
  for (int index = 0; index < count; ++index) {
    if (state_->players[who]->pile.empty()) continue;
    EntityPointer card = state_->players[who]->pile[0];
    events.append(insertMovedHandCard(who, card, EntityArea{AreaType::Pile, who, 0, 0}, MoveReason::Draw));
  }
  return events;
}

Mutator::CreatedHandCard Mutator::createHandCard(int who, const EntityDefinition* definition, bool noOverflow) {
  if (definition->type != EntityType::Support && definition->type != EntityType::Equipment && definition->type != EntityType::EventCard) {
    throw GameDataError("Cannot create this entity type in hand");
  }
  EntityState card = entityFromDefinition(definition);
  card.id = 0;
  int idBefore = state_->idIterator;
  CreatedHandCard result;
  result.events = insertCreatedHandCard(who, card, noOverflow);
  card.id = idBefore;
  result.state = makeRef<EntityState>(card);
  return result;
}

void Mutator::insertPileCards(std::vector<PilePayload> payloads, PileInsertion strategy, int who) {
  EntityArea pile{AreaType::Pile, who, 0, 0};
  // The pile length as it was before any insertion: every index below is computed from it.
  const int pileCount = static_cast<int>(state_->players[who]->pile.size());
  int room = std::max(0, state_->config.maxPileCount - pileCount);
  if (static_cast<int>(payloads.size()) > room) payloads.resize(room);
  if (payloads.empty()) return;
  int count = static_cast<int>(payloads.size());
  auto insert = [&](PilePayload& payload, int targetIndex) {
    if (payload.created) {
      createEntity(state_, pile, *payload.created, targetIndex);
    } else {
      moveEntity(state_, payload.moved->id, pile, targetIndex, payload.reason);
    }
  };
  switch (strategy.strategy) {
    case PileStrategy::Top:
      for (PilePayload& payload : payloads) insert(payload, 0);
      break;
    case PileStrategy::Bottom:
      for (PilePayload& payload : payloads) insert(payload, pileCount);
      break;
    case PileStrategy::Random:
      for (PilePayload& payload : payloads) insert(payload, static_cast<int>(stepRandom() % (pileCount + 1)));
      break;
    case PileStrategy::SpaceAround: {
      int spaces = count + 1;
      int step = pileCount / spaces;
      int rest = pileCount % spaces;
      for (int index = 0, offset = step; index < count; ++index, offset += step) {
        if (index < rest) ++offset;
        insert(payloads[index], index + offset);
      }
      break;
    }
    case PileStrategy::TopRange: {
      int range = std::min(strategy.parameter, pileCount);
      for (PilePayload& payload : payloads) {
        int64_t randomValue = stepRandom();
        // JS: x % 0 is NaN and splice(NaN, ...) inserts at 0 (an empty pile).
        insert(payload, range == 0 ? 0 : static_cast<int>(randomValue % range));
      }
      break;
    }
    case PileStrategy::TopIndex: {
      int index = std::min(strategy.parameter, pileCount);
      for (PilePayload& payload : payloads) insert(payload, index);
      break;
    }
  }
}

InsertEntityResult Mutator::insertEntityOnStage(const EntityDefinition* definition, const EntityPointer& moving, const EntityArea& area,
                                                const CreateEntityOptions& options, std::optional<MoveReason> moveReason) {
  static const Name IMMUNE_CONTROL = intern("immuneControl");
  static const Name DISABLE_SKILL = intern("disableSkill");
  if (area.type == AreaType::Hands || area.type == AreaType::Pile) {
    throw GameDataError("insertEntityOnStage is for the board, not hands or pile");
  }
  InsertEntityResult result;
  std::vector<AnyState> entitiesAtArea = allEntitiesAtArea(*state_, area);
  bool immuneControl = std::any_of(entitiesAtArea.begin(), entitiesAtArea.end(), [](const AnyState& entity) {
    return entity.isEntity() && entity.entity()->definition->type == EntityType::Status && entity.entity()->definition->hasTag(IMMUNE_CONTROL);
  });
  if (immuneControl && definition->type == EntityType::Status && definition->hasTag(DISABLE_SKILL)) return result;
  EntityPointer oldState = findOverriddenEntity(entitiesAtArea, *definition);
  std::vector<std::pair<Name, double>> incoming = moving ? moving->variables.entries() : options.overrideVariables;
  auto newVariables = insertedStateVariables(*state_, oldState, *definition, incoming, options.modifyOverriddenVariablesOnly);
  std::optional<EntityArea> moveFrom;
  if (moving) moveFrom = getEntityArea(*state_, moving->id);
  int newStateId = 0;
  if (oldState) {
    if (moveFrom) {
      RemoveReason reason = moveReason == MoveReason::Equip           ? RemoveReason::EquipOverridden
                            : moveReason == MoveReason::CreateSupport ? RemoveReason::CreateSupportOverridden
                                                                      : RemoveReason::Other;
      removeEntity(state_, moving->id, moveFrom->who, reason);
    }
    for (const auto& [name, value] : newVariables) modifyEntityVariable(state_, oldState->id, name, value, 1);
    newStateId = oldState->id;
  } else {
    int limit = area.type == AreaType::Summons ? state_->config.maxSummonsCount
                : area.type == AreaType::Supports ? state_->config.maxSupportsCount : -1;
    int present = static_cast<int>(entitiesAtArea.size());
    if (limit >= 0 && present >= limit) return result;
    if (moveFrom) {
      for (const EntityPointer& attachment : moving->attachments) removeEntity(state_, attachment->id, moveFrom->who);
      moveEntity(state_, moving->id, area, std::nullopt, moveReason.value_or(MoveReason::Other));
      newStateId = moving->id;
    } else {
      EntityState created;
      created.definition = definition;
      for (const auto& [name, value] : newVariables) created.variables.set(name, value);
      newStateId = createEntity(state_, area, std::move(created));
    }
  }
  result.oldState = oldState;
  result.newState = getEntityById(*state_, newStateId).entity();
  if (!moveFrom) {
    result.events.push({EventName::OnEnter, std::make_shared<EnterEventArg>(state_, AnyState(result.newState), oldState)});
  }
  return result;
}

InsertEntityResult Mutator::createAttachment(const EntityPointer& host, const EntityDefinition* definition, const CreateEntityOptions& options) {
  EntityArea target = getEntityArea(*state_, host->id);
  if (target.type != AreaType::Hands && target.type != AreaType::Pile) {
    throw GameDataError("Attachments can only be created on hand or pile entities.");
  }
  std::vector<AnyState> existing(host->attachments.begin(), host->attachments.end());
  EntityPointer oldState = findOverriddenEntity(existing, *definition);
  auto newVariables = insertedStateVariables(*state_, oldState, *definition, options.overrideVariables, options.modifyOverriddenVariablesOnly);
  int newStateId = 0;
  if (oldState) {
    for (const auto& [name, value] : newVariables) modifyEntityVariable(state_, oldState->id, name, value, 1);
    newStateId = oldState->id;
  } else {
    EntityState created;
    created.definition = definition;
    for (const auto& [name, value] : newVariables) created.variables.set(name, value);
    newStateId = gitcg::createAttachment(state_, target.cardId, std::move(created));
  }
  InsertEntityResult result;
  result.oldState = oldState;
  result.newState = getEntityById(*state_, newStateId).entity();
  result.events.push({EventName::OnEnter, std::make_shared<EnterEventArg>(state_, AnyState(result.newState), oldState)});
  return result;
}

EventList Mutator::switchActive(int who, const CharacterPointer& target, const SwitchActiveOptions& options) {
  static const Name IMMUNE_CONTROL = intern("immuneControl");
  const PlayerState& player = *state_->players[who];
  CharacterPointer from = player.activeCharacterId == 0 ? CharacterPointer() : activeCharacter(player);
  if (from && from->id == target->id) return {};
  if (options.via && from) {
    for (const EntityPointer& entity : from->entities) {
      if (entity->definition->hasTag(IMMUNE_CONTROL)) return {};
    }
  }
  setActiveCharacter(state_, who, target->id);
  setPlayerFlag(state_, who, PlayerFlag::CanPlunging, true);
  SwitchActiveInfo info{who, from, options.via, target, options.fromReaction != reaction::None, options.fast};
  observeSwitch(info);
  EventList events;
  events.push({EventName::OnSwitchActive, std::make_shared<SwitchActiveEventArg>(state_, info)});
  return events;
}

void Mutator::reroll(int who, int times) {
  if (!io_) throw IoNotProvidedError();
  for (int round = 0; round < times; ++round) {
    std::vector<DiceType> oldDice = state_->players[who]->dice;
    std::vector<DiceType> toReroll = io_->reroll(who);
    if (toReroll.empty()) return;
    for (DiceType type : toReroll) {
      auto found = std::find(oldDice.begin(), oldDice.end(), type);
      if (found == oldDice.end()) throw GameIoError(who, "Requested to-be-rerolled dice does not exist");
      oldDice.erase(found);
    }
    std::vector<DiceType> newDice = randomDice(static_cast<int>(toReroll.size()));
    oldDice.insert(oldDice.end(), newDice.begin(), newDice.end());
    resetDice(state_, who, sortDice(*state_->players[who], oldDice));
  }
}

EventList Mutator::switchHands(int who) {
  if (!io_) throw IoNotProvidedError();
  std::vector<int> removedIds = io_->switchHands(who);
  std::vector<EntityPointer> swapIn;
  for (int id : removedIds) {
    const auto& hands = state_->players[who]->hands;
    auto card = std::find_if(hands.begin(), hands.end(), [&](const EntityPointer& hand) { return hand->id == id; });
    if (card == hands.end()) throw GameIoError(who, "switchHands returned an unknown card");
    swapIn.push_back(*card);
  }
  std::vector<int> swapInDefinitionIds;
  for (const EntityPointer& card : swapIn) swapInDefinitionIds.push_back(card->definition->id);
  for (const EntityPointer& card : swapIn) {
    int index = static_cast<int>(stepRandom() % (state_->players[who]->pile.size() + 1));
    moveEntity(state_, card->id, EntityArea{AreaType::Pile, who, 0, card->id}, index, MoveReason::Switch);
  }
  EventList events;
  size_t topIndex = 0;
  for (size_t round = 0; round < swapIn.size(); ++round) {
    const auto& pile = state_->players[who]->pile;
    while (topIndex < pile.size() &&
           std::find(swapInDefinitionIds.begin(), swapInDefinitionIds.end(), pile[topIndex]->definition->id) != swapInDefinitionIds.end()) {
      ++topIndex;
    }
    EntityPointer candidate = topIndex >= pile.size() ? pile[0] : pile[topIndex];
    moveEntity(state_, candidate->id, EntityArea{AreaType::Hands, who, 0, candidate->id}, std::nullopt, MoveReason::Switch);
    events.push({EventName::OnHandCardInserted,
                 std::make_shared<HandCardInsertedEventArg>(state_, who, candidate, HandInsertReason::Switch, false)});
  }
  return events;
}

EventList Mutator::selectCard(int who, std::shared_ptr<const SkillInfo> via, const SelectCardInfo& info) {
  if (!io_) throw IoNotProvidedError();
  std::vector<int> candidates;
  for (const EntityDefinition* card : info.cards) candidates.push_back(card->id);
  int selected = io_->selectCard(who, candidates);
  const EntityDefinition* definition = state_->data->entities.find(selected);
  if (!definition) throw GameDataError("Unknown card definition id");
  switch (info.kind) {
    case SelectCardInfo::Kind::CreateHandCard: {
      if (definition->type != EntityType::Support && definition->type != EntityType::Equipment && definition->type != EntityType::EventCard) {
        throw GameInternalError("Invalid action card type");
      }
      return createHandCard(who, definition).events;
    }
    case SelectCardInfo::Kind::CreateEntity: {
      if (definition->type != EntityType::Summon) throw GameDataError("Entity type not supported now");
      InsertEntityResult inserted = insertEntityOnStage(definition, nullptr, EntityArea{AreaType::Summons, who, 0, 0}, {});
      EventList events;
      if (inserted.newState) {
        events.push({EventName::OnEnter, std::make_shared<EnterEventArg>(state_, AnyState(inserted.newState), inserted.oldState)});
      }
      return events;
    }
    case SelectCardInfo::Kind::RequestPlayCard: {
      CreatedHandCard created = createHandCard(who, definition, true);
      EventList events;
      events.push({EventName::RequestPlayCard, std::make_shared<PlayCardRequestArg>(via, who, created.state, info.target, true)});
      return events;
    }
  }
  return {};
}

CharacterPointer Mutator::chooseActive(int who) {
  if (!io_) throw IoNotProvidedError();
  const PlayerState& player = *state_->players[who];
  std::vector<int> candidates;
  for (const CharacterPointer& character : player.characters) {
    if (variable(character, nameAlive()) != 0 && character->id != player.activeCharacterId) candidates.push_back(character->id);
  }
  if (candidates.empty()) throw GameInternalError("No available candidate active character");
  int chosen = io_->chooseActive(who, candidates);
  return getEntityById(*state_, chosen).character();
}

}  // namespace gitcg
