/**
 * An TF2EconDynAttribute is an abstraction over the KeyValues representation of an attribute
 * definition.
 */
methodmap TF2EconDynAttribute < Handle {
	/**
	 * Creates a new, empty TF2EconDynAttribute handle.
	 * 
	 * The attribute is not injected into the schema until TF2EconDynAttribute.Register() is
	 * called.
	 * 
	 * This handle should be freed by the calling plugin.
	 */
	public native TF2EconDynAttribute();
	
	/**
	 * Sets the attribute class.  All values under the same attribute class are combined.
	 * This is required.
	 */
	public native void SetClass(const char[] attrClass);
	
	/**
	 * Sets the attribute name.  Only one instance of this per game entity can be active.
	 * If this is empty or NULL_STRING, it will be set to the attribute class on registration.
	 */
	public native void SetName(const char[] attrName);
	
	/**
	 * Sets the description format.  This determines how multiple values with the same attribute
	 * class are combined.
	 * 
	 * Generally, "value_is_additive" and "value_is_percentage" are the main ones that matter,
	 * but being precise is ideal in case you want to use the description format when displaying
	 * values to players as with actual schema attributes.
	 */
	public native void SetDescriptionFormat(const char[] attrDescFmt);
	
	/**
	 * Sets a custom key / value pair.
	 * 
	 * Note that "injected" is a key reserved by the plugin that is set to 1 when registered,
	 * overriding any other values set.
	 * 
	 * The game has set other keys in the schema in the past, and it may use new ones in the
	 * future.
	 */
	public native void SetCustom(const char[] key, const char[] value);
	
	/**
	 * Sets the attribute's definition index.
	 * 
	 * By default, the extension will automatically allocate a previously unused index.  As the
	 * schema may be reloaded between maps, there is no guarantee that an attribute will
	 * continue to exist at a specific index.  If an index is manually specified and a
	 * attribute now exists at the index, the injection of the attribute will fail.
	 * 
	 * Plugins are expected to either perform a lookup by attribute name, or preferably, use the
	 * hook-style natives with the attribute class.
	 * 
	 * This native is solely provided for backwards compatibility with schema formats that
	 * manually specify indices.
	 */
	public native void SetDefIndex(int attrdef);
	
	/**
	 * Clears the attribute's specified definition index, allowing subsequent registrations to
	 * be allocated to unused indices.
	 */
	public native void ClearDefIndex();
	
	/**
	 * Stores a copy of the attribute properties in the core plugin to be injected.
	 * 
	 * This operation does not clear existing values on the TF2EconDynAttribute, so you can
	 * create multiple attributes by repeatedly calling TF2EconDynAttribute.SetName() and
	 * TF2EconDynAttribute.Register().
	 * 
	 * If an attribute with the same name already exists and is not an injected attribute
	 * managed by the core plugin, this will fail.
	 * 
	 * The attribute definition index may be reallocated if a new copy of the schema is loaded
	 * in and the index is occupied by a non-injected attribute.
	 * 
	 * @return    True if the attribute was registered in the extension's internal list and
	 *            injected in the current active schema; false if either operation fails.
	 */
	public native bool Register();
	
	/**
	 * Imports the properties of an attribute by name to the TF2EconDynAttribute handle,
	 * replacing all values currently set.
	 * 
	 * Note that the definition index will be updated to the value from the imported attribute.
	 * You will want to update the assigned value using SetDefIndex or ClearDefIndex.
	 * 
	 * @return    True if an attribute with that name exists, otherwise false.
	 */
	public native bool Import(const char[] name);
	
	/**
	 * Resets the container to its initial state.
	 */
	public native void Clear();
}

/**
 * Do not edit below this line!
 */
public Extension __ext_tf_econ_dynamic =
{
	name = "TF2 Econ Dynamic",
	file = "tf2econdynamic.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
