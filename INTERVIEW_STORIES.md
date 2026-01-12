
i have till now only did cpp for dsa, so to use it to access, read files, is well..not difficult but new, especially when i am making the code modular
and in such professional behavious, it just increase difficulty and chances to learn something new for me, its good, that i can learn imp things ahead
of when i will really need them, so i will be ready then.

I have to leanr so many new things in it because thats how we make proffesional code, from Cmake to polymorphism, this idea and code of polymorphism was
kinda hard for me, and to understand its working, especially  virtual functions, abstract base class, and how we are using header files, 
one of my big decisions was to use hashmap as baseline for all policies, as actually finding the element in those data structure can be hardware 
dependent, so i am by this way ensuring that that time will not dominate time metrics, and it will not pollute it too since we are making a simulation that 
doesnt capture hardware related any parameter. And well real cache policies do use hashmaps in them too. 
Hashing is used to make lookup cost predictable and similar across policies.
Clock Time =
  lookup (hashing)
+ eviction logic
+ metadata updates
+ pointer chasing
+ heap operations
+ allocator behavior

To clarify further, every cache in prod uses lookups, no dev ships O(n) chache, its too bad. Even fifo would be 
shipped with a hash map. We are also mirroring that here. IF we don’t use lookups, in our experiment, the difference 
of O1 lookups and On linear scan will dominate the metric results, that would be a bad experiment, we are not 
researching that question, we are also not cheating in any way since real fifo would also use hash in prod.
Our experiment asks “Given reasonable lookup mechanisms, what are the trade-offs between replacement policies?” 
not “which policy can do better lookups, or more clever lookups “. 
