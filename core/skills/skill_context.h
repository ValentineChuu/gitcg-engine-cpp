// What card code runs against (core/src/runtime/skill_context.ts SkillContext): one skill execution with its own
// mutator over a copy of the state, the event argument, and every operation cards use. Method names follow the
// JS API so the translator maps calls one to one. Targets are resolved handles; a query is evaluated by the caller
// with queryAll at the same point the JS evaluates it.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/events/damage_event_args.h"
#include "core/events/entity_event_args.h"
#include "core/events/event_list.h"
#include "core/events/request_args.h"
#include "core/mutations/mutator.h"
#include "core/skills/skill_outcome.h"
#include "core/skills/state_handles.h"
#include "core/skills/random_entity_choice.h"

namespace gitcg {

struct QueryExpression;

// Per-definition options shared by its skills (core/src/base/skill.ts SkillContextOptions).
struct ContextOptions {
  std::optional<int> associatedExtensionId;
  std::map<std::string, std::function<void(SkillContext&)>> snippets;
};

using SkillOperation = std::function<void(SkillContext&)>;

class SkillContext {
 public:
  SkillContext(GameStatePointer state, const SkillInfo& skillInfo, SkillArgument& eventArgument, const ContextOptions& options);

  // Wraps an operation into a skill action: run it, close the event list, return the state and events.
  static SkillAction encapsulate(std::shared_ptr<const ContextOptions> options, SkillOperation operation);
  // Runs a read-only operation (filters, target getters, descriptions) and returns its value.
  template <typename Result>
  static std::function<Result(const GameStatePointer&, const SkillInfo&, SkillArgument&)> encapsulateForResult(
      std::shared_ptr<const ContextOptions> options, std::function<Result(SkillContext&)> operation) {
    return [options, operation](const GameStatePointer& state, const SkillInfo& skill, SkillArgument& argument) {
      SkillContext context(state, skill, argument, *options);
      return operation(context);
    };
  }

  // --- reading ---
  const SkillInfo& skillInfo() const { return skillInfo_; }
  SkillArgument& eventArgument() const { return *eventArgument_; }
  template <typename Argument>
  Argument& e() const { return static_cast<Argument&>(*eventArgument_); }
  AnyHandle self();
  CharacterHandle selfCharacter() { return CharacterHandle(this, skillInfo_.caller.id()); }
  EntityHandle selfEntity() { return EntityHandle(this, skillInfo_.caller.id()); }
  int selfWho();
  const GameStatePointer& rawState() const { return mutator_.state(); }
  const GameState& state() const { return *mutator_.state(); }
  const PlayerState& player();
  const PlayerState& oppPlayer();
  int roundNumber() const { return state().roundNumber; }
  Phase phase() const { return state().phase; }
  bool isMyTurn();
  bool isPreview() const { return skillInfo_.environment == SkillEnvironment::Preview; }
  std::optional<double> fromReactionValue() const;

  std::vector<AnyHandle> queryAll(const QueryExpression& query);
  std::optional<AnyHandle> query(const QueryExpression& query);
  AnyHandle get(int id);
  AnyHandle get(const AnyState& state) { return get(state.id()); }
  EntityArea entityArea(int id);

  const JsonValue& getExtensionState();
  void setExtensionState(const std::function<void(JsonValue&)>& setter);
  int countOfSkill();
  int countOfSkill(int characterDefinitionId, double skillId);
  bool hasPhaseDamage(bool allPlayers, const std::function<bool(DamageOrHealEventArg&)>& filter = nullptr);
  bool hasPhaseReaction(bool allPlayers, const std::function<bool(ReactionEventArg&)>& filter = nullptr);
  std::vector<EntityHandle> maxCostHands(int count, bool opponent = false, const std::function<bool(const EntityHandle&)>& filter = nullptr);
  bool isInInitialPile(const EntityHandle& card, bool opponent = false);
  int remainingSupportCount(bool opponent = false);
  std::vector<const EntityDefinition*> allCardDefinitions(const std::function<bool(const EntityDefinition&)>& filter = nullptr);
  bool isSelfDisposeCausedByDefeatedHeuristically(const std::function<bool(DamageOrHealEventArg&)>& extraCondition = nullptr);

