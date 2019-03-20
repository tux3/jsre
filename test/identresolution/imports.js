import symbol from './file1'; // decl 1
import * as glob from './file2'; // decl 2
import {
    elem, // decl 4
    Class, // decl 5
    type SomeType // decl 6
    
} from './file3';
import type { AnotherType } from './aliases';  // decl 9

type alias1 // decl 11
            = AnotherType; // decl 9
type alias2 // decl 13
        = SomeType // decl 6
            | Class; // decl 5
            
symbol // decl 1
        = glob // decl 2
                .elem;

(elem); // decl  4

type secondOrderAlias // decl 23
                = alias1 // decl 11
                  & alias2; // decl 13
