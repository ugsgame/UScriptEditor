// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SCRIPTBREAKPOINTS_NSCRIPTHOOK_H_
#define FLATBUFFERS_GENERATED_SCRIPTBREAKPOINTS_NSCRIPTHOOK_H_

#include "flatbuffers/flatbuffers.h"

namespace NScriptHook {

struct FBreakPoint;

struct FBreakPoints;

struct FBreakPoint FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FILEPATH = 4,
    VT_LINES = 6
  };
  const flatbuffers::String *FilePath() const {
    return GetPointer<const flatbuffers::String *>(VT_FILEPATH);
  }
  const flatbuffers::Vector<int32_t> *Lines() const {
    return GetPointer<const flatbuffers::Vector<int32_t> *>(VT_LINES);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_FILEPATH) &&
           verifier.VerifyString(FilePath()) &&
           VerifyOffset(verifier, VT_LINES) &&
           verifier.VerifyVector(Lines()) &&
           verifier.EndTable();
  }
};

struct FBreakPointBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_FilePath(flatbuffers::Offset<flatbuffers::String> FilePath) {
    fbb_.AddOffset(FBreakPoint::VT_FILEPATH, FilePath);
  }
  void add_Lines(flatbuffers::Offset<flatbuffers::Vector<int32_t>> Lines) {
    fbb_.AddOffset(FBreakPoint::VT_LINES, Lines);
  }
  explicit FBreakPointBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FBreakPointBuilder &operator=(const FBreakPointBuilder &);
  flatbuffers::Offset<FBreakPoint> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<FBreakPoint>(end);
    return o;
  }
};

inline flatbuffers::Offset<FBreakPoint> CreateFBreakPoint(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> FilePath = 0,
    flatbuffers::Offset<flatbuffers::Vector<int32_t>> Lines = 0) {
  FBreakPointBuilder builder_(_fbb);
  builder_.add_Lines(Lines);
  builder_.add_FilePath(FilePath);
  return builder_.Finish();
}

inline flatbuffers::Offset<FBreakPoint> CreateFBreakPointDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *FilePath = nullptr,
    const std::vector<int32_t> *Lines = nullptr) {
  auto FilePath__ = FilePath ? _fbb.CreateString(FilePath) : 0;
  auto Lines__ = Lines ? _fbb.CreateVector<int32_t>(*Lines) : 0;
  return NScriptHook::CreateFBreakPoint(
      _fbb,
      FilePath__,
      Lines__);
}

struct FBreakPoints FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_GROUP = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<FBreakPoint>> *Group() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<FBreakPoint>> *>(VT_GROUP);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_GROUP) &&
           verifier.VerifyVector(Group()) &&
           verifier.VerifyVectorOfTables(Group()) &&
           verifier.EndTable();
  }
};

struct FBreakPointsBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_Group(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FBreakPoint>>> Group) {
    fbb_.AddOffset(FBreakPoints::VT_GROUP, Group);
  }
  explicit FBreakPointsBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FBreakPointsBuilder &operator=(const FBreakPointsBuilder &);
  flatbuffers::Offset<FBreakPoints> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<FBreakPoints>(end);
    return o;
  }
};

inline flatbuffers::Offset<FBreakPoints> CreateFBreakPoints(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FBreakPoint>>> Group = 0) {
  FBreakPointsBuilder builder_(_fbb);
  builder_.add_Group(Group);
  return builder_.Finish();
}

inline flatbuffers::Offset<FBreakPoints> CreateFBreakPointsDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<FBreakPoint>> *Group = nullptr) {
  auto Group__ = Group ? _fbb.CreateVector<flatbuffers::Offset<FBreakPoint>>(*Group) : 0;
  return NScriptHook::CreateFBreakPoints(
      _fbb,
      Group__);
}

inline const NScriptHook::FBreakPoints *GetFBreakPoints(const void *buf) {
  return flatbuffers::GetRoot<NScriptHook::FBreakPoints>(buf);
}

inline const NScriptHook::FBreakPoints *GetSizePrefixedFBreakPoints(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<NScriptHook::FBreakPoints>(buf);
}

inline bool VerifyFBreakPointsBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<NScriptHook::FBreakPoints>(nullptr);
}

inline bool VerifySizePrefixedFBreakPointsBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<NScriptHook::FBreakPoints>(nullptr);
}

inline void FinishFBreakPointsBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<NScriptHook::FBreakPoints> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedFBreakPointsBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<NScriptHook::FBreakPoints> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace NScriptHook

#endif  // FLATBUFFERS_GENERATED_SCRIPTBREAKPOINTS_NSCRIPTHOOK_H_
