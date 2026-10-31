#include "bridge/worker/worker.h"

#include <cstdio>
#include "bridge/search/engine_work.h"

#include "bridge/session/seeded_shuffle.h"
#include "core/game/initial_state.h"
#include "core/json/json_text.h"
#include "core/mutations/state_mutations.h"
#include "definitions/attributes/lambda_slots.h"
#include "programs/support/load_game_state.h"

namespace gitcg {

namespace {

std::vector<int> intList(const JsonValue& values) {
  std::vector<int> result;
  for (const JsonValue& value : values.asArray()) result.push_back(value.asInt());
  return result;
}

}  // namespace

void Worker::emit(const JsonValue& message) {
  ++emittedMessageCount;
  std::string text = writeJson(message);
  text.push_back('\n');
  std::fwrite(text.data(), 1, text.size(), stdout);
}

void Worker::create(const JsonValue& message) {
  int gameId = message.at("game").asInt();
  try {
    double seed = message.at("seed").asNumber();
    SeededRandom random(seed);
    DeckList decks[2];
    for (int who = 0; who < 2; ++who) {
      const JsonValue& deck = message.at("decks").asArray().at(who);
      decks[who].characters = intList(deck.at("characters"));
      decks[who].cards = seededShuffle(intList(deck.at("cards")), random);
    }
    GameStatePointer state = createInitialState(data_, decks[0], decks[1], static_cast<int64_t>(seed));
    if (const auto* snapshot = message.find("initialState")) {
      if (!allowTestStates_) throw std::runtime_error("initialState requires --test-snapshots");
      state = testing::loadGameState(data_, *snapshot);
    }
    if (const JsonValue* equipment = message.find("equipment")) {
      for (const JsonValue& item : equipment->asArray()) {
        int who = item.at("who").asInt();
        const EntityDefinition* definition = data_.entities.find(item.at("definitionId").asInt());
        if (!definition) throw std::runtime_error("unknown equipment definition");
        const CharacterPointer& character = state->players[who]->characters.at(item.at("characterIndex").asInt());
        EntityState equipped;
        equipped.definition = definition;
        for (const auto& [name, config] : definition->varConfigs) equipped.variables.set(name, config.initialValue);
        createEntity(state, EntityArea{AreaType::Characters, who, character->id, 0}, std::move(equipped));
      }
    }
    auto session = std::make_unique<GameSession>(
        gameId, state, scheduler_, [this](const JsonValue& out) { emit(out); },
        [this](GameSession& waiting, int who) { queuedDecisions_.emplace_back(waiting.gameId(), who); });
    auto enabled = [&](const char* key) { const auto* value = message.find(key); return value && value->isBool() && value->asBool(); };
    std::optional<int> humanSeat;
    if (const JsonValue* value = message.find("humanSeat")) humanSeat = value->asInt();
    session->configure(enabled("previews"), enabled("replies"), message.find("replyViews"), seed,
                       enabled("privileged"), humanSeat);
    GameSession& started = *session;
    sessions_[gameId] = std::move(session);
    started.start();
  } catch (const std::exception& error) {
    JsonValue failed = JsonValue::object();
    failed["type"] = "failed";
    failed["game"] = gameId;
    failed["message"] = std::string(error.what());
    emit(failed);
  }
}

void Worker::settle() {
  scheduler_.runUntilIdle();
  for (const auto& [gameId, who] : queuedDecisions_) {
    auto session = sessions_.find(gameId);
    if (session == sessions_.end() || session->second->done()) continue;
    if (auto notification = session->second->notificationMessage()) emit(*notification);
    emit(session->second->decisionMessage(who));
  }
  queuedDecisions_.clear();
  std::erase_if(sessions_, [](const auto& entry) { return entry.second->done(); });
  std::fflush(stdout);
}

void Worker::copy(const JsonValue& message) {
  int copyId = message.at("copy").asInt();
  try {
    auto source = sessions_.find(message.at("game").asInt());
    if (source == sessions_.end()) throw std::runtime_error("copy for unknown game");
    auto session = source->second->copy(copyId, message.at("who").asInt(), message.at("response"),
                                        message.at("seed").asNumber(), message.find("opponentCards"));
    GameSession& started = *session;
    sessions_[copyId] = std::move(session);
    started.start();
  } catch (const std::exception& error) {
    JsonValue failed = JsonValue::object();
    failed["type"] = "failed";
    failed["game"] = copyId;
    failed["message"] = std::string(error.what());
    emit(failed);
  }
}

bool Worker::handleLine(const std::string& line) {
  JsonValue message = parseJson(line);
  const std::string& type = message.at("type").asString();
  if (type == "create") {
    create(message);
  } else if (type == "copy") {
    copy(message);
  } else if (type == "replies") {
    int gameId = message.at("game").asInt();
    try {
      auto session = sessions_.find(gameId);
      if (session == sessions_.end()) throw std::runtime_error("replies for unknown game");
      session->second->computeReplies(message.at("who").asInt(), message.at("indices"));
    } catch (const std::exception& error) {
      JsonValue failed = JsonValue::object(); failed["type"] = "failed"; failed["game"] = gameId; failed["message"] = error.what(); emit(failed);
    }
  } else if (type == "respond") {
    int gameId = message.at("game").asInt();
    auto session = sessions_.find(gameId);
    if (session == sessions_.end() || !session->second->respond(message.at("who").asInt(), message.at("response"))) {
      JsonValue failed = JsonValue::object();
      failed["type"] = "failed";
      failed["game"] = gameId;
      failed["message"] = "respond for unknown game";
      emit(failed);
    }
  } else if (type == "stop") {
    auto session = sessions_.find(message.at("game").asInt());
    if (session != sessions_.end()) session->second->stop();
  } else if (type == "shutdown") {
    reportEngineWork();
    std::fflush(stdout);
    return false;
  }
  settle();
  return true;
}

}  // namespace gitcg