  // --- events ---
  void eventBoundary();
  void emitEvent(EventName name, std::shared_ptr<SkillArgument> argument);
  void emitCustomEvent(const CustomEvent* event, JsonValue argument = JsonValue());
  void handleCustomEventInline(const CustomEvent* event, JsonValue argument = JsonValue(), const SkillArgument* rawArgument = nullptr);
  void abortPreview();
  void callSnippet(const std::string& name, SkillArgument* argument);

  // --- characters ---
  void switchActive(const std::vector<AnyHandle>& targets);
  void gainEnergy(double value, const std::vector<AnyHandle>& targets);
  void heal(double value, const std::vector<AnyHandle>& targets, HealKind kind = HealKind::Common);
  void immune(double newHealth);
  void increaseMaxHealth(double value, const std::vector<AnyHandle>& targets, bool alsoHeal = true);
  void damage(DamageType type, double value);  // to the opponent's active character
  void damage(DamageType type, double value, const std::vector<AnyHandle>& targets);
  void apply(DamageType type, const std::vector<AnyHandle>& targets);
  void cleanAura(Aura what, const std::vector<AnyHandle>& targets);  // what = -1: all
  void swapCharacterPosition(const std::vector<AnyHandle>& first, const std::vector<AnyHandle>& second);
  void consumeNightsoul(const std::vector<AnyHandle>& targets, double count = 1);
  void gainNightsoul(const std::vector<AnyHandle>& targets, double count = 1);

  // --- entities ---
  std::optional<EntityHandle> createEntity(EntityType type, int definitionId, std::optional<EntityArea> area = std::nullopt,
                                           const CreateEntityOptions& options = {});
  std::optional<EntityHandle> moveEntity(const EntityHandle& entity, const EntityArea& area, MoveReason reason = MoveReason::Other);
  std::optional<EntityHandle> summon(int definitionId, bool opponent = false, const CreateEntityOptions& options = {});
  void characterStatus(int definitionId, const CreateEntityOptions& options = {});  // on self
  void characterStatus(int definitionId, const std::vector<AnyHandle>& targets, const CreateEntityOptions& options = {});
  std::optional<EntityHandle> combatStatus(int definitionId, bool opponent = false, const CreateEntityOptions& options = {});
  void equip(int definitionId, const std::vector<AnyHandle>& targets, const CreateEntityOptions& options = {});
  void equip(const EntityHandle& card, const std::vector<AnyHandle>& targets);
  void unequip(const EntityHandle& equipment);
  void attach(int attachmentId, const EntityHandle& target, const CreateEntityOptions& options = {});
  void attachCostIncrease(const std::vector<AnyHandle>& targets, double value = 1);
  void attachCostReduction(const std::vector<AnyHandle>& targets, double value = 1);
  void dispose(std::optional<std::vector<AnyHandle>> targets = std::nullopt, RemoveReason reason = RemoveReason::Other, bool direct = false);
  double getVariable(Name name);
  double getVariable(Name name, const AnyHandle& target);
  void setVariable(Name name, double value);
  void setVariable(Name name, double value, const AnyHandle& target);
  void addVariable(Name name, double value);
  void addVariable(Name name, double value, const AnyHandle& target);
  void consumeUsage(double count = 1);
  void consumeUsage(double count, const AnyHandle& target);
  void consumeUsagePerRound(double count = 1);
  void transformDefinition(const std::vector<AnyHandle>& targets, int newDefinitionId);

  // --- dice and cards ---
  std::vector<DiceType> absorbDice(bool different, int count);
  void convertDice(DiceType target, int count, bool opponent = false);  // count < 0: all
  void generateDice(DiceType type, int count);                           // a fixed type
  void generateRandomElementDice(int count, bool includeOmni = false, bool allowDuplicate = false);
  std::optional<EntityHandle> createHandCard(int cardId, bool opponent = false);
  void drawCards(int count, bool opponent = false, std::optional<Name> withTag = std::nullopt,
                 std::optional<int> withAttachment = std::nullopt, std::optional<int> withDefinition = std::nullopt);
  void drawCards(const std::vector<EntityHandle>& cards);
  void createPileCards(int cardId, int count, PileInsertion strategy, bool opponent = false);
  void undrawCards(const std::vector<EntityHandle>& cards, PileInsertion strategy, bool opponent = false);
  void stealHandCard(const EntityHandle& card);
  void mutateEntityVariable(int id, Name name, double value, int direction);  // raw :mutate, without variable events
  void setPlayerFlag(int who, PlayerFlag flag, bool value);  // `:mutate({ type: "setPlayerFlag", ... })`
  void swapPlayerHandCards();
  void discard(const std::vector<EntityHandle>& cards);
  std::vector<EntityHandle> discardMaxCostHands(int count, bool allowPreview = false);
  void continueNextTurn(bool opponent = false);

