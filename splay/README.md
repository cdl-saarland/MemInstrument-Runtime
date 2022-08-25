# Splay

The memory safety instrumentation by Jones & Kelly registers the sizes of new
objects upon allocation by inserting the bounds into a splay-tree datastructure.
Whenever a pointer is modified and dereferenced, it uses the information from
the splay tree to check whether the new pointer value is still within the range
of the previously pointed-to object.

## Publication

* Jones, Richard W. M. and P. Kelly. “Backwards-Compatible Bounds Checking for
  Arrays and Pointers in C Programs.” AADEBUG (1997).
