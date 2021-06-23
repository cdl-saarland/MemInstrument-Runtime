The folder contains (currently exclusively) auto-generated files that communicate information from a run-time system to the LLVM-based instrumentation.

## Defined by SoftBound Run-Time (`meminstrument-rt/softbound`)
* `SBWrapper.h`: Defines all (standard) library wrappers that the SoftBound run-time implements.
* `SBRTInfo.h`: Defines if the run-time is currently build to ensure temporal safety, spatial safety, or both.
