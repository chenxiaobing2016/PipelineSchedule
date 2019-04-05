#ifndef PIPELINESCHEDULE_UTIL_H_
#define PIPELINESCHEDULE_UTIL_H_

#define GET_ACCESS(TYPE, NAME, FIELD) \
  TYPE get##NAME() const {            \
    return FIELD;                     \
  }

#define SET_ACCESS(TYPE, NAME, FIELD) \
  void set##NAME(TYPE value) {        \
    FIELD = value;                    \
  }

#define GET_SET_ACCESS(TYPE, NAME, FIELD) \
  GET_ACCESS(TYPE, NAME, FIELD)           \
  SET_ACCESS(TYPE, NAME, FIELD)

#define ENABLE_SHARED_CLASS(CLASSNAME)                \
  class CLASSNAME;                                    \
  using CLASSNAME##Sptr = std::shared_ptr<CLASSNAME>; \
  using CLASSNAME##Wptr = std::weak_ptr<CLASSNAME>;   \
  class CLASSNAME : public std::enable_shared_from_this<CLASSNAME>

#define NOT_IMPLEMENTED                                                                   \
  std::cout << "[" << __FILE__ << ":" << __FUNCTION__ << "] unimplemented." << std::endl; \
  assert(0);

#endif  // PIPELINESCHEDULE_UTIL_H_
