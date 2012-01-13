/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=78:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsobj_h___
#define jsobj_h___

/*
 * JS object definitions.
 *
 * A JS object consists of a possibly-shared object descriptor containing
 * ordered property names, called the map; and a dense vector of property
 * values, called slots.  The map/slot pointer pair is GC'ed, while the map
 * is reference counted and the slot vector is malloc'ed.
 */
#include "jsapi.h"
#include "jsatom.h"
#include "jsclass.h"
#include "jsfriendapi.h"
#include "jsinfer.h"
#include "jshash.h"
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"
#include "jscell.h"

#include "gc/Barrier.h"
#include "vm/String.h"

namespace js {

class AutoPropDescArrayRooter;
class ProxyHandler;
class CallObject;
struct GCMarker;
struct NativeIterator;

namespace mjit { class Compiler; }

static inline PropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

static inline StrictPropertyOp
CastAsStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(StrictPropertyOp, object);
}

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline JSObject *
CastAsObject(StrictPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline Value
CastAsObjectJsval(PropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

inline Value
CastAsObjectJsval(StrictPropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

/*
 * JSPropertySpec uses JSAPI JSPropertyOp and JSStrictPropertyOp in function
 * signatures, but with JSPROP_NATIVE_ACCESSORS the actual values must be
 * JSNatives. To avoid widespread casting, have JS_PSG and JS_PSGS perform
 * type-safe casts.
 */
#define JS_PSG(name,getter,flags)                                             \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,              \
     (JSPropertyOp)getter, NULL}
#define JS_PSGS(name,getter,setter,flags)                                     \
    {name, 0, (flags) | JSPROP_SHARED | JSPROP_NATIVE_ACCESSORS,              \
     (JSPropertyOp)getter, (JSStrictPropertyOp)setter}
#define JS_PS_END {0, 0, 0, 0, 0}

/******************************************************************************/

/*
 * A representation of ECMA-262 ed. 5's internal Property Descriptor data
 * structure.
 */
struct PropDesc {
    /*
     * Original object from which this descriptor derives, passed through for
     * the benefit of proxies.
     */
    js::Value pd;

    js::Value value, get, set;

    /* Property descriptor boolean fields. */
    uint8_t attrs;

    /* Bits indicating which values are set. */
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;

    friend class js::AutoPropDescArrayRooter;

    PropDesc();

    /*
     * 8.10.5 ToPropertyDescriptor(Obj)
     *
     * If checkAccessors is false, skip steps 7.b and 8.b, which throw a
     * TypeError if .get or .set is neither a callable object nor undefined.
     *
     * (DebuggerObject_defineProperty uses this: the .get and .set properties
     * are expected to be Debugger.Object wrappers of functions, which are not
     * themselves callable.)
     */
    bool initialize(JSContext* cx, const js::Value &v, bool checkAccessors=true);

    /*
     * 8.10.4 FromPropertyDescriptor(Desc)
     *
     * initFromPropertyDescriptor sets pd to undefined and populates all the
     * other fields of this PropDesc from desc.
     *
     * makeObject populates pd based on the other fields of *this, creating a
     * new property descriptor JSObject and defining properties on it.
     */
    void initFromPropertyDescriptor(const PropertyDescriptor &desc);
    bool makeObject(JSContext *cx);

    /* 8.10.1 IsAccessorDescriptor(desc) */
    bool isAccessorDescriptor() const {
        return hasGet || hasSet;
    }

    /* 8.10.2 IsDataDescriptor(desc) */
    bool isDataDescriptor() const {
        return hasValue || hasWritable;
    }

    /* 8.10.3 IsGenericDescriptor(desc) */
    bool isGenericDescriptor() const {
        return !isAccessorDescriptor() && !isDataDescriptor();
    }

    bool configurable() const {
        return (attrs & JSPROP_PERMANENT) == 0;
    }

    bool enumerable() const {
        return (attrs & JSPROP_ENUMERATE) != 0;
    }

    bool writable() const {
        return (attrs & JSPROP_READONLY) == 0;
    }

    JSObject* getterObject() const {
        return get.isUndefined() ? NULL : &get.toObject();
    }
    JSObject* setterObject() const {
        return set.isUndefined() ? NULL : &set.toObject();
    }

    const js::Value &getterValue() const {
        return get;
    }
    const js::Value &setterValue() const {
        return set;
    }

    PropertyOp getter() const {
        return js::CastAsPropertyOp(getterObject());
    }
    StrictPropertyOp setter() const {
        return js::CastAsStrictPropertyOp(setterObject());
    }

    /*
     * Throw a TypeError if a getter/setter is present and is neither callable
     * nor undefined. These methods do exactly the type checks that are skipped
     * by passing false as the checkAccessors parameter of initialize.
     */
    inline bool checkGetter(JSContext *cx);
    inline bool checkSetter(JSContext *cx);
};

typedef Vector<PropDesc, 1> PropDescArray;

} /* namespace js */

/*
 * On success, and if id was found, return true with *objp non-null and with a
 * property of *objp stored in *propp. If successful but id was not found,
 * return true with both *objp and *propp null.
 */
extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);

namespace js {

inline bool
LookupProperty(JSContext *cx, JSObject *obj, PropertyName *name,
               JSObject **objp, JSProperty **propp)
{
    return js_LookupProperty(cx, obj, ATOM_TO_JSID(name), objp, propp);
}

}

extern JS_FRIEND_API(JSBool)
js_LookupElement(JSContext *cx, JSObject *obj, uint32_t index,
                 JSObject **objp, JSProperty **propp);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value *value,
                  JSPropertyOp getter, JSStrictPropertyOp setter, uintN attrs);

extern JSBool
js_DefineElement(JSContext *cx, JSObject *obj, uint32_t index, const js::Value *value,
                 JSPropertyOp getter, JSStrictPropertyOp setter, uintN attrs);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, js::Value *vp);

extern JSBool
js_GetElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t, js::Value *vp);

inline JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp)
{
    return js_GetProperty(cx, obj, obj, id, vp);
}

inline JSBool
js_GetElement(JSContext *cx, JSObject *obj, uint32_t index, js::Value *vp)
{
    return js_GetElement(cx, obj, obj, index, vp);
}

namespace js {

extern JSBool
GetPropertyDefault(JSContext *cx, JSObject *obj, jsid id, const Value &def, Value *vp);

} /* namespace js */

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     js::Value *vp, JSBool strict);

namespace js {

inline bool
SetPropertyHelper(JSContext *cx, JSObject *obj, PropertyName *name, uintN defineHow,
                  Value *vp, JSBool strict)
{
    return !!js_SetPropertyHelper(cx, obj, ATOM_TO_JSID(name), defineHow, vp, strict);
}

} /* namespace js */

