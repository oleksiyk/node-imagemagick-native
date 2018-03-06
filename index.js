module.exports = require(__dirname + '/build/Release/imagemagick.node');

// Idea for stream conversions from
// http://codewinds.com/blog/2013-08-20-nodejs-transform-streams.html
var stream = require('stream');
var util = require('util');

function Convert(options) {
  // allow use without new
  if (!(this instanceof Convert)) {
    return new Convert(options);
  }

  this._options = options;
  this._bufs = [];

  // init Transform
  stream.Transform.call(this, options);
}
util.inherits(Convert, stream.Transform);

Convert.prototype._transform = function (chunk, enc, done) {
  this._bufs.push(chunk);
  done();
};

Convert.prototype._flush = function(done) {
  this._options.srcData = Buffer.concat(this._bufs);
  var self = this;
  module.exports.convert(this._options,function(err,data){
    if(err) {
      return done(err);
    }
    self.push(data);
    done();
  });
}

module.exports.streams = { convert : Convert };

function promisify(func) {
  return function(options) {
    return new Promise((resolve, reject) => {
      func(options, function(err, buff) {
        if (err) {
          return reject(err);
        }
        resolve(buff);
      });
    });
  };
}

module.exports.promises = {
  convert: promisify(module.exports.convert),
  identify: promisify(module.exports.identify),
  composite: promisify(module.exports.composite),
};

var commandIds = require('./pipeline').commands;

module.exports.image = function image(srcData) {
  var obj = {
    srcData: srcData,
    debug: true,
    cmds: [],
    sourceFormat: function sourceFormat(fmt) {
      this.srcFormat = fmt;
      return this;
    },
    outFormat: function outFormat(fmt) {
      this.format = fmt;
      return this;
    },
    exec: function exec(callback) {
      if (!callback) {
        return new Promise(function (resolve, reject) {
          module.exports.pipeline(this, function (err, buff) {
            if (err) {
              return reject(err);
            }
            resolve(buff);
          });
        });
      }
      module.exports.pipeline(this, callback);
    }
  }

  for (let key of Object.keys(commandIds)) {
    obj[key] = function(options) {
      this.cmds.push({ id: commandIds[key], options });
      return obj;
    }
  }

  return obj;
};
