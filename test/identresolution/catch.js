let x = 2; // decl 1
(x); // decl 1

try {
    let x = 3; // decl 5
    (x); // decl 5
} catch(x) { // decl 7
    (x); // decl 7
    if (true) {
        let x = 3; // decl 10
        (x); // decl 10
    }
    
    var leaky; // decl 14
}

let e = 0; // decl 17
try {} catch(e) { // decl 18
    (leaky); // decl 14
}
(e); // decl 17