extern JSBool
js_SetElementHelper(JSContext *cx, JSObject *obj, uint32_t index, uintN defineHow,
                    js::Value *vp, JSBool strict);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_GetElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_SetElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, js::PropertyName *name, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteElement(JSContext *cx, JSObject *obj, uint32_t index, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteSpecial(JSContext *cx, JSObject *obj, js::SpecialId sid, js::Value *rval, JSBool strict);

extern JSBool
js_DeleteGeneric(JSContext *cx, JSObject *obj, jsid id, js::Value *rval, JSBool strict);

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

namespace js {

/* ES5 8.12.8. */
extern JSBool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);

extern Class ArrayClass;
extern Class ArrayBufferClass;
extern Class BlockClass;
extern Class BooleanClass;
extern Class CallableObjectClass;
extern Class DateClass;
extern Class SipClass;
extern Class ErrorClass;
extern Class GeneratorClass;
extern Class IteratorClass;
extern Class JSONClass;
extern Class MathClass;
extern Class NumberClass;
extern Class NormalArgumentsObjectClass;
extern Class ObjectClass;
extern Class ProxyClass;
extern Class RegExpClass;
extern Class SlowArrayClass;
extern Class StopIterationClass;
extern Class StringClass;
extern Class StrictArgumentsObjectClass;
extern Class WeakMapClass;
extern Class WithClass;
extern Class XMLFilterClass;

class ArgumentsObject;
class BlockObject;
class BooleanObject;
class ClonedBlockObject;
class DeclEnvObject;
class GlobalObject;
class NestedScopeObject;
class NormalArgumentsObject;
class NumberObject;
class ScopeObject;
class StaticBlockObject;
class StrictArgumentsObject;
class StringObject;
class RegExpObject;
class WithObject;

/*
 * Header structure for object element arrays. This structure is immediately
 * followed by an array of elements, with the elements member in an object
 * pointing to the beginning of that array (the end of this structure).
 * See below for usage of this structure.
 */
class ObjectElements
{
    friend struct ::JSObject;

    /* Number of allocated slots. */
    uint32_t capacity;

    /*
     * Number of initialized elements. This is <= the capacity, and for arrays
     * is <= the length. Memory for elements above the initialized length is
     * uninitialized, but values between the initialized length and the proper
     * length are conceptually holes.
     */
    uint32_t initializedLength;

    /* 'length' property of array objects, unused for other objects. */
    uint32_t length;

    /* :XXX: bug 586842 store state about sparse slots. */
    uint32_t unused;

    void staticAsserts() {
        JS_STATIC_ASSERT(sizeof(ObjectElements) == VALUES_PER_HEADER * sizeof(Value));
    }

  public:

    ObjectElements(uint32_t capacity, uint32_t length)
        : capacity(capacity), initializedLength(0), length(length)
    {}

    HeapValue * elements() { return (HeapValue *)(uintptr_t(this) + sizeof(ObjectElements)); }
    static ObjectElements * fromElements(HeapValue *elems) {
        return (ObjectElements *)(uintptr_t(elems) - sizeof(ObjectElements));
    }

    static int offsetOfCapacity() {
        return (int)offsetof(ObjectElements, capacity) - (int)sizeof(ObjectElements);
    }
    static int offsetOfInitializedLength() {
        return (int)offsetof(ObjectElements, initializedLength) - (int)sizeof(ObjectElements);
    }
    static int offsetOfLength() {
        return (int)offsetof(ObjectElements, length) - (int)sizeof(ObjectElements);
    }

    static const size_t VALUES_PER_HEADER = 2;
};

/* Shared singleton for objects with no elements. */
extern HeapValue *emptyObjectElements;

}  /* namespace js */

/*
 * JSObject struct. The JSFunction struct is an extension of this struct
 * allocated from a larger GC size-class.
 *
 * The |shape_| member stores the shape of the object, which includes the
 * object's class and the layout of all its properties.
 *
 * The type member stores the type of the object, which contains its prototype
 * object and the possible types of its properties.
 *
 * The rest of the object stores its named properties and indexed elements.
 * These are stored separately from one another. Objects are followed by an
 * variable-sized array of values for inline storage, which may be used by
 * either properties of native objects (fixed slots) or by elements.
 *
 * Two native objects with the same shape are guaranteed to have the same
 * number of fixed slots.
 *
 * Named property storage can be split between fixed slots and a dynamically
 * allocated array (the slots member). For an object with N fixed slots, shapes
 * with slots [0..N-1] are stored in the fixed slots, and the remainder are
 * stored in the dynamic array. If all properties fit in the fixed slots, the
 * 'slots' member is NULL.
 *
 * Elements are indexed via the 'elements' member. This member can point to
 * either the shared emptyObjectElements singleton, into the inline value array
 * (the address of the third value, to leave room for a ObjectElements header;
 * in this case numFixedSlots() is zero) or to a dynamically allocated array.
 *
 * Only certain combinations of properties and elements storage are currently
 * possible. This will be changing soon :XXX: bug 586842.
 *
 * - For objects other than arrays and typed arrays, the elements are empty.
 *
 * - For 'slow' arrays, both elements and properties are used, but the
 *   elements have zero capacity --- only the length member is used.
 *
 * - For dense arrays, elements are used and properties are not used.
 *
 * - For typed array buffers, elements are used and properties are not used.
 *   The data indexed by the elements do not represent Values, but primitive
 *   unboxed integers or floating point values.
 */
struct JSObject : js::gc::Cell
{
  private:
    friend struct js::Shape;
    friend struct js::GCMarker;

    /*
     * Shape of the object, encodes the layout of the object's properties and
     * all other information about its structure. See jsscope.h.
     */
    js::HeapPtrShape shape_;

#ifdef DEBUG
    void checkShapeConsistency();
#endif

    /*
     * The object's type and prototype. For objects with the LAZY_TYPE flag
     * set, this is the prototype's default 'new' type and can only be used
     * to get that prototype.
     */
    js::HeapPtrTypeObject type_;

    /* Make the type object to use for LAZY_TYPE objects. */
    void makeLazyType(JSContext *cx);

  public:
    inline js::Shape *lastProperty() const {
        JS_ASSERT(shape_);
        return shape_;
    }

    /*
     * Update the last property, keeping the number of allocated slots in sync
     * with the object's new slot span.
     */
    bool setLastProperty(JSContext *cx, const js::Shape *shape);

    /* As above, but does not change the slot span. */
    inline void setLastPropertyInfallible(const js::Shape *shape);

    /* Make a non-array object with the specified initial state. */
    static inline JSObject *create(JSContext *cx,
                                   js::gc::AllocKind kind,
                                   js::HandleShape shape,
                                   js::HandleTypeObject type,
                                   js::HeapValue *slots);

    /* Make a dense array object with the specified initial state. */
    static inline JSObject *createDenseArray(JSContext *cx,
                                             js::gc::AllocKind kind,
                                             js::HandleShape shape,
                                             js::HandleTypeObject type,
                                             uint32_t length);

    /*
     * Remove the last property of an object, provided that it is safe to do so
     * (the shape and previous shape do not carry conflicting information about
     * the object itself).
     */
    inline void removeLastProperty(JSContext *cx);
    inline bool canRemoveLastProperty();

    /*
     * Update the slot span directly for a dictionary object, and allocate
     * slots to cover the new span if necessary.
     */
    bool setSlotSpan(JSContext *cx, uint32_t span);

    static inline size_t offsetOfShape() { return offsetof(JSObject, shape_); }
    inline js::HeapPtrShape *addressOfShape() { return &shape_; }

    const js::Shape *nativeLookup(JSContext *cx, jsid id);

    inline bool nativeContains(JSContext *cx, jsid id);
    inline bool nativeContains(JSContext *cx, const js::Shape &shape);

    /* Upper bound on the number of elements in an object. */
    static const uint32_t NELEMENTS_LIMIT = JS_BIT(28);

  private:
    js::HeapValue   *slots;     /* Slots for object properties. */
    js::HeapValue   *elements;  /* Slots for object elements. */

  public:

    inline bool isNative() const;

    inline js::Class *getClass() const;
    inline JSClass *getJSClass() const;
    inline bool hasClass(const js::Class *c) const;
    inline const js::ObjectOps *getOps() const;

    /*
     * An object is a delegate if it is on another object's prototype or scope
     * chain, and therefore the delegate might be asked implicitly to get or
     * set a property on behalf of another object. Delegates may be accessed
     * directly too, as may any object, but only those objects linked after the
     * head of any prototype or scope chain are flagged as delegates. This
     * definition helps to optimize shape-based property cache invalidation
     * (see Purge{Scope,Proto}Chain in jsobj.cpp).
     */
    inline bool isDelegate() const;
    inline bool setDelegate(JSContext *cx);

    inline bool isBoundFunction() const;

    /*
     * The meaning of the system object bit is defined by the API client. It is
     * set in JS_NewSystemObject and is queried by JS_IsSystemObject, but it
     * has no intrinsic meaning to SpiderMonkey.
     */
    inline bool isSystem() const;
    inline bool setSystem(JSContext *cx);

    inline bool hasSpecialEquality() const;

    inline bool watched() const;
    inline bool setWatched(JSContext *cx);

    /* See StackFrame::varObj. */
    inline bool isVarObj() const;
    inline bool setVarObj(JSContext *cx);

    /*
     * Objects with an uncacheable proto can have their prototype mutated
     * without inducing a shape change on the object. Property cache entries
     * and JIT inline caches should not be filled for lookups across prototype
     * lookups on the object.
     */
    inline bool hasUncacheableProto() const;
    inline bool setUncacheableProto(JSContext *cx);

    bool generateOwnShape(JSContext *cx, js::Shape *newShape = NULL) {
        return replaceWithNewEquivalentShape(cx, lastProperty(), newShape);
    }

  private:
    js::Shape *replaceWithNewEquivalentShape(JSContext *cx, js::Shape *existingShape,
                                             js::Shape *newShape = NULL);

