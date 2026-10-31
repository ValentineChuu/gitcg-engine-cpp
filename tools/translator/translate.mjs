// GTS attributes to C++ input trees. Lambdas and queries stay opaque, typed binding slots.
import {mkdirSync,writeFileSync,readFileSync,existsSync} from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {moduleGraph,ts,property,positionals,attributeChildren,unwrap,root,dataRoot,outputRoot} from './compiler.mjs';
function writeIfChanged(file, text) {
  if(!existsSync(file) || readFileSync(file,'utf8')!==text)writeFileSync(file,text);
}
const graph=moduleGraph(), slots=[], slotIds=new Map(), cache=new Map();
const enums={
  DiceType:{Void:0,Cryo:1,Hydro:2,Pyro:3,Electro:4,Anemo:5,Geo:6,Dendro:7,Omni:8,Aligned:8,Energy:9,Legend:10},
  DamageType:{Physical:0,Cryo:1,Hydro:2,Pyro:3,Electro:4,Anemo:5,Geo:6,Dendro:7,Piercing:8,Heal:9},
  Aura:{None:0,Cryo:1,Hydro:2,Pyro:3,Electro:4,Dendro:7,CryoDendro:113},
  Reaction:{Melt:101,Vaporize:102,Overloaded:103,Superconduct:104,ElectroCharged:105,Frozen:106,SwirlCryo:107,SwirlHydro:108,SwirlPyro:109,SwirlElectro:110,CrystallizeCryo:111,CrystallizeHydro:112,CrystallizePyro:113,CrystallizeElectro:114,Burning:115,Bloom:116,Quicken:117,LunarElectroCharged:3001,LunarBloom:3002,LunarCrystallizeHydro:3003}
};
function query(node) {
  node=unwrap(node);
  if(ts.isIdentifier(node))return node.text==='$';
  if(ts.isPropertyAccessExpression(node)||ts.isElementAccessExpression(node)||ts.isCallExpression(node))return query(node.expression);
  return false;
}
// Source maps use base64 VLQ deltas, with the original coordinates carried between generated lines.
const mappingCache=new WeakMap();
function sourcePosition(module,node) {
  if(!mappingCache.has(module)) {
    const alphabet='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    let originalLine=0,originalColumn=0,source=0;
    const entries=[];
    for(const [line,text] of (module.sourceMap?.mappings.split(';')??[]).entries()) {
      let column=0;
      for(const segment of text.split(',')) {
        if(!segment)continue;
        const values=[];let value=0,shift=0;
        for(const c of segment) {const digit=alphabet.indexOf(c);value|=(digit&31)<<shift;if(digit&32){shift+=5;}else{values.push(value&1?-(value>>1):value>>1);value=0;shift=0;}}
        column+=values[0];
        if(values.length>=4){source+=values[1];originalLine+=values[2];originalColumn+=values[3];entries.push({offset:module.ast.getPositionOfLineAndCharacter(line,column),line:originalLine+1,column:originalColumn+1});}
      }
    }
    mappingCache.set(module,entries);
  }
  const start=node.getStart(),end=node.end,entries=mappingCache.get(module);
  // Compiler-injected lambda parameters may be unmapped: use the first mapped card token in that slot.
  let mapped=entries.find(entry=>entry.offset>=start && entry.offset<end);
  if(!mapped)for(const entry of entries) {if(entry.offset>start)break;mapped=entry;}
  const generated=module.ast.getLineAndCharacterOfPosition(start);
  if(!mapped)throw Error(`No source position for ${module.file}:${generated.line+1}`);
  return {gts:{line:mapped.line,column:mapped.column},generated:{line:generated.line+1,column:generated.character+1,offset:start,end}};
}
function slot(module,node,kind) {
  const key=module.file+':'+node.getStart()+':'+node.end;
  if(!slotIds.has(key)) {
    const id=slots.length+1;slotIds.set(key,id);
    slots.push({id,kind,file:path.relative(dataRoot,module.file),...sourcePosition(module,node),source:node.getText(module.ast)});
  }
  return {$lambda:slotIds.get(key)};
}
function symbol(module,name) {
  const key=module.file+':'+name;
  if(cache.has(key))return cache.get(key);
  const imported=module.imports.get(name);
  if(imported) {const dep=graph.modules.get(imported.file);const result=symbol(dep,dep.exports.get(imported.name)??imported.name);cache.set(key,result);return result;}
  const node=module.declarations.get(name);
  if(!node)throw Error(`Unresolved identifier ${name} in ${module.file}`);
  const result=evaluate(module,node);cache.set(key,result);return result;
}
function bindings(module,node) {
  const found=[];
  function visit(attribute) {
    if(property(attribute,'binding')) {
      const name=property(attribute,'name').text;
      const first=positionals(attribute)[0];
      if(name==='id'||name==='idHint')found.push(evaluate(module,first)+(name==='idHint'?50000000:0));
      else throw Error(`Unknown binding ${name}`);
    }
    attributeChildren(attribute).forEach(visit);
  }
  visit(node);return found;
}
function evaluate(module,expression) {
  const node=unwrap(expression);
  if(ts.isArrowFunction(node)||ts.isFunctionExpression(node))return slot(module,node,'function');
  if(query(node))return slot(module,node,'query');
  if(ts.isNumericLiteral(node))return Number(node.text.replaceAll('_',''));
  if(ts.isStringLiteral(node)||ts.isNoSubstitutionTemplateLiteral(node))return node.text;
  if(node.kind===ts.SyntaxKind.TrueKeyword)return true;
  if(node.kind===ts.SyntaxKind.FalseKeyword)return false;
  if(node.kind===ts.SyntaxKind.NullKeyword)return null;
  if(ts.isPrefixUnaryExpression(node)) {const v=evaluate(module,node.operand);if(node.operator===ts.SyntaxKind.MinusToken)return -v;if(node.operator===ts.SyntaxKind.PlusToken)return +v;}
  if(ts.isArrayLiteralExpression(node))return node.elements.map(e=>evaluate(module,e));
  if(ts.isObjectLiteralExpression(node))return Object.fromEntries(node.properties.map(p=>{
    if(!ts.isPropertyAssignment(p))throw Error(`Unsupported object member ${p.getText()}`);
    return [p.name.text,evaluate(module,p.initializer)];
  }));
  if(ts.isIdentifier(node)) {if(node.text==='undefined')return null;return symbol(module,node.text);}
  if(ts.isElementAccessExpression(node))return evaluate(module,node.expression)[evaluate(module,node.argumentExpression)];
  if(ts.isPropertyAccessExpression(node)) {
    if(enums[node.expression.getText()] && Object.hasOwn(enums[node.expression.getText()],node.name.text))return enums[node.expression.getText()][node.name.text];
    const value=evaluate(module,node.expression);if(Object.hasOwn(value,node.name.text))return value[node.name.text];
  }
  if(ts.isCallExpression(node)) {
    const name=node.expression.getText();
    if(name==='__gts_createBinding')return bindings(module,module.declarations.get(node.arguments[1].text));
    if(name==='pair') {const value=evaluate(module,node.arguments[0]);return [structuredClone(value),structuredClone(value)];}
    if(name==='customEvent')return {$event:node.arguments.length?evaluate(module,node.arguments[0]):'',identity:path.relative(dataRoot,module.file)+':'+node.getStart()};
    if(name==='typeHint')return {$typeHint:true};
    if(name==='type' || name.startsWith('type.'))return {$schema:node.getText()};
  }
  throw Error(`Unsupported ${ts.SyntaxKind[node.kind]} in ${module.file}: ${node.getText()}`);
}
function attribute(module,node) {
  return {name:property(node,'name').text,positionals:positionals(node).map(e=>evaluate(module,e)),attributes:attributeChildren(node).map(n=>attribute(module,n))};
}
const manifest=[];
for(const module of graph.order.filter(m=>m.file.endsWith('.gts'))) {
  // Record nested arrows and query expressions as well, excluding only actual compiler positional thunks.
  const thunks=new Set();
  function collectThunks(node) {thunks.add(property(node,'positionals'));attributeChildren(node).forEach(collectThunks);}
  for(const [name,node] of module.declarations)if(name.startsWith('__gts_node_'))collectThunks(node);
  function visit(node) {
    if(ts.isImportDeclaration(node) || ts.isExportDeclaration(node))return;
    if(ts.isArrowFunction(node) && !thunks.has(node))slot(module,node,'function');
    if(query(node) && !(node.parent && query(node.parent) && node.parent.expression===node))slot(module,node,'query');
    ts.forEachChild(node,visit);
  }
  visit(module.ast);
  const definitions=[];
  for(const statement of module.ast.statements) if(ts.isExpressionStatement(statement)&&ts.isCallExpression(statement.expression)&&statement.expression.expression.getText()==='__gts_createDefine') {
    definitions.push(attribute(module,module.declarations.get(statement.expression.arguments[1].text)));
  }
  const file=path.relative(dataRoot,module.file), functionName='register_'+createHash('sha256').update(file).digest('hex').slice(0,16);
  const output=path.join(outputRoot,file+'.cpp');mkdirSync(path.dirname(output),{recursive:true});
  writeIfChanged(output,`// Generated from ${file}; AGPL-3.0. Run tools/translator/translate.mjs to regenerate.\n#include "definitions/registry/registry.h"\n\nnamespace gitcg {\nvoid ${functionName}(DefinitionRegistry& registry) {\n  registry.addModule(R"GTS(${JSON.stringify(definitions)})GTS");\n}\n}  // namespace gitcg\n`);
  manifest.push({file,functionName,definitions:definitions.length,sha256:createHash('sha256').update(module.source).digest('hex')});
}
writeIfChanged(path.join(outputRoot,'register_all.cpp'),`// Generated registration order: imports first, depth first, each module once. AGPL-3.0.\n#include "definitions/registry/registry.h"\n\nnamespace gitcg {\n${manifest.map(m=>`void ${m.functionName}(DefinitionRegistry&);`).join('\n')}\nvoid registerAllDefinitions(DefinitionRegistry& registry) {\n${manifest.map(m=>`  ${m.functionName}(registry);  // ${m.file}`).join('\n')}\n}\n}  // namespace gitcg\n`);
writeIfChanged(path.join(outputRoot,'lambda_slots.json'),JSON.stringify(slots,null,2)+'\n');
writeIfChanged(path.join(outputRoot,'modules.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(`Translated ${manifest.length} modules, ${manifest.reduce((n,m)=>n+m.definitions,0)} definitions, ${slots.length} lambda slots.`);
