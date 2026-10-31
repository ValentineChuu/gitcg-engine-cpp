// Random-vs-random throughput on NPC decks, single process: the C++ counterpart of reference/benchmark_throughput.mjs
// (same deck filter, same kind of random answers; no JSON, no protocol). Games that reach a card lambda not yet
// translated end there, so decisions per second is the comparable number.
// Usage: benchmark_random_games [gameCount] [deck table] [seed offset] [twelve-process output directory]
// The deck table is Dimbreath's GCGDeckExcelConfigData.json (AutoGITCG's scripts/data/fetch_dimbreath.py downloads
// it). Without the argument it is looked up in GITCG_DATA/raw/, then in the data/raw/ folder of the repository the
// engine is checked out in (AutoGITCG keeps the engine at engine/).

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "core/game/game.h"
#include "core/game/initial_state.h"
#include "core/json/json_text.h"
#include "definitions/attributes/lambda_slots.h"
#include "definitions/registry/registry.h"
#include "programs/support/benchmark_processes.h"

namespace gitcg {
void registerAllDefinitions(DefinitionRegistry& registry);
void bindTranslatedCardCode(LambdaSlots& slots);
void bindExtensionHooks(LambdaSlots& slots);
void bindHandPorts(LambdaSlots& slots);
namespace js {
void useCustomEventRegistry(const DefinitionRegistry* registry);
}
}  // namespace gitcg

using namespace gitcg;

namespace {

// The JS benchmark's generator (in doubles, as JS computes it).
struct BenchmarkRandom {
  double seed;
  double next() {
    seed = std::fmod(seed * 1103515245.0 + 12345.0, 2147483648.0);
    return seed / 2147483648.0;
  }
  template <typename Item>
  const Item& pick(const std::vector<Item>& items) { return items[static_cast<size_t>(next() * items.size())]; }
};

class RandomPlayers : public PlayerIo {
 public:
  explicit RandomPlayers(BenchmarkRandom& random) : random_(random) {}
  long decisions = 0;

  ActionAnswer action(int who, const std::vector<ActionInfo>& actions) override {
    ++decisions;
    std::vector<int> valid;
    for (size_t index = 0; index < actions.size(); ++index) {
      if (actions[index].validity == action_validity::Valid) valid.push_back(static_cast<int>(index));
    }
    int chosen = random_.pick(valid);
    return {chosen, actions[chosen].autoSelectedDice};
  }
  int chooseActive(int who, const std::vector<int>& candidateIds) override { ++decisions; return random_.pick(candidateIds); }
  std::vector<DiceType> rerollDice(int who) override { ++decisions; return {}; }
  std::vector<int> switchHands(int who) override { ++decisions; return {}; }
  int selectCard(int who, const std::vector<int>& candidates) override { ++decisions; return random_.pick(candidates); }

 private:
  BenchmarkRandom& random_;
};

}  // namespace

std::string defaultDeckTable() {
  const char* table = "raw/GCGDeckExcelConfigData.json";
  if (const char* data = std::getenv("GITCG_DATA")) return (std::filesystem::path(data) / table).string();
  std::error_code error;
  auto executable = std::filesystem::read_symlink("/proc/self/exe", error);  // <engine>/build/<mode>/benchmark_random_games
  if (error) return (std::filesystem::path("../data") / table).string();
  return (executable.parent_path() / "../../../data" / table).lexically_normal().string();
}

int main(int argc, char** argv) {
  int gameCount = argc > 1 ? std::stoi(argv[1]) : 200;
  std::string deckPath = argc > 2 ? argv[2] : defaultDeckTable();
  int offset = argc > 3 ? std::stoi(argv[3]) : 0;
  BenchmarkProcesses processes{.offset = offset, .children = {}};
  if (argc > 4) processes = launchBenchmarkProcesses(gameCount, offset, argv[4]);
  offset = processes.offset;
  std::ifstream deckFile(deckPath);
  if (!deckFile) {
    std::fprintf(stderr, "cannot open %s: give the deck table as the second argument or set GITCG_DATA to the folder "
                         "holding raw/GCGDeckExcelConfigData.json\n", deckPath.c_str());
    return 1;
  }
  std::stringstream deckText;
  deckText << deckFile.rdbuf();
  std::vector<DeckList> decks;
  for (const JsonValue& entry : parseJson(deckText.str()).asArray()) {
    const JsonValue* characters = entry.find("characterList");
    const JsonValue* cards = entry.find("cardList");
    if (!characters || !cards || characters->asArray().size() != 3 || cards->asArray().size() != 30) continue;
    DeckList deck;
    for (const JsonValue& id : characters->asArray()) deck.characters.push_back(id.asInt());
    for (const JsonValue& id : cards->asArray()) deck.cards.push_back(id.asInt());
    decks.push_back(deck);
  }

  DefinitionRegistry registry;
  registerAllDefinitions(registry);
  bindTranslatedCardCode(*registry.lambdaSlots());
  bindExtensionHooks(*registry.lambdaSlots());
  bindHandPorts(*registry.lambdaSlots());
  js::useCustomEventRegistry(&registry);
  GameData data = registry.resolve("v7.1.0");

  BenchmarkRandom random{12345};
  RandomPlayers players(random);
  int finished = 0, untranslated = 0, different = 0;
  long rounds = 0;
  std::map<std::string, int> errors;
  auto started = std::chrono::steady_clock::now();
  for (int index = 0; index < gameCount; ++index) {
    random.seed = 12345.0 + offset + index;
    const DeckList& first = random.pick(decks);
    const DeckList& second = random.pick(decks);
    GameStatePointer state = createInitialState(data, first, second, static_cast<int64_t>(random.next() * 2147483648.0));
    Game game(state, players);
    try {
      game.start();
      ++finished;
      rounds += game.state()->roundNumber;
    } catch (const std::exception& error) {
      std::string message = error.what();
      if (message.find("not translated") != std::string::npos) ++untranslated; else ++different;
      ++errors[message.substr(0, 80)];
    }
  }
  double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
  std::printf("cpp: finished %d/%d in %.1fs (%d untranslated, %d other errors)\n", finished, gameCount, seconds, untranslated, different);
  std::printf("games/s %.2f  decisions/s %.0f  avg rounds %.2f\n", finished / seconds, players.decisions / seconds,
              finished ? static_cast<double>(rounds) / finished : 0.0);
  int shown = 0;
  for (const auto& [message, count] : errors) {
    if (shown++ < 5) std::printf("  error x%d: %s\n", count, message.c_str());
  }
  std::printf("{\"backend\":\"cpp\",\"attempted\":%d,\"finished\":%d,\"untranslated\":%d,\"different\":%d,"
              "\"decisions\":%ld,\"rounds\":%ld,\"seconds\":%.6f,\"gamesPerSecond\":%.3f,\"decisionsPerSecond\":%.3f}\n",
              gameCount, finished, untranslated, different, players.decisions, rounds, seconds, finished / seconds, players.decisions / seconds);
  std::fflush(stdout);
  bool childrenPassed = processes.finish();
  return different || !childrenPassed ? 1 : 0;
}