    enum GenerateShape {
        GENERATE_NONE,
        GENERATE_SHAPE
    };

    bool setFlag(JSContext *cx, /*BaseShape::Flag*/ uint32_t flag,
                 GenerateShape generateShape = GENERATE_NONE);

  public:
    inline bool nativeEmpty() const;

    js::Shape *methodShapeChange(JSContext *cx, const js::Shape &shape);
    bool shadowingShapeChange(JSContext *cx, const js::Shape &shape);

    /*
     * Read barrier to clone a joined function object stored as a method.
     * Defined in jsobjinlines.h, but not declared inline per standard style in
     * order to avoid gcc warnings.
     */
    js::Shape *methodReadBarrier(JSContext *cx, const js::Shape &shape, js::Value *vp);

    /* Whether method shapes can be added to this object. */
    inline bool canHaveMethodBarrier() const;

    /* Whether there may be indexed properties on this object. */
    inline bool isIndexed() const;

    /*
     * Return true if this object is a native one that has been converted from
     * shared-immutable prototype-rooted shape storage to dictionary-shapes in
     * a doubly-linked list.
     */
    inline bool inDictionaryMode() const;

    inline uint32_t propertyCount() const;

    inline bool hasPropertyTable() const;

    inline size_t structSize() const;
    inline size_t slotsAndStructSize() const;
    inline size_t dynamicSlotSize(JSMallocSizeOfFun mallocSizeOf) const;

    inline size_t numFixedSlots() const;

    static const uint32_t MAX_FIXED_SLOTS = 16;

  private:
    inline js::HeapValue* fixedSlots() const;

    /*
     * Get internal pointers to the range of values starting at start and
     * running for length.
     */
    void getSlotRange(size_t start, size_t length,
                      js::HeapValue **fixedStart, js::HeapValue **fixedEnd,
                      js::HeapValue **slotsStart, js::HeapValue **slotsEnd);
  public:

    /* Accessors for properties. */

    /* Whether a slot is at a fixed offset from this object. */
    inline bool isFixedSlot(size_t slot);

    /* Index into the dynamic slots array to use for a dynamic slot. */
    inline size_t dynamicSlotIndex(size_t slot);

    /* Get a raw pointer to the object's properties. */
    inline const js::HeapValue *getRawSlots();

    /* JIT Accessors */
    static inline size_t getFixedSlotOffset(size_t slot);
    static inline size_t getPrivateDataOffset(size_t nfixed);
    static inline size_t offsetOfSlots() { return offsetof(JSObject, slots); }

    /* Minimum size for dynamically allocated slots. */
    static const uint32_t SLOT_CAPACITY_MIN = 8;

    /*
     * Grow or shrink slots immediately before changing the slot span.
     * The number of allocated slots is not stored explicitly, and changes to
     * the slots must track changes in the slot span.
     */
    bool growSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount);
    void shrinkSlots(JSContext *cx, uint32_t oldCount, uint32_t newCount);

    bool hasDynamicSlots() const { return slots != NULL; }

    /*
     * Get the number of dynamic slots to allocate to cover the properties in
     * an object with the given number of fixed slots and slot span. The slot
     * capacity is not stored explicitly, and the allocated size of the slot
     * array is kept in sync with this count.
     */
    static inline size_t dynamicSlotsCount(size_t nfixed, size_t span);

    /* Compute dynamicSlotsCount() for this object. */
    inline size_t numDynamicSlots() const;

  protected:
    inline bool hasContiguousSlots(size_t start, size_t count) const;

    inline void initializeSlotRange(size_t start, size_t count);
    inline void invalidateSlotRange(size_t start, size_t count);

    inline bool updateSlotsForSpan(JSContext *cx, size_t oldSpan, size_t newSpan);

  public:
    /*
     * Trigger the write barrier on a range of slots that will no longer be
     * reachable.
     */
    inline void prepareSlotRangeForOverwrite(size_t start, size_t end);
    inline void prepareElementRangeForOverwrite(size_t start, size_t end);

    /*
     * Initialize a flat array of slots to this object at a start slot.  The
     * caller must ensure that are enough slots.
     */
    void initSlotRange(size_t start, const js::Value *vector, size_t length);

    /*
     * Copy a flat array of slots to this object at a start slot. Caller must
     * ensure there are enough slots in this object.
     */
    void copySlotRange(size_t start, const js::Value *vector, size_t length);

    inline uint32_t slotSpan() const;

    void rollbackProperties(JSContext *cx, uint32_t slotSpan);

#ifdef DEBUG
    enum SentinelAllowed {
        SENTINEL_NOT_ALLOWED,
        SENTINEL_ALLOWED
    };

    /*
     * Check that slot is in range for the object's allocated slots.
     * If sentinelAllowed then slot may equal the slot capacity.
     */
    bool slotInRange(uintN slot, SentinelAllowed sentinel = SENTINEL_NOT_ALLOWED) const;
