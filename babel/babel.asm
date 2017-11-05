global babelScriptStart, babelScriptSize
default rel

section .rodata

babelScriptStart:
    incbin "babel.js"

babelScriptSize:
    dd $-babelScriptStart
