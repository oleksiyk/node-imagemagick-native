
var test        = require('tap').test;
var imagemagick = require('..');
var debug       = 0;

process.chdir(__dirname);

function saveToFileIfDebug (buffer, file) {
  if (debug) {
    require('fs').writeFileSync( file, buffer, 'binary' );
    console.log( 'wrote file: '+file );
  }
}

var srcData = require('fs').readFileSync('./test.trim.jpg')

test( 'pipeline sample', function (t) {
  t.plan(1);

  imagemagick.image(srcData)
    .resize({ width: 150, height: 150 })
    .blur(5)
    .flip()
    .rotate(90)
    .composite({ srcData: srcData, gravity: 'NorthGravity', op: 'OverCompositeOp' })
    .flip()
    .outFormat('png')
    .exec(function (err, buffer) {
      t.equal( Buffer.isBuffer(buffer), true, 'buffer is Buffer' );
      saveToFileIfDebug( buffer, 'out.pipeline.png' );
      t.end();
    });

});
