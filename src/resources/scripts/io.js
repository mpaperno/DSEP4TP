/*
Dynamic Script Engine Plugin for Touch Portal
Copyright Maxim Paperno; all rights reserved.

This file may be used under the terms of the GNU
General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at <http://www.gnu.org/licenses/>.

This project may also use 3rd-party Open Source software under the terms
of their respective licenses. The copyright notice above does not apply
to any 3rd-party components used within.
*/


if (!File.readAsync) {
	File.readAsync = function(file, mode, cb) {
		if (typeof cb === 'function' || typeof mode === 'function')
			return File.readAsyncCb(file, mode, cb);
		return new Promise((resolve, reject) => {
			File.readAsyncCb(file, mode, (err, data) => {
				if (err) reject(err);
				else resolve(data);
			});
		});
	}
}

if (!File.writeAsync) {
	File.writeAsync = function(file, data, mode, cb) {
		if (typeof cb === 'function' || typeof mode === 'function')
			return File.writeAsyncCb(file, data, mode, cb);
		return new Promise((resolve, reject) => {
			File.writeAsyncCb(file, data, mode, (err, data) => {
				if (err) reject(err);
				else resolve(data);
			});
		});
	}
}

if (!File.copyAsync) {
	File.copyAsync = function(from, to, mode, cb) {
		if (typeof cb === 'function' || typeof mode === 'function')
			return File.copyAsyncCb(from, to, mode, cb);
		return new Promise((resolve, reject) => {
			File.copyAsyncCb(from, to, mode, (err, data) => {
				if (err) reject(err);
				else resolve(data);
			});
		});
	}
}

if (!File.renameAsync) {
	File.renameAsync = function(from, to, mode, cb) {
		if (typeof cb === 'function' || typeof mode === 'function')
			return File.renameAsyncCb(from, to, mode, cb);
		return new Promise((resolve, reject) => {
			File.renameAsyncCb(from, to, mode, (err, data) => {
				if (err) reject(err);
				else resolve(data);
			});
		});
	}
}

if (!File.removeAsync) {
	File.removeAsync = function(file, cb) {
		if (typeof cb === 'function')
			return File.removeAsyncCb(file, cb);
		return new Promise((resolve, reject) => {
			File.removeAsyncCb(file, (err, data) => {
				if (err) reject(err);
				else resolve(data);
			});
		});
	}
}
