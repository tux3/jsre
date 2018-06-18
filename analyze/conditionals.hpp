#ifndef CONDITIONALS_HPP
#define CONDITIONALS_HPP

class Module;

void analyzeConditionals(Module& module);

void findEmptyBodyConditionals(Module& module);
void findDuplicateIfTests(Module& module);

#endif // CONDITIONALS_HPP