  // --- requests (carried out after the skill) ---
  void switchCards();
  void rerollDice(int times);
  void triggerEndPhaseSkill(const EntityHandle& target);
  void useSkill(double skillId, bool asPrepared = false);
  void useNormalSkill(bool asPrepared = false);
  void playCard(const EntityHandle& card, PlayCardTarget target);
  void selectAndSummon(const std::vector<int>& summonIds);
  void selectAndCreateHandCard(const std::vector<int>& cardIds);
  void selectAndPlay(const std::vector<int>& cardIds, PlayCardTarget target = {});
  void adventure();
  void finishAdventure();

  // --- randomness ---
  int64_t stepRandom() { return mutator_.stepRandom(); }
  RandomEntityChoice random(const std::vector<EntityHandle>& items) {
    int64_t value = stepRandom();
    if (items.empty()) return {this, std::nullopt};
    return {this, items[static_cast<size_t>(value % static_cast<int64_t>(items.size()))].id()};
  }
  template <typename Item>
  Item random(const std::vector<Item>& items) {
    int64_t value = stepRandom();
    if (items.empty()) throw GameDataError("Undefined random choice requires a nullable translated result");
    // In particular vector<bool> returns a proxy value, never a reference to a temporary bool.
    return items[static_cast<size_t>(value % static_cast<int64_t>(items.size()))];
  }
  template <typename Item>
  std::vector<Item> shuffle(std::vector<Item> items) {
    size_t count = items.size();
    return shuffleTail(std::move(items), count);
  }
  template <typename Item>
  std::vector<Item> randomSubset(std::vector<Item> items, int count) {
    if (count <= 0) return {};
    size_t taken = std::min<size_t>(count, items.size());
    std::vector<Item> shuffled = shuffleTail(std::move(items), taken);
    size_t start = shuffled.size() >= static_cast<size_t>(count) ? shuffled.size() - count : 0;
    return std::vector<Item>(shuffled.begin() + start, shuffled.end());
  }

  // Set by reaction skills (CALLED_FROM_REACTION).
  Reaction fromReaction = reaction::None;

 private:
  friend class CharacterHandle;
  friend class EntityHandle;

  template <typename Item>
  std::vector<Item> shuffleTail(std::vector<Item> items, size_t count) {
    // JS: for (i = length - 1; i >= stopIndex; i--) swap(i, random % (i + 1)).
    size_t stopIndex = items.size() >= count ? items.size() - count : 0;
    for (size_t index = items.size(); index-- > stopIndex;) {
      size_t swapIndex = static_cast<size_t>(stepRandom() % static_cast<int64_t>(index + 1));
      std::swap(items[index], items[swapIndex]);
    }
    return items;
  }

  SkillOutcome terminate(std::exception_ptr error);
  std::vector<EventAndArgument> preprocessEvents();
  void callAndEmit(EventList events) { currentEvents_.append(events); }
  std::vector<CharacterHandle> charactersOf(const std::vector<AnyHandle>& targets);
  void setVariableImpl(const AnyHandle& target, VariableChangeInfo info);
  void attachCostChange(const EntityHandle& target, double value, bool increase);
  std::vector<Reaction> enabledLunarReactions(int targetWho);
  std::shared_ptr<const SkillInfo> sharedSkillInfo();
  std::vector<AnyHandle> handlesOf(const std::vector<AnyState>& states);

  Mutator mutator_;
  const SkillInfo& skillInfo_;
  std::shared_ptr<const SkillInfo> sharedSkillInfo_;
  SkillArgument* eventArgument_;
  const ContextOptions& options_;
  EventList currentEvents_;
  std::vector<EventAndArgument> processedEvents_;
  bool causeDefeated_ = false;
  std::shared_ptr<const DamageInfo> mainDamage_;
};

}  // namespace gitcg
