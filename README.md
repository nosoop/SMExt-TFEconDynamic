# Econ Dynamic

Provides a scripting interface to inject new attributes to TF2's in-memory schema.  Sort of
like Hidden Dev Attributes but more flexible.

Intended to be the successor to the [Custom Attribute Framework][]; plugins should use
[tf2attributes][] in tandem with this extension.

Note that, of course, these attributes are strictly server-side.  The attributes will not be
recognized on the client even if they match an existing attribute class.

[Custom Attribute Framework]: https://github.com/nosoop/SM-TFCustAttr
[tf2attributes]: https://github.com/FlaminSarge/tf2attributes

## Example

```
#include <sdkhooks>
#include <tf2attributes>
#include <tf_econ_dynamic>

public void OnPluginStart() {
	// register a new attribute class
	TF2EconDynAttribute attrib = new TF2EconDynAttribute();
	attrib.SetName("gravity decreased");
	attrib.SetClass("mult_gravity");
	
	// this determines how attributes are combined (added, multiplied, etc.)
	attrib.SetDescriptionFormat("value_is_percentage");
	
	// this saves a copy of the attribute in the extension; it will be reapplied as needed
	attrib.Register();
	
	// the handle isn't cleared, so you can reuse the class / description under multiple names
	attrib.SetName("gravity increased");
	attrib.Register();
	
	// you could also allocate new attributes to avoid updating existing ones
	attrib.SetClass("mult_player_movespeed");
	attrib.SetName("move speed bonus in combat");
	attrib.Register();
	
	delete attrib;
}

// callbacks below are stubs
void OnGroundEntChangedPost(int client) {
	// determine the gravity effect
	SetEntityGravity(client, TF2Attrib_HookValueFloat(1.0, "mult_gravity", client));
}

void OnTakeDamage(int victim) {
	// apply a temporary modified move speed modifier
	TF2Attrib_AddCustomPlayerAttribute(client, "move speed bonus in combat", 1.3, 10.0);
}
```
