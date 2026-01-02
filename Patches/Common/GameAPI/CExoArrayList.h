/*
 * CExoArrayList<T> are structured as follows:
 * {
 * T* data;
 * int size;
 * int capacity;
 * }
 * 
 * They are 0xC bytes in size in memory
 * 
 * Because of the nature of the MSVC compilation, a different CExoArrayList<T> structure was generated for each `T`, with different versions of the same functions for these other types. So we'll need to be clever about how we reference remote functions. Not to mention many member functions for these different classes are shared, especially when the size_of(T) is the same for two different types.
 * The way I see it is there are two valid approaches here:
 *	- We could attempt to match the supplied type to existing classes in the data base, and selectively pull the functions based on each of those (if they even exists, which they often don't as unused function were optimized out of the compilation). While this may be "more correct", I think this would get rather messy and out of hand quite quickly.
 *	- We could directly implement our own version of the Core CExoArrayList member functions that mimic the logic of the underlying functions properly. This would allow use to truly support any type `T`, and be a lot cleaner than trying to use game function for manipulating these structures. The only draw back here is we would need to be very sure that our implementations are accurate, lest we create game structures and results that are unexpected within the game.
 * 
 * I believe the second of the above options is preferable
 * 
 * Regardless, we need to store the size of `T` as a member, so that we are able to allocate the correct sized buffers when necessary
 * 
 * CExoArrayList has the following member function:
 *	- Add(T value) | Appends value to the data array, and increases size by 1
 *	- AddUnique(T value) | Performs Add if value doesn't already exists on the array (makes use of the Count function for this)
 *  - Allocate(int capacity) | Updates capacity field, Allocates a new array of length size_of(T) * capacity, Copies data to it, frees old data
 *	- CExoArrayList() | Default constructor, size and capacity are 0, data is nullptr
 *	- CExoArrayList(int capacity) | Allocation Constructor, the default constructor with a call to Allocate(capacity)
 *	- CExoArrayList(CExoArrayList<T> copy) | Copy constructor copies copy into this
 *	- Clear() | Frees all data, size and capacity set to 0
 *	- Count(T value) | Returns the number of times values occurs in the array
 *	- DeleteAt(int index) | Deletes the values at index in the data array, and shifts all other values back 1. Shrinking size
 *	- IndexOf(T value) | Gets the index of the first occurrence of value in the array. Returns -1 if not found
 *	- Insert(T value, int index) | Inserts value into the array such that it will be at index. Other values are shifted forward. Growing size
 *	- operator=(CExoArrayList<T> rhs) | Copies the content of rhs into this array
 *	- Remove(T value) | Removes the *last* occurrence of value from the array, shifting other values backwards, shrinking size
 *	- RemoveAll(T value) | Removes all occurrences of values from the array, shifting other values back, shrinking size
 *	- SetSize(int size) | Typically used for shrinking the size of an array, deleting off-cut data, and setting a new size/capacity
 *	- ~CExoArrayList() | Destructor, essentially the same as Clear
 * 
 * Other functions that may be useful:
 *	- Other operators such as `==`, `!=`, `[]`, `+`, and `+=` would be worth while
 *	- A sort function would be neat, provided T is sortable. Maybe we require a compare function be provided?
 *	- Of course we'll have getters for the internal fields, `data` at 0x0, `size` at 0x4, `capacity` at 0x8
 * 
 * Some other important notes:
 *	- When size is increased, we need to check if it exceeds capacity. If so a call to Allocate will be necessary to grow the array. Kotor typically doubles the capacity, unless the previous capacity was 0, in which case it sets it to 10.
 *	- When clearing, removing, and deleting elements from the array, if T requires a destructor it should be properly invoked. Presumably there's a way to make this simple
 *	- Note that if T is a pointer type, then each value will be 4 bytes. Though sometimes much larger objects are used, so it's important to be aware of this when allocating
 *	- The bulk of the above functions are `__thiscall`s
 *	- Like other classes I want to ability to wrap an actual game object, or construct one ourself. The primary difference here is instead of pulling functions from the DB, we're just implementing it ourself.
 *	- This should still inherit from GameAPIObject though
 */