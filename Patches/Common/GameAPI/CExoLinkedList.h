/*
* The CExoLinkedList<T> class is shaped like so:
* {
*	0x0: CExoLinkedListInternal *
* }
* CExoLinkedListInternal:
* {
*	0x0: CExoLinkedListNode * head
*	0x4: CExoLinkedListNode * tail
*	0x8: int count
* }
* CExoLinkedListNode:
* {
*	0x0: CExoLinkedListNode * prev
*	0x4: CExoLinkedListNode * next
*	0x8: void* data (is cast to T by CExoLinkedList<T> when accessed)
* }
* 
* CExoLinkedListInternal functions from DB:
* id	class_name	function_name	address	notes	calling_convention	param_size_bytes
1224	CExoLinkedListInternal	AddBefore	6200016	CExoLinkedListInternal * this, void * value, CExoLinkedListNode * position	__thiscall	8
1225	CExoLinkedListInternal	AddHead	6200688	CExoLinkedListInternal * this, void * value	__thiscall	4
1226	CExoLinkedListInternal	AddTail	6200768	CExoLinkedListInternal * this, void * value	__thiscall	4
1227	CExoLinkedListInternal	Constructor	6199968	CExoLinkedListInternal * this	__thiscall	0
1228	CExoLinkedListInternal	Contains	6200640	CExoLinkedListInternal * this, void * value	__thiscall	4
1229	CExoLinkedListInternal	GetAtPos	6200160	CExoLinkedListInternal * this, CExoLinkedListNode * position	__thiscall	4
1230	CExoLinkedListInternal	GetNext	6200208	CExoLinkedListInternal * this, CExoLinkedListNode * * position	__thiscall	4
1232	CExoLinkedListInternal	GetPrev	6200272	CExoLinkedListInternal * this, CExoLinkedListNode * * position	__thiscall	4
1233	CExoLinkedListInternal	Remove	6200336	CExoLinkedListInternal * this, CExoLinkedListNode * position	__thiscall	4
1234	CExoLinkedListInternal	RemoveHead	6200448	CExoLinkedListInternal * this	__thiscall	0
1235	CExoLinkedListInternal	RemoveTail	6200544	CExoLinkedListInternal * this	__thiscall	0

* Due to how template functions compile in MSVC the game contains a separate constructor for every T for CExoLinkedList<T>. So instead of dispatching to the game constructor, we'll just do it ourself using the above structure layout (take note of the offsets). That way we're able to handle the templating ourself
* 
* Unlike with CExoArrayList though, the internal linked list functions are type agnostic, so we can pull those from the DB.
* 
* Like with CExoArrayList, we should be able to wrap in-game linked list or create our both, with the usual shouldFree pattern.
* 
* I also wouldn't mind having some of the other useful functions we added for CExoArrayList, like a copy constructor, assignment operator, a clear function, etc.
* 
* This will also need similar destructor checks to ensure that the data in the nodes is freed correctly. I will note that the type `CExoLinkedListNode` has no member functions within the game, and is effectively just a structure used by the CExoLinkedListInternal.
*/