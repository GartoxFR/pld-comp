#pragma once

namespace ir {
    class Visitor;

    // Anything that can be visited by a Visitor
    class Visitable {
      public:
        virtual ~Visitable() = default;
        virtual void accept(Visitor& visitor) = 0;
    };
}
