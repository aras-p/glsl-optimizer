
#include "AMDGPUSubtarget.h"

using namespace llvm;

#define GET_SUBTARGETINFO_ENUM
#define GET_SUBTARGETINFO_TARGET_DESC
#include "AMDGPUGenSubtargetInfo.inc"

AMDGPUSubtarget::AMDGPUSubtarget(StringRef TT, StringRef CPU, StringRef FS) :
  AMDILSubtarget(TT, CPU, FS) {
    InstrItins = getInstrItineraryForCPU(CPU);

  memset(CapsOverride, 0, sizeof(*CapsOverride)
      * AMDILDeviceInfo::MaxNumberCapabilities);
  // Default card
  std::string GPU = "rv770";
  GPU = CPU;
  mIs64bit = false;
  mVersion = 0;
  SmallVector<StringRef, DEFAULT_VEC_SLOTS> Features;
  SplitString(FS, Features, ",");
  mDefaultSize[0] = 64;
  mDefaultSize[1] = 1;
  mDefaultSize[2] = 1;
  std::string newFeatures = "";
#if defined(_DEBUG) || defined(DEBUG)
  bool useTest = false;
#endif
  for (size_t x = 0; x < Features.size(); ++x) {
    if (Features[x].startswith("+mwgs")) {
      SmallVector<StringRef, DEFAULT_VEC_SLOTS> sizes;
      SplitString(Features[x], sizes, "-");
      size_t mDim = ::atoi(sizes[1].data());
      if (mDim > 3) {
        mDim = 3;
      }
      for (size_t y = 0; y < mDim; ++y) {
        mDefaultSize[y] = ::atoi(sizes[y+2].data());
      }
#if defined(_DEBUG) || defined(DEBUG)
    } else if (!Features[x].compare("test")) {
      useTest = true;
#endif
    } else if (Features[x].startswith("+cal")) {
      SmallVector<StringRef, DEFAULT_VEC_SLOTS> version;
      SplitString(Features[x], version, "=");
      mVersion = ::atoi(version[1].data());
    } else {
      GPU = CPU;
      if (x > 0) newFeatures += ',';
      newFeatures += Features[x];
    }
  }
  // If we don't have a version then set it to
  // -1 which enables everything. This is for
  // offline devices.
  if (!mVersion) {
    mVersion = (uint32_t)-1;
  }
  for (int x = 0; x < 3; ++x) {
    if (!mDefaultSize[x]) {
      mDefaultSize[x] = 1;
    }
  }
#if defined(_DEBUG) || defined(DEBUG)
  if (useTest) {
    GPU = "kauai";
  }
#endif
  ParseSubtargetFeatures(GPU, newFeatures);
#if defined(_DEBUG) || defined(DEBUG)
  if (useTest) {
    GPU = "test";
  }
#endif
  mDevName = GPU;
  mDevice = AMDILDeviceInfo::getDeviceFromName(mDevName, this, mIs64bit);
}
