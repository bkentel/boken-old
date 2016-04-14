## Objects ##

All objects, `item` and `entity`, are created from associated definition types, `item_definition` and `entity_definition` respectively and have a type-specific *instance id* and definition-specific *definition id*.

See: `item`, `entity`

### Ids ###

##### Instance Ids #####

The *instance id* is a type-specific opaque integral handle to an object managed by the `world`; instance ids for different types have separate namespaces. i.e. `item_instance_id {1} != entity_instance_id {1}`. Furthermore, ids are reused as objects come into and go out of existence, and as such are not necessarily unique over the lifetime of the game.

See: `item_instance_id`, `entity_instance_id`

##### Definition Ids #####

Similarly, the *definition id* is a hash of the string used to define an object in the data files. Definition data is managed by the `game_database`.

See: `item_id`, `entity_id`

### Properties ###

All objects have a set of *instance properties* and *definition properties*. These properties are {**key**, **value**} pairs where **key** is a 32-bit hash of a string identifier and **value** is a 32-bit blob of data.

##### Instance Properties #####

These are properties unique to a specific instance of an object and override any like-named (hashed) *definition properties*. For example, a stack of arrows might have a property `"current_stack_size"` to track how many arrows are stacked together as one `item`. Or, as an example of overriding, potions or enchantments can override the usual intrinsic properties of various `item` or `entity` instances.

##### Definition Properties #####

These are default base properties inherent to all newly created instances of an `item` or `entity`. They can be overridden by like-named (hashed) *instance properties* for a specific object.
