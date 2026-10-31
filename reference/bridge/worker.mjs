// Engine worker process driven by Python over stdio, one JSON message per line.
//
// Python -> worker:
//   {"type": "create", "game": id, "decks": [{characters, cards}, {characters, cards}], "seed": int,
//    "equipment": [{who, characterIndex, definitionId}] (optional, equipped before the game starts),
//    "privileged": true (optional, training analysis only: every decision's state also carries opponentPrivate),
//    "humanSeat": 0|1 (optional: that seat is a person on a web board; it gets "notify" messages and action previews),
//    "compactNotifications": true (optional with humanSeat, C++ worker only: notifications holding only handleEvent
//                                  markers are merged into the next one; the same mutations, fewer messages),
//    "previews": true (optional: every action carries the engine's preview, for networks that read it; about 2x slower),
//    "replies": true (optional: every action also carries the opponent's possible replies, opponent_replies.mjs; implies previews),
//    "replyViews": [view or null, view or null] (optional with replies: per seat, its view of the opponent's cards, null = no replies)}
//   {"type": "respond", "game": id, "who": 0|1, "response": {"$case": ..., "value": ...}}
//   {"type": "replies", "game": id, "who": 0|1, "indices": [action indices]}  (a decision with repliesOnDemand: check these)
//   {"type": "copy", "game": id, "copy": new id, "who": 0|1, "response": {"$case": "action", "value": ...}, "seed": int,
//    "opponentCards": [definition id, ...] (optional: a guess for the opponent's hand and deck)}
//     (look-ahead: a copy of the game at `who`'s pending action decision, hidden cards re-dealt, which plays
//      `response` and then asks for every later decision like a normal game; see bridge/game_session.mjs)
//   {"type": "stop", "game": id} (ends a game silently)
//   {"type": "shutdown"}
// Worker -> Python:
//   {"type": "decision", "game": id, "who": 0|1, "request": {"$case": ..., "value": ...}, "state": <exposed state>,
//    "counters": {<board entity id>: {usage, duration, ...}}, "previews": true (only in games created with previews),
//    "replies": [per action: null (invalid) or {next: "opponent"|"self"|"other"|"finished", previews: [[...], ...],
//                stopped: true (a fast action: previews are after ending the turn there), cardPreviews: [[...], ...],
//                cards: "open"|"guessed"}] (replies on, the seat has a view and more than one valid action),
//    "repliesOnDemand": true (instead of "replies" for a view with onDemand: send a "replies" message, then answer)}
//   {"type": "replies", "game": id, "who": 0|1, "replies": [as above, null for actions not checked], "checked": [indices]}
//   {"type": "finished", "game": id, "winner": 0|1|null, "rounds": int, "defeated": [int, int] (characters defeated per player),
//    "reactions": [{code: count}, {code: count}] (per player), "reactionCauses": [{"code|caller type|skill type": count}, ...],
//    "damageDealt": [int, int]}
//   {"type": "failed", "game": id, "message": str}
//   {"type": "notify", "game": id, "who": 0|1, "notification": <engine Notification>} (humanSeat only)

import { createInterface } from "node:readline";
import getData from "@gi-tcg/data";
import { reportEngineWork } from "./engine_work.mjs";
import { GameSession } from "./game_session.mjs";

const data = getData();
const sessions = new Map();

function emit(message) {
  process.stdout.write(JSON.stringify(message) + "\n");
}

function handle(message) {
  switch (message.type) {
    case "create": {
      let session;
      try {
        session = GameSession.create(message.game, message.decks, message.seed, data, emit, message.equipment ?? [],
                                     message.humanSeat ?? null, message.previews === true, message.replies === true,
                                     message.replyViews ?? null);
        session.privileged = message.privileged === true;
      } catch (error) {
        emit({ type: "failed", game: message.game, message: String(error?.stack ?? error) });
        return;
      }
      sessions.set(message.game, session);
      session.run().finally(() => sessions.delete(message.game));
      return;
    }
    case "copy": {
      let session;
      try {
        session = sessions.get(message.game).copy(message.copy, message.who, message.response, message.seed, message.opponentCards ?? null);
      } catch (error) {
        emit({ type: "failed", game: message.copy, message: String(error?.stack ?? error) });
        return;
      }
      sessions.set(message.copy, session);
      session.run().finally(() => sessions.delete(message.copy));
      return;
    }
    case "stop": {
      sessions.get(message.game)?.stop();
      sessions.delete(message.game);
      return;
    }
    case "replies": {
      const session = sessions.get(message.game);
      if (!session) {
        emit({ type: "failed", game: message.game, message: "replies for unknown game" });
        return;
      }
      try {
        session.computeReplies(message.who, message.indices);
      } catch (error) {
        emit({ type: "failed", game: message.game, message: String(error?.stack ?? error) });
      }
      return;
    }
    case "respond": {
      const session = sessions.get(message.game);
      if (!session) {
        emit({ type: "failed", game: message.game, message: "respond for unknown game" });
        return;
      }
      session.respond(message.who, message.response);
      return;
    }
    case "shutdown":
      reportEngineWork();
      process.exit(0);
  }
}

const lines = createInterface({ input: process.stdin });
lines.on("line", (line) => handle(JSON.parse(line)));
lines.on("close", () => process.exit(0));
