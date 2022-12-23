import fs from 'fs';
import path from 'path';
import { execSync } from 'child_process';

const zip = process.env['ZIP_BIN'] || "zip"
const dirname = path.dirname(process.argv0);  // __dirname

// function copyFiles(srcDir, destDir, files) {
// 	files.map(f => {
// 		fs.copyFileSync(path.join(srcDir, f), path.join(destDir, f));
// 	});
// }

export default function makeDistro(platform = null, dirs = {}, buildInfo = null)
{
	const root = dirs.root || path.resolve(dirname, "../");
	const dist = dirs.dist || path.join(root, "dist");
	const build = dirs.build || path.join(dist, "Release");
	//const bins = dirs.bins || path.join(build, "bin");
	platform = platform || process.platform === "win32" ? "Windows" : process.platform === "darwin" ? "MacOS" : "Linux";
	buildInfo = buildInfo || JSON.parse(fs.readFileSync("./version.json", "utf8"));

	const packageName = path.join(dist, `${buildInfo.SYSTEM_NAME}-${platform}-${buildInfo.VERSION_STR}.tpp`);
	var result;

	console.info("Generating entry.tp");
	result = execSync(`node ${path.resolve(dirname, "./gen_entry.js")} -v ${buildInfo.VERSION_STR} -o ${build}`);
	console.info(String(result));

	console.info("Creating archive", packageName);
	result =  execSync(`${zip} -FS -r ${packageName} . -x *.log`,    { cwd: build	});
	result += execSync(`${zip} ${packageName} ./*.md ./LICENSE.txt`, { cwd: root	});
	console.info(String(result));

	console.info("Finished!");
}

makeDistro();
