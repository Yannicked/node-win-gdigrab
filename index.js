'use strict'
var path = require('path')
var native_module = require(path.join(__dirname, 'build', 'Release', 'gdi'));

class gdi {
	constructor() {
		this._created = false;
		this.create(...arguments)
	}

	create(){
		if (this._created) return;
		this._created = true;
		native_module.create(...arguments);
	}

	grab() {
		if (!this._created) {
			this._create();
		}
		var da = [];
		var d = native_module.grab();
		for (var i = 0; i<(d.length); i+=4) {
			da.push([d[i+2], d[i+1], d[i]]); // BGRA to RGB
		}
		return da
	}

	destroy() {
		if (!this._created) return;
		this._created = false;
		native_module.destroy();
	}
}
module.exports = gdi;