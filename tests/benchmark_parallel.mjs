// Twelve Node engine processes total: this parent performs one shard and waits for its eleven children.
import { spawn } from 'node:child_process';
import { openSync, closeSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { benchmarkGames } from './benchmark_games.mjs';

const games = Number(process.argv[2]), directory = process.argv[3];
const children = [];
for (let index = 0; index < 11; ++index) {
  const output = openSync(join(directory, `${index}.log`), 'w');
  const child = spawn(process.execPath, [new URL('./benchmark_games.mjs', import.meta.url).pathname,
    String(games), String(index * games)], { stdio: ['ignore', output, output] });
  closeSync(output);
  children.push(new Promise((resolve, reject) => {
    child.on('error', reject);
    child.on('exit', (code, signal) => resolve(code === 0 && signal === null));
  }));
}
const result = await benchmarkGames(games, 11 * games);
writeFileSync(join(directory, '11.log'), JSON.stringify(result) + '\n');
const finished = await Promise.all(children);
if (!finished.every(Boolean)) process.exitCode = 1;
