let rec; // decl 1

let recursive // decl 3
            = function rec( // decl 4
                i // decl 5
            ) {
    if (i) // decl 5
        return rec( // decl 4
            i-1 // decl 5
        )+1;
    else
        return 0;
}

let Class; // decl 15
let cls // decl 16
    = class Class { // decl 17
    constructor() {
        new Class(); // decl 17
    }
}

(rec); // decl 1
(Class); // decl 15
