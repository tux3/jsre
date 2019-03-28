// A var with the same name as a parameter is initialzed with the parameter's value during hoisting (per the standard).
// This tests that typecheck correctly sees the vars as strings, and not undefined.

function foo(s: string) {}

(function(s: string) {
    var s;
    foo(s)
})

(function(s: string = "") {
    var s;
    foo(s)
})

(function(s: string) {
    foo(s)
    if (false) {{
        for (var s;;)
            ;
    }}
})

(function(s: string = "") {
    var s;
    foo(s)
    if (false) {{
        for (var s;;)
            ;
    }}
})
