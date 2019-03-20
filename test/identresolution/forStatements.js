let a = []; // decl 1
let b = NaN; // decl 2
 
(function() {
    return a // decl 9
            + b; // decl 2
    
    if (true) {
        for (var a = 0; false; ) {} // decl 9
        for (let b = 0; false; ) {} // decl 10
    }
})

{
    let a = 0; // decl 15
    for (let a = 2; // decl 16
         a < 3; // decl 16
         a += 1) { // decl 16
        (a); // decl 16
    }
    // When multiple vars create the same binding, we pick the earliest one as the target for all references (except for declarations, which always resolve to themselves)
    for (b of [0]) // decl 22
        (b); // decl 2
    for (c in [0]) // decl 24
        (c); // decl 24
    (a); // decl 15
    (b); // decl 2
    (c); // decl 24
}

for (let a in [5]) // decl 31
    console.log(a); // decl 31

for (let a of [5]) // decl 34
    console.log(a); // decl 34
    
(a); // decl 1
