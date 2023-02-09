// Minifies JavaScript sources into one file.
// Uses babel, which is expected to be either installed in the build tree or globally with NODE_PATH set up.
// Current node global module path can be obtained with `npm root --quiet --location=global`
// Windows: FOR /F "tokens=*" %g IN ('npm root --quiet --location=global') do (SET NODE_PATH=%g)
// POSIX: export NODE_PATH=$(npm root --quiet --location=global)


import { opendirSync, readFileSync, writeFileSync } from "fs";
import { resolve, join, dirname } from "path";
// import { babel } from '@babel/core';  //  ?!?

var NODE_PATH = process.env.NODE_PATH;
if (!NODE_PATH) {
	const { execSync } = await import('child_process');
	NODE_PATH = String(execSync('npm root --quiet --location=global')).trim();
	console.log(`Set NODE_PATH: '${NODE_PATH}'`);
}

const babel = await import('file:///'+ NODE_PATH + '/@babel/core/lib/index.js');

const script_dir = process.argv0.indexOf("node") > -1 ? dirname(process.argv[1]) : dirname(process.argv0);

export default function minify(src = null, dst = null)
{
  src = src || resolve(script_dir, "../src/resources/scripts/");
  dst = dst || join(src, "jslib.min.js");

  const dir = opendirSync(src);
  let dirent;
  let code = "";
  while ((dirent = dir.readSync()) !== null) {
    if (!dirent.name.endsWith('.js') || dirent.name.endsWith('.min.js') /*|| dirent.name == 'global.js'*/)
      continue;
    const fn = join(src, dirent.name);
    console.log("Reading source file " + fn);
    const js = readFileSync(fn).toString();
    const minified = babel.transform(js,
    {
      presets: [["babel-preset-minify", { builtIns: false }]],
      plugins: ["@babel/plugin-syntax-import-meta"],
      comments: false
    });
    code += '\n' + minified.code;
  }

  writeFileSync(dst, code);

  console.log("Wrote minified source to " + dst);
}

minify();
