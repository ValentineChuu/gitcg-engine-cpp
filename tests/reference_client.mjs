// In-process adapter around the real Node GameSession, with the worker's message dispatch. This also works in
// sandboxes that allow native workers but refuse nested Node executable launches.
import getData from '@gi-tcg/data';
import { GameSession } from '../reference/bridge/game_session.mjs';
const data = getData();
export function startReference() {
  const sessions = new Map(), queue = [];
  let waiting;
  function emit(message) {
    if (waiting) { const { resolve, timer } = waiting; waiting = null; clearTimeout(timer); resolve(message); }
    else queue.push(message);
  }
  function start(id, session) { sessions.set(id, session); session.run().finally(() => sessions.delete(id)); }
  return {
    send(message) {
      // A protocol input is an event-loop callback. Drain the current promise reactions first.
      setImmediate(() => {
        const id = message.type === 'copy' ? message.copy : message.game;
        try {
          if (message.type === 'create') {
            const session = GameSession.create(id, message.decks, message.seed, data, emit, message.equipment ?? [], null,
              message.previews === true, message.replies === true, message.replyViews ?? null);
            session.privileged = message.privileged === true; start(id, session);
          } else if (message.type === 'copy') {
            start(id, sessions.get(message.game).copy(id, message.who, message.response, message.seed, message.opponentCards ?? null));
          } else if (message.type === 'respond') sessions.get(id).respond(message.who, message.response);
          else if (message.type === 'replies') sessions.get(id).computeReplies(message.who, message.indices);
          else if (message.type === 'stop') { sessions.get(id)?.stop(); sessions.delete(id); }
        } catch (error) { emit({ type: 'failed', game: id, message: String(error?.stack ?? error) }); }
      });
    },
    next(timeout = 60000) {
      if (queue.length) return Promise.resolve(queue.shift());
      return new Promise((resolve, reject) => { waiting = { resolve, timer: setTimeout(() => { waiting = null; reject(new Error('Node reference timeout')); }, timeout) }; });
    },
    async close() {
      for (const session of sessions.values()) session.stop();
      if (waiting) { clearTimeout(waiting.timer); waiting = null; }
    },
  };
}
