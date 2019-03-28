 
var v = 0; // decl 2
let l = 1; // decl 3
const c = 2; // decl 4

function f1(){ // decl 6
  var v = 1; // decl 7
  function f2(){ // decl 8
    return v; // decl 7
  };
  return f2(); // decl 8
}

class C { // decl 14
    method( // Methods (and properties) aren't declarations in any scope. They're keys of the object.
        arg // decl 16
    ) {
        // Function declarations shadow vars during hoisting (and assignment takes priority back, but that's not resolvable statically)
        let y // decl 19
                = x; // decl 23
        var x // decl 21
                = arg; // decl 16
        function x(){}; // decl 23
    }
    
    prop = 2;
}

if (true) {
    let {
        l
                : new_name, // decl 32
        p // decl 33
    } = {
        p: 2,
        l:
            l, // decl 3
    }
}
