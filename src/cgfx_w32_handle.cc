/*/////////////////////////////////////////////////////////////////////////////
/// @summary Defines macros for working with CGFX handles and implements an 
/// object storage template for mapping CGFX handles to the underlying object.
/// The number of objects stored in a handle table is limited to 64K, and all
/// core object and index storage is statically allocated.
///////////////////////////////////////////////////////////////////////////80*/

/*////////////////
//   Includes   //
////////////////*/

/*////////////////////
//   Preprocessor   //
////////////////////*/

/*/////////////////
//   Constants   //
/////////////////*/
// Handle Layout:
// I = object ID (32 bits)
// Y = object type (24 bits)
// T = table index (8 bits)
// 63                                                             0
// ................................................................
// TTTTTTTTYYYYYYYYYYYYYYYYYYYYYYYYIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
// 
// CG_HANDLE_MASK_x_P => Mask in a packed value
// CG_HANDLE_MASK_x_U => Mask in an unpacked value
#define CG_MAX_OBJECTS_PER_TABLE    (64 * 1024)
#define CG_OBJECT_INDEX_INVALID      0xFFFF
#define CG_OBJECT_INDEX_MASK         0xFFFF
#define CG_NEW_OBJECT_ID_ADD         0x10000
#define CG_HANDLE_MASK_I_P           0x00000000FFFFFFFFULL
#define CG_HANDLE_MASK_I_U           0x00000000FFFFFFFFULL
#define CG_HANDLE_MASK_Y_P           0x00FFFFFF00000000ULL
#define CG_HANDLE_MASK_Y_U           0x0000000000FFFFFFULL
#define CG_HANDLE_MASK_T_P           0xFF00000000000000ULL
#define CG_HANDLE_MASK_T_U           0x00000000000000FFULL
#define CG_HANDLE_SHIFT_I            0
#define CG_HANDLE_SHIFT_Y            32
#define CG_HANDLE_SHIFT_T            56

/*///////////////////
//   Local Types   //
///////////////////*/
/// @summary Describes the location of an object within an object table.
struct CG_OBJECT_INDEX
{
    uint32_t          Id;           /// The ID is the index-in-table + generation.
    uint16_t          Index;        /// The zero-based index into the tightly-packed array.
    uint16_t          Next;         /// The zero-based index of the next free slot.
};

/// @summary Defines a table mapping handles to internal data. Each table may be several MB. Do not allocate on the stack.
/// The type T must have a field uint32_t ObjectId.
template <typename T>
struct CG_OBJECT_TABLE
{
    #define N         CG_MAX_OBJECTS_PER_TABLE
    size_t            ObjectCount;  /// The number of live objects in the table.
    uint16_t          FreeListTail; /// The index of the most recently freed item.
    uint16_t          FreeListHead; /// The index of the next available item.
    uint32_t          ObjectType;   /// One of cg_object_e specifying the type of object in this table.
    size_t            TableIndex;   /// The zero-based index of this table, if there are multiple tables of this type.
    CG_OBJECT_INDEX   Indices[N];   /// The sparse array used to look up the data in the packed array. 512KB.
    T                 Objects[N];   /// The tightly packed array of object data.
    #undef  N
};

/*///////////////
//   Globals   //
///////////////*/

/*///////////////////////
//   Local Functions   //
///////////////////////*/

/*////////////////////////
//   Public Functions   //
////////////////////////*/
/// @summary Construct a handle out of its constituent parts.
/// @param object_id The object identifier within the parent object table.
/// @param object_type The object type identifier. One of cg_object_e.
/// @param table_index The zero-based index of the parent object table. Max value 255.
/// @return The unique object handle.
public_function inline cg_handle_t
cgMakeHandle
(
    uint32_t object_id, 
    uint32_t object_type,
    size_t   table_index=0
)
{
    assert(table_index < UINT8_MAX);
    uint64_t I =(uint64_t(object_id  ) & CG_HANDLE_MASK_I_U) << CG_HANDLE_SHIFT_I;
    uint64_t Y =(uint64_t(object_type) & CG_HANDLE_MASK_Y_U) << CG_HANDLE_SHIFT_Y;
    uint64_t T =(uint64_t(table_index) & CG_HANDLE_MASK_T_U) << CG_HANDLE_SHIFT_T;
    return  (I | Y | T);
}

