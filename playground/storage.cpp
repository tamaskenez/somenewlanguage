
#include <cassert>
#include <chrono>
#include <cstddef>
#include <deque>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

using std::array;
using std::deque;
using std::make_unique;
using std::move;
using std::unique_ptr;
using std::vector;

using hrclock = std::chrono::high_resolution_clock;
using ddur = std::chrono::duration<double>;
using time_point = hrclock::time_point;

const int N_CHILDREN = 3;
const int TOTAL_LEVELS = 15;

int counter = 0;

namespace usual {

struct Allocator
{};

struct Node
{
    vector<unique_ptr<Node>> children;
    int num = counter++;

    Node(Allocator&, int n_max_children) { children.reserve(n_max_children); }
    void add_child(Allocator& a, int n_max_children)
    {
        children.emplace_back(make_unique<Node>(a, n_max_children));
    }
};

}  // namespace usual

namespace optim {

class BlockStorage
{
public:
    ~BlockStorage() {}

    virtual void* allocate_block() = 0;
    virtual int block_size() = 0;
};

template <int N>
class BlockStorageOfGivenSize : public BlockStorage
{
    using Block = typename std::aligned_storage<N>::type;
    deque<Block> blocks;

public:
    virtual int block_size() override { return N; }
    virtual void* allocate_block() override
    {
        blocks.emplace_back();
        return &blocks.back();
    }
};

class SingleBlock
{
    unique_ptr<std::max_align_t[]> block;

public:
    explicit SingleBlock(int N)
        : block(make_unique<std::max_align_t[]>((N + sizeof(std::max_align_t) - 1) /
                                                sizeof(std::max_align_t)))
    {
        fprintf(stderr, "SingleBlock of size %d\n", N);
    }

    void* get() const { return block.get(); }
};

const int MIN_BLOCK_SIZE = 16;
const int N_FIXED_SIZE_STORAGES = 4;

class Allocator
{
    array<unique_ptr<BlockStorage>, N_FIXED_SIZE_STORAGES> fixed_size_block_storages;
    deque<SingleBlock> variable_size_blocks;

public:
    Allocator()
    {
        fixed_size_block_storages[0] = make_unique<BlockStorageOfGivenSize<MIN_BLOCK_SIZE>>();
        fixed_size_block_storages[1] = make_unique<BlockStorageOfGivenSize<2 * MIN_BLOCK_SIZE>>();
        fixed_size_block_storages[2] = make_unique<BlockStorageOfGivenSize<4 * MIN_BLOCK_SIZE>>();
        fixed_size_block_storages[3] = make_unique<BlockStorageOfGivenSize<8 * MIN_BLOCK_SIZE>>();
    }
    void* allocate_block(int size)
    {
        int idx = -1;
        if (size <= MIN_BLOCK_SIZE)
            idx = 0;
        else if (size <= 2 * MIN_BLOCK_SIZE)
            idx = 1;
        else if (size <= 4 * MIN_BLOCK_SIZE)
            idx = 2;
        else if (size <= 8 * MIN_BLOCK_SIZE)
            idx = 3;
        if (idx >= 0)
            return fixed_size_block_storages[idx]->allocate_block();
        else {
            variable_size_blocks.emplace_back(size);
            return variable_size_blocks.back().get();
        }
    }

    template <class T, class... Args>
    T* new_object(Args&&... args)
    {
        void* p = allocate_block(sizeof(T));
        return new (p) T(std::forward<Args>(args)...);
    }
};

template <class T>
class myvector
{
    T* items;
    int size = 0;
    const int max_size;

public:
    explicit myvector(Allocator& a, int max_size)
        : items((T*)(a.allocate_block(max_size * sizeof(T)))), max_size(max_size)
    {}
    void push_back(const T& x)
    {
        assert(size < max_size);
        new (&(items[size++])) T(x);
    }
    T* begin() { return items; }
    T* end() { return items + size; }
    const T* begin() const { return items; }
    const T* end() const { return items + size; }
};

struct Node
{
    myvector<Node*> children;
    int num = counter++;

    Node(Allocator& a, int n_max_children) : children(a, n_max_children) {}
    void add_child(Allocator& a, int n_max_children)
    {
        Node* p = a.new_object<Node>(a, n_max_children);
        children.push_back(p);
    }
};
}  // namespace optim

template <class Allocator, class Node>
void build_subtree(Allocator& allocator, Node& node, int levels_left)
{
    for (int i = 0; i < N_CHILDREN; ++i)
        node.add_child(allocator, N_CHILDREN);
    if (--levels_left <= 0) {
        return;
    }
    for (auto& c : node.children) {
        build_subtree(allocator, *c, levels_left);
    }
}

template <class Node>
int traverse(const Node& node)
{
    int result = node.num;
    for (auto& c : node.children) {
        result += traverse(*c);
    }
    return result;
}

template <class Allocator, class Node>
Node build_tree(Allocator& allocator)
{
    Node root(allocator, N_CHILDREN);
    build_subtree(allocator, root, TOTAL_LEVELS);
    return root;
}

template <class Allocator, class Node>
void test(const char* name)
{
    counter = 0;
    fprintf(stderr, "-- Testing: %s\n", name);
    time_point t0, t1;
    {
        Allocator allocator;
        t0 = hrclock::now();
        auto r = build_tree<Allocator, Node>(allocator);
        t1 = hrclock::now();
        printf("Node count: %d, build time: %f ms\n", counter, 1000.0 * ddur(t1 - t0).count());
        t0 = hrclock::now();
        int n = traverse(r);
        t1 = hrclock::now();
        printf("traverse result: %d, time: %f ms\n", n, 1000.0 * ddur(t1 - t0).count());
        t0 = hrclock::now();
    }
    t1 = hrclock::now();
    printf("Destruction: %f ms\n", 1000.0 * ddur(t1 - t0).count());
}

int main()
{
    test<usual::Allocator, usual::Node>("Usual");
    test<optim::Allocator, optim::Node>("Optim");
}
