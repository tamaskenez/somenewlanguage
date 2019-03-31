
#include <chrono>
#include <memory>
#include <vector>

template <class T>
using up = std::unique_ptr<T>;

using std::make_unique;
using std::vector;

const int N_CHILDREN = 3;
const int TOTAL_LEVELS = 15;

int counter = 0;

namespace usual {
struct Node
{
    vector<up<Node>> children;
    int num = counter++;

    explicit Node(int n_max_children) { children.reserve(n_max_children); }
    void add_child(int n_max_children) { children.emplace_back(make_unique<Node>(n_max_children)); }
};
}  // namespace usual

namespace optim {

template <class T>
class myvector
{
public:
    explicit myvector(int max_size) {}
    void push_back(const T& x) {}
};

struct Node
{
    myvector<Node*> children;
    int num = counter++;

    explicit Node(int n_max_children) : children(n_max_children) {}
    void add_child(int n_max_children) { children.push_back(nullptr); }
};
}  // namespace optim

using Node = usual::Node;

void build(Node& node, int levels_left)
{
    for (int i = 0; i < N_CHILDREN; ++i)
        node.add_child(N_CHILDREN);
    if (--levels_left <= 0) {
        return;
    }
    for (auto& c : node.children) {
        build(*c, levels_left);
    }
}

int traverse(const Node& node)
{
    int result = node.num;
    for (auto& c : node.children) {
        result += traverse(*c);
    }
    return result;
}

Node build()
{
    Node root(N_CHILDREN);
    build(root, TOTAL_LEVELS);
    return root;
}

using hrc = std::chrono::high_resolution_clock;
using ddur = std::chrono::duration<double>;
using tp = hrc::time_point;

int main()
{
    tp t0, t1;
    {
        t0 = hrc::now();
        auto r = build();
        t1 = hrc::now();
        printf("Node count: %d, build time: %f ms\n", counter, 1000.0 * ddur(t1 - t0).count());
        t0 = hrc::now();
        int n = traverse(r);
        t1 = hrc::now();
        printf("traverse result: %d, time: %f ms\n", n, 1000.0 * ddur(t1 - t0).count());
        t0 = hrc::now();
    }
    t1 = hrc::now();
    printf("Destruction: %f ms", 1000.0 * ddur(t1 - t0).count());
}
