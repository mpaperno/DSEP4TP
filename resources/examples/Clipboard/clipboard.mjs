// Since TP currently has no way to copy text values or file contents to or from clipboard,
// this module provides these functions by using operating system features (or `xclip` utility on Linux).

// Copies any string `value` to the clipboard.  Linux requires `xclip` installed.
export function valueToClipboard(value)
{
	var cmd;
	if (DSE.PLATFORM_OS.startsWith("win"))
		cmd = 'cmd /C "@echo.|set /p temp=' + value + '|clip"';
	else if (DSE.PLATFORM_OS === "osx")
		cmd = 'echo -n "' + value + '"|pbcopy';
	// Linux requires 'xclip' installed.
	else if (DSE.PLATFORM_OS === "linux")
		cmd = 'echo -n "' + value + '"|xclip -selection clipboard';
	else
		return;

	runCommand(cmd);
}

// Copies the contents of `file` to the clipboard.  Linux requires `xclip` installed.
export function fileToClipboard(file)
{
	var cmd;
	if (DSE.PLATFORM_OS.startsWith("win"))
		cmd = 'cmd /C "@type ""' + file + '"" | clip"';
	else if (DSE.PLATFORM_OS === "osx")
		cmd = 'cat "' + file + '" | pbcopy';
	// Linux requires 'xclip' installed.
	else if (DSE.PLATFORM_OS === "linux")
		cmd = 'xclip -selection clipboard "' + file + '"';
	else
		return;

	runCommand(cmd);
}

// Returns contents of clipboard as a string value.  Linux requires `xclip` installed.
export function clipboardToValue()
{
	var cmd;
	if (DSE.PLATFORM_OS.startsWith("win"))
		cmd = 'powershell.exe -Command "& { Get-Clipboard -Raw | Write-Host -NoNewLine }"';
	else if (DSE.PLATFORM_OS === "osx")
		cmd = 'pbpaste';
	// Linux requires 'xclip' installed.
	else if (DSE.PLATFORM_OS === "linux")
		cmd = 'xclip -selection clipboard -o';
	else
		return "Unknown operating system!";

	const p = runCommand(cmd);
	return String(p.readAll());
}

// Writes contents of clipboard to `file`.  Linux requires `xclip` installed.
export function clipboardToFile(file)
{
	var cmd;
	if (DSE.PLATFORM_OS.startsWith("win"))
		cmd = 'powershell.exe -Command "& { Get-Clipboard -Raw | Write-Host -NoNewLine }" 1> "' + file + '"';
	else if (DSE.PLATFORM_OS === "osx")
		cmd = 'pbpaste > "' + file + '"';
	// Linux requires 'xclip' installed.
	else if (DSE.PLATFORM_OS === "linux")
		cmd = 'xclip -selection clipboard -o > "' + file + '"';
	else
		return;

	runCommand(cmd);
}

function runCommand(cmd)
{
	const p = new Process();
	p.startCommand(cmd);
	p.waitForFinished();
	return p;
}