#endif

    js::HeapValue *getSlotAddressUnchecked(uintN slot) {
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots() + slot;
        return slots + (slot - fixed);
    }

    js::HeapValue *getSlotAddress(uintN slot) {
        /*
         * This can be used to get the address of the end of the slots for the
         * object, which may be necessary when fetching zero-length arrays of
         * slots (e.g. for callObjVarArray).
         */
        JS_ASSERT(slotInRange(slot, SENTINEL_ALLOWED));
        return getSlotAddressUnchecked(slot);
    }

    js::HeapValue &getSlotRef(uintN slot) {
        JS_ASSERT(slotInRange(slot));
        return *getSlotAddress(slot);
    }

    inline js::HeapValue &nativeGetSlotRef(uintN slot);

    const js::Value &getSlot(uintN slot) const {
        JS_ASSERT(slotInRange(slot));
        size_t fixed = numFixedSlots();
        if (slot < fixed)
            return fixedSlots()[slot];
        return slots[slot - fixed];
    }

    inline const js::Value &nativeGetSlot(uintN slot) const;
    inline JSFunction *nativeGetMethod(const js::Shape *shape) const;

    inline void setSlot(uintN slot, const js::Value &value);
    inline void initSlot(uintN slot, const js::Value &value);
    inline void initSlotUnchecked(uintN slot, const js::Value &value);

    inline void nativeSetSlot(uintN slot, const js::Value &value);
    inline void nativeSetSlotWithType(JSContext *cx, const js::Shape *shape, const js::Value &value);

    inline const js::Value &getReservedSlot(uintN index) const;
    inline js::HeapValue &getReservedSlotRef(uintN index);
    inline void setReservedSlot(uintN index, const js::Value &v);

    /* For slots which are known to always be fixed, due to the way they are allocated. */

    js::HeapValue &getFixedSlotRef(uintN slot) {
        JS_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    const js::Value &getFixedSlot(uintN slot) const {
        JS_ASSERT(slot < numFixedSlots());
        return fixedSlots()[slot];
    }

    inline void setFixedSlot(uintN slot, const js::Value &value);
    inline void initFixedSlot(uintN slot, const js::Value &value);

    /*
     * Whether this is the only object which has its specified type. This
     * object will have its type constructed lazily as needed by analysis.
     */
    bool hasSingletonType() const { return !!type_->singleton; }

    /*
     * Whether the object's type has not been constructed yet. If an object
     * might have a lazy type, use getType() below, otherwise type().
     */
    bool hasLazyType() const { return type_->lazy(); }

    /*
     * Marks this object as having a singleton type, and leave the type lazy.
     * Constructs a new, unique shape for the object.
     */
    inline bool setSingletonType(JSContext *cx);

    inline js::types::TypeObject *getType(JSContext *cx);

    js::types::TypeObject *type() const {
        JS_ASSERT(!hasLazyType());
        return type_;
    }

    const js::HeapPtr<js::types::TypeObject> &typeFromGC() const {
        /* Direct field access for use by GC. */
        return type_;
    }

    static inline size_t offsetOfType() { return offsetof(JSObject, type_); }
    inline js::HeapPtrTypeObject *addressOfType() { return &type_; }

    inline void setType(js::types::TypeObject *newType);

    js::types::TypeObject *getNewType(JSContext *cx, JSFunction *fun = NULL);

#ifdef DEBUG
    bool hasNewType(js::types::TypeObject *newType);
#endif

    /*
     * Mark an object that has been iterated over and is a singleton. We need
     * to recover this information in the object's type information after it
     * is purged on GC.
     */
    inline bool setIteratedSingleton(JSContext *cx);

    /*
     * Mark an object as requiring its default 'new' type to have unknown
     * properties.
     */
    bool setNewTypeUnknown(JSContext *cx);

    /* Set a new prototype for an object with a singleton type. */
    bool splicePrototype(JSContext *cx, JSObject *proto);

    /*
     * For bootstrapping, whether to splice a prototype for Function.prototype
     * or the global object.
     */
    bool shouldSplicePrototype(JSContext *cx);

    JSObject * getProto() const {
        return type_->proto;
    }

    /*
     * Parents and scope chains.
     *
     * All script-accessible objects with a NULL parent are global objects,
     * and all global objects have a NULL parent. Some builtin objects which
     * are not script-accessible also have a NULL parent, such as parser
     * created functions for non-compileAndGo scripts.
     *
     * Except for the non-script-accessible builtins, the global with which an
     * object is associated can be reached by following parent links to that
     * global (see global()).
     *
     * The scope chain of an object is the link in the search path when a
     * script does a name lookup on a scope object. For JS internal scope
     * objects --- Call, DeclEnv and block --- the chain is stored in
     * the first fixed slot of the object, and the object's parent is the
     * associated global. For other scope objects, the chain is stored in the
     * object's parent.
     *
     * In compileAndGo code, scope chains can contain only internal scope
     * objects with a global object at the root as the scope of the outermost
     * non-function script. In non-compileAndGo code, the scope of the
     * outermost non-function script might not be a global object, and can have
     * a mix of other objects above it before the global object is reached.
     */

    /* Access the parent link of an object. */
    inline JSObject *getParent() const;
    bool setParent(JSContext *cx, JSObject *newParent);

    /*
     * Get the enclosing scope of an object. When called on non-scope object,
     * this will just be the global (the name "enclosing scope" still applies
     * in this situation because non-scope objects can be on the scope chain).
     */
    inline JSObject *enclosingScope();

    inline js::GlobalObject &global() const;

    /* Private data accessors. */

    inline bool hasPrivate() const;
    inline void *getPrivate() const;
    inline void setPrivate(void *data);

    /* Access private data for an object with a known number of fixed slots. */
    inline void *getPrivate(size_t nfixed) const;

    /* N.B. Infallible: NULL means 'no principal', not an error. */
    inline JSPrincipals *principals(JSContext *cx);

    /* Remove the type (and prototype) or parent from a new object. */
    inline bool clearType(JSContext *cx);
    bool clearParent(JSContext *cx);

    /*
     * ES5 meta-object properties and operations.
     */

  private:
    enum ImmutabilityType { SEAL, FREEZE };

    /*
     * The guts of Object.seal (ES5 15.2.3.8) and Object.freeze (ES5 15.2.3.9): mark the
     * object as non-extensible, and adjust each property's attributes appropriately: each
     * property becomes non-configurable, and if |freeze|, data properties become
     * read-only as well.
     */
    bool sealOrFreeze(JSContext *cx, ImmutabilityType it);

    bool isSealedOrFrozen(JSContext *cx, ImmutabilityType it, bool *resultp);

    static inline uintN getSealedOrFrozenAttributes(uintN attrs, ImmutabilityType it);

    inline void *&privateRef(uint32_t nfixed) const;

  public:
    inline bool isExtensible() const;
    bool preventExtensions(JSContext *cx, js::AutoIdVector *props);

    /* ES5 15.2.3.8: non-extensible, all props non-configurable */
    inline bool seal(JSContext *cx) { return sealOrFreeze(cx, SEAL); }
    /* ES5 15.2.3.9: non-extensible, all properties non-configurable, all data props read-only */
    bool freeze(JSContext *cx) { return sealOrFreeze(cx, FREEZE); }

    bool isSealed(JSContext *cx, bool *resultp) { return isSealedOrFrozen(cx, SEAL, resultp); }
    bool isFrozen(JSContext *cx, bool *resultp) { return isSealedOrFrozen(cx, FREEZE, resultp); }
        
    /*
     * Primitive-specific getters and setters.
     */

  private:
    static const uint32_t JSSLOT_PRIMITIVE_THIS = 0;

  public:
    inline const js::Value &getPrimitiveThis() const;
    inline void setPrimitiveThis(const js::Value &pthis);

    static size_t getPrimitiveThisOffset() {
        /* All primitive objects have their value in a fixed slot. */
        return getFixedSlotOffset(JSSLOT_PRIMITIVE_THIS);
    }

    /* Accessors for elements. */

    js::ObjectElements *getElementsHeader() const {
        return js::ObjectElements::fromElements(elements);
    }

    inline bool ensureElements(JSContext *cx, uintN cap);
    bool growElements(JSContext *cx, uintN cap);
    void shrinkElements(JSContext *cx, uintN cap);

    inline js::HeapValue* fixedElements() const {
        JS_STATIC_ASSERT(2 * sizeof(js::Value) == sizeof(js::ObjectElements));
        return &fixedSlots()[2];
    }

    void setFixedElements() { this->elements = fixedElements(); }

    inline bool hasDynamicElements() const {
        /*
         * Note: for objects with zero fixed slots this could potentially give
         * a spurious 'true' result, if the end of this object is exactly
         * aligned with the end of its arena and dynamic slots are allocated
         * immediately afterwards. Such cases cannot occur for dense arrays
         * (which have at least two fixed slots) and can only result in a leak.
         */
        return elements != js::emptyObjectElements && elements != fixedElements();
    }

    /* JIT Accessors */
    static inline size_t offsetOfElements() { return offsetof(JSObject, elements); }
    static inline size_t offsetOfFixedElements() {
        return sizeof(JSObject) + sizeof(js::ObjectElements);
    }

    /*
     * Array-specific getters and setters (for both dense and slow arrays).
     */

    bool allocateSlowArrayElements(JSContext *cx);

    inline uint32_t getArrayLength() const;
    inline void setArrayLength(JSContext *cx, uint32_t length);

    inline uint32_t getDenseArrayCapacity();
    inline uint32_t getDenseArrayInitializedLength();
    inline void setDenseArrayLength(uint32_t length);
    inline void setDenseArrayInitializedLength(uint32_t length);
    inline void ensureDenseArrayInitializedLength(JSContext *cx, uintN index, uintN extra);
    inline js::HeapValueArray getDenseArrayElements();
    inline const js::Value &getDenseArrayElement(uintN idx);
    inline void setDenseArrayElement(uintN idx, const js::Value &val);
    inline void initDenseArrayElement(uintN idx, const js::Value &val);
    inline void setDenseArrayElementWithType(JSContext *cx, uintN idx, const js::Value &val);
    inline void initDenseArrayElementWithType(JSContext *cx, uintN idx, const js::Value &val);
    inline void copyDenseArrayElements(uintN dstStart, const js::Value *src, uintN count);
    inline void initDenseArrayElements(uintN dstStart, const js::Value *src, uintN count);
    inline void moveDenseArrayElements(uintN dstStart, uintN srcStart, uintN count);
    inline bool denseArrayHasInlineSlots() const;

    /* Packed information for this array. */
    inline void markDenseArrayNotPacked(JSContext *cx);

    /*
     * ensureDenseArrayElements ensures that the dense array can hold at least
     * index + extra elements. It returns ED_OK on success, ED_FAILED on
     * failure to grow the array, ED_SPARSE when the array is too sparse to
     * grow (this includes the case of index + extra overflow). In the last
     * two cases the array is kept intact.
     */
    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseArrayElements(JSContext *cx, uintN index, uintN extra);

    /*
     * Check if after growing the dense array will be too sparse.
     * newElementsHint is an estimated number of elements to be added.
     */
    bool willBeSparseDenseArray(uintN requiredCapacity, uintN newElementsHint);

    JSBool makeDenseArraySlow(JSContext *cx);

    /*
     * If this array object has a data property with index i, set *vp to its
     * value and return true. If not, do vp->setMagic(JS_ARRAY_HOLE) and return
     * true. On OOM, report it and return false.
     */
    bool arrayGetOwnDataElement(JSContext *cx, size_t i, js::Value *vp);

  public:
    bool allocateArrayBufferSlots(JSContext *cx, uint32_t size);
    inline uint32_t arrayBufferByteLength();
    inline uint8_t * arrayBufferDataOffset();

  public:
    /*
     * Date-specific getters and setters.
     */

    static const uint32_t JSSLOT_DATE_UTC_TIME = 0;

    /*
     * Cached slots holding local properties of the date.
     * These are undefined until the first actual lookup occurs
     * and are reset to undefined whenever the date's time is modified.
     */
    static const uint32_t JSSLOT_DATE_COMPONENTS_START = 1;

    static const uint32_t JSSLOT_DATE_LOCAL_TIME = 1;
    static const uint32_t JSSLOT_DATE_LOCAL_YEAR = 2;
    static const uint32_t JSSLOT_DATE_LOCAL_MONTH = 3;
    static const uint32_t JSSLOT_DATE_LOCAL_DATE = 4;
    static const uint32_t JSSLOT_DATE_LOCAL_DAY = 5;
    static const uint32_t JSSLOT_DATE_LOCAL_HOURS = 6;
    static const uint32_t JSSLOT_DATE_LOCAL_MINUTES = 7;
    static const uint32_t JSSLOT_DATE_LOCAL_SECONDS = 8;

    static const uint32_t DATE_CLASS_RESERVED_SLOTS = 9;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    /*
     * Function-specific getters and setters.
     */

    friend struct JSFunction;

    inline JSFunction *toFunction();
    inline const JSFunction *toFunction() const;

  public:
    /*
     * Iterator-specific getters and setters.
     */

    static const uint32_t ITER_CLASS_NFIXED_SLOTS = 1;

    inline js::NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(js::NativeIterator *);

    /*
     * XML-related getters and setters.
     */

    /*
     * Slots for XML-related classes are as follows:
     * - NamespaceClass.base reserves the *_NAME_* and *_NAMESPACE_* slots.
     * - QNameClass.base, AttributeNameClass, AnyNameClass reserve
     *   the *_NAME_* and *_QNAME_* slots.
     * - Others (XMLClass, js_XMLFilterClass) don't reserve any slots.
     */
  private:
    static const uint32_t JSSLOT_NAME_PREFIX          = 0;   // shared
    static const uint32_t JSSLOT_NAME_URI             = 1;   // shared

    static const uint32_t JSSLOT_NAMESPACE_DECLARED   = 2;

    static const uint32_t JSSLOT_QNAME_LOCAL_NAME     = 2;

  public:
    static const uint32_t NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32_t QNAME_CLASS_RESERVED_SLOTS     = 3;

    inline JSLinearString *getNamePrefix() const;
    inline jsval getNamePrefixVal() const;
    inline void setNamePrefix(JSLinearString *prefix);
    inline void clearNamePrefix();

    inline JSLinearString *getNameURI() const;
    inline jsval getNameURIVal() const;
    inline void setNameURI(JSLinearString *uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline JSAtom *getQNameLocalName() const;
    inline jsval getQNameLocalNameVal() const;
    inline void setQNameLocalName(JSAtom *name);

    /*
     * Proxy-specific getters and setters.
     */
    inline js::Wrapper *getWrapperHandler() const;

    /*
     * Back to generic stuff.
     */
    inline bool isCallable();

    inline void finish(JSContext *cx);
    JS_ALWAYS_INLINE void finalize(JSContext *cx, bool background);

    inline bool hasProperty(JSContext *cx, jsid id, bool *foundp, uintN flags = 0);

    /*
     * Allocate and free an object slot.
     *
     * FIXME: bug 593129 -- slot allocation should be done by object methods
     * after calling object-parameter-free shape methods, avoiding coupling
     * logic across the object vs. shape module wall.
     */
    bool allocSlot(JSContext *cx, uint32_t *slotp);
    void freeSlot(JSContext *cx, uint32_t slot);

  public:
    bool reportReadOnly(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, uintN report = JSREPORT_ERROR);

    /*
     * Get the property with the given id, then call it as a function with the
     * given arguments, providing this object as |this|. If the property isn't
     * callable a TypeError will be thrown. On success the value returned by
     * the call is stored in *vp.
     */
    bool callMethod(JSContext *cx, jsid id, uintN argc, js::Value *argv, js::Value *vp);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::StackShape &child);

  protected:
    /*
     * Internal helper that adds a shape not yet mapped by this object.
     *
     * Notes:
     * 1. getter and setter must be normalized based on flags (see jsscope.cpp).
     * 2. !isExtensible() checking must be done by callers.
     */
    js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                   JSPropertyOp getter, JSStrictPropertyOp setter,
                                   uint32_t slot, uintN attrs,
                                   uintN flags, intN shortid, js::Shape **spp,
                                   bool allowDictionary);

  private:
    bool toDictionaryMode(JSContext *cx);

    struct TradeGutsReserved;
    static bool ReserveForTradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                                    TradeGutsReserved &reserved);

    static void TradeGuts(JSContext *cx, JSObject *a, JSObject *b,
                          TradeGutsReserved &reserved);

  public:
    /* Add a property whose id is not yet in this scope. */
    js::Shape *addProperty(JSContext *cx, jsid id,
                           JSPropertyOp getter, JSStrictPropertyOp setter,
                           uint32_t slot, uintN attrs,
                           uintN flags, intN shortid, bool allowDictionary = true);

    /* Add a data property whose id is not yet in this scope. */
    js::Shape *addDataProperty(JSContext *cx, jsid id, uint32_t slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    /* Add or overwrite a property for id in this scope. */
    js::Shape *putProperty(JSContext *cx, jsid id,
                           JSPropertyOp getter, JSStrictPropertyOp setter,
                           uint32_t slot, uintN attrs,
                           uintN flags, intN shortid);
    inline js::Shape *
    putProperty(JSContext *cx, js::PropertyName *name,
                JSPropertyOp getter, JSStrictPropertyOp setter,
                uint32_t slot, uintN attrs, uintN flags, intN shortid) {
        return putProperty(cx, js_CheckForStringIndex(ATOM_TO_JSID(name)), getter, setter, slot, attrs, flags, shortid);
    }

    /* Change the given property into a sibling with the same id in this scope. */
    js::Shape *changeProperty(JSContext *cx, js::Shape *shape, uintN attrs, uintN mask,
                              JSPropertyOp getter, JSStrictPropertyOp setter);

    /* Remove the property named by id from this object. */
    bool removeProperty(JSContext *cx, jsid id);

    /* Clear the scope, making it empty. */
    void clear(JSContext *cx);

    inline JSBool lookupGeneric(JSContext *cx, jsid id, JSObject **objp, JSProperty **propp);
    inline JSBool lookupProperty(JSContext *cx, js::PropertyName *name, JSObject **objp, JSProperty **propp);
    inline JSBool lookupElement(JSContext *cx, uint32_t index,
                                JSObject **objp, JSProperty **propp);
    inline JSBool lookupSpecial(JSContext *cx, js::SpecialId sid,
                                JSObject **objp, JSProperty **propp);

    inline JSBool defineGeneric(JSContext *cx, jsid id, const js::Value &value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                uintN attrs = JSPROP_ENUMERATE);
    inline JSBool defineProperty(JSContext *cx, js::PropertyName *name, const js::Value &value,
                                 JSPropertyOp getter = JS_PropertyStub,
                                 JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                 uintN attrs = JSPROP_ENUMERATE);

    inline JSBool defineElement(JSContext *cx, uint32_t index, const js::Value &value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                uintN attrs = JSPROP_ENUMERATE);
    inline JSBool defineSpecial(JSContext *cx, js::SpecialId sid, const js::Value &value,
                                JSPropertyOp getter = JS_PropertyStub,
                                JSStrictPropertyOp setter = JS_StrictPropertyStub,
                                uintN attrs = JSPROP_ENUMERATE);

    inline JSBool getGeneric(JSContext *cx, JSObject *receiver, jsid id, js::Value *vp);
    inline JSBool getProperty(JSContext *cx, JSObject *receiver, js::PropertyName *name,
                              js::Value *vp);
    inline JSBool getElement(JSContext *cx, JSObject *receiver, uint32_t index, js::Value *vp);
    /* If element is not present (e.g. array hole) *present is set to
       false and the contents of *vp are unusable garbage. */
    inline JSBool getElementIfPresent(JSContext *cx, JSObject *receiver, uint32_t index,
                                      js::Value *vp, bool *present);
    inline JSBool getSpecial(JSContext *cx, JSObject *receiver, js::SpecialId sid, js::Value *vp);

    inline JSBool getGeneric(JSContext *cx, jsid id, js::Value *vp);
    inline JSBool getProperty(JSContext *cx, js::PropertyName *name, js::Value *vp);
    inline JSBool getElement(JSContext *cx, uint32_t index, js::Value *vp);
    inline JSBool getSpecial(JSContext *cx, js::SpecialId sid, js::Value *vp);

    inline JSBool setGeneric(JSContext *cx, jsid id, js::Value *vp, JSBool strict);
    inline JSBool setProperty(JSContext *cx, js::PropertyName *name, js::Value *vp, JSBool strict);
    inline JSBool setElement(JSContext *cx, uint32_t index, js::Value *vp, JSBool strict);
    inline JSBool setSpecial(JSContext *cx, js::SpecialId sid, js::Value *vp, JSBool strict);

    JSBool nonNativeSetProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict);
    JSBool nonNativeSetElement(JSContext *cx, uint32_t index, js::Value *vp, JSBool strict);

    inline JSBool getGenericAttributes(JSContext *cx, jsid id, uintN *attrsp);
    inline JSBool getPropertyAttributes(JSContext *cx, js::PropertyName *name, uintN *attrsp);
    inline JSBool getElementAttributes(JSContext *cx, uint32_t index, uintN *attrsp);
    inline JSBool getSpecialAttributes(JSContext *cx, js::SpecialId sid, uintN *attrsp);

    inline JSBool setGenericAttributes(JSContext *cx, jsid id, uintN *attrsp);
    inline JSBool setPropertyAttributes(JSContext *cx, js::PropertyName *name, uintN *attrsp);
    inline JSBool setElementAttributes(JSContext *cx, uint32_t index, uintN *attrsp);
    inline JSBool setSpecialAttributes(JSContext *cx, js::SpecialId sid, uintN *attrsp);

    inline bool deleteProperty(JSContext *cx, js::PropertyName *name, js::Value *rval, bool strict);
    inline bool deleteElement(JSContext *cx, uint32_t index, js::Value *rval, bool strict);
    inline bool deleteSpecial(JSContext *cx, js::SpecialId sid, js::Value *rval, bool strict);
    bool deleteByValue(JSContext *cx, const js::Value &property, js::Value *rval, bool strict);

    inline bool enumerate(JSContext *cx, JSIterateOp iterop, js::Value *statep, jsid *idp);
    inline bool defaultValue(JSContext *cx, JSType hint, js::Value *vp);
    inline JSType typeOf(JSContext *cx);
    inline JSObject *thisObject(JSContext *cx);

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    inline JSObject *getThrowTypeError() const;

    bool swap(JSContext *cx, JSObject *other);

    inline void initArrayClass();

    static inline void writeBarrierPre(JSObject *obj);
    static inline void writeBarrierPost(JSObject *obj, void *addr);
    inline void privateWriteBarrierPre(void **oldval);
    inline void privateWriteBarrierPost(void **oldval);

    /*
     * In addition to the generic object interface provided by JSObject,
     * specific types of objects may provide additional operations. To access,
     * these addition operations, callers should use the pattern:
     *
     *   if (obj.isX()) {
     *     XObject &x = obj.asX();
     *     x.foo();
     *   }
     *
     * These XObject classes form a hierarchy. For example, for a cloned block
     * object, the following predicates are true: isClonedBlock, isBlock,
     * isNestedScope and isScope. Each of these has a respective class that
     * derives and adds operations.
     *
     * A class XObject is defined in a vm/XObject{.h, .cpp, -inl.h} file
     * triplet (along with any class YObject that derives XObject).
     *
     * Note that X represents a low-level representation and does not query the
     * [[Class]] property of object defined by the spec (for this, see
     * js::ObjectClassIs).
     *
     * SpiderMonkey has not been completely switched to the isX/asX/XObject
     * pattern so in some cases there is no XObject class and the engine
     * instead pokes directly at reserved slots and getPrivate. In such cases,
     * consider adding the missing XObject class.
     */

    /* Direct subtypes of JSObject: */
    inline bool isArguments() const;
    inline bool isArrayBuffer() const;
    inline bool isArray() const;
    inline bool isDate() const;
    inline bool isDenseArray() const;
    inline bool isError() const;
    inline bool isFunction() const;
    inline bool isGenerator() const;
    inline bool isGlobal() const;
    inline bool isIterator() const;
    inline bool isNamespace() const;
    inline bool isObject() const;
    inline bool isQName() const;
    inline bool isPrimitive() const;
    inline bool isProxy() const;
    inline bool isRegExp() const;
    inline bool isScope() const;
    inline bool isScript() const;
    inline bool isSlowArray() const;
    inline bool isStopIteration() const;
    inline bool isWeakMap() const;
    inline bool isXML() const;
    inline bool isXMLId() const;

    /* Subtypes of ScopeObject. */
    inline bool isBlock() const;
    inline bool isCall() const;
    inline bool isDeclEnv() const;
    inline bool isNestedScope() const;
    inline bool isWith() const;
    inline bool isClonedBlock() const;
    inline bool isStaticBlock() const;

    /* Subtypes of PrimitiveObject. */
    inline bool isBoolean() const;
    inline bool isNumber() const;
    inline bool isString() const;

    /* Subtypes of ArgumentsObject. */
    inline bool isNormalArguments() const;
    inline bool isStrictArguments() const;

    /* Subtypes of Proxy. */
    inline bool isWrapper() const;
    inline bool isFunctionProxy() const;
    inline bool isCrossCompartmentWrapper() const;

    inline js::ArgumentsObject &asArguments();
    inline js::BlockObject &asBlock();
    inline js::BooleanObject &asBoolean();
    inline js::CallObject &asCall();
    inline js::ClonedBlockObject &asClonedBlock();
    inline js::DeclEnvObject &asDeclEnv();
    inline js::GlobalObject &asGlobal();
    inline js::NestedScopeObject &asNestedScope();
    inline js::NormalArgumentsObject &asNormalArguments();
    inline js::NumberObject &asNumber();
    inline js::RegExpObject &asRegExp();
    inline js::ScopeObject &asScope();
    inline js::StrictArgumentsObject &asStrictArguments();
    inline js::StaticBlockObject &asStaticBlock();
    inline js::StringObject &asString();
    inline js::WithObject &asWith();

    static inline js::ThingRootKind rootKind() { return js::THING_ROOT_OBJECT; }

  private:
    static void staticAsserts() {
        /* Check alignment for any fixed slots allocated after the object. */
        JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(js::Value) == 0);

        JS_STATIC_ASSERT(offsetof(JSObject, shape_) == offsetof(js::shadow::Object, shape));
        JS_STATIC_ASSERT(offsetof(JSObject, slots) == offsetof(js::shadow::Object, slots));
        JS_STATIC_ASSERT(offsetof(JSObject, type_) == offsetof(js::shadow::Object, type));
        JS_STATIC_ASSERT(sizeof(JSObject) == sizeof(js::shadow::Object));
    }
};

