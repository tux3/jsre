// Copyright 2009 the Sputnik authors.  All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
info: |
    Every execution context has associated with it a scope chain.
    A scope chain is a list of objects that are searched when evaluating an
    Identifier
es5id: 10.2.2_A1_T1
description: Checking scope chain containing function declarations
---*/

var x = 0; // decl 13

function f1(){ // decl 15
  var x = 1; // decl 16
  function f2(){ // decl 17
    return x; // decl 16
  };
  return f2(); // decl 17
}

