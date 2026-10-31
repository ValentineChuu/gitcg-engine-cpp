// The state holder that applies the core's primitive operations (core/src/mutator.ts StateMutator): damage,
// heal, elements, cards into hands and piles, entities onto the board, switching, and the four player choices.
// Notifications are not produced: the bridge runs every game with notifications off.
#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "core/events/action_event_args.h"
#include "core/events/damage_event_args.h"
#include "core/events/entity_event_args.h"
#include "core/events/event_list.h"
#include "core/events/request_args.h"
#include "core/state/game_state.h"

namespace gitcg {

// The four requests a mutator can make of a player (answered by the game, possibly after suspending).
class MutatorIo {
 public:
  virtual ~MutatorIo() = default;
  virtual std::vector<int> switchHands(int who) = 0;     // hand card ids to swap out
  virtual std::vector<DiceType> reroll(int who) = 0;     // dice values to reroll
  virtual int selectCard(int who, const std::vector<int>& candidateDefinitionIds) = 0;
  virtual int chooseActive(int who, const std::vector<int>& candidateIds) = 0;
};

struct EventsAndDefeat {
  EventList events;
  bool causeDefeated = false;
};

struct DamageResult {
  DamageInfo damageInfo;
  EventList events;
  bool causeDefeated = false;
};

struct CreateEntityOptions {
  bool modifyOverriddenVariablesOnly = false;
  std::vector<std::pair<Name, double>> overrideVariables;
};

struct InsertEntityResult {
  EntityPointer oldState;  // the entity of the same definition that was overridden, if any
  EntityPointer newState;  // null when nothing entered
  EventList events;
};

enum class PileStrategy { Top, Bottom, Random, SpaceAround, TopRange, TopIndex };
struct PileInsertion {
  PileStrategy strategy = PileStrategy::Top;
  int parameter = 0;  // TopRange: range; TopIndex: index
};

// A card going into a pile: a new card from a template, or an existing card moving.
struct PilePayload {
  std::optional<EntityState> created;
  EntityPointer moved;
  EntityArea movedFrom;
  MoveReason reason = MoveReason::Other;
};

struct SwitchActiveOptions {
  std::shared_ptr<const SkillInfo> via;
  std::optional<bool> fast;
  Reaction fromReaction = reaction::None;
};

class Mutator {
 public:
  explicit Mutator(GameStatePointer state, MutatorIo* io = nullptr) : state_(std::move(state)), io_(io) {}

  const GameStatePointer& state() const { return state_; }
  GameStatePointer& mutableState() { return state_; }
  void resetState(GameStatePointer newState) { state_ = std::move(newState); }

  int64_t stepRandom();
  int stepId();
  std::vector<DiceType> randomDice(int count, bool alwaysOmni = false);

  // Runs one skill on its own context and adopts the resulting state; rethrows the skill's error.
  EventsAndDefeat executeInlineSkill(const SkillInfo& skill, SkillArgument& argument);
  // Runs every skill listening to an inline event (modifyDamage*, modifyHeal*, modifyReaction, ...).
  EventsAndDefeat handleInlineEvent(const SkillInfo& parentSkill, EventName event, EventArg& argument);

  EventsAndDefeat apply(const CharacterPointer& target, DamageType type, const DamageOption& option,
                        std::shared_ptr<const DamageInfo> fromDamage);
  EventsAndDefeat heal(double value, const CharacterPointer& target, std::shared_ptr<const SkillInfo> via, HealKind kind);
  DamageResult damage(DamageInfo damageInfo, const DamageOption& option);

  // A card enters a hand, created or moved; over the hand limit it is discarded (unless noOverflow).
  EventList insertCreatedHandCard(int who, EntityState card, bool noOverflow = false);
  EventList insertMovedHandCard(int who, const EntityPointer& card, const EntityArea& from, MoveReason reason, bool noOverflow = false);
  EventList drawCardsPlain(int who, int count);
  struct CreatedHandCard {
    EntityPointer state;  // as created (id filled in)
    EventList events;
  };
  CreatedHandCard createHandCard(int who, const EntityDefinition* definition, bool noOverflow = false);
  void insertPileCards(std::vector<PilePayload> payloads, PileInsertion strategy, int who);
  // An entity enters the board from a definition (or moves there when `moving` is set).
  InsertEntityResult insertEntityOnStage(const EntityDefinition* definition, const EntityPointer& moving, const EntityArea& area,
                                         const CreateEntityOptions& options, std::optional<MoveReason> moveReason = std::nullopt);
  InsertEntityResult createAttachment(const EntityPointer& host, const EntityDefinition* definition, const CreateEntityOptions& options);
  EventList switchActive(int who, const CharacterPointer& target, const SwitchActiveOptions& options = {});

  // Player choices (through the io; each may suspend the game).
  void reroll(int who, int times);
  EventList switchHands(int who);
  EventList selectCard(int who, std::shared_ptr<const SkillInfo> via, const SelectCardInfo& info);
  CharacterPointer chooseActive(int who);

 private:
  EventAndArgument recordReaction(const ReactionInfo& info);

  GameStatePointer state_;
  MutatorIo* io_;
};

}  // namespace gitcg
