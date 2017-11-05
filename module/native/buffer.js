(function() {
'use strict';
let exports = {}; 

exports.Buffer = function Buffer(arg, encodingOrOffset, length) {
    const fakeBuf = {}
    fakeBuf.fill = () => {};
    return fakeBuf;
};

return exports;
})()
 
