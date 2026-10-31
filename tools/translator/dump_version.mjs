// Reuse the reference dumper unchanged, selecting an older getData version for registry regression checks.
import {readFileSync} from 'node:fs';
const original=readFileSync(new URL('../../tests/dump_definitions.mjs',import.meta.url),'utf8');
const dataUrl=new URL('../../reference/piovium/packages/data/dist/index.js',import.meta.url);
const source=original.replace('"@gi-tcg/data"',JSON.stringify(dataUrl.href)).replace('getData();',`getData(${JSON.stringify(process.argv[2]??'v7.1.0')});`);
await import('data:text/javascript;base64,'+Buffer.from(source).toString('base64'));
