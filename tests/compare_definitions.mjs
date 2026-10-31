// Run the live Node and C++ dumps, preserving array/Map order and reporting the first unequal field.
import {spawnSync} from 'node:child_process';
import {readFileSync,mkdtempSync,openSync,closeSync,rmSync} from 'node:fs';
import {fileURLToPath} from 'node:url';
import path from 'node:path';
import {tmpdir} from 'node:os';
const root=path.resolve(fileURLToPath(new URL('..',import.meta.url)));
const args=process.argv.slice(2);
function option(name,fallback) {const index=args.indexOf(name);return index===-1?fallback:args[index+1];}
function run(command,argv) {
  // Regular files avoid pipe-capacity restrictions in the shared development sandbox.
  const directory=mkdtempSync(path.join(tmpdir(),'gitcg-definitions-'));
  const stdout=path.join(directory,'stdout'),stderr=path.join(directory,'stderr');
  const out=openSync(stdout,'w'),err=openSync(stderr,'w');
  try {
    const result=spawnSync(command,argv,{cwd:root,stdio:['ignore',out,err]});
    if(result.error)throw result.error;
    if(result.status!==0)throw Error(`${command} failed (${result.status}):\n${readFileSync(stderr,'utf8')}`);
    return JSON.parse(readFileSync(stdout,'utf8'));
  } finally {closeSync(out);closeSync(err);rmSync(directory,{recursive:true,force:true});}
}
export function firstDifference(expected,actual,location='$') {
  if(Object.is(expected,actual))return null;
  if(expected===null||actual===null||typeof expected!=='object'||typeof actual!=='object')return {location,expected,actual};
  if(Array.isArray(expected)!==Array.isArray(actual))return {location,expected,actual};
  if(Array.isArray(expected)) {
    const shared=Math.min(expected.length,actual.length);
    for(let i=0;i<shared;i++) {const result=firstDifference(expected[i],actual[i],`${location}[${i}]${expected[i]?.id!==undefined?`(id=${expected[i].id})`:''}`);if(result)return result;}
    if(expected.length!==actual.length)return {location:location+'.length',expected:expected.length,actual:actual.length};
  } else {
    const expectedKeys=Object.keys(expected),actualKeys=Object.keys(actual);
    if(expectedKeys.length!==actualKeys.length||expectedKeys.some(key=>!Object.hasOwn(actual,key)))return {location:location+' keys',expected:expectedKeys,actual:actualKeys};
    for(const key of expectedKeys) {const result=firstDifference(expected[key],actual[key],`${location}.${key}`);if(result)return result;}
  }
  return null;
}
try {
  const memory=spawnSync('free',['-g'],{encoding:'utf8'});
  if(memory.status!==0)throw Error('free -g failed');
  process.stderr.write(memory.stdout);
  const reference=option('--reference',null);
  const version=option('--version','v7.1.0');
  const binary=option('--binary',path.join(root,'build/release/dump_definitions'));
  const expected=reference?JSON.parse(readFileSync(path.resolve(reference),'utf8')):run(process.execPath,version==='v7.1.0'?['tests/dump_definitions.mjs']:['tools/translator/dump_version.mjs',version]);
  const actual=run(binary,[version]);
  const difference=firstDifference(expected,actual);
  if(difference) {
    console.error(`First difference: ${difference.location}\n  Node: ${JSON.stringify(difference.expected)}\n  C++:  ${JSON.stringify(difference.actual)}`);
    process.exitCode=1;
  } else {
    const skills=Object.values(actual).flat().reduce((count,value)=>count+value.skills.length,0);
    console.log(`Definition dumps match exactly (${version}): ${Object.entries(actual).map(([name,values])=>`${values.length} ${name}`).join(', ')}, ${skills} skills.`);
    const nodeBuiltins=run(process.execPath,['tools/translator/probe_builtins.mjs']);
    const cppBuiltins=run(binary,['--builtins',path.join(root,'tools/translator/fixtures/attributes.json')]);
    const builtinDifference=firstDifference(nodeBuiltins,cppBuiltins,'builtins');
    if(builtinDifference) {
      console.error(`First difference: ${builtinDifference.location}\n  Node: ${JSON.stringify(builtinDifference.expected)}\n  C++:  ${JSON.stringify(builtinDifference.actual)}`);
      process.exitCode=1;
    } else console.log(`Executable VM built-ins match: ${Object.keys(cppBuiltins).length} result checks, including late slot binding and unbound-slot failure.`);
  }
} catch(error) {console.error(error.message);process.exitCode=1;}