/// @summary Update the object identifier of a handle.
/// @param handle The handle to update.
/// @param object_id The object identifier within the parent object table.
/// @return The modified handle value.
public_function inline cg_handle_t
cgSetHandleId
(
    cg_handle_t handle, 
    uint32_t    object_id
)
{
    return (handle & ~CG_HANDLE_MASK_I_P) | ((uint64_t(object_id  ) & CG_HANDLE_MASK_I_U) << CG_HANDLE_SHIFT_I);
}

/// @summary Update the object type of a handle.
/// @param handle The handle to update.
/// @param object_type The object type identifier. One of cg_object_e.
/// @return The modified handle value.
public_function inline cg_handle_t
cgSetHandleObjectType
(
    cg_handle_t handle, 
    uint32_t    object_type
)
{
    return (handle & ~CG_HANDLE_MASK_Y_P) | ((uint64_t(object_type) & CG_HANDLE_MASK_Y_U) << CG_HANDLE_SHIFT_Y);
}

/// @summary Update the table index of a handle.
/// @param handle The handle to update.
/// @param table_index The zero-based index of the owning object table. Max value 255.
/// @return The modified handle value.
public_function inline cg_handle_t
cgSetHandleTableIndex
(
    cg_handle_t handle, 
    size_t      table_index
)
{
    assert(table_index < UINT8_MAX);
    return (handle & ~CG_HANDLE_MASK_T_P) | ((uint64_t(table_index) & CG_HANDLE_MASK_T_U) << CG_HANDLE_SHIFT_T);
}

/// @summary Break a handle into its constituent parts.
/// @param handle The handle to crack.
/// @param object_id On return, stores the object identifier within the owning object table.
/// @param object_type On return, stores the object type identifier, one of cg_object_e.
/// @param table_index On return, stores the zero-based index of the parent object table.
public_function inline void
cgHandleParts
(
    cg_handle_t handle, 
    uint32_t   &object_id, 
    uint32_t   &object_type,
    size_t     &table_index
)
{
    object_id      = (uint32_t) ((handle & CG_HANDLE_MASK_I_P) >> CG_HANDLE_SHIFT_I);
    object_type    = (uint32_t) ((handle & CG_HANDLE_MASK_Y_P) >> CG_HANDLE_SHIFT_Y);
    table_index    = (size_t  ) ((handle & CG_HANDLE_MASK_T_P) >> CG_HANDLE_SHIFT_T);
}

/// @summary Extract the object identifier from an object handle.
/// @param handle The handle to inspect.
/// @return The object identifier.
public_function inline uint32_t
cgGetObjectId
(
    cg_handle_t handle
)
{
    return (uint32_t) ((handle & CG_HANDLE_MASK_I_P) >> CG_HANDLE_SHIFT_I);
}

/// @summary Extract the object type identifier from an object handle.
/// @param handle The handle to inspect.
/// @return The object type identifier, one of cg_object_e.
public_function inline uint32_t
cgGetObjectType
(
    cg_handle_t handle
)
{
    return (uint32_t) ((handle & CG_HANDLE_MASK_Y_P) >> CG_HANDLE_SHIFT_Y);
}

/// @summary Extract the index of the parent object table from an object handle.
/// @param handle The handle to inspect.
/// @return The zero-based index of the parent object table.
public_function inline size_t
cgGetTableIndex
(
    cg_handle_t handle
)
{
    return (size_t) ((handle & CG_HANDLE_MASK_T_P) >> CG_HANDLE_SHIFT_T);
}

/// @summary Initialize an object table to empty. This touches at least 512KB of memory.
/// @param table The object table to initialize or clear.
/// @param object_type The object type identifier for all objects in the table, one of cg_object_e.
/// @param table_index The zero-based index of the object table. Max value 255.
template <typename T>
public_function inline void
cgObjectTableInit
(
    CG_OBJECT_TABLE<T> *table, 
    uint32_t            object_type, 
    size_t              table_index=0
)
{
    table->ObjectCount   = 0;
    table->FreeListTail  = CG_MAX_OBJECTS_PER_TABLE - 1;
    table->FreeListHead  = 0;
    table->ObjectType    = object_type;
    table->TableIndex    = table_index;
    for (size_t i = 0; i < CG_MAX_OBJECTS_PER_TABLE; ++i)
    {
        table->Indices[i].Id   = uint32_t(i);
        table->Indices[i].Next = uint16_t(i-1);
    }
}

