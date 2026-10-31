// The core's state mutations (core/src/base/mutation.ts doMutation), applied to a state root in place:
// a node shared with any snapshot is copied first, so snapshots never change.
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "core/json/json_value.h"
#include "core/events/entity_event_args.h"
#include "core/state/entity_area.h"
#include "core/state/game_state.h"

namespace gitcg {

struct CharacterDefinition;
struct EntityDefinition;

int64_t stepRandom(GameStatePointer& state);  // the new random value
int stepId(GameStatePointer& state);          // the current id; the iterator moves down
bool changePhase(GameStatePointer& state, Phase newPhase);  // whether the phase changed
void stepRound(GameStatePointer& state);
void switchTurn(GameStatePointer& state);
void setWinner(GameStatePointer& state, int winner);
void setActiveCharacter(GameStatePointer& state, int who, int characterId);
void swapCharacterPosition(GameStatePointer& state, int who, int firstId, int secondId);

// Adds an entity to an area (`value.id` 0 takes the next id); returns the id. `targetIndex` out of range appends.
int createEntity(GameStatePointer& state, const EntityArea& area, EntityState value, std::optional<int> targetIndex = std::nullopt);
int createAttachment(GameStatePointer& state, int cardId, EntityState value);
// Records the creation of an entity already in `state` as a pending notification mutation (createEntity does this
// itself; this is for entities placed before a game's notifications start, like a stage's starting equipment).
void recordEntityCreated(const GameStatePointer& state, const EntityArea& area, int id);
// Moves an entity (its latest state) from wherever it is into `area`. `passedValue` is the entity as the caller
// handed it to the core's mutation when that differs from the latest state (taken before its variables were reset
// or its attachments removed): previews and notifications show that object, as the core does.
void moveEntity(GameStatePointer& state, int id, const EntityArea& area, std::optional<int> targetIndex = std::nullopt,
                MoveReason reason = MoveReason::Other, const EntityState* passedValue = nullptr);
// Removes an entity or attachment and appends its latest state to `fromWho`'s removed entities.
void removeEntity(GameStatePointer& state, int id, int fromWho, RemoveReason reason = RemoveReason::Other);
// Sets a variable; values outside int32 are skipped (as the core does). Returns the old value (0 when unset).
double modifyEntityVariable(GameStatePointer& state, int id, Name name, double value, int direction = 0);
void transformCharacterDefinition(GameStatePointer& state, int id, const CharacterDefinition* definition);
void transformEntityDefinition(GameStatePointer& state, int id, const EntityDefinition* definition);
void resetVariables(GameStatePointer& state, int id, bool usagePerRoundOnly);
// Why dice were reset (PbResetDiceReason), for notifications.
enum class DiceReason { Other = 0, Roll = 1, Consume = 2, ElementalTuning = 3, Generate = 4, Convert = 5, Absorb = 6 };
void resetDice(GameStatePointer& state, int who, const std::vector<DiceType>& dice,  // sorted for that player
               DiceReason reason = DiceReason::Other, std::optional<DiceType> conversionTargetHint = std::nullopt);
void setPlayerFlag(GameStatePointer& state, int who, PlayerFlag flag, bool value);
// An extension state equal to the current one leaves the game state unchanged, as immer's produce does in the core,
// unless the setter assigned a new object or array (`replacedContainer`): immer counts that as a change even when
// the contents are equal.
void mutateExtensionState(GameStatePointer& state, int extensionId, JsonValue newState, bool replacedContainer = false);
// Called by an extension setter that assigns a whole new object or array (see mutateExtensionState).
void noteExtensionContainerReplaced();
bool takeExtensionContainerReplaced();
void pushRoundSkillLog(GameStatePointer& state, int callerDefinitionId, int callerId, double skillId);
void removeRoundSkillLog(GameStatePointer& state, int callerDefinitionId, int callerId);
void clearRoundLogs(GameStatePointer& state);
void clearRemovedEntities(GameStatePointer& state);
void pushPhaseDamageLog(GameStatePointer& state, int sourceWho, std::shared_ptr<DamageOrHealEventArg> damageEvent);
void pushPhaseReactionLog(GameStatePointer& state, int viaWho, std::shared_ptr<ReactionEventArg> reactionEvent);
void clearPhaseLogs(GameStatePointer& state);

}  // namespace gitcg
