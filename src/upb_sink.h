/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * upb_sink is a general purpose interface for pushing the contents of a
 * protobuf from one component to another in a streaming fashion.  We call the
 * component that calls a upb_sink a "source".  By "pushing" we mean that the
 * source calls into the sink; the opposite (where a sink calls into the
 * source) is known as "pull".  In the push model the source gets the main
 * loop; in a pull model the sink does.
 *
 * This interface is used as general-purpose glue in upb.  For example, the
 * parser interface works by implementing a source.  Likewise the serialization
 * simply implements a sink.  Copying one protobuf to another is just a matter
 * of using one message as a source and another as a sink.
 *
 * In terms of efficiency, we would generally expect "push" to be faster if the
 * source had more state to track, and "pull" to be faster if the sink had more
 * state.  The reason is that whoever has the main loop can keep state on the
 * stack (and possibly even in callee-save registers), whereas the the
 * component that is "called into" always needs to reload its state from
 * memory.
 *
 * In terms of programming complexity, it is easier and simpler to have the
 * main loop, because you can store state in local variables.
 *
 * So the assumption inherent in using the push model is that sources are
 * generally more complicated and stateful than consumers.  For example, in the
 * parser case, it has to deal with malformed input and associated errors; in
 * comparison, the serializer deals with known-good input.
 */

#ifndef UPB_SINK_H
#define UPB_SINK_H

#ifdef __cplusplus
extern "C" {
#endif

struct upb_fielddef;

// Each of the upb_sink callbacks returns a status of this type.
typedef enum {
  // The normal case, where the consumer wants to continue consuming.
  UPB_SINK_CONTINUE,

  // The consumer has consumed the current value, but it wants to stop for now.
  // When the source is next invoked, it should resume at the next value.  Note
  // that sources are not necessarily resumable; if a source is not resumable,
  // returning UPB_SINK_SUSPEND will simply halt it.
  UPB_SINK_SUSPEND,

  // The consumer wants to skip to the end of the current submessage and
  // continue consuming.  If we are at the top-level, the rest of the
  // data is discarded.
  UPB_SINK_SKIP
} upb_sink_status;


typedef struct {
  struct upb_sink_callbacks *vtbl;
} upb_sink;

/* upb_sink callbacks *********************************************************/

// The value callback is called when a regular value (ie. not a string or
// submessage) is pushed.
typedef upb_sink_status (*upb_value_cb)(upb_sink *s, struct upb_fielddef *f,
                                        union upb_value val);

// The string callback is called when a string is pushed.  "str" is the string
// in which the data lives, but it may contain more data than the effective
// string.  "start" and "end" indicate the substring of "str" that is the
// effective string.  If "start" is <0, this string is a continuation of the
// previous string for this field.  If end > upb_strlen(str) then there is more
// data to follow for this string.  "end" can also be used as a hint for how
// much data follows, but this is only a hint and is not guaranteed.
//
// The data is supplied this way to give you the opportunity to reference this
// data instead of copying it (perhaps using upb_strslice), or to minimize
// copying if it is unavoidable.
typedef upb_sink_status (*upb_str_cb)(upb_sink *s, struct upb_fielddef *f,
                                      upb_strptr str,
                                      int32_t start, uint32_t end);

// The start and end callbacks are called when a submessage begins and ends,
// respectively.
typedef upb_sink_status (*upb_start_cb)(upb_sink *s, struct upb_fielddef *f);
typedef upb_sink_status (*upb_end_cb)(upb_sink *s);


/* upb_sink implementation *************************************************/

typedef struct upb_sink_callbacks {
  upb_value_cb value_cb;
  upb_str_cb str_cb;
  upb_start_cb start_cb;
  upb_end_cb end_cb;
} upb_sink_callbacks;

// We could potentially define these later to also be capable of calling a C++
// virtual method instead of doing the virtual dispatch manually.  This would
// make it possible to write C++ sinks in a more natural style.  We could have
// a flag in upb_sink defining whether it is a C sink or a C++ one.
#define upb_sink_onvalue(s, f, val) s->vtbl->value_cb(s, f, val)
#define upb_sink_onstr(s, f, str, start, end) s->vtbl->str_cb(s, f, str, start, end)
#define upb_sink_onstart(s, f) s->vtbl->start_cb(s, f)
#define upb_sink_onend(s) s->vtbl->end_cb(s)

// Initializes a plain C visitor with the given vtbl.  The visitor must have
// been allocated separately.
INLINE void upb_sink_init(upb_sink *s, upb_sink_callbacks *vtbl) {
  s->vtbl = vtbl;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif
