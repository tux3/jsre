#ifndef MAYBE_H
#define MAYBE_H

#include <cassert>

// Real cheap homemade three valued guesswork
enum class Tribool {
    Maybe = 0,
    Nope,
    Yep
};

Tribool inline operator!(const Tribool& other)
{
    switch (other) {
        case Tribool::Maybe:
            return Tribool::Maybe;
        case Tribool::Yep:
            return Tribool::Nope;
        case Tribool::Nope:
            return Tribool::Yep;
    }
    assert(false && "Fell out of exhaustive switch");
}

#endif // MAYBE_H
