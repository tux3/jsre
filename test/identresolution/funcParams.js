
var a = 1; // decl 2
let b = 2; // decl 3

function f( // decl 5
    x, // decl 6
    y // decl 7
        =a // decl 2
) {
    const a // decl 10
            = y; // decl 14
    return a  // decl 10
            + y; // decl 14
    var y = 2; // decl 14
}

function h( // decl 17
    a // decl 18
        =a // decl 18
) {
    return a; // decl 18
}

function g( // decl 24
    x // decl 25
        = h( // decl 17
            b // decl 3
        )
) {
    var b = "foo"; // decl 30
}
