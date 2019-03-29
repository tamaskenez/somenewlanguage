#if 0
#include <memory>
#include <vector>

template<class T>
using up = std::unique_ptr<T>;

using std::vector;
using std::make_unique;

namespace usual {
    struct Node {
        vector<up<Node>> children;
        void add_child() {
            children.emplace_back(make_unique<Node>());
        }
        int num;
    };
}

namespace optim {

    template<class T>
    class myvector {
    public:
    };

    struct Node {
        myvector<Node*> children;
    };
}

using Node = usual::Node;

void build(Node& node, int levels_left) {
    const int N_CHILDREN = 3;
    node.children.reserve(N_CHILDREN);
    node.add_child();
    if(--levels_left<=0) { return; }
    for(auto&c:node.children) {
        build(*c, levels_left);
    }
}

void build() {
    Node root;
    build(root);
}
#endif
int main() {}