/*
 * The only sensible way to compare JSObject with == is by identity. We use
 * const& instead of * as a syntactic way to assert non-null. This leads to an
 * abundance of address-of operators to identity. Hence this overload.
 */
static JS_ALWAYS_INLINE bool
operator==(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs == &rhs;
}

static JS_ALWAYS_INLINE bool
operator!=(const JSObject &lhs, const JSObject &rhs)
{
    return &lhs != &rhs;
}

inline js::HeapValue*
JSObject::fixedSlots() const
{
    return (js::HeapValue *) (uintptr_t(this) + sizeof(JSObject));
}

inline size_t
JSObject::numFixedSlots() const
{
    return reinterpret_cast<const js::shadow::Object *>(this)->numFixedSlots();
}

/* static */ inline size_t
JSObject::getFixedSlotOffset(size_t slot) {
    return sizeof(JSObject) + (slot * sizeof(js::Value));
}

/* static */ inline size_t
JSObject::getPrivateDataOffset(size_t nfixed) {
    return getFixedSlotOffset(nfixed);
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

#ifdef JS_THREADSAFE

/*
 * The GC runs only when all threads except the one on which the GC is active
 * are suspended at GC-safe points, so calling obj->getSlot() from the GC's
 * thread is safe when rt->gcRunning is set. See jsgc.cpp for details.
 */
#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#endif /* JS_THREADSAFE */

class JSValueArray {
  public:
    jsval *array;
    size_t length;

    JSValueArray(jsval *v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value *array;
    size_t length;

    ValueArray(js::Value *v, size_t c) : array(v), length(c) {}
};

/* For manipulating JSContext::sharpObjectMap. */
#define SHARP_BIT       ((jsatomid) 1)
#define BUSY_BIT        ((jsatomid) 2)
#define SHARP_ID_SHIFT  2
#define IS_SHARP(he)    (uintptr_t((he)->value) & SHARP_BIT)
#define MAKE_SHARP(he)  ((he)->value = (void *) (uintptr_t((he)->value)|SHARP_BIT))
#define IS_BUSY(he)     (uintptr_t((he)->value) & BUSY_BIT)
#define MAKE_BUSY(he)   ((he)->value = (void *) (uintptr_t((he)->value)|BUSY_BIT))
#define CLEAR_BUSY(he)  ((he)->value = (void *) (uintptr_t((he)->value)&~BUSY_BIT))

extern JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);

/*
 * Mark objects stored in map if GC happens between js_EnterSharpObject
 * and js_LeaveSharpObject. GC calls this when map->depth > 0.
 */
extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, js::LookupGenericOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, js::LookupGenericOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

#if JS_HAS_OBJ_PROTO_PROP
extern JSPropertySpec object_props[];
#else
#define object_props NULL
#endif

extern JSFunctionSpec object_methods[];
extern JSFunctionSpec object_static_methods[];

namespace js {

bool
IsStandardClassResolved(JSObject *obj, js::Class *clasp);

void
MarkStandardClassInitializedNoProto(JSObject *obj, js::Class *clasp);

/*
 * Cache for speeding up repetitive creation of objects in the VM.
 * When an object is created which matches the criteria in the 'key' section
 * below, an entry is filled with the resulting object.
 */
class NewObjectCache
{
    struct Entry
    {
        /* Class of the constructed object. */
        Class *clasp;

        /*
         * Key with one of three possible values:
         *
         * - Global for the object. The object must have a standard class for
         *   which the global's prototype can be determined, and the object's
         *   parent will be the global.
         *
         * - Prototype for the object (cannot be global). The object's parent
         *   will be the prototype's parent.
         *
         * - Type for the object. The object's parent will be the type's
         *   prototype's parent.
         */
        gc::Cell *key;

        /* Allocation kind for the constructed object. */
        gc::AllocKind kind;

        /* Number of bytes to copy from the template object. */
        uint32_t nbytes;

        /*
         * Template object to copy from, with the initial values of fields,
         * fixed slots (undefined) and private data (NULL).
         */
        JSObject_Slots16 templateObject;
    };

    Entry entries[41];

    void staticAsserts() {
        JS_STATIC_ASSERT(gc::FINALIZE_OBJECT_LAST == gc::FINALIZE_OBJECT16_BACKGROUND);
    }

  public:

    typedef int EntryIndex;

    void reset() { PodZero(this); }

    /*
     * Get the entry index for the given lookup, return whether there was a hit
     * on an existing entry.
     */
    inline bool lookupProto(Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupGlobal(Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry);
    inline bool lookupType(Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, EntryIndex *pentry);

    /* Return a new object from a cache hit produced by a lookup method. */
    inline JSObject *newObjectFromHit(JSContext *cx, EntryIndex entry);

    /* Fill an entry after a cache miss. */
    inline void fillProto(EntryIndex entry, Class *clasp, JSObject *proto, gc::AllocKind kind, JSObject *obj);
    inline void fillGlobal(EntryIndex entry, Class *clasp, js::GlobalObject *global, gc::AllocKind kind, JSObject *obj);
    inline void fillType(EntryIndex entry, Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, JSObject *obj);

    /* Invalidate any entries which might produce an object with shape/proto. */
    void invalidateEntriesForShape(JSContext *cx, Shape *shape, JSObject *proto);

  private:
    inline bool lookup(Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry);
    inline void fill(EntryIndex entry, Class *clasp, gc::Cell *key, gc::AllocKind kind, JSObject *obj);
};

} /* namespace js */

/*
 * Select Object.prototype method names shared between jsapi.cpp and jsobj.cpp.
 */
extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif

extern JSBool
js_PopulateObject(JSContext *cx, JSObject *newborn, JSObject *props);

/*
 * Fast access to immutable standard objects (constructors and prototypes).
 */
extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

/*
 * If protoKey is not JSProto_Null, then clasp is ignored. If protoKey is
 * JSProto_Null, clasp must non-null.
 */
extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey key,
                   js::Value *vp, js::Class *clasp = NULL);

// Specialized call for constructing |this| with a known function callee,
// and a known prototype.
extern JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto);

