// Runs Doxygen to generate HTML docs.  doxygen[.exe] must either be in PATH already,
// or environment variable DOXYGEN_HOME should be set (expects binary to be at `DOXYGEN_HOME/bin/doxygen[.exe]`).
// If invoked as a module then the full path to doxygen[.exe] (including the executable name) can be passed in 2nd argument to `generate()`.
// -ng : no generate
// -m : mirror to gh-pages folder

import { readdirSync, rmSync, cpSync } from 'fs';
import { resolve, join, dirname } from 'path';
import { execSync } from 'child_process';

const script_dir = process.argv0.indexOf("node") > -1 ? dirname(process.argv[1]) : dirname(process.argv0);  // __dirname

export default function generate(doc_path = null, doxygen = null)
{
	doc_path = doc_path || resolve(script_dir, "../doc/doxygen");
	doxygen = doxygen || process.env['DOXYGEN_HOME'] ? join(process.env['DOXYGEN_HOME'], 'bin', 'doxygen') : 'doxygen';
	const result = execSync(`${doxygen} -s Doxyfile`, { cwd: doc_path	});
	console.log(String(result));
}

export function mirror(from_path = null, to_path = null)
{
	from_path = from_path || resolve(script_dir, "../doc/doxygen/html");
	to_path = to_path || resolve(script_dir, "../../gh-pages");
  readdirSync(to_path, { withFileTypes: true }).forEach(f => { if (f.isFile()) rmSync(join(to_path, f.name), { force: true }) });
  rmSync(join(to_path, "search"), { recursive : true});
	cpSync(from_path, to_path, { dereference: true, force: false, preserveTimestamps: true, recursive: true });
}

var gen = true,
	mir = false;

// Handle CLI arguments
for (let i=2; i < process.argv.length; ++i) {
    const arg = process.argv[i];
    if      (arg == "-ng") gen = false;
    else if (arg == "-m") mir = true;
}

if (gen)
	generate();
if (mir)
	mirror();
