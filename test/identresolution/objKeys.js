
var x = 0; // decl 2
var y = 1; // decl 3

var obj1 = { // decl 5
    x: "obj",
    z:
        z, // decl 17
};

var obj2 = { // decl 11
    x, // decl 2
    [y]: // decl 3
        obj1,  // decl 5
};

var z = 2; // decl 17

let objMethods = { // decl 19
    method( // decl 20
        i // decl 21
    ) {
        if (i) { // decl 21
            method( // decl 20
                i-1 // decl 21
            )
        }
    }
}
