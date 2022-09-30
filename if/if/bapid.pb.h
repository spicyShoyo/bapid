// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: if/bapid.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_if_2fbapid_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_if_2fbapid_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021006 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_if_2fbapid_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_if_2fbapid_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_if_2fbapid_2eproto;
namespace bapid {
class PingReply;
struct PingReplyDefaultTypeInternal;
extern PingReplyDefaultTypeInternal _PingReply_default_instance_;
class PingRequest;
struct PingRequestDefaultTypeInternal;
extern PingRequestDefaultTypeInternal _PingRequest_default_instance_;
}  // namespace bapid
PROTOBUF_NAMESPACE_OPEN
template<> ::bapid::PingReply* Arena::CreateMaybeMessage<::bapid::PingReply>(Arena*);
template<> ::bapid::PingRequest* Arena::CreateMaybeMessage<::bapid::PingRequest>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace bapid {

// ===================================================================

class PingRequest final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:bapid.PingRequest) */ {
 public:
  inline PingRequest() : PingRequest(nullptr) {}
  ~PingRequest() override;
  explicit PROTOBUF_CONSTEXPR PingRequest(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  PingRequest(const PingRequest& from);
  PingRequest(PingRequest&& from) noexcept
    : PingRequest() {
    *this = ::std::move(from);
  }

  inline PingRequest& operator=(const PingRequest& from) {
    CopyFrom(from);
    return *this;
  }
  inline PingRequest& operator=(PingRequest&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const PingRequest& default_instance() {
    return *internal_default_instance();
  }
  static inline const PingRequest* internal_default_instance() {
    return reinterpret_cast<const PingRequest*>(
               &_PingRequest_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(PingRequest& a, PingRequest& b) {
    a.Swap(&b);
  }
  inline void Swap(PingRequest* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(PingRequest* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  PingRequest* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<PingRequest>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const PingRequest& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const PingRequest& from) {
    PingRequest::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(PingRequest* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "bapid.PingRequest";
  }
  protected:
  explicit PingRequest(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNameFieldNumber = 1,
  };
  // string name = 1 [json_name = "name"];
  void clear_name();
  const std::string& name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_name();
  PROTOBUF_NODISCARD std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // @@protoc_insertion_point(class_scope:bapid.PingRequest)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_if_2fbapid_2eproto;
};
// -------------------------------------------------------------------

class PingReply final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:bapid.PingReply) */ {
 public:
  inline PingReply() : PingReply(nullptr) {}
  ~PingReply() override;
  explicit PROTOBUF_CONSTEXPR PingReply(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  PingReply(const PingReply& from);
  PingReply(PingReply&& from) noexcept
    : PingReply() {
    *this = ::std::move(from);
  }

  inline PingReply& operator=(const PingReply& from) {
    CopyFrom(from);
    return *this;
  }
  inline PingReply& operator=(PingReply&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const PingReply& default_instance() {
    return *internal_default_instance();
  }
  static inline const PingReply* internal_default_instance() {
    return reinterpret_cast<const PingReply*>(
               &_PingReply_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(PingReply& a, PingReply& b) {
    a.Swap(&b);
  }
  inline void Swap(PingReply* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(PingReply* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  PingReply* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<PingReply>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const PingReply& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const PingReply& from) {
    PingReply::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(PingReply* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "bapid.PingReply";
  }
  protected:
  explicit PingReply(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kMessageFieldNumber = 1,
  };
  // string message = 1 [json_name = "message"];
  void clear_message();
  const std::string& message() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_message(ArgT0&& arg0, ArgT... args);
  std::string* mutable_message();
  PROTOBUF_NODISCARD std::string* release_message();
  void set_allocated_message(std::string* message);
  private:
  const std::string& _internal_message() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_message(const std::string& value);
  std::string* _internal_mutable_message();
  public:

  // @@protoc_insertion_point(class_scope:bapid.PingReply)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr message_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_if_2fbapid_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// PingRequest

// string name = 1 [json_name = "name"];
inline void PingRequest::clear_name() {
  _impl_.name_.ClearToEmpty();
}
inline const std::string& PingRequest::name() const {
  // @@protoc_insertion_point(field_get:bapid.PingRequest.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void PingRequest::set_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.name_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:bapid.PingRequest.name)
}
inline std::string* PingRequest::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:bapid.PingRequest.name)
  return _s;
}
inline const std::string& PingRequest::_internal_name() const {
  return _impl_.name_.Get();
}
inline void PingRequest::_internal_set_name(const std::string& value) {
  
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* PingRequest::_internal_mutable_name() {
  
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* PingRequest::release_name() {
  // @@protoc_insertion_point(field_release:bapid.PingRequest.name)
  return _impl_.name_.Release();
}
inline void PingRequest::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    
  } else {
    
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:bapid.PingRequest.name)
}

// -------------------------------------------------------------------

// PingReply

// string message = 1 [json_name = "message"];
inline void PingReply::clear_message() {
  _impl_.message_.ClearToEmpty();
}
inline const std::string& PingReply::message() const {
  // @@protoc_insertion_point(field_get:bapid.PingReply.message)
  return _internal_message();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void PingReply::set_message(ArgT0&& arg0, ArgT... args) {
 
 _impl_.message_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:bapid.PingReply.message)
}
inline std::string* PingReply::mutable_message() {
  std::string* _s = _internal_mutable_message();
  // @@protoc_insertion_point(field_mutable:bapid.PingReply.message)
  return _s;
}
inline const std::string& PingReply::_internal_message() const {
  return _impl_.message_.Get();
}
inline void PingReply::_internal_set_message(const std::string& value) {
  
  _impl_.message_.Set(value, GetArenaForAllocation());
}
inline std::string* PingReply::_internal_mutable_message() {
  
  return _impl_.message_.Mutable(GetArenaForAllocation());
}
inline std::string* PingReply::release_message() {
  // @@protoc_insertion_point(field_release:bapid.PingReply.message)
  return _impl_.message_.Release();
}
inline void PingReply::set_allocated_message(std::string* message) {
  if (message != nullptr) {
    
  } else {
    
  }
  _impl_.message_.SetAllocated(message, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.message_.IsDefault()) {
    _impl_.message_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:bapid.PingReply.message)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace bapid

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_if_2fbapid_2eproto