/// @summary Check whether an object table contains a given item.
/// @param table The object table to query.
/// @param handle The handle of the object to query.
/// @return true if the handle references an object within the table.
template <typename T>
public_function inline bool
cgObjectTableHas
(
    CG_OBJECT_TABLE<T> *table, 
    cg_handle_t         handle
)
{
    uint32_t        const  objid = cgGetObjectId(handle);
    CG_OBJECT_INDEX const &index = table->Indices[objid & CG_OBJECT_INDEX_MASK];
    return (index.Id == objid && index.Index != CG_OBJECT_INDEX_INVALID);
}

/// @summary Retrieve an item from an object table.
/// @param table The object table to query.
/// @param handle The handle of the object to query.
/// @return A pointer to the object within the table, or NULL.
template <typename T>
public_function inline T* 
cgObjectTableGet
(
    CG_OBJECT_TABLE<T> *table,
    cg_handle_t         handle
)
{
    uint32_t        const  objid = cgGetObjectId(handle);
    CG_OBJECT_INDEX const &index = table->Indices[objid & CG_OBJECT_INDEX_MASK];
    if (index.Id == objid && index.Index != CG_OBJECT_INDEX_INVALID)
    {
        return &table->Objects[index.Index];
    }
    else return NULL;
}

/// @summary Add a new object to an object table.
/// @param table The object table to update.
/// @param data The object to insert into the table.
/// @return The handle of the object within the table, or CG_INVALID_HANDLE.
template <typename T>
public_function inline cg_handle_t
cgObjectTableAdd
(
    CG_OBJECT_TABLE<T> *table, 
    T const            &data
)
{
    if (table->ObjectCount < CG_MAX_OBJECTS_PER_TABLE)
    {
        uint32_t const    type = table->ObjectType;
        size_t   const    tidx = table->TableIndex;
        CG_OBJECT_INDEX &index = table->Indices[table->FreeListHead]; // retrieve the next free object index
        table->FreeListHead    = index.Index;                         // pop the item from the free list
        index.Id              += CG_NEW_OBJECT_ID_ADD;                // assign the new item a unique ID
        index.Index            =(uint16_t) table->ObjectCount;        // allocate the next unused slot in the packed array
        T &object              = table->Objects[index.Index];         // object references the allocated slot
        object                 = data;                                // copy data into the newly allocated slot
        object.ObjectId        = index.Id;                            // and then store the new object ID in the data
        table->ObjectCount++;                                         // update the number of valid items in the table
        return cgMakeHandle(index.Id, type, tidx);
    }
    else return CG_INVALID_HANDLE;
}

/// @summary Remove an item from an object table.
/// @param table The object table to update.
/// @param handle The handle of the item to remove from the table.
/// @param existing On return, a copy of the removed item is placed in this location. This can be useful if the removed object has memory references that need to be cleaned up.
/// @return true if the item was removed and the data copied to existing.
template <typename T>
public_function inline bool
cgObjectTableRemove
(
    CG_OBJECT_TABLE<T> *table, 
    cg_handle_t         handle, 
    T                  &existing
)
{
    uint32_t       const  objid = cgGetObjectId(handle);
    CG_OBJECT_INDEX      &index = table->Indices[objid & CG_OBJECT_INDEX_MASK];
    if (index.Id == objid && index.Index != CG_OBJECT_INDEX_INVALID)
    {   // object refers to the object being deleted.
        // save the data for the caller in case they need to free memory.
        // copy the data for the last object in the packed array over the item being deleted.
        // then, update the index of the item that was moved to reference its new location.
        T &object = table->Objects[index.Index];
        existing  = object;
        object    = table->Objects[table->ObjectCount-1];
        table->Indices[object.ObjectId & CG_OBJECT_INDEX_MASK].Index = index.Index;
        table->ObjectCount--;
        // mark the deleted object as being invalid.
        // return the object index to the free list.
        index.Index  = CG_OBJECT_INDEX_INVALID;
        table->Indices[table->FreeListTail].Next = objid & CG_OBJECT_INDEX_MASK;
        table->FreeListTail = objid & CG_OBJECT_INDEX_MASK;
        return true;
    }
    else return false;
}

/// @summary Create a handle to the 'i-th' object in a table.
/// @param table The object table to query.
/// @param object_index The zero-based index of the object within the table.
/// @return The handle value used to reference the object externally.
template <typename T>
public_function inline cg_handle_t
cgMakeHandle
(
    CG_OBJECT_TABLE<T> *table, 
    size_t              object_index
)
{
    return cgMakeHandle(table->Objects[object_index].ObjectId, table->ObjectType, table->TableIndex);
}