// Specialized call for constructing |this| with a known function callee.
extern JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee, bool newType);

// Generic call for constructing |this|.
extern JSObject *
js_CreateThis(JSContext *cx, JSObject *callee);

extern jsid
js_CheckForStringIndex(jsid id);

/*
 * Find or create a property named by id in obj's scope, with the given getter
 * and setter, slot, attributes, and other members.
 */
extern js::Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     JSPropertyOp getter, JSStrictPropertyOp setter, uint32_t slot,
                     uintN attrs, uintN flags, intN shortid);

/*
 * Change shape to have the given attrs, getter, and setter in scope, morphing
 * it into a potentially new js::Shape.  Return a pointer to the changed
 * or identical property.
 */
extern js::Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             js::Shape *shape, uintN attrs, uintN mask,
                             JSPropertyOp getter, JSStrictPropertyOp setter);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const js::Value &descriptor, JSBool *bp);

namespace js {

/*
 * Flags for the defineHow parameter of js_DefineNativeProperty.
 */
const uintN DNP_CACHE_RESULT = 1;   /* an interpreter call from JSOP_INITPROP */
const uintN DNP_DONT_PURGE   = 2;   /* suppress js_PurgeScopeChain */
const uintN DNP_SET_METHOD   = 4;   /* DefineNativeProperty,js_SetPropertyHelper
                                       must pass the js::Shape::METHOD
                                       flag on to JSObject::{add,put}Property */
const uintN DNP_UNQUALIFIED  = 8;   /* Unqualified property set.  Only used in
                                       the defineHow argument of
                                       js_SetPropertyHelper. */
const uintN DNP_SKIP_TYPE = 0x10;   /* Don't update type information */

/*
 * Return successfully added or changed shape or NULL on error.
 */
extern const Shape *
DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const Value &value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs,
                     uintN flags, intN shortid, uintN defineHow = 0);

