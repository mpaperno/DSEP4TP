import fs from 'fs';
import path from 'path';
import { execSync } from 'child_process';

const zip = process.env['ZIP_BIN'] || "zip"
const dirname = process.argv0.indexOf("node") > -1 ? dirname(process.argv[1]) : dirname(process.argv0);

function copyFiles(srcDir, destDir, files) {
	files.map(f => {
		fs.copyFileSync(path.join(srcDir, f), path.join(destDir, f));
	});
}

export default function makeDistro(platform = null, dirs = {}, buildInfo = null)
{
	buildInfo = buildInfo || JSON.parse(fs.readFileSync(path.join(dirname, "version.json"), "utf8"));
	const root = dirs.root || path.resolve(dirname, "../");
	const dist = dirs.dist || path.join(root, "dist");
	//const bins = dirs.bins || path.join(build, "bin");
	platform = platform || buildInfo.PLATFORM_OS || (process.platform === "win32" ? "Windows" : process.platform === "darwin" ? "MacOS" : "Linux");
	const build = dirs.build || path.join(dist, platform, buildInfo.SYSTEM_NAME);

	const packageName = path.join(dist, `${buildInfo.SYSTEM_NAME}-${platform}-${buildInfo.VERSION_STR}.tpp`);
	var result;

	console.info("Generating entry.tp");
	result = execSync(`node ${path.resolve(dirname, "./gen_entry.js")} -v ${buildInfo.VERSION_STR} -o ${build}`);
	console.info(String(result));

	console.info("Copying files to", build);
	copyFiles(root, build, [ 'README.md', 'CHANGELOG.md', 'LICENSE.txt' ])

	console.info("Creating archive", packageName);
	result =  execSync(`${zip} -FS -r ${packageName} . -x *.log`, { cwd: path.join(build, "..") });
	console.info(String(result));

	console.info("Finished!");
}

makeDistro();
