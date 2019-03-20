// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
info: |
    Every execution context has associated with it a scope chain.
    A scope chain is a list of objects that are searched when evaluating an
    Identifier
es5id: 10.2.2_A1_T5
description: Checking scope chain containing function declarations and "with"
flags: [noStrict]
---*/

var x = 0; // decl 14

var myObj = {x : "obj"}; // decl 16

function f1(){ // decl 18
  var x = 1; // decl 19
  function f2(){ // decl 20
    let x  // decl 21
          = myObj  // decl 16
          .x;
    return x; // decl 21
  };
  return f2(); // decl 20
}

