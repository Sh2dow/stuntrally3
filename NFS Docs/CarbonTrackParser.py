#!/usr/bin/env python3
"""
Compatibility wrapper for the legacy CarbonTrackParser entry point.

The old implementation drifted away from the validated Carbon research and
contained speculative parsing paths. The enhanced parser is now the canonical
tool. This wrapper keeps older docs and commands working:

    python CarbonTrackParser.py <carbon_tracks_dir> <output_dir> [region]
"""

from CarbonTrackParser_Enhanced import (  # noqa: F401
    CARBON_CHUNK_IDS,
    CARBON_ZONE_TYPES,
    CarbonBundleChunk,
    CarbonStreamingSection,
    CarbonTrackBarrier,
    CarbonTrackInfo,
    CarbonTrackParser,
    CarbonTrackZone,
    main as enhanced_main,
)


if __name__ == "__main__":
    raise SystemExit(enhanced_main())