inline const Shape *
DefineNativeProperty(JSContext *cx, JSObject *obj, PropertyName *name, const Value &value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs,
                     uintN flags, intN shortid, uintN defineHow = 0)
{
    return DefineNativeProperty(cx, obj, ATOM_TO_JSID(name), value, getter, setter, attrs, flags,
                                shortid, defineHow);
}

/*
 * Specialized subroutine that allows caller to preset JSRESOLVE_* flags.
 */
extern bool
LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                        JSObject **objp, JSProperty **propp);

inline bool
LookupPropertyWithFlags(JSContext *cx, JSObject *obj, PropertyName *name, uintN flags,
                        JSObject **objp, JSProperty **propp)
{
    return LookupPropertyWithFlags(cx, obj, ATOM_TO_JSID(name), flags, objp, propp);
}

/*
 * Call the [[DefineOwnProperty]] internal method of obj.
 *
 * If obj is an array, this follows ES5 15.4.5.1.
 * If obj is any other native object, this follows ES5 8.12.9.
 * If obj is a proxy, this calls the proxy handler's defineProperty method.
 * Otherwise, this reports an error and returns false.
 */
extern bool
DefineProperty(JSContext *cx, JSObject *obj, const jsid &id, const PropDesc &desc, bool throwError,
               bool *rval);

/*
 * Read property descriptors from props, as for Object.defineProperties. See
 * ES5 15.2.3.7 steps 3-5.
 */
extern bool
ReadPropertyDescriptors(JSContext *cx, JSObject *props, bool checkAccessors,
                        AutoIdVector *ids, AutoPropDescArrayRooter *descs);

/*
 * Constant to pass to js_LookupPropertyWithFlags to infer bits from current
 * bytecode.
 */
static const uintN RESOLVE_INFER = 0xffff;

