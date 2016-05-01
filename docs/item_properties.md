weight
------
**Description**: The base (unmodified) weight of the item in grams.
**Applies to**: instance, definition
**Range**: any non-negative integer
**Default**: 1

capacity
------
**Description**: The maximum number of items the item can contain.
**Applies to**: instance, definition
**Range**: any non-negative integer
**Default**: 0

stack_size
------
**Description**: The maximum number of items of this type that can be stacked together.
**Applies to**: instance, definition
**Range**: any non-negative integer
**Default**: 1

current_stack_size
------
**Description**: The current number of items of this type stacked together.
**Applies to**: instance
**Range**: any non-negative integer
**Default**: 1

identified
------
**Description**: The identification state (level) of the item.
**Applies to**: instance
**Range**: any non-negative integer
**Default**: 0
**Details**:

 - Level 0: nothing known about the item other than its appearance.
 - Level 1: basic knowledge such as number of contained items.
