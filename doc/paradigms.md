In logical, descriptive languages, the primitive is the proposition. Build the system proposition by proposition. We make statement about
the end result, the desired outcome of the computation, and about the boundary (initial) condition. The runtime solver supposed to construct
the solution, find a configuration which satisfies all our conditions.

- Primitive: proposition
- Pros:
  + No need to worry about the implementation, just describe what is and what we want.
  + Composable in terms of assertions
- Cons:
  + Certain problems are difficult to describe with statements
  + The solver and the search can be overly complex, slow

Instead of describing the whole system in one step, let's identifies valid subconfigurations and the paths between them. This is functional
programming.

- Primitive: algorithm how to get from A to B where A and B are valid configurations.
- Pro:
  + The better we can encode the validity of a configuration in the data the more compile-time guarantees 
  + Composable in terms of computational steps
- Cons:
  + Either ineffecient (can't store/mutate state) or needs a complex, optimizing, opaque machinery
  + Need more thinking than the logical model
  
  v empty | return s
  otherwise | return head v + sum (tail v)

Imperative model:

- Primitive: examine the current state, select (mutate) the next state
- Pros:
  + conceptually the simplest
  + composable in terms of state changes
- Cons:
  + No vision of past and future, very local reasoning, no guarantees, dead reckoning

    s = 0
    for(int i = 0; i < v.size(); ++i)
        s += v[i];
    
    state: (s, i)
    
    init state (0, 0)
    L1: i >= v.size() => goto end
    s += v[i]
    ++i
    goto L1
    
    
Actor model:

- Primitive: substate and related operations
- Pros:
  + very efficient to reduce huge systems with loosely coupled substatespaces
  + composable in terms of actors
- Cons:
  + Suboptimal partitioning leads excessive coupling, messageing
  + Hard to ensure invariants across actors
  