/*
 * If cacheResult is false, return JS_NO_PROP_CACHE_FILL on success.
 */
extern bool
FindPropertyHelper(JSContext *cx, PropertyName *name, bool cacheResult, bool global,
                   JSObject **objp, JSObject **pobjp, JSProperty **propp);

/*
 * Search for name either on the current scope chain or on the scope chain's
 * global object, per the global parameter.
 */
extern bool
FindProperty(JSContext *cx, PropertyName *name, bool global,
             JSObject **objp, JSObject **pobjp, JSProperty **propp);

extern JSObject *
FindIdentifierBase(JSContext *cx, JSObject *scopeChain, PropertyName *name);

}

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);

/*
 * JSGET_CACHE_RESULT is the analogue of JSDNP_CACHE_RESULT for js_GetMethod.
 *
 * JSGET_METHOD_BARRIER (the default, hence 0 but provided for documentation)
 * enables a read barrier that preserves standard function object semantics (by
 * default we assume our caller won't leak a joined callee to script, where it
 * would create hazardous mutable object sharing as well as observable identity
 * according to == and ===.
 *
 * JSGET_NO_METHOD_BARRIER avoids the performance overhead of the method read
 * barrier, which is not needed when invoking a lambda that otherwise does not
 * leak its callee reference (via arguments.callee or its name).
 */
const uintN JSGET_METHOD_BARRIER    = 0; // get can leak joined function object
const uintN JSGET_NO_METHOD_BARRIER = 1; // call to joined function can't leak
const uintN JSGET_CACHE_RESULT      = 2; // from a caching interpreter opcode

/*
 * NB: js_NativeGet and js_NativeSet are called with the scope containing shape
 * (pobj's scope for Get, obj's for Set) locked, and on successful return, that
 * scope is again locked.  But on failure, both functions return false with the
 * scope containing shape unlocked.
 */
extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const js::Shape *shape, uintN getHow,
             js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const js::Shape *shape, bool added,
             bool strict, js::Value *vp);

namespace js {

bool
GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32_t getHow, Value *vp);

inline bool
GetPropertyHelper(JSContext *cx, JSObject *obj, PropertyName *name, uint32_t getHow, Value *vp)
{
    return GetPropertyHelper(cx, obj, ATOM_TO_JSID(name), getHow, vp);
}

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, PropertyDescriptor *desc);

bool
GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, Value *vp);

bool
NewPropertyDescriptorObject(JSContext *cx, const PropertyDescriptor *desc, Value *vp);

} /* namespace js */

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, js::Value *vp);

namespace js {

inline bool
GetMethod(JSContext *cx, JSObject *obj, PropertyName *name, uintN getHow, Value *vp)
{
    return js_GetMethod(cx, obj, ATOM_TO_JSID(name), getHow, vp);
}

} /* namespace js */

/*
 * Change attributes for the given native property. The caller must ensure
 * that obj is locked and this function always unlocks obj on return.
 */
extern JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, js::Shape *shape,
                       uintN attrs);

namespace js {

/*
 * If obj has an already-resolved data property for methodid, return true and
 * store the property value in *vp.
 */
extern bool
HasDataProperty(JSContext *cx, JSObject *obj, jsid methodid, js::Value *vp);

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
            js::Value *vp, uintN *attrsp);

} /* namespace js */

extern bool
js_IsDelegate(JSContext *cx, JSObject *obj, const js::Value &v);

/*
 * Wrap boolean, number or string as Boolean, Number or String object.
 * *vp must not be an object, null or undefined.
 */
extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);

extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, JSObject **objp);

/* Throws if v could not be converted to an object. */
extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

namespace js {

/*
 * Invokes the ES5 ToObject algorithm on *vp, writing back the object to vp.
 * If *vp might already be an object, use ToObject.
 */
extern JSObject *
ToObjectSlow(JSContext *cx, Value *vp);

JS_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, Value *vp)
{
    if (vp->isObject())
        return &vp->toObject();
    return ToObjectSlow(cx, vp);
}

/* As for ToObject, but preserves the original value. */
inline JSObject *
ValueToObject(JSContext *cx, const Value &v)
{
    if (v.isObject())
        return &v.toObject();
    return js_ValueToNonNullObject(cx, v);
}

} /* namespace js */

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern bool
js_ClearNative(JSContext *cx, JSObject *obj);

extern bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32_t index, js::Value *vp);

extern bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32_t index, const js::Value &v);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);

/* Object constructor native. Exposed only so the JIT can know its address. */
JSBool
js_Object(JSContext *cx, uintN argc, js::Value *vp);

namespace js {

extern bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles);

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

extern JSBool
eval(JSContext *cx, uintN argc, Value *vp);

/*
 * Performs a direct eval for the given arguments, which must correspond to the
 * currently-executing stack frame, which must be a script frame. On completion
 * the result is returned in args.rval.
 */
extern bool
DirectEval(JSContext *cx, const CallArgs &args);

/*
 * True iff |v| is the built-in eval function for the global object that
 * corresponds to |scopeChain|.
 */
extern bool
IsBuiltinEvalForScope(JSObject *scopeChain, const js::Value &v);

/* True iff fun is a built-in eval function. */
extern bool
IsAnyBuiltinEval(JSFunction *fun);

/* 'call' should be for the eval/Function native invocation. */
extern JSPrincipals *
PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx);

extern JSObject *
NonNullObject(JSContext *cx, const Value &v);

extern const char *
InformalValueTypeName(const Value &v);

/*
 * Report an error if call.thisv is not compatible with the specified class.
 *
 * NB: most callers should be calling or NonGenericMethodGuard,
 * HandleNonGenericMethodClassMismatch, or BoxedPrimitiveMethodGuard (so that
 * transparent proxies are handled correctly). Thus, any caller of this
 * function better have a good explanation for why proxies are being handled
 * correctly (e.g., by IsCallable) or are not an issue (E4X).
 */
extern void
ReportIncompatibleMethod(JSContext *cx, CallReceiver call, Class *clasp);

/*
 * A non-generic method is specified to report an error if args.thisv is not an
 * object with a specific [[Class]] internal property (ES5 8.6.2).
 * NonGenericMethodGuard performs this checking. Canonical usage is:
 *
 *   CallArgs args = ...
 *   bool ok;
 *   JSObject *thisObj = NonGenericMethodGuard(cx, args, clasp, &ok);
 *   if (!thisObj)
 *     return ok;
 *
 * Specifically: if obj is a proxy, NonGenericMethodGuard will call the
 * object's ProxyHandler's nativeCall hook (which may recursively call
 * args.callee in args.thisv's compartment). Thus, there are three possible
 * post-conditions:
 *
 *   1. thisv is an object of the given clasp: the caller may proceed;
 *
 *   2. there was an error: the caller must return 'false';
 *
 *   3. thisv wrapped an object of the given clasp and the native was reentered
 *      and completed succesfully: the caller must return 'true'.
 *
 * Case 1 is indicated by a non-NULL return value; case 2 by a NULL return
 * value with *ok == false; and case 3 by a NULL return value with *ok == true.
 *
 * NB: since this guard may reenter the native, the guard must be placed before
 * any effectful operations are performed.
 */
inline JSObject *
NonGenericMethodGuard(JSContext *cx, CallArgs args, Native native, Class *clasp, bool *ok);

/*
 * NonGenericMethodGuard tests args.thisv's class using 'clasp'. If more than
 * one class is acceptable (viz., isDenseArray() || isSlowArray()), the caller
 * may test the class and delegate to HandleNonGenericMethodClassMismatch to
 * handle the proxy case and error reporting. The 'clasp' argument is only used
 * for error reporting (clasp->name).
 */
extern bool
HandleNonGenericMethodClassMismatch(JSContext *cx, CallArgs args, Native native, Class *clasp);

/*
 * Implement the extraction of a primitive from a value as needed for the
 * toString, valueOf, and a few other methods of the boxed primitives classes
 * Boolean, Number, and String (e.g., ES5 15.6.4.2). If 'true' is returned, the
 * extracted primitive is stored in |*v|. If 'false' is returned, the caller
 * must immediately 'return *ok'. For details, see NonGenericMethodGuard.
 */
template <typename T>
inline bool
BoxedPrimitiveMethodGuard(JSContext *cx, CallArgs args, Native native, T *v, bool *ok);

}  /* namespace js */

#endif /* jsobj_h___ */
